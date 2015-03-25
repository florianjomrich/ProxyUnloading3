//
// Copyright (C) 2005 Andras Varga
// Copyright (C) 2005 Wei Yang, Ng
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU Lesser General Public License
// as published by the Free Software Foundation; either version 2
// of the License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public License
// along with this program; if not, see <http://www.gnu.org/licenses/>.
//

#include <omnetpp.h>
#include "IPv6.h"
#include "InterfaceTableAccess.h"
#include "RoutingTable6Access.h"
#include "ICMPv6Access.h"
#include "IPv6NeighbourDiscoveryAccess.h"
#include "IPv6TunnelingAccess.h"
#include "IPv6ControlInfo.h"
#include "IPv6NDMessage_m.h"
#include "Ieee802Ctrl_m.h"
#include "ICMPv6Message_m.h"
#include "MobilityHeader_m.h"
#include "IPv6ExtensionHeaders_m.h"
#include "xMIPv6Access.h"
#include "UDPPacket.h"

//PROXY UNLOADING
#include "RequetConnectionToLegacyServer_m.h"
#include "FlowBindingUpdate_m.h"
#include "ACK_FlowBindingUpdate_m.h"

#define FRAGMENT_TIMEOUT 60   // 60 sec, from IPv6 RFC
#define REQUEST_FOR_CONNECTION_TO_LEGACY_SERVER 145

Define_Module(IPv6);

void IPv6::initialize() {
    QueueBase::initialize();

    ift = InterfaceTableAccess().get();
    rt = RoutingTable6Access().get();
    nd = IPv6NeighbourDiscoveryAccess().get();
    icmp = ICMPv6Access().get();
    tunneling = IPv6TunnelingAccess().get();

    mapping.parseProtocolMapping(par("protocolMapping"));
    isActive = par("isActive");

    curFragmentId = 0;
    lastCheckTime = 0;
    fragbuf.init(icmp);

    numMulticast = numLocalDeliver = numDropped = numUnroutable = numForwarded =
            0;
    numDropNonActive = 0;

    evIPv6Active.setName("IPv6Active_Event");
    evIPv6Active.record(1); // Bruno Sousa  - by default IPv6 is enabled.

    WATCH(numMulticast);
    WATCH(numLocalDeliver);
    WATCH(numDropped);
    WATCH(numUnroutable);
    WATCH(numForwarded);
    WATCH(numDropNonActive);

    //PROXY UNLOADING
    isMN = par("isMN");
    isHA = par("isHA");
    isCN = par("isCN");
    myHumanReadableName =
            par("humanReadableNameForAutomaticAdding").stringValue();


    //get Instance of the FlowBindingTable - but only if MN or CN or HA - not a normal router - they do not have such a module
    // this module could be added in a further investigation
    if (isMN || isHA || isCN) {
        flowBindingTable = FlowBindingTableAccess().get();
    }

    //cout<<"Initialisiere IPv6 Layer"<<endl;

}

void IPv6::updateDisplayString() {
    char buf[80] = "";
    if (numForwarded > 0)
        sprintf(buf + strlen(buf), "fwd:%d ", numForwarded);
    if (numLocalDeliver > 0)
        sprintf(buf + strlen(buf), "up:%d ", numLocalDeliver);
    if (numMulticast > 0)
        sprintf(buf + strlen(buf), "mcast:%d ", numMulticast);
    if (numDropped > 0)
        sprintf(buf + strlen(buf), "DROP:%d ", numDropped);
    if (numUnroutable > 0)
        sprintf(buf + strlen(buf), "UNROUTABLE:%d ", numUnroutable);
    getDisplayString().setTagArg("t", 0, buf);
}

void IPv6::endService(cPacket *msg) {

    //*********************************************************************************************************
    //PROXY UNLOADING
    //*********************************************************************************************************
    //check if it is a control message form the ProxyUnloading_Control_App:
    if (dynamic_cast<RequetConnectionToLegacyServer*>(msg) && isCN) {
        cout
                << "The network layer of CN inserts a new entry into it's FlowBindingTable"
                << endl;
        RequetConnectionToLegacyServer* newFlowBindingEntryMessage =
                check_and_cast<RequetConnectionToLegacyServer *>(msg);

        flowBindingTable->insertNewFlowBindingEntry(newFlowBindingEntryMessage);
        flowBindingTable->getFlowBindingEntryFromTable(
                newFlowBindingEntryMessage->getFlowSourceAddress());
        return;
    }

    if (dynamic_cast<ACK_RequestConnectionToLegacyServer*>(msg)
            && (isHA || isMN)) {

        //the flow-Binding-Table on the network layer has to be updated for the MN and the HA as well
        cout << "MN/HA" << isMN << isHA
                << " haben Bestätigung vom CN erhalten und aktualisieren jetzt ebenfalls ihre FlowBindingTable"
                << endl;
        ACK_RequestConnectionToLegacyServer* newFlowBindingEntryMessage =
                check_and_cast<ACK_RequestConnectionToLegacyServer *>(msg);
        flowBindingTable->insertNewFlowBindingEntry(newFlowBindingEntryMessage);
        flowBindingTable->getFlowBindingEntryFromTable(
                newFlowBindingEntryMessage->getFlowSourceAddress());
        return;

    }

    if(dynamic_cast<FlowBindingUpdate*>(msg) && (isHA || isCN)){
        FlowBindingUpdate* receivedFlowBindingUpdate = check_and_cast<FlowBindingUpdate *>(msg);

        cout<<"HA/CN: "<<isHA<<isCN<<" Netzwerklayer aktualisiert jetzt die FlowBindingTable"<<endl;
        flowBindingTable->updateExistingFlowBindingEntry(receivedFlowBindingUpdate);
        return;
    }


    if(dynamic_cast<ACK_FlowBindingUpdate*>(msg) && (isMN)){
        ACK_FlowBindingUpdate* ackFlowBindingUpdate = check_and_cast<ACK_FlowBindingUpdate *>(msg);

        cout<<"MN Netzwerklayer aktualisiert jetzt die FlowBindingTable"<<endl;
        flowBindingTable->updateExistingFlowBindingEntry(ackFlowBindingUpdate);
        return;
    }

    //*********************************************************************************************************
    //OTHERWISE IT is a normal data package - that has to be dealed with in the following:

    EV << "\n<<=======THIS IS THE IPv6::endService() function=========>>\n";
    bool isTunneled = false;

    // 28.09.07 - CB
    // support for rescheduling datagrams which are supposed to be sent over
    // a tentative address.
    if (msg->isSelfMessage()) {
        ScheduledDatagram* sDgram = check_and_cast<ScheduledDatagram*>(msg);

        // take care of datagram which was supposed to be sent over a tentative address
        if (sDgram->ie->ipv6Data()->isTentativeAddress(
                sDgram->datagram->getSrcAddress())) {
            // address is still tentative - enqueue again
            queue.insert(sDgram);
        } else {
            // address is not tentative anymore - send out datagram
            if (isActive) {
                numForwarded++;
                sendDatagramToOutput(sDgram->datagram, sDgram->ie,
                        sDgram->macAddr);
            } else {
                numDropNonActive++;
            }
            delete sDgram;

        }
    } else if (msg->getArrivalGate()->isName("transportIn")
            || (msg->getArrivalGate()->isName("upperTunnelingIn"))
            || // for tunneling support-CB
            (msg->getArrivalGate()->isName("ndIn")
                    && dynamic_cast<IPv6NDMessage*>(msg))
            || (msg->getArrivalGate()->isName("icmpIn")
                    && dynamic_cast<ICMPv6Message*>(msg)) || //Added this for ICMP msgs from ICMP module-WEI
            (msg->getArrivalGate()->isName("xMIPv6In")
                    && dynamic_cast<MobilityHeader*>(msg))) // Zarrar
            {
        // packet from upper layers, tunnel link-layer output or ND: encapsulate and send out
        EV << "\n<<=======HandleMessagefromHL() Called=========>>\n";
        handleMessageFromHL(msg);
    } else {
        EV
                  << "\n<<=======checking_and_casting msg into IPv6Datagram=========>>\n";
        // datagram from network or from ND: localDeliver and/or route
        EV << "Message received on port " << msg->getArrivalGate()->getName()
                  << endl; //lowerTunnelingIn
        if (msg->getArrivalGate()->isName("lowerTunnelingIn")) {
            EV << "Setting Tunnel to true" << endl;
            //curIE = getSourceInterfaceFrom(msg);
            isTunneled = true;
        }
        EV << "=== Received on interface "
                  << getSourceInterfaceFrom(msg, isTunneled)->getInterfaceId()
                  << endl;
        IPv6Datagram *dgram = check_and_cast<IPv6Datagram *>(msg);
        handleDatagramFromNetwork(dgram, isTunneled);
    }

    if (ev.isGUI())
        updateDisplayString();
}

