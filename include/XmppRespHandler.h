/*
 * XmppRespHandler.h
 *
 *  Created on: 01-Mar-2020
 *      Author: tstone10
 */
//sgn

#ifndef XMPPRESPHANDLER_H_
#define XMPPRESPHANDLER_H_

#include "JabberResponse.h"
#include "MessageHandler.h"

class XmppRespHandler : public JabberResponse {
    MessageHandler *pMsgHandler;
    pthread_t msgHandlerThread;

public:
    XmppRespHandler();
    virtual ~XmppRespHandler();
    virtual void onXmppMessage(std::string strResp, std::string strFrom);
    virtual void onXmppConnect();
    virtual void onXmppDisconnect(int iErr);
};




#endif /* XMPPRESPHANDLER_H_ */
