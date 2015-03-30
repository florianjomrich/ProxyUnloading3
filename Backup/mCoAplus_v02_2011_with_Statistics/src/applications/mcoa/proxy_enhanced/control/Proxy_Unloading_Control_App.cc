//
// Copyright (C) 2010 Bruno Sousa
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

//
// based on the video streaming app of the similar name by Johnny Lai
//

#include "Proxy_Unloading_Control_App.h"
#include "IPAddressResolver.h"
#include "IPv6ControlInfo.h"

#include "RoutingTable6.h"
#include "RoutingTable6Access.h"

#include "UDPPacket.h"

#define PROXY_ENHANCED_BU_MESSAGE  42
#define PROXY_CN_MESSAGE_TO_MOBILE_NODE 43
#define PROXY_MESSAGE_FROM_CN_TO_MN 51
#define REQUEST_FOR_CONNECTION_TO_LEGACY_SERVER 145
#define REQUEST_FOR_CONNECTION_TO_LEGACY_SERVER_CONTROL_APP 146
#define REQUEST_CONNECTION_HA_TIMEOUT 149
#define PAY_LOAD_OF_PROXYING_PACKET 150

using std::cout;

Define_Module(Proxy_Unloading_Control_App);

void Proxy_Unloading_Control_App::initialize() {
    //PROXY UNLOADING FJ
    cout << "Initializing Proxy_Unloading_Control_App module" << endl;
    isMN = par("isMN");
    isHA = par("isHA");
    isCN = par("isCN");
    isCapableCN = par("isCapableCN");
    humanReadableName = par("humanReadableName");
    proxyRequestForConnectionTimeOut = &par("proxyRequestForConnectionTimeOut");
    startTime = par("startTime");
    //cout << "START ZEIT: " << startTime << endl;

    MCoAUDPBase::startMCoAUDPBase();

    PktRcv.setName("MCOA VIDEO Packet Rcv");
    PktLost.setName("MCOA VIDEO Packet Lost");
    PktDelay.setName("MCOA VIDEO Delay");

    lastSeq = 0;

    int localPort = par("localPort");
    bindToPort(localPort);

}

void Proxy_Unloading_Control_App::finish() {
    //Delete StatsPkt
    StatsPkt.clear();

}