InterfaceEntry *IPv6::getSourceInterfaceFrom(cPacket *msg, bool isTunneled) {
    cGate *g = msg->getArrivalGate();
    InterfaceEntry * aux;
    if (!isTunneled) {
        if (g) {
            EV << "GATE getIndex= " << g->getIndex() << endl; //" and getId =" << g->getId() << endl;
        }

        // EV << "Interface ID with getIndex= " << ift->getInterfaceByNetworkLayerGateIndex(g->getIndex())->getInterfaceId() << endl;
        // EV << "Interface ID with getId= " << ift->getInterfaceByNetworkLayerGateIndex(g->getId())->getInterfaceId() << endl;

        aux = g ?
                ift->getInterfaceByNetworkLayerGateIndex(g->getIndex()) : NULL;
    } else {
        //aux = curIE;

        // remove control info
        cObject *crtrl = msg->getControlInfo();
        if (dynamic_cast<IPv6ControlInfo*>(crtrl) != NULL) {
            IPv6ControlInfo *controlInfo = check_and_cast<IPv6ControlInfo*>(
                    crtrl);
            EV << "IPv6 Control Tunneled = " << controlInfo->getInterfaceId()
                      << endl;
            aux = ift->getInterfaceById(controlInfo->getInterfaceId());

            if (aux == NULL) {
                EV << "IPv6 Control Tunneled aux interface NULL" << endl;
            }
        } else {
            aux = NULL;
        }

    }

    return aux;
}

void IPv6::handleDatagramFromNetwork(IPv6Datagram *datagram, bool isTunnelled) {

    EV << "\n<<=======HandleMessagefromNetwork() Called=========>>  " << endl;
    // check for header biterror
    if (datagram->hasBitError()) {
        EV << "bit error\n";
        return; // revise!
        /*FIXME revise
         // probability of bit error in header = size of header / size of total message
         // (ignore bit error if in payload)
         double relativeHeaderLength = datagram->getHeaderLength() / (double)datagram->getByteLength();
         if (dblrand() <= relativeHeaderLength)
         {
         EV << "bit error found, sending ICMP_PARAMETER_PROBLEM\n";
         icmp->sendErrorMessage(datagram, ICMP_PARAMETER_PROBLEM, 0);
         return;
         }
         */
    }

    // remove control info
    //delete datagram->removeControlInfo();
    if (!isTunnelled) { //Such condition might not be good!! FIXME: Free memory on tunnels?!
        cObject *crtrl = datagram->removeControlInfo();
        if (dynamic_cast<IPv6ControlInfo*>(crtrl) != NULL) {
            IPv6ControlInfo *controlInfo = check_and_cast<IPv6ControlInfo*>(
                    crtrl);
            EV << "IPv6 Control = " << controlInfo->getInterfaceId() << endl;
        }
        delete crtrl;
    }

    // routepacket
    if (!datagram->getDestAddress().isMulticast())
        routePacket(datagram, NULL, false, isTunnelled);
    else
        routeMulticastPacket(datagram, NULL, getSourceInterfaceFrom(datagram));
}

void IPv6::handleMessageFromHL(cPacket *msg) {

    // if no interface exists, do not send datagram
    if (ift->getNumInterfaces() == 0) {
        EV << "No interfaces exist, dropping packet\n";
        delete msg;
        return;
    }

    // encapsulate upper-layer packet into IPv6Datagram
    InterfaceEntry *destIE; // to be filled in by encapsulate()

    IPv6Datagram *datagram = encapsulate(msg, destIE);

    if (datagram == NULL) {
        EV << "Encapsulation failed - dropping packet." << endl;
        delete msg;
        return;
    }

    // possibly fragment (in IPv6, only the source node does that), then route it
    fragmentAndRoute(datagram, destIE);
}

