//
// Generated file, do not edit! Created by opp_msgc 4.4 from applications/mcoa/proxy_enhanced/messages/control_messages/FlowBindingUpdate.msg.
//

#ifndef _FLOWBINDINGUPDATE_M_H_
#define _FLOWBINDINGUPDATE_M_H_

#include <omnetpp.h>

// opp_msgc version check
#define MSGC_VERSION 0x0404
#if (MSGC_VERSION!=OMNETPP_VERSION)
#    error Version mismatch! Probably this file was generated by an earlier version of opp_msgc: 'make clean' should help.
#endif



/**
 * Class generated from <tt>applications/mcoa/proxy_enhanced/messages/control_messages/FlowBindingUpdate.msg</tt> by opp_msgc.
 * <pre>
 * packet FlowBindingUpdate {
 *    string HomeAddress;
 *    string NewCoAdress;
 *    string DestAddress;
 *    bool wasSendFromHA;
 * }
 * </pre>
 */
class FlowBindingUpdate : public ::cPacket
{
  protected:
    opp_string HomeAddress_var;
    opp_string NewCoAdress_var;
    opp_string DestAddress_var;
    bool wasSendFromHA_var;

  private:
    void copy(const FlowBindingUpdate& other);

  protected:
    // protected and unimplemented operator==(), to prevent accidental usage
    bool operator==(const FlowBindingUpdate&);

  public:
    FlowBindingUpdate(const char *name=NULL, int kind=0);
    FlowBindingUpdate(const FlowBindingUpdate& other);
    virtual ~FlowBindingUpdate();
    FlowBindingUpdate& operator=(const FlowBindingUpdate& other);
    virtual FlowBindingUpdate *dup() const {return new FlowBindingUpdate(*this);}
    virtual void parsimPack(cCommBuffer *b);
    virtual void parsimUnpack(cCommBuffer *b);

    // field getter/setter methods
    virtual const char * getHomeAddress() const;
    virtual void setHomeAddress(const char * HomeAddress);
    virtual const char * getNewCoAdress() const;
    virtual void setNewCoAdress(const char * NewCoAdress);
    virtual const char * getDestAddress() const;
    virtual void setDestAddress(const char * DestAddress);
    virtual bool getWasSendFromHA() const;
    virtual void setWasSendFromHA(bool wasSendFromHA);
};

inline void doPacking(cCommBuffer *b, FlowBindingUpdate& obj) {obj.parsimPack(b);}
inline void doUnpacking(cCommBuffer *b, FlowBindingUpdate& obj) {obj.parsimUnpack(b);}


#endif // _FLOWBINDINGUPDATE_M_H_
