//
// Generated file, do not edit! Created by opp_msgc 4.4 from networklayer/arp/ARPPacket.msg.
//

#ifndef _ARPPACKET_M_H_
#define _ARPPACKET_M_H_

#include <omnetpp.h>

// opp_msgc version check
#define MSGC_VERSION 0x0404
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of opp_msgc: 'make clean' should help.
#endif

// cplusplus {{
#include "IPAddress.h"
#include "MACAddress.h"


#define ARP_HEADER_BYTES   28
// }}



/**
 * Enum generated from <tt>networklayer/arp/ARPPacket.msg</tt> by opp_msgc.
 * <pre>
 * enum ARPOpcode
 * {
 * 
 *     ARP_REQUEST = 1;      
 *     ARP_REPLY = 2;        
 *     ARP_RARP_REQUEST = 3; 
 *     ARP_RARP_REPLY = 4;   
 * }
 * </pre>
 */
enum ARPOpcode {
    ARP_REQUEST = 1,
    ARP_REPLY = 2,
    ARP_RARP_REQUEST = 3,
    ARP_RARP_REPLY = 4
};

/**
 * Class generated from <tt>networklayer/arp/ARPPacket.msg</tt> by opp_msgc.
 * <pre>
 * packet ARPPacket
 * {
 *     int opcode enum(ARPOpcode);
 *     MACAddress srcMACAddress;
 *     MACAddress destMACAddress;
 *     IPAddress srcIPAddress;
 *     IPAddress destIPAddress;
 * }
 * </pre>
 */
class ARPPacket : public ::cPacket
{
  protected:
    int opcode_var;
    MACAddress srcMACAddress_var;
    MACAddress destMACAddress_var;
    IPAddress srcIPAddress_var;
    IPAddress destIPAddress_var;

  private:
    void copy(const ARPPacket& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const ARPPacket&);

  public:
    ARPPacket(const char *name=NULL, int kind=0);
    ARPPacket(const ARPPacket& other);
    virtual ~ARPPacket();
    ARPPacket& operator=(const ARPPacket& other);
    virtual ARPPacket *dup() const {return new ARPPacket(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual int getOpcode() const;
    virtual void setOpcode(int opcode);
    virtual MACAddress& getSrcMACAddress();
    virtual const MACAddress& getSrcMACAddress() const {return const_cast<ARPPacket*>(this)->getSrcMACAddress();}
    virtual void setSrcMACAddress(const MACAddress& srcMACAddress);
    virtual MACAddress& getDestMACAddress();
    virtual const MACAddress& getDestMACAddress() const {return const_cast<ARPPacket*>(this)->getDestMACAddress();}
    virtual void setDestMACAddress(const MACAddress& destMACAddress);
    virtual IPAddress& getSrcIPAddress();
    virtual const IPAddress& getSrcIPAddress() const {return const_cast<ARPPacket*>(this)->getSrcIPAddress();}
    virtual void setSrcIPAddress(const IPAddress& srcIPAddress);
    virtual IPAddress& getDestIPAddress();
    virtual const IPAddress& getDestIPAddress() const {return const_cast<ARPPacket*>(this)->getDestIPAddress();}
    virtual void setDestIPAddress(const IPAddress& destIPAddress);
};

inline void doPacking(cCommBuffer *b, ARPPacket& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, ARPPacket& obj) {obj.parsimUnpack(b);}


#endif // _ARPPACKET_M_H_