void IPv6::fragmentAndRoute(IPv6Datagram *datagram, InterfaceEntry *destIE) {
    /*
     FIXME implement fragmentation here.
     1. determine output interface
     2. compare packet size with interface MTU
     3. if bigger, do fragmentation
     int mtu = ift->interfaceByPortNo(outputGateIndex)->getMTU();
     */
    EV << "fragmentation not implemented yet\n";

    // route packet
    if (destIE != NULL) {
        EV << "fragmentAndRoute: sending immediately to output." << endl;
        IPv6Address nextHop;
        MACAddress mac_to_use = MACAddress::BROADCAST_ADDRESS;
        int interfaceId;
        nextHop = rt->lookupDestCache(datagram->getDestAddress(), interfaceId);

        if (nextHop != IPv6Address::UNSPECIFIED_ADDRESS) {
            mac_to_use = nd->resolveNeighbour(nextHop, interfaceId);
            EV << "FOUND netxHop " << nextHop << " with Mac Address "
                      << mac_to_use << endl;

            //Bruno SOusa
            parse_ipv6_datagram_for_mcoa(datagram, destIE);
        }

        //Bruno Sousa
        // Check Interface output when inerface is not a tunnel
        if (interfaceId != destIE->getInterfaceId() && interfaceId != -1) {
            EV << "IN IPV6 LAYER IT WAS CHOSEN A DIFFERENT INTERFACE!!!!!!!!!!"
                      << interfaceId << " , " << destIE->getInterfaceId()
                      << endl;
            EV << "Setting Correct MAC ADDRESS FROM  " << mac_to_use;
            mac_to_use = nd->resolveNeighbour_MCoA(nextHop,
                    destIE->getInterfaceId());
            EV << " to " << mac_to_use << endl;
        }

        if (mac_to_use == MACAddress::UNSPECIFIED_ADDRESS) {
            mac_to_use = MACAddress::BROADCAST_ADDRESS;
        }

        // FIXME what MAC address to use?
        //Bruno Sousa - possible fix
        if (isActive) {
            sendDatagramToOutput(datagram, destIE, mac_to_use);
            numForwarded++;
        } else {
            numDropNonActive++;
            delete datagram;
        }
    } else if (!datagram->getDestAddress().isMulticast())
        routePacket(datagram, destIE, true);
    else
        routeMulticastPacket(datagram, destIE, NULL);
}

void IPv6::routePacket(IPv6Datagram *datagram, InterfaceEntry *destIE,
        bool fromHL, bool isTunneled) {
    // TBD add option handling code here
    IPv6Address destAddress = datagram->getDestAddress();

    EV << "Routing datagram '" << datagram->getName() << "' with dest="
              << destAddress << ":\n";

    // local delivery of unicast packets
    if (rt->isLocalAddress(destAddress)) {
        EV << "local delivery\n";

        if (datagram->getSrcAddress().isUnspecified())
            datagram->setSrcAddress(destAddress); // allows two apps on the same host to communicate
        numLocalDeliver++;

        isLocalAddress(datagram, isTunneled);
        return;
    }

    if (!fromHL) {
        // if datagram arrived from input gate and IP forwarding is off, delete datagram
        //yes but datagrams from the ND module is getting dropped too!-WEI
        //so we add a 2nd condition
        // FIXME rewrite code so that condition is cleaner --Andras
        //if (!rt->isRouter())
        if (!rt->isRouter() && !(datagram->getArrivalGate()->isName("ndIn"))) {
            EV << "forwarding is off, dropping packet\n";
            numDropped++;
            delete datagram;
            return;
        }

        // don't forward link-local addresses or weaker
        if (destAddress.isLinkLocal() || destAddress.isLoopback()) {
            EV
                      << "dest address is link-local (or weaker) scope, doesn't get forwarded\n";
            delete datagram;
            return;
        }

        // hop counter decrement: only if datagram arrived from network, and will be
        // sent out to the network (hoplimit check will be done just before sending
        // out datagram)
        // TBD: in IPv4, arrange TTL check like this
        datagram->setHopLimit(datagram->getHopLimit() - 1);
    }

    // routing
    int interfaceId = -1;
    IPv6Address nextHop;

    // restructured code from below due for mobility - CB
    /*if ( datagram->srcAddress().isUnspecified() )
     {
     if ( !determineOutputInterface(destAddress, nextHop, interfaceId, datagram) )
     // no interface found; sent to ND or to ICMP for error processing
     opp_error("No interface found!");//return;
     return;
     // TODO: don't raise error if sent to ND or ICMP!

     // else we can set the source address of the datagram with the
     // preferred address from the interface we've just specified
     InterfaceEntry *ie = ift->interfaceAt(interfaceId);
     //IPv6Address sourceAddr = ie->ipv6()->preferredAddress();
     IPv6Address sourceAddr = ie->ipv6()->globalAddress(IPv6InterfaceData::HoA);

     if ( sourceAddr.isUnspecified() )
     sourceAddr = ie->ipv6()->preferredAddress();

     ASSERT( !sourceAddr.isUnspecified() );
     datagram->setSrcAddress(sourceAddr);
     EV << "Source address of datagram was set to " << sourceAddr << endl;
     }*/

    // tunneling support - CB
    // check if destination is covered by tunnel lists
    if ((datagram->getTransportProtocol() != IP_PROT_IPv6) && // if datagram was already tunneled, don't tunnel again
            (datagram->getExtensionHeaderArraySize() == 0) && // we do not already have extension headers - FIXME: check for RH2 existence
            ((rt->isMobileNode() && rt->isHomeAddress(datagram->getSrcAddress()))
                    || // for MNs: only if source address is a HoA // 27.08.07 - CB
                    rt->isHomeAgent() || // but always check for tunnel if node is a HA
                    !rt->isMobileNode() // or if it is a correspondent or non-MIP node
            )) {
        if (datagram->getTransportProtocol() == IP_PROT_IPv6EXT_MOB)
            // in case of mobility header we can only search for "real" tunnels
            // as T2RH or HoA Opt. are not allowed with these messages
            interfaceId = tunneling->getVIfIndexForDest(destAddress,
                    IPv6Tunneling::NORMAL); // 10.06.08 - CB
        //getVIfIndexForDestForXSplitTunnel(destAddress);
        else
            // otherwise we can search for everything
            interfaceId = tunneling->getVIfIndexForDest(destAddress);
    }
    //else
    //interfaceId = -1;

    if (interfaceId > ift->getNumInterfaces()) {
        // a virtual tunnel interface provides a path to the destination: do tunneling
        EV << "tunneling: src addr=" << datagram->getSrcAddress()
                  << ", dest addr=" << destAddress << std::endl;

        //EV << "sending datagram to encapsulation..." << endl;
        send(datagram, "lowerTunnelingOut");
        return;
    }

    if (interfaceId == -1)
        if (!determineOutputInterface(destAddress, nextHop, interfaceId,
                datagram))
            // no interface found; sent to ND or to ICMP for error processing
            //opp_error("No interface found!");//return;
            return;// don't raise error if sent to ND or ICMP!

    InterfaceEntry *ie = ift->getInterfaceById(interfaceId);
    ASSERT(ie!=NULL);
    EV << "next hop for " << destAddress << " is " << nextHop << ", interface "
              << ie->getName() << "\n";
    ASSERT(!nextHop.isUnspecified() && ie!=NULL);

    MACAddress macAddr = nd->resolveNeighbour(nextHop, interfaceId);
    if (macAddr.isUnspecified()) {
        EV
                  << "no link-layer address for next hop yet, passing datagram to Neighbour Discovery module\n";
        send(datagram, "ndOut");
        return;
    }
    EV << "link-layer address: " << macAddr << "\n";

    // set datagram source address if not yet set
    if (datagram->getSrcAddress().isUnspecified()) {
        const IPv6Address& srcAddr = ie->ipv6Data()->getPreferredAddress();
        ASSERT(!srcAddr.isUnspecified()); // FIXME what if we don't have an address yet?
        datagram->setSrcAddress(srcAddr);

        // if the datagram has a tentative address as source we have to reschedule it
        // as it can not be sent before the address' tentative status is cleared - CB
        if (ie->ipv6Data()->isTentativeAddress(srcAddr)) {
            EV
                      << "Source address is tentative - enqueueing datagram for later resubmission."
                      << endl;
            ScheduledDatagram* sDgram = new ScheduledDatagram();
            sDgram->datagram = datagram;
            sDgram->ie = ie;
            sDgram->macAddr = macAddr;
            queue.insert(sDgram);
            return;
        }
    }

    //Bruno SOusa
    parse_ipv6_datagram_for_mcoa(datagram, ie);

    // send out datagram
    if (isActive) {
        numForwarded++;
        sendDatagramToOutput(datagram, ie, macAddr);
    } else {
        numDropNonActive++;
        delete datagram;
    }
}

