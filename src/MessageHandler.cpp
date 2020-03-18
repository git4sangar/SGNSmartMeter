/*
 * MessageHandler.cpp
 *
 *  Created on: 29-Feb-2020
 *      Author: tstone10
 */


#include <iostream>
#include <queue>
#include <pthread.h>
#include "Constants.h"
#include "JabberClient.h"
#include "MessageHandler.h"
#include "MessageStructure.h"
#include "JsonException.h"
#include "JsonFactory.h"
#include "HttpClient.h"
#include "FileHandler.h"

MessageHandler *MessageHandler::pMsgH;

MessageHandler::MessageHandler() {
    qLock   = PTHREAD_MUTEX_INITIALIZER;
    qCond   = PTHREAD_COND_INITIALIZER;
    pthread_t msg_handler_thread;
    pthread_create(&msg_handler_thread, NULL, &run, this);
    pthread_detach(msg_handler_thread);
}

void MessageHandler:: smartMeterUpdate(std::string strPkt) {
    std::vector<Version> newVersions;
    bool isUpdateRequired = false;

    Config *pConfig = Config::getInstance();
    newVersions     = getNewVersions(strPkt);

    //  Compare if current version is lesser than new
    for(Version newVer : newVersions) {
        Version curVer  = pConfig->getVerForProc(newVer.getProcName());
        if(curVer < newVer) {
            std::cout << "Message Handler: Current Version < New Version" << std::endl;
            isUpdateRequired    = true;
            break;
        }
    }
    if(isUpdateRequired) {
        JsonFactory jsRoot;
        jsRoot.setJsonString(strPkt);
        std::string strUrl;
        int cmdNo = 0;
        try {
            jsRoot.validateJSONAndGetValue("url", strUrl);
            jsRoot.validateJSONAndGetValue("command_no", cmdNo);
            HttpClient *pHttpClient = HttpClient::getInstance();
            std::cout << "Message Handler: Triggering software update" << std::endl;

            std::pair<std::string, int> reqPair	= std::make_pair(strUrl, cmdNo);
            pHttpClient->pushToQ(reqPair);
        } catch(JsonException &jed) {
            std::cout << jed.what() << std::endl;
        }
    } else {
        std::cout << "Message Handler: Current Version >= New Version" << std::endl;
    }
}

/*  {
 *      "command":"smart_meter_update",
 *      "url":"https://www.somedomain.ext/path/to/zip/file.zip",
 *      "processes" :
        [
          {"name" : "process1","version":{"major":1, "minor":0, "patch":0}},
          {"name" : "process2","version":{"major":1, "minor":0, "patch":0}}
        ]
 *   }*/
std::vector<Version> MessageHandler:: getNewVersions(std::string strNewVersions) {
    std::vector<Version> newVersions;
    Version ver;
    JsonFactory jsRoot;
    json_t *jsProcs;
    int iLoop = 0;
    try {
        jsRoot.setJsonString(strNewVersions);
        jsRoot.validateJSONAndGetValue("processes", jsProcs);
        for(iLoop = 0; iLoop < jsRoot.getArraySize(jsProcs); iLoop++) {
            JsonFactory jsProc  = jsRoot.getObjAt(jsProcs, iLoop);
            ver.parseFromJsonFactory(jsProc);
            newVersions.push_back(ver);
        }

    } catch(JsonException &jed) {
        std::cout << jed.what() << std::endl;
    }
    return newVersions;
}

void MessageHandler:: uploadLogs() {
    std::cout << "Message Handler: Uploading Logs" << std::endl;
}

void MessageHandler:: enable_log_level() {
    std::cout << "Message Handler: Enabling log level" << std::endl;
}

void MessageHandler:: reboot() {
    std::cout << "Message Handler: Rebooting system" << std::endl;
}

MessageHandler:: ~MessageHandler() {}

MessageHandler *MessageHandler::getInstance() {
    if(NULL == pMsgH) {
            pMsgH = new MessageHandler();
    }
    return pMsgH;
}

void MessageHandler::onXmppConnect() {
    std::cout << "Xmpp connected \n" << std::endl;
}

void MessageHandler::onXmppDisconnect(int iErr) {
    std::cout << "Xmpp disconnected \n" << std::endl;
}

void MessageHandler::onXmppMessage(std::string strMsg, std::string strFrom) {
    pushToQ(strMsg);
}


void MessageHandler::pushToQ(std::string strMsg) {
    pthread_mutex_lock(&qLock);
    msgQ.push(strMsg);
    pthread_cond_signal(&qCond);
    pthread_mutex_unlock(&qLock);
}

void *MessageHandler::run(void *pUserData) {
    MessageHandler *pThis    = reinterpret_cast<MessageHandler *>(pUserData);
    std::string strMsg;
    std::string strCmd;
    while(1) {
        pthread_mutex_lock(&pThis->qLock);
        if(pThis->msgQ.empty()) {
            std::cout << "MessageHandler: Waiting for messages" << std::endl;
            pthread_cond_wait(&pThis->qCond, &pThis->qLock);
        }
        strMsg = pThis->msgQ.front();
        pThis->msgQ.pop();
        std::cout << "&& Got a message " << strMsg << std::endl;
        pthread_mutex_unlock(&pThis->qLock);
        try {
            JsonFactory jsRoot;
            jsRoot.setJsonString(strMsg);
            jsRoot.validateJSONAndGetValue("command", strCmd);
            int iCmd    = pThis->msgStruct.getCommandVal(strCmd);
            switch(iCmd) {
                case SMART_METER_UPDATE:
                    pThis->smartMeterUpdate(strMsg);
                    break;

                case UPLOAD_LOGS:
                    pThis->uploadLogs();
                    break;

                case ENABLE_LOG_LEVEL:
                    pThis->enable_log_level();
                    break;

                case REBOOT_SYSTEM:
                    pThis->reboot();
                    break;
                }
            } catch(JsonException &jed) {
                std::cout << jed.what() << std::endl;
            }
    }
    return NULL;
}

void MessageHandler::onDownloadSuccess(int iResp, int iCmdNo) {
	std::pair<std::string, int> reqPair;
    std::string dwldFile	= std::string(TECHNO_SPURS_ROOT_PATH) + std::string(TECHNO_SPURS_DOWNLOAD_FILE);
    FileHandler *pFH		= FileHandler::getInstance();
    reqPair			= std::make_pair(dwldFile, iCmdNo);
    pFH->pushToQ(reqPair);
}

void MessageHandler::onDownloadFailure(int iResp, int iCmdNo) {
    //  Inform server jid regarding failure
}
















