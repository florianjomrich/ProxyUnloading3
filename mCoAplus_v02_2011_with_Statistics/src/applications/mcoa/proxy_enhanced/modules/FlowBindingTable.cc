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

void FlowBindingTable::initialize() {
    // isCNandNotCapable = par("isCNandNotCapable");
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
    //cout << "war noch hier" << endl;

    newEntryToInsert.setLocalHostIdentifier(localHostCounter);
    newEntryToInsert.setForThisConncectionCNisCapable(false);
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

//check for the MN if he has to send a new legacy_server_request or not
bool FlowBindingTable::entryAlreadyExistsInTableForMobileNode(int& dport,
        int& sport, const char* destAddress) {
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

bool FlowBindingTable::cnOfConnectionIsNotCapable(const char* destAddress) {

    std::vector<FlowBindingEntry>::iterator it;

    for (it = existingFlowBindingEntries.begin();
            it < existingFlowBindingEntries.end(); it++) {
        if (!strcmp(it->destAddress, destAddress)) {
            return !it->forThisConncectionCNisCapable;
        }
    }
    return false;

}

//#############################################################

//CN or HA should replace the destination address - relating to their own rules !!!
//it has to be checked first if the entry really exist by calling the above method first !
const char* FlowBindingTable::getCorrectDestinationAddressForConnection(
        int& dport, int& sport, const char* destAddress,
        const char* sourceAddress) {

    cout
            << "Home Agent or capable CN is replacing the Dest-Address with the current set CoA Address of the MN"
            << endl;

    std::vector<FlowBindingEntry>::iterator it;

    int localHostIdentifier;

    for (it = existingFlowBindingEntries.begin();
            it < existingFlowBindingEntries.end(); it++) {

        if (it->destPort == dport && it->srcPort == sport
                && !strcmp(it->destAddress, sourceAddress)
                && !strcmp(it->srcAddress, destAddress)) {
            cout << "GetCorrectDestination-Function:\n  DestinationAddress: "
                    << it->destAddress << " SourceAddress: " << it->srcAddress
                    << "Local Host Identifier: " << it->localHostIdentifier
                    << "  DPort: " << it->destPort << " SPort: " << it->srcPort
                    << "\n\n" << endl;

            localHostIdentifier = it->localHostIdentifier;

            //now get the active source Address of the Mobile Node for the connection:
            for (it = existingFlowBindingEntries.begin();
                    it < existingFlowBindingEntries.end(); it++) {

                if ((localHostIdentifier == it->localHostIdentifier)
                        && it->isActive) {

                    return it->srcAddress;

                }

            }

        }

    }

    cout << "Eintrag mit den Werten: Dport:" << dport << " Sport:" << sport
            << " DestAddress:" << destAddress << " SourceAddress: "
            << sourceAddress << " wurde nicht gefunden.";
    return NULL;
}

void FlowBindingTable::updateExistingFlowBindingEntry(
        FlowBindingUpdate* update) {

    cout << "FlowBindingTable des "
            << "CN/HA updatet sich selbst für HomeAddress:"
            << update->getHomeAddress() << " und New Care of Address: "
            << update->getNewCoAdress() << endl;

    std::vector<FlowBindingEntry>::iterator it;
    std::vector<FlowBindingEntry> updatedEntries; //hier werden die neuen Einträge gespeichert.

    int localHostidentifierWhoHasToBeUpdated = -1; //initially not set

    //first get the correct local host identifier whichs host entries have to be updated
    for (it = existingFlowBindingEntries.begin();
            it < existingFlowBindingEntries.end(); it++) {
        if (!strcmp(update->getHomeAddress(), it->srcAddress)) {
            localHostidentifierWhoHasToBeUpdated = it->getLocalHostIdentifier();
            break;
        }
    }

    FlowBindingEntry newEntryToInsert;

    //get the information of the home address of the mobile node - for the new entry
    for (it = existingFlowBindingEntries.begin();
            it < existingFlowBindingEntries.end(); it++) {
        if (!strcmp(update->getHomeAddress(), it->srcAddress)) {
            cout << "Übereinstimmung gefunden" << " destPort: " << it->destPort
                    << " srcPort: " << it->srcPort << " Src-Address: "
                    << it->srcAddress << " DestAddress: " << it->destAddress
                    << " CN is capable: " << it->forThisConncectionCNisCapable
                    << endl;
            newEntryToInsert = FlowBindingEntry();
            newEntryToInsert.setDestAddress(it->getDestAddress());
            newEntryToInsert.setSrcAddress(update->getNewCoAdress());
            newEntryToInsert.setDestPort(it->getDestPort());
            newEntryToInsert.setSrcPort(it->getSrcPort());
            newEntryToInsert.setLocalHostIdentifier(
                    it->getLocalHostIdentifier());
            newEntryToInsert.setForThisConncectionCNisCapable(
                    it->getForThisConncectionCNisCapable());
            newEntryToInsert.setIsActive(true);

            //no duplicates are handled
            if (entryAlreadyExistsInTable(newEntryToInsert.destPort,
                    newEntryToInsert.srcPort, newEntryToInsert.destAddress,
                    newEntryToInsert.srcAddress)) {
                break;
            } else { //add the new entry:
                updatedEntries.push_back(newEntryToInsert);
            }

        }
    }

    //else
    //set all other entries not active - who have the same localHostIdentifier-ID
    //but are not the same entry - if entry already exists set it active again
    for (it = existingFlowBindingEntries.begin();
            it < existingFlowBindingEntries.end(); it++) {

        if ((it->getLocalHostIdentifier()
                == localHostidentifierWhoHasToBeUpdated)) {
            it->isActive = false;
            if (newEntryToInsert.destPort == it->destPort
                    && newEntryToInsert.srcPort == it->srcPort
                    && !strcmp(newEntryToInsert.destAddress, it->destAddress)
                    && !strcmp(newEntryToInsert.srcAddress, it->srcAddress)) {
                //cout<<"SETZE AKTIV"<<endl;
                it->isActive = true;
            }
        }
        //add all the others and also the old entry again.
        updatedEntries.push_back(*it);

    }

    //replace old table with new table
    existingFlowBindingEntries = updatedEntries;

}

void FlowBindingTable::setEntryActive(const char* ipAddressThatShouldBeActive) {

    std::vector<FlowBindingEntry>::iterator it;

    int localHostIdentifierToCareAboutNow;

    //set the existing fitting entry active
    for (it = existingFlowBindingEntries.begin();
            it < existingFlowBindingEntries.end(); it++) {

        if (!strcmp(it->srcAddress, ipAddressThatShouldBeActive)) {
            cout << "AKTIV gesetzt" << endl;
            it->isActive = true;
            localHostIdentifierToCareAboutNow = it->localHostIdentifier;

            //set all the other entries to inactive now
            std::vector<FlowBindingEntry>::iterator it2;
            for (it2 = existingFlowBindingEntries.begin();
                    it2 < existingFlowBindingEntries.end(); it2++) {
                if (localHostIdentifierToCareAboutNow == it2->localHostIdentifier && strcmp(it2->srcAddress, ipAddressThatShouldBeActive)) {
                    it2->isActive = false;

                }

            }

        }

    }



}

void FlowBindingTable::updateEntriesWithNewCapableCN(const char* addressOfCN) {
    std::vector<FlowBindingEntry>::iterator it;
    std::vector<FlowBindingEntry> updatedEntries; //hier werden die neuen Einträge gespeichert.

    cout << "ADDRESSES OF CN: " << addressOfCN << endl;
    for (it = existingFlowBindingEntries.begin();
            it < existingFlowBindingEntries.end(); it++) {
        if (!strcmp(it->destAddress, addressOfCN)) {
            it->forThisConncectionCNisCapable = true;
        }
        updatedEntries.push_back(*it);
    }

    //replace old table with new table
    existingFlowBindingEntries = updatedEntries;

    //just for Control
    cout << "Show new capable CN's in list of MN now: " << endl;
    this->printoutContentOftable();

}

std::vector<FlowBindingEntry> FlowBindingTable::getCNsToBeInformed(FlowBindingUpdate* receivedFlowBindingUpdate){

    std::vector<FlowBindingEntry>::iterator it;
    std::vector<FlowBindingEntry> entriesToInform;

    for (it = existingFlowBindingEntries.begin();
            it < existingFlowBindingEntries.end(); it++) {
        if(!strcmp(receivedFlowBindingUpdate->getNewCoAdress(),it->srcAddress)){
            entriesToInform.push_back(*it);
        }
    }
    return entriesToInform;
}


void FlowBindingTable::setIPAddressActive(SetAddressActive* fromHA){
    std::vector<FlowBindingEntry>::iterator it;

    cout<<"Flow Binding Table will aktive Adresse ändern zu: "<<fromHA->getAddressToBeSetActive()<<endl;
    //activate given IP Address
    int localHostIdentifier;
    for (it = existingFlowBindingEntries.begin();
            it < existingFlowBindingEntries.end(); it++) {
        if(!strcmp(it->getSrcAddress(),fromHA->getAddressToBeSetActive())){
            localHostIdentifier = it->getLocalHostIdentifier();
            it->setIsActive(true);
            cout<<"Übereinstimmung gefunden"<<endl;
        }
    }

    //deactivate everything else
    for (it = existingFlowBindingEntries.begin();
              it < existingFlowBindingEntries.end(); it++) {
          if(strcmp(it->getSrcAddress(),fromHA->getAddressToBeSetActive()) && localHostIdentifier == it->localHostIdentifier){
              it->setIsActive(false);
          }
      }


    printoutContentOftable();
}

void FlowBindingTable::printoutContentOftable() {

    std::vector<FlowBindingEntry>::iterator it;

    for (it = existingFlowBindingEntries.begin();
            it < existingFlowBindingEntries.end(); it++) {
        cout << "MN[" << it->localHostIdentifier
                << "] Tabelleneintrag  DestAddress:" << it->destAddress
                << " SrcAddress: " << it->srcAddress << "  DPort: "
                << it->destPort << " SPort: " << it->srcPort << " isActive: "
                << it->isActive << " CN is capable: "
                << it->forThisConncectionCNisCapable << endl;

    }
}
