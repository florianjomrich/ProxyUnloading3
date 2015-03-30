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

void FlowBindingTable::insertNewFlowBindingEntry(
        RequetConnectionToLegacyServer *newFlowBindingEntry) {

    FlowBindingEntry newEntryToInsert = FlowBindingEntry();
    newEntryToInsert.setDestAddress(newFlowBindingEntry->getDestAddress());
    newEntryToInsert.setSrcAddress(newFlowBindingEntry->getSrcAddress());
    newEntryToInsert.setDestPort(newFlowBindingEntry->getDestPort());
    newEntryToInsert.setSrcPort(newFlowBindingEntry->getSrcPort());
    newEntryToInsert.setFlowSourceAddress(
            newFlowBindingEntry->getFlowSourceAddress());

    //to remove duplicates in the table - not updates
    if (!entryAlreadyExistsInTable(newEntryToInsert.destPort,
            newEntryToInsert.srcPort, newEntryToInsert.destAddress,
            newEntryToInsert.srcAddress)) {

        this->existingFlowBindingEntries.push_back(newEntryToInsert);
    }

}

void FlowBindingTable::insertNewFlowBindingEntry(
        ACK_RequestConnectionToLegacyServer *newFlowBindingEntry) {

    FlowBindingEntry newEntryToInsert = FlowBindingEntry();
    newEntryToInsert.setDestAddress(newFlowBindingEntry->getDestAddress());
    newEntryToInsert.setSrcAddress(newFlowBindingEntry->getSrcAddress());
    newEntryToInsert.setDestPort(newFlowBindingEntry->getDestPort());
    newEntryToInsert.setSrcPort(newFlowBindingEntry->getSrcPort());
    newEntryToInsert.setFlowSourceAddress(
            newFlowBindingEntry->getFlowSourceAddress());

    //  cout << "Flow Entry: src:" << newEntryToInsert.srcAddress << "dest: "
    //         << newEntryToInsert.destAddress << endl;

    if (!entryAlreadyExistsInTable(newEntryToInsert.destPort,
            newEntryToInsert.srcPort, newEntryToInsert.destAddress,
            newEntryToInsert.srcAddress)) {

        this->existingFlowBindingEntries.push_back(newEntryToInsert);
        // cout << "FRONT ENTRY IN TABLE: "
        //          << existingFlowBindingEntries.front().srcAddress << endl;
    }

}

//for deep packet inspection on MN-Side
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




//for the Replacement of Flow Source Address through CN
//Tabelle speichert Verbindungen vom MN (Src Adresse) zum CN (Destination Addresse)
bool FlowBindingTable::flowSourceAdressAlreadyExistsInTable(int& dport,int& sport, const char* destAddress,const char* sourceAddress){
    std::vector<FlowBindingEntry>::iterator it;

    for (it = existingFlowBindingEntries.begin();
            it < existingFlowBindingEntries.end(); it++) {
        if (it->destPort = dport && it->srcPort == sport && !strcmp(it->destAddress,sourceAddress) && !strcmp(it->flowSourceAddress, destAddress)) {
            return true;
        }
    }
    return false;
}



//it has to be checked first if the entry really exist by calling the above method first !
const char* FlowBindingTable::getCorrectDestinationAddressForConnection(int& dport,int& sport, const char* destAddress,const char* sourceAddress) {

    std::vector<FlowBindingEntry>::iterator it;

       for (it = existingFlowBindingEntries.begin();
               it < existingFlowBindingEntries.end(); it++) {
           if (it->destPort = dport && it->srcPort == sport && !strcmp(it->destAddress,sourceAddress) && !strcmp(it->flowSourceAddress, destAddress)) {
               cout << "GetCorrectDestination-Function:\n  DestinationAddress:"<< it->destAddress
                              << " SRC_ADDRESS: " << it->srcAddress << "FlowSourceAddress: "<<it->flowSourceAddress<<"  DPort: "
                              << it->destPort << " SPort: " << it->srcPort <<"\n\n"<< endl;
               return it->srcAddress;
           }
       }


}

//to set the address for further upper layer connections
const char* FlowBindingTable::getFlowSourceAddressForConnection(int& dport,
        int& sport, const char* destAddress, const char* sourceAddress) {

    std::vector<FlowBindingEntry>::iterator it;

    for (it = existingFlowBindingEntries.begin();
            it < existingFlowBindingEntries.end(); it++) {
        if (it->destPort == dport && it->srcPort == sport
                && !strcmp(it->srcAddress, sourceAddress)
                && !strcmp(it->destAddress, destAddress)) {
            return it->flowSourceAddress;
        }
    }

}

