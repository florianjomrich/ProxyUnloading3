//
// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU Lesser General Public License as published by
// the Free Software Foundation, either version 3 of the License, or
// (at your option) any later version.
// 
// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU Lesser General Public License for more details.
// 
// You should have received a copy of the GNU Lesser General Public License
// along with this program.  If not, see http://www.gnu.org/licenses/.
// 

#include "FlowBindingTable.h"
#include "RequetConnectionToLegacyServer_m.h"

Define_Module(FlowBindingTable);

using std::cout;

FlowBindingTable::FlowBindingTable() {
    // TODO Auto-generated constructor stub

}

FlowBindingTable::~FlowBindingTable() {
    // TODO Auto-generated destructor stub
}

void FlowBindingTable::handleMessage(cMessage* msg) {
    if (msg->isSelfMessage()) {

    } else {

    }
}

//#############################################################

void FlowBindingTable::insertNewFlowBindingEntry(
        RequetConnectionToLegacyServer *newRequest) {

    FlowBindingEntry newEntryToInsert = FlowBindingEntry();

    newEntryToInsert.setDestAddress(newRequest->getDestAddress());
    newEntryToInsert.setSrcAddress(newRequest->getSrcAddress());
    newEntryToInsert.setDestPort(newRequest->getDestPort());
    newEntryToInsert.setSrcPort(newRequest->getSrcPort());
    cout<<"war noch hier"<<endl;

    newEntryToInsert.setLocalHostIdentifier(localHostCounter);

    newEntryToInsert.setIsActive(true);



    //to remove duplicates in the table - because of duplicate message sending - not updates
    if (!entryAlreadyExistsInTable(newEntryToInsert.destPort,
            newEntryToInsert.srcPort, newEntryToInsert.destAddress,
            newEntryToInsert.srcAddress)) {

        this->existingFlowBindingEntries.push_back(newEntryToInsert);

        //update the counter for the next mobile node who wants to register himself as well
        localHostCounter++;
    }

}


//#############################################################


//check fo the MN if he has to send a new legacy_server_request or not
bool FlowBindingTable::entryAlreadyExistsInTableForMobileNode(int& dport,int& sport, const char* destAddress){
    std::vector<FlowBindingEntry>::iterator it;

    for (it = existingFlowBindingEntries.begin();
            it < existingFlowBindingEntries.end(); it++) {
        if (it->destPort == dport && it->srcPort == sport
                    && !strcmp(it->destAddress, destAddress)) {
            return true;
        }
    }
    return false;
}


//#############################################################

//for deep packet inspection on CN-Side
bool FlowBindingTable::entryAlreadyExistsInTable(int& dport, int& sport,
        const char* destAddress, const char* sourceAddress) {

    std::vector<FlowBindingEntry>::iterator it;

    for (it = existingFlowBindingEntries.begin();
            it < existingFlowBindingEntries.end(); it++) {
        if (it->destPort == dport && it->srcPort == sport
                && !strcmp(it->srcAddress, sourceAddress)
                && !strcmp(it->destAddress, destAddress)) {
            return true;
        }
    }
    return false;
}

//#############################################################

//CN or HA should replace the destination address - relating to their own rules !!!
//it has to be checked first if the entry really exist by calling the above method first !
const char* FlowBindingTable::getCorrectDestinationAddressForConnection(int& dport,int& sport, const char* destAddress,const char* sourceAddress) {

    std::vector<FlowBindingEntry>::iterator it;

    int localHostIdentifier;

       for (it = existingFlowBindingEntries.begin();
               it < existingFlowBindingEntries.end(); it++) {

           if (it->destPort = dport && it->srcPort == sport && !strcmp(it->destAddress,sourceAddress) && !strcmp(it->srcAddress, destAddress)) {
               cout << "GetCorrectDestination-Function:\n  DestinationAddress: "<< it->destAddress
                              << " SourceAddress: " << it->srcAddress << "Local Host Identifier: "<<it->localHostIdentifier<<"  DPort: "
                              << it->destPort << " SPort: " << it->srcPort <<"\n\n"<< endl;

               localHostIdentifier = it->localHostIdentifier;

               //now get the active source Address of the Mobile Node for the connection:
               for (it = existingFlowBindingEntries.begin();
                          it < existingFlowBindingEntries.end(); it++) {

                   if ( (localHostIdentifier== it->localHostIdentifier) && it->isActive) {

                       return it->srcAddress;

                   }


               }



           }

       }

       cout<<"Eintrag mit den Werten: Dport:"<<dport<<" Sport:"<<sport<<" DestAddress:"<<destAddress<<" SourceAddress: "<<sourceAddress<<" wurde nicht gefunden.";
       return NULL;
}

void FlowBindingTable::updateExistingFlowBindingEntry(
        FlowBindingUpdate* update) {



    cout << "FlowBindingTable des "<< "CN/HA updatet sich selbst für HomeAddress:"
            << update->getHomeAddress() << " und New Care of Address: "
            << update->getNewCoAdress() << endl;

    std::vector<FlowBindingEntry>::iterator it;
    std::vector<FlowBindingEntry> updatedEntries; //hier werden die neuen Einträge gespeichert.

    int localHostidentifierWhoHasToBeUpdated = -1; //initially not set

    for (it = existingFlowBindingEntries.begin();
            it < existingFlowBindingEntries.end(); it++) {
        if (!strcmp(update->getHomeAddress(), it->srcAddress)) {
            cout << "Übereinstimmung gefunden"<<endl;
            FlowBindingEntry newEntryToInsert = FlowBindingEntry();
            newEntryToInsert.setDestAddress(it->getDestAddress());
            newEntryToInsert.setSrcAddress(update->getNewCoAdress());
            newEntryToInsert.setDestPort(it->getDestPort());
            newEntryToInsert.setSrcPort(it->getSrcPort());
            newEntryToInsert.setLocalHostIdentifier(it->getLocalHostIdentifier());
            newEntryToInsert.setIsActive(true);


            localHostidentifierWhoHasToBeUpdated= it->getLocalHostIdentifier();


            //no duplicates are handled
            if (!entryAlreadyExistsInTable(newEntryToInsert.destPort,
                      newEntryToInsert.srcPort, newEntryToInsert.destAddress,
                      newEntryToInsert.srcAddress)) {

                updatedEntries.push_back(newEntryToInsert);

            }


        }

        //set all other entries not active - who have the same localHostIdentifier-ID
        if(it->getLocalHostIdentifier() == localHostidentifierWhoHasToBeUpdated){
            it->isActive=false;
        }
        //add all the others and also the old entry again.
        updatedEntries.push_back(*it);

    }


    existingFlowBindingEntries = updatedEntries;

}



void FlowBindingTable::printoutContentOftable() {

    std::vector<FlowBindingEntry>::iterator it;


    for (it = existingFlowBindingEntries.begin();
            it < existingFlowBindingEntries.end(); it++) {
        cout << "MN["<<it->localHostIdentifier << "] Tabelleneintrag  DestAddress:" << it->destAddress
                << " SrcAddress: " << it->srcAddress << "  DPort: "
                << it->destPort << " SPort: " << it->srcPort <<" isActive: "<<it->isActive<< endl;

    }
}