void IPv6::routeMulticastPacket(IPv6Datagram *datagram, InterfaceEntry *destIE,
        InterfaceEntry *fromIE) {
    const IPv6Address& destAddr = datagram->getDestAddress();

    EV << "destination address " << destAddr
              << " is multicast, doing multicast routing\n";
    numMulticast++;

    // if received from the network...
    if (fromIE != NULL) {
        // deliver locally
        if (rt->isLocalAddress(destAddr)) {
            EV << "local delivery of multicast packet\n";
            numLocalDeliver++;
            isLocalAddress((IPv6Datagram *) datagram->dup());
        }

        // if datagram arrived from input gate and IP forwarding is off, delete datagram
        if (!rt->isRouter()) {
            EV << "forwarding is off\n";
            delete datagram;
            return;
        }

        // make sure scope of multicast address is large enough to be forwarded to other links
        if (destAddr.getMulticastScope() <= 2) {
            EV << "multicast dest address is link-local (or smaller) scope\n";
            delete datagram;
            return;
        }

        // hop counter decrement: only if datagram arrived from network, and will be
        // sent out to the network (hoplimit check will be done just before sending
        // out datagram)
        // TBD: in IPv4, arrange TTL check like this
        datagram->setHopLimit(datagram->getHopLimit() - 1);
    }

    // for now, we just send it out on every interface except on which it came. FIXME better!!!
    EV << "sending out datagram on every interface (except incoming one)\n";
    for (int i = 0; i < ift->getNumInterfaces(); i++) {
        InterfaceEntry *ie = ift->getInterface(i);
        if (fromIE != ie) {
            if (isActive) {
                sendDatagramToOutput((IPv6Datagram *) datagram->dup(), ie,
                        MACAddress::BROADCAST_ADDRESS);
            } else {
                numDropNonActive++;
            }
        }
    }
    delete datagram;

    /* FIXME implement handling of multicast

     According to Gopi: "multicast routing table" should map
     srcAddr+multicastDestAddr to a set of next hops (interface+nexthopAddr)
     Where srcAddr is the multicast server, and destAddr sort of narrows it down to a given stream

     // FIXME multicast-->tunneling link (present in original IPSuite) missing from here

     // DVMRP: process datagram only if sent locally or arrived on the shortest
     // route (provided routing table already contains srcAddr); otherwise
     // discard and continue.
     int inputGateIndex = datagram->getArrivalGate() ? datagram->getArrivalGate()->getIndex() : -1;
     int shortestPathInputGateIndex = rt->outputGateIndexNo(datagram->getSrcAddress());
     if (inputGateIndex!=-1 && shortestPathInputGateIndex!=-1 && inputGateIndex!=shortestPathInputGateIndex)
     {
     // FIXME count dropped
     EV << "Packet dropped.\n";
     delete datagram;
     return;
     }

     // check for local delivery
     IPv6Address destAddress = datagram->getDestAddress();
     if (rt->isLocalMulticastAddress(destAddress))
     {
     IPv6Datagram *datagramCopy = (IPv6Datagram *) datagram->dup();

     // FIXME code from the MPLS model: set packet dest address to routerId (???)
     datagramCopy->setDestAddress(rt->getRouterId());

     isLocalAddress(datagramCopy);
     }

     // forward datagram only if IP forward is enabled, or sent locally
     if (inputGateIndex!=-1 && !rt->isRouter())
     {
     delete datagram;
     return;
     }

     MulticastRoutes routes = rt->getMulticastRoutesFor(destAddress);
     if (routes.size()==0)
     {
     // no destination: delete datagram
     delete datagram;
     }
     else
     {
     // copy original datagram for multiple destinations
     for (unsigned int i=0; i<routes.size(); i++)
     {
     int outputGateIndex = routes[i].interf->outputGateIndex();

     // don't forward to input port
     if (outputGateIndex>=0 && outputGateIndex!=inputGateIndex)
     {
     IPv6Datagram *datagramCopy = (IPv6Datagram *) datagram->dup();

     // set datagram source address if not yet set
     if (datagramCopy->getSrcAddress().isUnspecified())
     datagramCopy->setSrcAddress(ift->interfaceByPortNo(outputGateIndex)->ipv6Data()->getIPAddress());

     // send
     IPv6Address nextHopAddr = routes[i].gateway;
     sendDatagramToOutput(datagramCopy, outputGateIndex, macAddr);
     }
     }

     // only copies sent, delete original datagram
     delete datagram;
     }
     */
}

