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

#include "Proxy_Enhanced_MCoAVideoSrv.h"
#include "UDPControlInfo_m.h"
#include "IPAddressResolver.h"
#include "BindingUpdateInformationtoAPPmessageCN.h"
#include "RequestVideoStream_m.h"
#include "VideoMessage_m.h"

#define CN_APP_MESSAGE 50
#define PROXY_MESSAGE_FROM_CN_TO_MN 51

using std::cout;

Define_Module(Proxy_Enhanced_MCoAVideoSrv);

inline std::ostream& operator<<(std::ostream& out, const Proxy_Enhanced_MCoAVideoSrv::VideoStreamData& d) {
    out << "client=" << d.clientAddr << ":" << d.clientPort
        << "  size=" << d.videoSize << "  pksent=" << d.numPkSent << "  bytesleft=" << d.bytesLeft;
    return out;
}
Proxy_Enhanced_MCoAVideoSrv::Proxy_Enhanced_MCoAVideoSrv() {
	// TODO Auto-generated constructor stub

}

Proxy_Enhanced_MCoAVideoSrv::~Proxy_Enhanced_MCoAVideoSrv() {
	// TODO Auto-generated destructor stub
	for (unsigned int i=0; i<streamVector.size(); i++)
	        delete streamVector[i];
}

void Proxy_Enhanced_MCoAVideoSrv::initialize()
{
    cout<<"Initializing Proxy_Enhanced_MCoAVideoSrv module"<<endl;

	MCoAUDPBase::startMCoAUDPBase();

    waitInterval = &par("waitInterval");
    packetLen = &par("packetLen");
    videoSize = &par("videoSize");
    localPort = par("localPort");
    int destPort = par("destPort");
    //PROXY UNLOADING
    simtime_t startTime = par("startTime");
    seq_number_counter = 0;


    const char *address = par("destAddresses");
	IPvXAddress cliAddr = IPAddressResolver().resolve(address);
	if (cliAddr.isUnspecified())
	{
		EV << "Client address is unspecified, skip sending video stream request\n";
		return;
	}

    WATCH_PTRVECTOR(streamVector);

    bindToPort(localPort);

    PktSent.setName("MCOA VIDEO Packet Sent");



    //Support only for one client... extend if needed
    //FIXME:
    VideoStreamData *d = new VideoStreamData;
	d->clientAddr = cliAddr;
	d->clientPort = destPort;
	d->videoSize = (*videoSize);
	d->bytesLeft = d->videoSize;
	d->numPkSent = 0;
	d->seqTx = 0; //First Request
	streamVector.push_back(d);


   /* if (startTime>=0){
    	cMessage *timer = new cMessage("UDPVideoStart");
    	timer->setName("Proxy_Context_Message");
		//timer->setContextPointer(d);
		scheduleAt(startTime, timer);
    } */

   // cMessage *hurz = new cMessage("HURZ");
   // hurz->setKind(PROXY_CN_MESSAGE_TO_MOBILE_NODE);
   // scheduleAt(hurzTime,hurz);
}

void Proxy_Enhanced_MCoAVideoSrv::finish()
{
    //recordScalar("streams served", numStreams);
    //recordScalar("packets sent", numPkSent);
}

void Proxy_Enhanced_MCoAVideoSrv::handleMessage(cMessage *msg)
{
    if (msg->isSelfMessage())
    {
    	if (msg->getKind() == MK_REMOVE_ADDRESS_PAIR ){
			MCoAUDPBase::treatMessage(msg);

			return; // and that's it!
		}

    	//data send timer was finished
    	 if(dynamic_cast<RequestVideoStream*>(msg)){
    	     sendStreamData(msg);
    	 }



    }

    else{

        if(dynamic_cast<RequestVideoStream*>(msg)){
            cout<<"MCoASrv: Request for sending Video received"<<endl;

            RequestVideoStream* requestForVideoStream =  dynamic_cast<RequestVideoStream*>(msg);

            //Auslesen der Controll-Informationen - SRC und DEST IP usw. usf. zum Umsetzen des FlowSource_IP-Adressen-Konzeptes
            UDPControlInfo* myControllInfo = check_and_cast<UDPControlInfo*>(msg->getControlInfo());
            IPvXAddress srcIPAdresse = myControllInfo->getSrcAddr();
            cout<<"MCoASrv received Video-Request from SRC-IP-Adress: "<<srcIPAdresse<<" with sequence number:"<<requestForVideoStream->getSequenceNumber()<<endl;
            //TODO das hier noch mal fixen, das korrekt Daten gesendet werden dann


            cMessage *timer = new cMessage("UDPVideoStart");
                    //timer->setContextPointer(d);
            simtime_t interval = (*waitInterval);
                    scheduleAt(simTime()+interval, msg);



        }

    }


}

void Proxy_Enhanced_MCoAVideoSrv::sendStreamData(cMessage *msg)
{

    UDPControlInfo* myControllInfo = check_and_cast<UDPControlInfo*>(msg->getControlInfo());
                IPvXAddress srcIPAdresse = myControllInfo->getSrcAddr();

		cout<<"Src Adresse, an die Video geschickt wird: "<<myControllInfo->getSrcAddr()<<endl;
		IPvXAddress adresse = IPAddressResolver().resolve("MN[0]");
		VideoMessage* newVideoData = new VideoMessage();
		newVideoData->setName("Video Datei vom VideoSrv");
		newVideoData->setSequenceNumber(seq_number_counter);

		cout<<"MCoASrv-SimTime: "<<simTime()<<endl;
		sendToUDPMCOA(newVideoData, localPort,  adresse ,1000, true); //HIER GIBT ES PROBLEME !!!

		seq_number_counter++;

}

