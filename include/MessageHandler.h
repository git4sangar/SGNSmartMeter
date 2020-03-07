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

class MessageHandler : public HttpResponse {
    std::queue<std::string> msgQ;
    pthread_mutex_t qLock;
    pthread_cond_t qCond;

    MessageStructure msgStruct;

    MessageHandler();
    static MessageHandler *pMsgH;

public:
    virtual ~MessageHandler();

    void pushToQ(std::string msg);
    void updateSoftware(std::string strPkt);
    void uploadLogs();
    void enable_log_level();
    void reboot();
    std::vector<Version> getNewVersions(std::string strNewVersions);

    static MessageHandler *getInstance();
    static void *run(void *pUserData);

    virtual void onDownloadSuccess(int iResp);
    virtual void onDownloadFailure(int iResp);
};

#endif /* MESSAGEHANDLER_H_ */