void IPv6::isLocalAddress(IPv6Datagram *datagram, bool isTunnelled) {

    //*************************************************************
    //TEST OB MAN DIE SRC IP ADRESSE ÄNDERN KANN ZUM UPPER LAYER HOCH:

    if (datagram->getTransportProtocol() == IP_PROT_UDP) { //handelt es sich um ein UDP Paket ?
        if (isMN || isCN) {
            IPv6Address* neueAdresse = new IPv6Address("1111:111::1111:111"); /// ????
            // datagram->setSrcAddress(*neueAdresse); // hierüber ersetzt man die IP-Adresse nach oben hin zum UDP-Layer
        }
        //cout<<"SOURCE ADRESSE ="<<datagram->getSrcAddress()<<endl;
    }

    //*********************************************************++

    /* FIXME revise and complete defragmentation
     // Defragmentation. skip defragmentation if datagram is not fragmented
     if (datagram->getFragmentOffset()!=0 || datagram->getMoreFragments())
     {
     EV << "Datagram fragment: offset=" << datagram->getFragmentOffset()
     << ", MORE=" << (datagram->getMoreFragments() ? "true" : "false") << ".\n";

     // erase timed out fragments in fragmentation buffer; check every 10 seconds max
     if (simTime() >= lastCheckTime + 10)
     {
     lastCheckTime = simTime();
     fragbuf.purgeStaleFragments(simTime()-FRAGMENT_TIMEOUT);
     }

     datagram = fragbuf.addFragment(datagram, simTime());
     if (!datagram)
     {
     EV << "No complete datagram yet.\n";
     return;
     }
     EV << "This fragment completes the datagram.\n";
     }
     */
    // #### 29.08.07 - CB
    // check for extension headers
    if (!processExtensionHeaders(datagram))
        // ext. header processing not yet finished
        // datagram was sent to another module or dropped
        // -> interrupt local delivery process
        return;
    // #### end CB

    // decapsulate and send on appropriate output gate
    int protocol = datagram->getTransportProtocol();
    cPacket *packet = decapsulate(datagram, isTunnelled);

    if (protocol == IP_PROT_IPv6_ICMP && dynamic_cast<IPv6NDMessage*>(packet)) {
        EV << "Neigbour Discovery packet: passing it to ND module\n";
        send(packet, "ndOut");
    } else if (protocol == IP_PROT_IPv6EXT_MOB
            && dynamic_cast<MobilityHeader*>(packet)) {
        // added check for MIPv6 support to prevent nodes w/o the
        // xMIP module from processing related messages, 4.9.07 - CB
        if (rt->hasMIPv6Support()) {
            EV << "MIPv6 packet: passing it to xMIPv6 module\n";
            send(check_and_cast<MobilityHeader*>(packet), "xMIPv6Out");
        } else {
            // update 12.9.07 - CB
            /*RFC3775, 11.3.5
             Any node that does not recognize the Mobility header will return an
             ICMP Parameter Problem, Code 1, message to the sender of the packet*/
            EV << "No MIPv6 support on this node!\n";
            IPv6ControlInfo *ctrlInfo = check_and_cast<IPv6ControlInfo*>(
                    packet->removeControlInfo());
            icmp->sendErrorMessage(packet, ctrlInfo, ICMPv6_PARAMETER_PROBLEM,
                    1);

            //delete packet; // 13.9.07 - CB, update 21.9.07 - CB
        }
    } else if (protocol == IP_PROT_IPv6_ICMP
            && dynamic_cast<ICMPv6Message*>(packet)) {
        EV << "ICMPv6 packet: passing it to ICMPv6 module\n";
        send(packet, "icmpOut");
    }    	//Added by WEI to forward ICMPv6 msgs to ICMPv6 module.
    else if (protocol == IP_PROT_IP || protocol == IP_PROT_IPv6) {
        EV << "Tunnelled IP datagram\n";
        send(packet, "upperTunnelingOut");
    } else {
        int gateindex = mapping.getOutputGateForProtocol(protocol);
        // 21.9.07 - CB
        cGate* outGate = gate("transportOut", gateindex);
        if (!outGate->isConnected()) {
            EV << "Transport layer gate not connected - dropping packet!\n";
            delete packet;
        } else {
            EV << "Protocol " << protocol << ", passing up on gate "
                      << gateindex << "\n";

            //Bruno Sousa
            // Assign on receiving packets also... only for apps
            parse_ipv6_datagram_for_mcoa(datagram, NULL, true);

            //TODO: Indication of forward progress
            send(packet, "transportOut", gateindex);

        }
    }
}

void IPv6::handleReceivedICMP(ICMPv6Message *msg) {
    EV
              << "\n<<=======THIS IS THE IPv6::handleReceivedICMP() FUNCTION=========>>\n";
    switch (msg->getType()) {
    case ICMPv6_REDIRECT:  // TODO implement redirect handling
    case ICMPv6_DESTINATION_UNREACHABLE:
    case ICMPv6_PACKET_TOO_BIG:
    case ICMPv6_TIME_EXCEEDED:
    case ICMPv6_PARAMETER_PROBLEM: {
        // ICMP errors are delivered to the appropriate higher layer protocols
        //IPv6Datagram *bogusPacket = check_and_cast<IPv6Datagram *>(msg->getEncapsulatedMsg());
        IPv6Datagram *bogusPacket = check_and_cast<IPv6Datagram *>(
                msg->getEncapsulatedPacket());

        int protocol = bogusPacket->getTransportProtocol();
        int gateindex = mapping.getOutputGateForProtocol(protocol);
        send(msg, "transportOut", gateindex);
        break;
    }
    default: {
        // all others are delivered to ICMP:
        // ICMPv6_ECHO_REQUEST, ICMPv6_ECHO_REPLY, ICMPv6_MLD_QUERY, ICMPv6_MLD_REPORT,
        // ICMPv6_MLD_DONE, ICMPv6_ROUTER_SOL, ICMPv6_ROUTER_AD, ICMPv6_NEIGHBOUR_SOL,
        // ICMPv6_NEIGHBOUR_AD, ICMPv6_MLDv2_REPORT
        int gateindex = mapping.getOutputGateForProtocol(IP_PROT_ICMP);
        send(msg, "transportOut", gateindex);
    }
    }
}

