//
// Generated file, do not edit! Created by opp_msgc 4.4 from applications/mcoa/proxy_enhanced/messages/control_messages/ACK_RequestConnectionToLegacyServer.msg.
//

#ifndef _ACK_REQUESTCONNECTIONTOLEGACYSERVER_M_H_
#define _ACK_REQUESTCONNECTIONTOLEGACYSERVER_M_H_

#include <omnetpp.h>

// opp_msgc version check
#define MSGC_VERSION 0x0404
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of opp_msgc: 'make clean' should help.
#endif



/**
 * Class generated from <tt>applications/mcoa/proxy_enhanced/messages/control_messages/ACK_RequestConnectionToLegacyServer.msg</tt> by opp_msgc.
 * <pre>
 * packet ACK_RequestConnectionToLegacyServer {
 *     int SrcPort;
 *     int DestPort;
 *     string FlowSourceAddress;
 *     string SrcAddress;
 *     string DestAddress;
 * }
 * </pre>
 */
class ACK_RequestConnectionToLegacyServer : public ::cPacket
{
  protected:
    int SrcPort_var;
    int DestPort_var;
    opp_string FlowSourceAddress_var;
    opp_string SrcAddress_var;
    opp_string DestAddress_var;

  private:
    void copy(const ACK_RequestConnectionToLegacyServer& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const ACK_RequestConnectionToLegacyServer&);

  public:
    ACK_RequestConnectionToLegacyServer(const char *name=NULL, int kind=0);
    ACK_RequestConnectionToLegacyServer(const ACK_RequestConnectionToLegacyServer& other);
    virtual ~ACK_RequestConnectionToLegacyServer();
    ACK_RequestConnectionToLegacyServer& operator=(const ACK_RequestConnectionToLegacyServer& other);
    virtual ACK_RequestConnectionToLegacyServer *dup() const {return new ACK_RequestConnectionToLegacyServer(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual int getSrcPort() const;
    virtual void setSrcPort(int SrcPort);
    virtual int getDestPort() const;
    virtual void setDestPort(int DestPort);
    virtual const char * getFlowSourceAddress() const;
    virtual void setFlowSourceAddress(const char * FlowSourceAddress);
    virtual const char * getSrcAddress() const;
    virtual void setSrcAddress(const char * SrcAddress);
    virtual const char * getDestAddress() const;
    virtual void setDestAddress(const char * DestAddress);
};

inline void doPacking(cCommBuffer *b, ACK_RequestConnectionToLegacyServer& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, ACK_RequestConnectionToLegacyServer& obj) {obj.parsimUnpack(b);}


#endif // _ACK_REQUESTCONNECTIONTOLEGACYSERVER_M_H_