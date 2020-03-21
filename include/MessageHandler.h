/*
 * MsgHandler.h
 *
 *  Created on: 29-Feb-2020
 *      Author: tstone10
 */

#ifndef MESSAGEHANDLER_H_
#define MESSAGEHANDLER_H_

#include <iostream>
#include <queue>
#include <pthread.h>
#include "Constants.h"
#include "HttpResponse.h"
#include "MessageStructure.h"
#include "JabberResponse.h"
#include "FileLogger.h"

class MessageHandler : public HttpResponse, public JabberResponse {
    std::queue<std::string> msgQ;
    pthread_mutex_t qLock;
    pthread_cond_t qCond;
    Logger &log;
    std::string strHeartBeat;

    MessageStructure msgStruct;

    MessageHandler();
    static MessageHandler *pMsgH;

public:
    virtual ~MessageHandler();

    void pushToQ(std::string msg);
    void smartMeterUpdate(std::string strPkt);
    void uploadLogs();
    void enableLogLevel();
    void reboot();
    void reconnectJabber();
    void sendHeartBeat();
    std::vector<Version> getNewVersions(std::string strNewVersions);

    static MessageHandler *getInstance();
    static void *run(void *pUserData);

    //	Xmpp message handler
    void onXmppMessage(std::string strMsg, std::string strFrom);
    void onXmppConnect();
    void onXmppDisconnect(int iErr);

    //	Http response handler
    void onDownloadSuccess(int iResp, int iCmdNo);
    void onDownloadFailure(int iResp, int iCmdNo);
};

#endif /* MESSAGEHANDLER_H_ */