cPacket *IPv6::decapsulate(IPv6Datagram *datagram, bool isTunneled) {
    // decapsulate transport packet
    InterfaceEntry *fromIE = getSourceInterfaceFrom(datagram, isTunneled);

    if (isTunneled) { // Free memory on tunnels not previously freed
        cObject *crtrl = datagram->removeControlInfo();
        if (dynamic_cast<IPv6ControlInfo*>(crtrl) != NULL) {
            IPv6ControlInfo *controlInfo = check_and_cast<IPv6ControlInfo*>(
                    crtrl);
            EV << "IPv6 Control Tunnel delete = "
                      << controlInfo->getInterfaceId() << endl;
        }
        delete crtrl;
    }
    cPacket *packet = datagram->decapsulate();

    // create and fill in control info
    IPv6ControlInfo *controlInfo = new IPv6ControlInfo();
    controlInfo->setProtocol(datagram->getTransportProtocol());
    controlInfo->setSrcAddr(datagram->getSrcAddress());
    controlInfo->setDestAddr(datagram->getDestAddress());
    controlInfo->setHopLimit(datagram->getHopLimit());
    controlInfo->setInterfaceId(fromIE ? fromIE->getInterfaceId() : -1);

    EV << " Set Interface ID = " << controlInfo->getInterfaceId() << endl;

    // original IP datagram might be needed in upper layers to send back ICMP error message
    controlInfo->setOrigDatagram(datagram);

    // attach control info
    packet->setControlInfo(controlInfo);

    return packet;
}

//PROXY UNLOADING EXTENSION
IPv6Datagram* IPv6::calculateFlowSourceAddress(IPv6Datagram *datagram) {

    //check first if the src and destination ip adresses are set properly otherwise return and do nothing here
    if (datagram->getDestAddress().isUnspecified()) {
        cout
                << "@@DEST-ADRESSE WAR UNSPEZIFIZIERT FlowSourceAdresse konnte nicht berechnet werden"
                << endl;
        return datagram;
    }

    if (datagram->getSrcAddress().isUnspecified()
            && datagram->getTransportProtocol() == IP_PROT_UDP) {
        cout
                << "@@SRC-ADRESSE  DES UDP-PAKETS WAR UNSPEZIFIZIERT und wurde automatisch ergänzt"
                << endl;
        IPvXAddress newSourceAdress = IPAddressResolver().resolve(
                myHumanReadableName);

        datagram->setSrcAddress(newSourceAdress.get6());
    }

    //UPPER LAYER PROTOCOL INFORMATION READ OUT FROM IP DATAGRAMM siehe: https://groups.google.com/forum/#!topic/omnetpp/PzYmRXP4eUI
    int protocol = datagram->getTransportProtocol();
    int sport = -1;
    int dport = -1;
    if (protocol == IP_PROT_UDP) { //only UDP-Protocol is checked currently
        sport =
                dynamic_cast<UDPPacket*>(datagram->getEncapsulatedPacket())->getSourcePort();
        dport =
                dynamic_cast<UDPPacket*>(datagram->getEncapsulatedPacket())->getDestinationPort();

        if (dport != -1 && sport != -1) { //only if the ports are set go further

            cout << "DEST PORT: " << dport << " und SRC PORT: " << sport
                    << endl;
            cout << "DEST ADDR: " << datagram->getDestAddress()
                    << " und SRC ADDR: " << datagram->getSrcAddress() << endl;

            uint32_t srcAdressPart0 =
                    datagram->getSrcAddress().getIPAdressAsIntegerPart0(); //4 Methods for 4*32Bit = 128 Bit IPv6 Adress-Length
            uint32_t srcAdressPart1 =
                    datagram->getSrcAddress().getIPAdressAsIntegerPart1();
            uint32_t srcAdressPart2 =
                    datagram->getSrcAddress().getIPAdressAsIntegerPart2();
            uint32_t srcAdressPart3 =
                    datagram->getSrcAddress().getIPAdressAsIntegerPart3();

            uint32_t destAdressPart0 =
                    datagram->getDestAddress().getIPAdressAsIntegerPart0(); //4 Methods for 4*32Bit = 128 Bit IPv6 Adress-Length
            uint32_t destAdressPart1 =
                    datagram->getDestAddress().getIPAdressAsIntegerPart1();
            uint32_t destAdressPart2 =
                    datagram->getDestAddress().getIPAdressAsIntegerPart2();
            uint32_t destAdressPart3 =
                    datagram->getDestAddress().getIPAdressAsIntegerPart3();

            //hahsFunction now! - Knuths Multiplicativ Method - siehe http://stackoverflow.com/questions/664014/what-integer-hash-function-are-good-that-accepts-an-integer-hash-key
            uint32_t flowSourceAddress0 = srcAdressPart0 + sport + dport
                    + destAdressPart0;
            uint32_t flowSourceAddress1 = srcAdressPart1 + sport + dport
                    + destAdressPart1;
            uint32_t flowSourceAddress2 = srcAdressPart2 + sport + dport
                    + destAdressPart2;
            uint32_t flowSourceAddress3 = srcAdressPart3 + sport + dport
                    + destAdressPart3;

            IPv6Address* flowSourceAddress = new IPv6Address(flowSourceAddress0,
                    flowSourceAddress1, flowSourceAddress2, flowSourceAddress3);

            //cout << "FLOW SOURCE ADDRESS: " << flowSourceAddress << endl;

            //IF NO MESSAGE WAS ALREADY SENT TO the other HOST - AND the Message is not for the HA itself - SEND ONE CONTROL MESSAGE TO HA

            if(!flowBindingTable->entryAlreadyExistsInTable(
                    dport, sport, datagram->getDestAddress().str().c_str(),
                    datagram->getSrcAddress().str().c_str(),flowSourceAddress->str().c_str())
                    && datagram->getDestAddress()
                            != IPAddressResolver().resolve("HA").get6()

                            ) {
                RequetConnectionToLegacyServer* legacyRequestPacket =
                        new RequetConnectionToLegacyServer();
                legacyRequestPacket->setKind(
                REQUEST_FOR_CONNECTION_TO_LEGACY_SERVER);
                legacyRequestPacket->setSrcAddress(
                        datagram->getSrcAddress().str().c_str());
                legacyRequestPacket->setDestAddress(
                        datagram->getDestAddress().str().c_str());
                legacyRequestPacket->setFlowSourceAddress(
                        flowSourceAddress->str().c_str());
                legacyRequestPacket->setSrcPort(sport);
                legacyRequestPacket->setDestPort(dport);

                //add entry if not already sent
                //requestForConnectionToLegacyServerTable->setRequest(dport,
                 //       sport, datagram->getDestAddress(),
                 //       datagram->getSrcAddress(), flowSourceAddress);

                send(legacyRequestPacket, "uDPControllAppConnection$o");
            }

            /**
             * Replace now the Source-IP-Address of the package if there already exists a connection which has been acknowledged
             */
            if (flowBindingTable->entryAlreadyExistsInTable(
                    flowSourceAddress->str().c_str())) {
                cout << "IP Adresse wird nun ersetzt durch FlowSourceAdresse"
                        << endl;

                IPv6Address neueSrcAdresse = IPv6Address();
                neueSrcAdresse.set(flowSourceAddress->str().c_str()); /// ????
                datagram->setSrcAddress(neueSrcAdresse);

                //for the reason when the HomeAgent has to take over the proxying functionality - it should be sent to him not to the CN
                IPv6Address neueDestAdresse = IPv6Address();
                neueDestAdresse.set(
                        flowBindingTable->getFlowBindingEntryFromTable(
                                flowSourceAddress->str().c_str())->getDestAddress());
                datagram->setDestAddress(neueDestAdresse);

            }
            /*
             * IP ADRESSE SO ERSETZEN
             *
             *Entweder so:
             *
             *
             *           IPv6Address neueDestAdresse = IPv6Address();
             neueDestAdresse.set(flowBindingTable->getFlowBindingEntryFromTable(
             flowSourceAddress->str().c_str())->getDestAddress())
             datagram->setDestAddress(neueDestAdresse);
             *
             * Alternativ auch so:
             IPv6Address* neueAdresse = new IPv6Address(flowSourceAddress->str().c_str());/// ????
             datagram->setSrcAddress(*neueAdresse);
             */

        } //only if the ports are set go further END

    } //only UDP-Protocol is checked currently END

    //Return the IP-Datagram now whether or not the Flow-Source-IP-Adresse got added by the FlowBindingTable
    return datagram;
}

