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

#ifndef FLOWBINDINGENTRY_H_
#define FLOWBINDINGENTRY_H_

class FlowBindingEntry {
public:
    int srcPort;
    int destPort;
    const char* srcAddress;
    const char* destAddress;
    const char* flowSourceAddress;

    FlowBindingEntry();
    FlowBindingEntry(int srcPort, int destPort,const char* srcAddress,const char* destAddress);
    virtual ~FlowBindingEntry();

    const char* getDestAddress() const {
        return destAddress;
    }

    void setDestAddress(const char* destAddress) {
        this->destAddress = destAddress;
    }

    int getDestPort() const {
        return destPort;
    }

    void setDestPort(int destPort) {
        this->destPort = destPort;
    }

    const char* getSrcAddress() const {
        return srcAddress;
    }

    void setSrcAddress(const char* srcAddress) {
        this->srcAddress = srcAddress;
    }


    const char* getFlowSourceAddress() const {
         return flowSourceAddress;
     }

     void setFlowSourceAddress(const char* flowSourceAddress) {
         this->flowSourceAddress = flowSourceAddress;
     }

    int getSrcPort() const {
        return srcPort;
    }

    void setSrcPort(int srcPort) {
        this->srcPort = srcPort;
    }
};

#endif /* FLOWBINDINGENTRY_H_ */