//to set the address for proxying context
const char* FlowBindingTable::getDestinationAddressForConnectionFromMNtoHA(int& dport,
        int& sport, const char* destAddress, const char* sourceAddress) {

    std::vector<FlowBindingEntry>::iterator it;

    for (it = existingFlowBindingEntries.begin();
            it < existingFlowBindingEntries.end(); it++) {
        if (it->destPort == dport && it->srcPort == sport
                && !strcmp(it->srcAddress, sourceAddress)
                && !strcmp(it->destAddress, destAddress)) {
            return it->destAddress;
        }
    }

}

//to set the address for proxying context
int FlowBindingTable::getDestinationPort(int& dport,
        int& sport, const char* destAddress, const char* sourceAddress) {

    std::vector<FlowBindingEntry>::iterator it;

    for (it = existingFlowBindingEntries.begin();
            it < existingFlowBindingEntries.end(); it++) {
        if (it->destPort == dport && it->srcPort == sport
                && !strcmp(it->srcAddress, sourceAddress)
                && !strcmp(it->destAddress, destAddress)) {
            return it->destPort;
        }
    }

}

void FlowBindingTable::updateExistingFlowBindingEntry(
        FlowBindingUpdate* update) {
    cout << "FlowBindingTable des CN/HA updatet sich selbst für HomeAddress:"
            << update->getHomeAddress() << " und New Care of Address: "
            << update->getNewCoAdress() << endl;

    std::vector<FlowBindingEntry>::iterator it;
    std::vector<FlowBindingEntry> updatedEntries; //hier werden die neuen Einträge gespeichert.

    for (it = existingFlowBindingEntries.begin();
            it < existingFlowBindingEntries.end(); it++) {
        if (!strcmp(update->getHomeAddress(), it->srcAddress)) {
            cout << "Übereinstimmung gefunden"<<endl;
            FlowBindingEntry newEntryToInsert = FlowBindingEntry();
            newEntryToInsert.setDestAddress(it->getDestAddress());
            newEntryToInsert.setSrcAddress(update->getNewCoAdress());
            newEntryToInsert.setDestPort(it->getDestPort());
            newEntryToInsert.setSrcPort(it->getSrcPort());
            newEntryToInsert.setFlowSourceAddress(it->getFlowSourceAddress());

            updatedEntries.push_back(newEntryToInsert);
        }
        //add all the others and also the old entry again.
        updatedEntries.push_back(*it);

    }

    //TEST AUSGABE WIEDER LÖSCHE
  //  int counter = 1;
    cout << "updatedEntries Size: " << updatedEntries.size() << endl;

  /*  for (it = updatedEntries.begin(); it < updatedEntries.end(); it++) {
        cout << "updatedEntries: " << counter
                << ".Tabelleneintrag  DestAddress:" << it->destAddress
                << " SrcAddress: " << it->srcAddress << "  DPort: "
                << it->destPort << " SPort: " << it->srcPort << endl;
        counter++;
    }*/

    //////

    existingFlowBindingEntries = updatedEntries;

}

void FlowBindingTable::updateExistingFlowBindingEntry(
        ACK_FlowBindingUpdate* update) {
    cout << "FlowBindingTable des MN updatet sich selbst für HomeAddress:"
            << update->getHomeAddress() << " und New Care of Address: "
            << update->getNewCoAdress() << endl;

    std::vector<FlowBindingEntry>::iterator it;
    std::vector<FlowBindingEntry> updatedEntries; //hier werden die neuen Einträge gespeichert.

    for (it = existingFlowBindingEntries.begin();
            it < existingFlowBindingEntries.end(); it++) {
         if (!strcmp(update->getHomeAddress(), it->srcAddress)) {
             cout << "Übereinstimmung gefunden"<<endl;
                FlowBindingEntry newEntryToInsert = FlowBindingEntry();
                newEntryToInsert.setDestAddress(it->getDestAddress());
                newEntryToInsert.setSrcAddress(update->getNewCoAdress());
                newEntryToInsert.setDestPort(it->getDestPort());
                newEntryToInsert.setSrcPort(it->getSrcPort());
                newEntryToInsert.setFlowSourceAddress(
                        it->getFlowSourceAddress());

                updatedEntries.push_back(newEntryToInsert);
            }
            //add all the others and also the old entry again.
            updatedEntries.push_back(*it);
    }

    existingFlowBindingEntries = updatedEntries;

}

void FlowBindingTable::printoutContentOftable() {

    std::vector<FlowBindingEntry>::iterator it;
    int counter = 1;

    for (it = existingFlowBindingEntries.begin();
            it < existingFlowBindingEntries.end(); it++) {
        cout << counter << ".Tabelleneintrag  DestAddress:" << it->destAddress
                << " SrcAddress: " << it->srcAddress << "  DPort: "
                << it->destPort << " SPort: " << it->srcPort << endl;
        counter++;
    }
}