IPv6Datagram *IPv6::encapsulate(cPacket *transportPacket,
        InterfaceEntry *&destIE) {
    EV << "\n<<=======THIS IS THE IPv6::encapsulate() FUNCTION=========>>\n";

    IPv6ControlInfo *controlInfo = check_and_cast<IPv6ControlInfo*>(
            transportPacket->removeControlInfo());

    EV << "Received on interface " << controlInfo->getInterfaceId() << endl;

    IPv6Datagram *datagram = new IPv6Datagram(transportPacket->getName());
    // -- moved the following two lines below, as otherwise the size of the extension headers would
    // not be taken into account, 30.08.07 - CB
    //datagram->setByteLength(datagram->calculateHeaderByteLength());
    //datagram->encapsulate(transportPacket);

    // IPV6_MULTICAST_IF option, but allow interface selection for unicast packets as well
    destIE = ift->getInterfaceById(controlInfo->getInterfaceId());

    // set source and destination address
    IPv6Address dest = controlInfo->getDestAddr();

    datagram->setDestAddress(dest);

    IPv6Address src = controlInfo->getSrcAddr();

    // when source address was given, use it; otherwise it'll get the address
    // of the outgoing interface after routing
    if (!src.isUnspecified()) {
        // if interface parameter does not match existing interface, do not send datagram
        if (rt->getInterfaceByAddress(src) == NULL) {
            opp_error(
                    "Wrong source address %s in (%s)%s: no interface with such address",
                    src.str().c_str(), transportPacket->getClassName(),
                    transportPacket->getFullName());
            delete datagram;
            return NULL;
        }
        datagram->setSrcAddress(src);
    }

    // set other fields
    datagram->setHopLimit(
            controlInfo->getHopLimit() > 0 ? controlInfo->getHopLimit() : 32); //FIXME use iface hop limit instead of 32?
    datagram->setTransportProtocol(controlInfo->getProtocol());

    // #### copy routing headers from ctrlInfo to datagram if present, 29.08.07 - CB ####
    // FIXME this is a nasty way of copying the extension headers
    for (uint i = 0; i < controlInfo->extensionHeaderArraySize(); i++) {
        IPv6ExtensionHeader* extHeader = controlInfo->extensionHeader(i);
        datagram->addExtensionHeader((IPv6ExtensionHeader*) extHeader->dup());
        EV << "Copied extension header to datagram." << endl;
    }

    delete controlInfo;

    datagram->setByteLength(datagram->calculateHeaderByteLength()); // 30.08.07 - CB
    datagram->encapsulate(transportPacket); // 30.08.07 - CB

    //PROXY UNLOADING XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX
    //calculate the flow source adress for the new packet and replace the src ip address through the flow source address
    //not necessary for HA, CN and Routers

    if (isMN) {
        cout << "Name des durch den MN zu sendenden Datagrams: "
                << datagram->getName() << endl;
        calculateFlowSourceAddress(datagram);
    }

    //PROXY UNLOADING XXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX

    return datagram;
}

void IPv6::sendDatagramToOutput(IPv6Datagram *datagram, InterfaceEntry *ie,
        const MACAddress& macAddr) {
    // hop counter check
    if (datagram->getHopLimit() <= 0) {
        // drop datagram, destruction responsibility in ICMP
        EV << "datagram hopLimit reached zero, sending ICMPv6_TIME_EXCEEDED\n";
        icmp->sendErrorMessage(datagram, ICMPv6_TIME_EXCEEDED, 0); // FIXME check icmp 'code'
        return;
    }

    // in link layer uses MAC addresses (basically, not PPP), add control info
    if (!macAddr.isUnspecified()) {
        Ieee802Ctrl *controlInfo = new Ieee802Ctrl();
        controlInfo->setDest(macAddr);
        datagram->setControlInfo(controlInfo);
    }

    // send datagram to link layer
    send(datagram, "queueOut", ie->getNetworkLayerGateIndex());
}

