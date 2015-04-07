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
    cout << "PROXY CONTROL APP of "<<humanReadableName<< " received a message" << endl;

    if (msg->isSelfMessage()) {
        if (msg->getKind() == MK_REMOVE_ADDRESS_PAIR) {
            MCoAUDPBase::treatMessage(msg);

            return; // and that's it!
        }


    } else {

        if (dynamic_cast<RequetConnectionToLegacyServer*>(msg)) {

            if (isMN) {

                cout
                        << "Netzwerkschicht des "<<humanReadableName<<" meldet ein Paket, dessen Server noch nicht auf ProxyUnloading-Funktionalität hin überprüft wurde"
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
                        << "HA hat eine RequestForConnectionToLegacyServer-Anfrage vom MN: "
                        << messageToCN->getSrcAddress() << " mit Destination: "
                        << messageToCN->getDestAddress()<<" erhalten"
                        << endl;

                //message to the network layer - to update the FlowBindingTable of HA
                send(messageToCN->dup(), "uDPControllAppConnection$o");


                //foreward the message to the CN - if he is capable - he will update his flowBinding-Table as well
                IPvXAddress cn = IPvXAddress();
                cn.set(messageToCN->getDestAddress());


                messageToCN->removeControlInfo(); //new ipv6 control info of the home Agent is needed to send the data properly to the correspondent node
                sendToUDPMCOA(messageToCN, localPort, cn, 2000, true);

                return;
            }
            if (isCN) {

                RequetConnectionToLegacyServer* messageFromHA = check_and_cast<
                        RequetConnectionToLegacyServer *>(msg);
                cout << "Nun hat auch"<<humanReadableName<<" mit der Adresse: "
                        << messageFromHA->getDestAddress()
                        << " die RequestConnectionToLegacyServer-Nachricht erhalten"
                        << endl;
                cout << "Ursprünglicher Absender war: " << messageFromHA->getSrcAddress()
                        << endl;

                //update the FlowBindingTable with this Information now for later Processing - If CN is capable of dealing with the ProxyUnloading-Protocol
                if (isCapableCN) {
                    cout
                            << "CN supports the protocol and is actualizing his own FlowBindingTable"
                            << endl;

                    //message to the network layer - to update there the FlowBindingTable
                    send(messageFromHA->dup(), "uDPControllAppConnection$o");

                    //ACK capability to the MN:
                    ACK_Request* messageACKtoMN = new ACK_Request();
                    messageACKtoMN->setName("ACK_Request");
                    messageACKtoMN->setSrcPort(messageFromHA->getSrcPort());
                    messageACKtoMN->setDestPort(messageFromHA->getDestPort());
                    messageACKtoMN->setSrcAddress(messageFromHA->getSrcAddress());
                    messageACKtoMN->setDestAddress(messageFromHA->getDestAddress());
                    sendToUDPMCOA(messageACKtoMN,localPort,messageFromHA->getSrcAddress(),2000,true);

                }
                return;
            }

        }


        //**********************************************************************

                if (dynamic_cast<ACK_Request*>(msg)) {
                    if (isMN) {
                        ACK_Request* messageACKfromCN =check_and_cast<
                                ACK_Request *>(msg);

                        cout<<humanReadableName<<" hat ein ACK_Request empfangen vom CN mit Absender Addresse: "<<messageACKfromCN->getDestAddress()<<endl;

                                //Update own table of the MN
                                                send(messageACKfromCN->dup(), "uDPControllAppConnection$o");

                                                return;
                    }

                }
        //**********************************************************************

        if (dynamic_cast<FlowBindingUpdate*>(msg)) {
            if (isMN) {

                //New Binding Update message
                cout << "Proxy Unloading Control App of "<<humanReadableName<<" sends "
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

               // IPvXAddress ha = IPAddressResolver().resolve("HA");
              //    sendToUDPMCOA(flowBindingUpdateToHA->dup(), localPort,  ha, 2000, true); //for MN[1] ????
               IPvXAddress testAddresse = IPvXAddress();
               testAddresse.set("2001:db8::2aa:1a2");
               sendToUDPMCOA(flowBindingUpdateToHA, localPort,
                        testAddresse, 2000, true);
              //  sendToUDPMCOA(flowBindingUpdateToHA, localPort,
                  //      rt6->getHomeNetHA_adr(), 2000, true);


                return;
            }
            if (isHA) {
                cout
                        << "HA hat FlowBindingUpdate vom MN erhalten, aktualisiert seine eigene Tabelle und leitet dieses jetzt entsprechend an die CNs weiter"
                        << endl;

                FlowBindingUpdate* messageFromMN = check_and_cast<
                        FlowBindingUpdate *>(msg);

                //Update own table of the HA
                send(messageFromMN->dup(), "uDPControllAppConnection$o");

                //INFORM The CNs in the list of the update
                //!!! TODO FIXEN !!! --> es müssen entsprechend nur die CNs mit der passenden Policy aktualisiert werden !!!

                FlowBindingUpdate* flowBindingUpdateToCN =
                        messageFromMN->dup();

                flowBindingUpdateToCN->setWasSendFromHA(true);

                IPvXAddress ha = IPAddressResolver().resolve("HA");

                IPvXAddress dest = IPvXAddress();
                dest.set(messageFromMN->getDestAddress());

                //!!! TODO FIXEN !!! - hier müsste auf die Tabelle zugegriffen werden, die dem HA zur Verfügung steht !!!
                IPvXAddress test = IPAddressResolver().resolve("CN[0]");
                cout << "Address of MN: " << messageFromMN->getHomeAddress()
                        << " Home Address des HomeAgents: " << ha
                        << " FlowBindingt_to_CN Destination Address: " << dest
                        << endl;
                if (!messageFromMN->getWasSendFromHA()) { //do not foreward self message --> otherwise infinite loop
                    sendToUDPMCOA(flowBindingUpdateToCN, localPort, test, 2000,
                            true);
                }

               return;
            }
            if (isCN && isCapableCN) {

                FlowBindingUpdate* messageFromHA = check_and_cast<
                        FlowBindingUpdate *>(msg);
                cout
                        << "CN hat nun auch das Flow-Binding-Update bekommen und aktualisiert damit seine Tabelle"
                        << endl;

                //update of the own table of CN
                send(messageFromHA, "uDPControllAppConnection$o");

               return;
            }
        }


        //**********************************************************************

        //otherwise the message is unkown and will only be outputed for debugging reasons
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

