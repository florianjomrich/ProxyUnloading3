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



void FlowBindingTable::handleMessage(cMessage* msg)
{
    if (msg->isSelfMessage())
    {


    }
    else
    {
        /*
         * A new entry has to be created
         */
        if (dynamic_cast<RequetConnectionToLegacyServer*>(msg)) {

        }
    }
}

void  FlowBindingTable::insertNewFlowBindingEntry(RequetConnectionToLegacyServer *newFlowBindingEntry){
    FlowBindingEntry newEntryToInsert = FlowBindingEntry();
    newEntryToInsert.setDestAddress(newFlowBindingEntry->getDestAddress());
    newEntryToInsert.setSrcAddress(newFlowBindingEntry->getSrcAddress());
    newEntryToInsert.setDestPort(newFlowBindingEntry->getDestPort());
    newEntryToInsert.setSrcPort(newFlowBindingEntry->getSrcPort());
    newEntryToInsert.setFlowSourceAddress(newFlowBindingEntry->getFlowSourceAddress());

    this->existingFlowBindingEntries.push_back(newEntryToInsert);


}

void  FlowBindingTable::insertNewFlowBindingEntry(ACK_RequestConnectionToLegacyServer *newFlowBindingEntry){
    FlowBindingEntry newEntryToInsert = FlowBindingEntry();
    newEntryToInsert.setDestAddress(newFlowBindingEntry->getDestAddress());
    newEntryToInsert.setSrcAddress(newFlowBindingEntry->getSrcAddress());
    newEntryToInsert.setDestPort(newFlowBindingEntry->getDestPort());
    newEntryToInsert.setSrcPort(newFlowBindingEntry->getSrcPort());
    newEntryToInsert.setFlowSourceAddress(newFlowBindingEntry->getFlowSourceAddress());


    this->existingFlowBindingEntries.push_back(newEntryToInsert);


}

//check if an entry already exists:
bool FlowBindingTable::entryAlreadyExistsInTable(const char* flowSourceAdress){
    const bool srcPortFound = this->srcPorts.find(flowSourceAdress) != srcPorts.end();
    const bool destPortFound = this->destPorts.find(flowSourceAdress) != destPorts.end();
    const bool srcAddressFound = this->srcAdresses.find(flowSourceAdress) != srcAdresses.end();
    const bool destAddressFound = this->destAdresses.find(flowSourceAdress) != destAdresses.end();

    cout<<"FlowBinding entry found:"<<(srcPortFound && destPortFound && srcAddressFound && destAddressFound)<<endl;

    return (srcPortFound && destPortFound && srcAddressFound && destAddressFound);
}
//reverse check
bool FlowBindingTable::entryAlreadyExistsInTable(int& dport,int& sport, const char* destAddress,const char* sourceAddress,const char* flowSourceAddress){

    std::vector<FlowBindingEntry>::iterator it;

    for(it = existingFlowBindingEntries.begin(); it < existingFlowBindingEntries.end(); it++){
        if(it->destPort==dport && it->srcPort==sport && it->srcAddress == sourceAddress && it->destAddress==destAddress && it->flowSourceAddress==flowSourceAddress){
            return true;
        }
    }
    return false;
}

//it has to be checked first if the entry really exist by calling the above method first !
FlowBindingEntry* FlowBindingTable::getFlowBindingEntryFromTable(const char* flowSourceAdress){



}


void FlowBindingTable::updateExistingFlowBindingEntry(FlowBindingUpdate* update){
    cout<<"FlowBindingTable updatet sich selbst"<<endl;
    update->getNewCoAdress();
    update->getHomeAddress();

    std::vector<FlowBindingEntry>::iterator it;
    std::vector<FlowBindingEntry> updatedEntries;//hier werden die neuen Einträge gespeichert.

    for(it = existingFlowBindingEntries.begin(); it < existingFlowBindingEntries.end(); it++){
           if(update->getHomeAddress()==it->srcAddress){
               FlowBindingEntry newEntryToInsert = FlowBindingEntry();
                   newEntryToInsert.setDestAddress(it->getDestAddress());
                   newEntryToInsert.setSrcAddress(update->getNewCoAdress());
                   newEntryToInsert.setDestPort(it->getDestPort());
                   newEntryToInsert.setSrcPort(it->getSrcPort());
                   newEntryToInsert.setFlowSourceAddress(it->getFlowSourceAddress());

                   updatedEntries.push_back(newEntryToInsert);
           }
       }



    for(it = updatedEntries.begin();it< updatedEntries.end();it++){
        existingFlowBindingEntries.push_back(*it);
    }

}


void FlowBindingTable::updateExistingFlowBindingEntry(ACK_FlowBindingUpdate* update){
    cout<<"FlowBindingTable des MN updatet sich selbst"<<endl;
    update->getNewCoAdress();
    update->getHomeAddress();

    std::vector<FlowBindingEntry>::iterator it;
    std::vector<FlowBindingEntry> updatedEntries;//hier werden die neuen Einträge gespeichert.

    for(it = existingFlowBindingEntries.begin(); it < existingFlowBindingEntries.end(); it++){
           if(update->getHomeAddress()==it->srcAddress){
               FlowBindingEntry newEntryToInsert = FlowBindingEntry();
                   newEntryToInsert.setDestAddress(it->getDestAddress());
                   newEntryToInsert.setSrcAddress(update->getNewCoAdress());
                   newEntryToInsert.setDestPort(it->getDestPort());
                   newEntryToInsert.setSrcPort(it->getSrcPort());
                   newEntryToInsert.setFlowSourceAddress(it->getFlowSourceAddress());

                   updatedEntries.push_back(newEntryToInsert);
           }
       }



    for(it = updatedEntries.begin();it< updatedEntries.end();it++){
        existingFlowBindingEntries.push_back(*it);
    }

}