void Proxy_Unloading_Control_App::handleMessage(cMessage* msg) {
    cout << "PROXY CONTROL APP of: MN:" << isMN << " HA:" << isHA << " CN:"
            << isCN << " received a message" << endl;

    if (msg->isSelfMessage()) {
        if (msg->getKind() == MK_REMOVE_ADDRESS_PAIR) {
            MCoAUDPBase::treatMessage(msg);

            return; // and that's it!
        }
        if (msg->getKind() == REQUEST_CONNECTION_HA_TIMEOUT) {
            cout
                    << "HA Time Out Timer timed out - correspondent CN does not seem to support the protocol."
                    << endl;
            RequetConnectionToLegacyServer* messageFromMN = check_and_cast<
                    RequetConnectionToLegacyServer *>(
                    msg->getObject("REQUEST_FOR_CONNECTION_TO_LEGACY_SERVER"));

            cout << "Daten des Timers waren: DestinationAddress:"
                    << messageFromMN->getDestAddress() << " Source Addresse: "
                    << messageFromMN->getSrcAddress() << endl;

            //bindToPort to identify new connection
            bindToPort (nextFreeProxyingPort);

            //   UDPPacket * payload = check_and_cast<UDPPacket *>(messageFromMN->getObject("MCoACli (MN) requests Video Stream from MCoASrv (CN)."));

            /*     if (dynamic_cast<UDPPacket *>(messageFromMN->getObject("MCoACli (MN) requests Video Stream from MCoASrv (CN)."))) {
             UDPPacket * payload = check_and_cast<UDPPacket *>(messageFromMN->getObject("MCoACli (MN) requests Video Stream from MCoASrv (CN).")->dup());

             IPvXAddress destionationAddress = IPvXAddress();
             destionationAddress.set(messageFromMN->getDestAddress());

             sendToUDPMCOA(payload, nextFreeProxyingPort,
             destionationAddress, 2000, true);
             nextFreeProxyingPort++;
             }*/

            //UPDATE OWN TABLE for the new proxying Address
            ACK_RequestConnectionToLegacyServer* acknowledgmentToHaHimself =
                               new ACK_RequestConnectionToLegacyServer();
            acknowledgmentToHaHimself->setSrcAddress(
                    messageFromMN->getSrcAddress());
            acknowledgmentToHaHimself->setDestAddress(
                    messageFromMN->getDestAddress());
            acknowledgmentToHaHimself->setFlowSourceAddress(
                    messageFromMN->getFlowSourceAddress());
            acknowledgmentToHaHimself->setDestPort(
                    messageFromMN->getSrcPort());
            acknowledgmentToHaHimself->setSrcPort(messageFromMN->getSrcPort());
            acknowledgmentToHaHimself->setName(
                               "ACK_RequestConnectionToLegacyServer");


            send(acknowledgmentToHaHimself, "uDPControllAppConnection$o");

            //Respond to MN that HA is now the Proxy Server
            IPvXAddress ha = IPAddressResolver().resolve("HA");

            ACK_RequestConnectionToLegacyServer* acknowledgmentToMN =
                    new ACK_RequestConnectionToLegacyServer();
            acknowledgmentToMN->setSrcAddress(messageFromMN->getSrcAddress());
            acknowledgmentToMN->setDestAddress(ha.str().c_str());
            acknowledgmentToMN->setFlowSourceAddress(
                    messageFromMN->getFlowSourceAddress());
            acknowledgmentToMN->setDestPort(localPort);
            acknowledgmentToMN->setSrcPort(messageFromMN->getSrcPort());
            acknowledgmentToMN->setName("ACK_RequestConnectionToLegacyServer");
            IPvXAddress mn = IPvXAddress();
            mn.set(messageFromMN->getSrcAddress());
            sendToUDPMCOA(acknowledgmentToMN, localPort, mn, 2000,
                    true);

        }

    } else {

        if (dynamic_cast<RequetConnectionToLegacyServer*>(msg)) {

            if (isMN) {

                cout
                        << "Netzwerkschicht des MN meldet ein Paket, dessen Server noch nicht auf ProxyUnloading-Funktionalität hin überprüft wurde"
                        << endl;

                IPvXAddress ha = IPAddressResolver().resolve("HA");
                RequetConnectionToLegacyServer* messageAnHA = check_and_cast<
                        RequetConnectionToLegacyServer *>(msg);
                sendToUDPMCOA(messageAnHA, localPort, ha, 2000, true);
                return;
            }

            if (isHA) {
                RequetConnectionToLegacyServer* messageToCN = check_and_cast<
                        RequetConnectionToLegacyServer *>(msg);
                cout
                        << "HA hat eine RequestForConnectionToLegacyServer-Anfrage vom MN:"
                        << messageToCN->getSrcAddress() << " mit Destination: "
                        << messageToCN->getDestAddress()
                        << " und FlowSourceAdresse: "
                        << messageToCN->getFlowSourceAddress() << " erhalten"
                        << endl;

                IPvXAddress cn = IPvXAddress();
                cn.set(messageToCN->getDestAddress());

                //create a copy for the timeout if necessary
                RequetConnectionToLegacyServer* messageForTimeOut =
                        messageToCN->dup();

                messageToCN->removeControlInfo(); //new ipv6 control info of the home Agent is needed to send the data properly to the correspondent node
                sendToUDPMCOA(messageToCN, localPort, cn, 2000, true);

                //Now Start the Timer for Time-Out - if the CN does not respond !!!
                simtime_t timeOutDelay = (*proxyRequestForConnectionTimeOut);
                timeOutMessage = new cMessage("TimeOut - CN did not respond");
                //timer->setContextPointer(d);
                timeOutMessage->setKind(REQUEST_CONNECTION_HA_TIMEOUT);
                timeOutMessage->addObject(messageForTimeOut);
                scheduleAt(simTime() + timeOutDelay, timeOutMessage);

                //cout << "ProxyUnloadingHA-SimTime: " << simTime() << endl;
                return;
            }
            if (isCN) {
                // cout << "ProxyUnloadingCN-SimTime: " << simTime() << endl;

                RequetConnectionToLegacyServer* messageFromHA = check_and_cast<
                        RequetConnectionToLegacyServer *>(msg);
                cout << "Nun hat auch der CN mit der Adresse:"
                        << messageFromHA->getDestAddress()
                        << " die RequetConnectionToLegacyServer-Nachricht erhalten"
                        << endl;
                cout << "Absender war: " << messageFromHA->getSrcAddress()
                        << endl;

                //update the FlowBindingTable with this Information now for later Processing - If CN is capable of dealing with the ProxyUnloading-Protocol
                if (isCapableCN) {
                    cout
                            << "CN supports the protocol and is actualizing his own FlowBindingTable"
                            << endl;

                    //message to the network layer - to update there the FlowBindingTable
                    send(messageFromHA, "uDPControllAppConnection$o");

                    //send back a binding Acknowledgment to the HomeAgent and the MN who requested the call
                    ACK_RequestConnectionToLegacyServer* acknowledgmentToHA =
                            new ACK_RequestConnectionToLegacyServer();
                    acknowledgmentToHA->setSrcAddress(
                            messageFromHA->getSrcAddress());
                    acknowledgmentToHA->setDestAddress(
                            messageFromHA->getDestAddress());
                    acknowledgmentToHA->setFlowSourceAddress(
                            messageFromHA->getFlowSourceAddress());
                    acknowledgmentToHA->setDestPort(
                            messageFromHA->getDestPort());
                    acknowledgmentToHA->setSrcPort(messageFromHA->getSrcPort());
                    acknowledgmentToHA->setName(
                            "ACK_RequestConnectionToLegacyServer");
                    IPvXAddress ha = IPAddressResolver().resolve("HA");
                    sendToUDPMCOA(acknowledgmentToHA, localPort, ha, 2000,
                            true);

                    ACK_RequestConnectionToLegacyServer* acknowledgmentToMN =
                            new ACK_RequestConnectionToLegacyServer();
                    acknowledgmentToMN->setSrcAddress(
                            messageFromHA->getSrcAddress());
                    acknowledgmentToMN->setDestAddress(
                            messageFromHA->getDestAddress());
                    acknowledgmentToMN->setFlowSourceAddress(
                            messageFromHA->getFlowSourceAddress());
                    acknowledgmentToMN->setDestPort(
                            messageFromHA->getDestPort());
                    acknowledgmentToMN->setSrcPort(messageFromHA->getSrcPort());
                    acknowledgmentToMN->setName(
                            "ACK_RequestConnectionToLegacyServer");
                    IPvXAddress mn = IPvXAddress();
                    mn.set(messageFromHA->getSrcAddress());
                    sendToUDPMCOA(acknowledgmentToMN, localPort, mn, 2000,
                            true);

                }
                return;
            }

        }

        //**********************************************************************

        if (dynamic_cast<ACK_RequestConnectionToLegacyServer*>(msg)) {
            if (isHA || isMN) {
                if (isHA) { //cancel the Timeout-Timer first
                    cout
                            << "Time Out Timer des HA cancelt, da ACK_RequestConnectionToLegacyServer durch CN erhalten"
                            << endl;
                    cancelEvent (timeOutMessage);
                }
                //the flow-Binding-Table on the network layer has to be updated for the MN and the HA as well
                send(msg, "uDPControllAppConnection$o");
            }
            return;
        }

        //**********************************************************************

        if (dynamic_cast<FlowBindingUpdate*>(msg)) {
            if (isMN) {                //New Binding Update message
                cout << "Proxy Unloading Control App of MN sends "
                        << "FlowBindingUpdate to HA" << endl;
                FlowBindingUpdate* messageFromIPLayer = check_and_cast<
                        FlowBindingUpdate *>(msg);

                FlowBindingUpdate* flowBindingUpdateToHA =
                        new FlowBindingUpdate();
                flowBindingUpdateToHA->setName("Flow Binding Update");
                flowBindingUpdateToHA->setHomeAddress(
                        messageFromIPLayer->getHomeAddress());
                flowBindingUpdateToHA->setNewCoAdress(
                        messageFromIPLayer->getNewCoAdress());
                flowBindingUpdateToHA->setDestAddress(
                        messageFromIPLayer->getDestAddress());
                flowBindingUpdateToHA->setWasSendFromHA(false); //for avoiding infinite loop at HomeAgent

                //get Home Agents correct address:
                RoutingTable6* rt6 = RoutingTable6Access().get();
                //  rt6->getHomeNetHA_adr()

                IPvXAddress ha = IPAddressResolver().resolve("HA");
                //  sendToUDPMCOA(flowBindingUpdateToHA, localPort,  ha, 2000, true);
                sendToUDPMCOA(flowBindingUpdateToHA, localPort,
                        rt6->getHomeNetHA_adr(), 2000, true);
                //Timer anwerfen, wenn der HA ggf. die Nachricht nicht erhalten hat.

                return;
            }
            if (isHA) {
                cout
                        << "HA hat FlowBindingUpdate vom MN erhalten, aktualisiert seine eigene Tabelle und leitet dieses jetzt entsprechend an die CNs weiter"
                        << endl;

                FlowBindingUpdate* messageFromMN = check_and_cast<
                        FlowBindingUpdate *>(msg);

                //UPDATE OWN TABLE
                send(messageFromMN, "uDPControllAppConnection$o");

                //TODO

                //INFORM The CNs in the list of the update
                FlowBindingUpdate* flowBindingUpdateToCN =
                        new FlowBindingUpdate();
                flowBindingUpdateToCN->setName("FlowBindingUpdate");
                flowBindingUpdateToCN->setHomeAddress(
                        messageFromMN->getHomeAddress());
                flowBindingUpdateToCN->setNewCoAdress(
                        messageFromMN->getNewCoAdress());
                flowBindingUpdateToCN->setDestAddress(
                        messageFromMN->getDestAddress());
                flowBindingUpdateToCN->setWasSendFromHA(true);

                IPvXAddress ha = IPAddressResolver().resolve("HA");

                //TODO FIXEN !!!
                IPvXAddress dest = IPvXAddress();
                dest.set(messageFromMN->getDestAddress());
                IPvXAddress test = IPAddressResolver().resolve("CN[0]");
                cout << "Address of MN: " << messageFromMN->getHomeAddress()
                        << " Home Address des HomeAgents: " << ha
                        << " FlowBindingt_to_CN Destination Address: " << dest
                        << endl;
                if (!messageFromMN->getWasSendFromHA()) { //do not foreward self message --> otherwise infinite loop
                    sendToUDPMCOA(flowBindingUpdateToCN, localPort, test, 2000,
                            true);
                }

                //Confirm the received Flow Binding Update to the sending mobile node, so that he can use this new address as well
                //TODO - get the confirmation from the CN first !!! Than acknowledge it !!
                IPvXAddress *mn = new IPvXAddress(
                        messageFromMN->getNewCoAdress());
                ACK_FlowBindingUpdate* flowBindingAckToMN =
                        new ACK_FlowBindingUpdate();
                flowBindingAckToMN->setName("ACK_FlowBindingUpdate");
                flowBindingAckToMN->setHomeAddress(
                        messageFromMN->getHomeAddress());
                flowBindingAckToMN->setNewCoAdress(
                        messageFromMN->getNewCoAdress());
                flowBindingAckToMN->setDestAddress(
                        messageFromMN->getDestAddress());
                flowBindingAckToMN->setWasSendFromHA(true);
                sendToUDPMCOA(flowBindingAckToMN, localPort, *mn, 2000, true);

                return;
            }
            if (isCN) {
                //TODO Flow-Binding-Update-Ergänzen
                FlowBindingUpdate* messageFromHA = check_and_cast<
                        FlowBindingUpdate *>(msg);
                cout
                        << "CN hat nun auch das Flow-Binding-Update bekommen und bestätigt es an den HA"
                        << endl;

                //update of the own table
                send(messageFromHA, "uDPControllAppConnection$o");

                //Confirm the received Flow Binding Update to the Home Agent, so that he can inform the Mobile Node as well
                IPvXAddress ha = IPAddressResolver().resolve("HA");
                ACK_FlowBindingUpdate* flowBindingAckToHA =
                        new ACK_FlowBindingUpdate();
                flowBindingAckToHA->setName("ACK_FlowBindingUpdate");
                flowBindingAckToHA->setHomeAddress(
                        messageFromHA->getHomeAddress());
                flowBindingAckToHA->setNewCoAdress(
                        messageFromHA->getNewCoAdress());
                flowBindingAckToHA->setDestAddress(
                        messageFromHA->getDestAddress());
                flowBindingAckToHA->setWasSendFromHA(false);
                sendToUDPMCOA(flowBindingAckToHA, localPort, ha, 2000, true);

                return;
            }
        }

        //**********************************************************************

        if (dynamic_cast<ACK_FlowBindingUpdate*>(msg)) {
            if (isMN) {
                //TODO
                ACK_FlowBindingUpdate* messageFromHA = check_and_cast<
                        ACK_FlowBindingUpdate *>(msg);

                cout
                        << "MN hat BindingUpdate Nachricht bestätigt bekommen - nun kann er den Timer für erneutes Senden löschen TODO"
                        << endl;
                cout
                        << "MN aktualisiert seine eigene Tabelle mit den erhaltenen Informationen"
                        << endl;
                send(messageFromHA, "uDPControllAppConnection$o");
                return;
            }
            if (isHA) {
                ACK_FlowBindingUpdate* messageFromCN = check_and_cast<
                        ACK_FlowBindingUpdate *>(msg);

                cout
                        << "HA hat eine ACK_FlowBindingUpdate Nachricht vom CN empfangen."
                        << endl;
            }
        }

        //**********************************************************************

        //Andernfalls ist die Nachricht unbekannt und es wird lediglich der Name ausgegeben zum Debuggen
        if (isHA) {
            cout << "HA hat folgende unbekannte Nachricht erhalten:"
                    << msg->getName() << endl;
        }

        if (isCN)
            cout << "CN hat folgende unbekannte Nachricht erhalten:"
                    << msg->getName() << endl;
        if (isMN)
            cout << "MN hat folgende unbekannte Nachricht erhalten:"
                    << msg->getName() << endl;
        return;

    }
}