bool IPv6::determineOutputInterface(const IPv6Address& destAddress,
        IPv6Address& nextHop, int& interfaceId, IPv6Datagram* datagram) {
    // try destination cache
    //IPv6Address nextHop = rt->lookupDestCache(destAddress, interfaceId);
    nextHop = rt->lookupDestCache(destAddress, interfaceId);

    if (interfaceId == -1) {
        // address not in destination cache: do longest prefix match in routing table
        EV << "do longest prefix match in routing table" << endl;
        const IPv6Route *route = rt->doLongestPrefixMatch(destAddress);
        EV << "finished longest prefix match in routing table" << endl;
        if (!route) {
            if (rt->isRouter()) {
                EV << "unroutable, sending ICMPv6_DESTINATION_UNREACHABLE\n";
                numUnroutable++;
                icmp->sendErrorMessage(datagram, ICMPv6_DESTINATION_UNREACHABLE,
                        0); // FIXME check ICMP 'code'
            } else // host
            {
                EV
                          << "no match in routing table, passing datagram to Neighbour Discovery module for default router selection\n";
                send(datagram, "ndOut");
            }
            return false;
        }
        interfaceId = route->getInterfaceId();
        nextHop = route->getNextHop();
        if (nextHop.isUnspecified())
            nextHop = destAddress;  // next hop is the host itself

        // add result into destination cache
        rt->updateDestCache(destAddress, nextHop, interfaceId);
    }

    return true;
}

bool IPv6::processExtensionHeaders(IPv6Datagram* datagram) {
    int noExtHeaders = datagram->getExtensionHeaderArraySize();
    EV << noExtHeaders << " extension header(s) for processing..." << endl;

    // walk through all extension headers
    for (int i = 0; i < noExtHeaders; i++) {
        IPv6ExtensionHeader* eh = datagram->popExtensionHeader();

        if (dynamic_cast<IPv6RoutingHeader*>(eh)) {
            IPv6RoutingHeader* rh = (IPv6RoutingHeader*) (eh);
            EV << "Routing Header with type=" << rh->getRoutingType() << endl;

            // type 2 routing header should be processed by MIPv6 module
            // if no MIP support, ignore the header
            if (rt->hasMIPv6Support() && rh->getRoutingType() == 2) {
                // for simplicity, we set a context pointer on the datagram
                datagram->setContextPointer(rh);
                EV << "Sending datagram with RH2 to MIPv6 module" << endl;
                send(datagram, "xMIPv6Out");
                return false;
            } else {
                delete rh;
                EV << "Ignoring unknown routing header" << endl;
            }
        } else if (dynamic_cast<IPv6DestinationOptionsHeader*>(eh)) {
            //IPv6DestinationOptionsHeader* doh = (IPv6DestinationOptionsHeader*) (eh);
            //EV << "object of type=" << typeid(eh).name() << endl;

            if (rt->hasMIPv6Support() && dynamic_cast<HomeAddressOption*>(eh)) {
                datagram->setContextPointer(eh);
                EV << "Sending datagram with HoA Option to MIPv6 module"
                          << endl;
                send(datagram, "xMIPv6Out");
                return false;
            } else {
                delete eh;
                EV << "Ignoring unknown destination options header" << endl;
            }
        } else {
            delete eh;
            EV << "Ignoring unknown extension header" << endl;
        }
    }

    // we have processed no extension headers -> the IPv6 module can continue
    // working on this datagram
    return true;
}

void IPv6::parse_ipv6_datagram_for_mcoa(IPv6Datagram* datagram,
        InterfaceEntry* ie, bool rcv) {

    if (rt->isMobileNode()) {
        /*
         * Bruno Sousa
         *
         * Update the structure with correspondent nodes of MN.
         */
        xMIPv6* mipv6 = NULL;
        mipv6 = xMIPv6Access().getIfExists();
        if (mipv6) {
            int proto = datagram->getTransportProtocol();

            // || proto == IP_PROT_IPv6_ICMP
            if (proto == IP_PROT_TCP || proto == IP_PROT_UDP
                    || proto == IP_PROT_SCTP) {

                if (!rcv) {
                    if (datagram->getDestAddress()
                            != ie->ipv6Data()->getHomeAgentAddress()) {
                        EV << "MCOA - UPDATING LIST OF CNs on sending CN= "
                                  << datagram->getDestAddress() << " src="
                                  << datagram->getSrcAddress() << endl;
                        mipv6->updateCnList(datagram->getDestAddress(),
                                mipv6->get_and_calcBID(
                                        datagram->getSrcAddress(), ie));
                    }
                } else {
                    if (!datagram->getSrcAddress().isUnspecified()
                            && !datagram->getDestAddress().isUnspecified()) {
                        EV << "MCOA - UPDATING LIST OF CNs on receiving CN= "
                                  << datagram->getSrcAddress() << " src="
                                  << datagram->getDestAddress() << endl;
                        mipv6->updateCnList(datagram->getSrcAddress(),
                                mipv6->get_and_calcBID(
                                        datagram->getDestAddress(), ie));
                    }
                }
            } else {
                EV << "Ignoring update of CN list for packet with protocol"
                          << proto << endl;
            }
        }
    }
}

void IPv6::processCommand(const cXMLElement& node) {
    //cXMLElement *child=node.getFirstChild();

    if (!strcmp(node.getTagName(), "new-IPv6Status")) {
        //int newCap = OPP_Global::atoul(node.getAttribute("Cap"));
        bool setAct = CnvtoBool(node.getAttribute("isActive"));

        //this->frameCapacity = newCap;
        //cPar &frameCap = par("frameCapacity");
        //frameCap = newCap;

        this->isActive = setAct;
        cPar &isActive = par("isActive");
        isActive = setAct;

        //EV << "BRUNO DBG - processCmd setting frameCapacity to " << frameCapacity << endl;
        EV << "BRUNO DBG - processCmd setting isActive to " << this->isActive
                  << endl;

        if (isActive) {
            evIPv6Active.record(1);
        } else {
            evIPv6Active.record(2);
        }

    }
}

bool IPv6::CnvtoBool(const char *s, const bool defaultValue) {
    if (!s)
        return defaultValue;
    return !strcmp(s, "on") || !strcmp(s, "true") || !strcmp(s, "yes");
}

void IPv6::finish() {
    recordScalar("IPv6_numDropNonActive", numDropNonActive);
    recordScalar("IPv6_Forwarded", numForwarded);

}
