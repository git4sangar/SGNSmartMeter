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
#include "FileLogger.h"
#include "Utils.h"

MessageHandler *MessageHandler::pMsgH;

MessageHandler::MessageHandler() : log(Logger::getInstance()) {
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
        	log << "MessageHandler: Cur Ver " << curVer.getVerString() << ", New Ver " << newVer.getVerString() << std::endl;
        	log << "Message Handler: Cur Ver < New Ver" << std::endl;
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
            log << "MessageHandler: Triggering software update" << std::endl;

            std::pair<std::string, int> reqPair	= std::make_pair(strUrl, cmdNo);
            pHttpClient->pushToQ(reqPair);
        } catch(JsonException &jed) {
        	log << jed.what() << std::endl;
        }
    } else {
        log << "Message Handler: Current Version >= New Version" << std::endl;
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
        log << jed.what() << std::endl;
    }
    return newVersions;
}

void MessageHandler::reconnectJabber() {
	Config *pConfig	= Config::getInstance();
	JabberClient *pJabberClient   = JabberClient::getJabberClient();

	XmppDetails xmppDetails = pConfig->getXmppDetails();
	pJabberClient->xmppShutDown();
	sleep(5);
	int iRet = pJabberClient->connect("", xmppDetails.getClientJid().c_str(), xmppDetails.getClientPwd().c_str());
	log << "Returned " << iRet << std::endl;
	while(0 != iRet) {
		pJabberClient->xmppShutDown();
		sleep(2);
		log << "Connecting again in loop " << iRet << std::endl;
		iRet	= pJabberClient->connect("", xmppDetails.getClientJid().c_str(), xmppDetails.getClientPwd().c_str());
	}
}

void MessageHandler::sendHeartBeat() {
	std::string strBinFile	= std::string(TECHNO_SPURS_ROOT_PATH) + std::string(TECHNO_SPURS_BIN_FILE);

	if(strHeartBeat.empty()) {
		JsonFactory jsRoot, jsVer;
		jsRoot.addStringValue("process_name", "JabberClient");
		jsRoot.addIntValue("pid_of_process", getpid());
		jsRoot.addStringValue("run_command", strBinFile);

		jsVer.addIntValue("major", MAJOR_VERSION);
		jsVer.addIntValue("minor", MINOR_VERSION);
		jsVer.addIntValue("patch", PATCH_VERSION);

		jsRoot.addJsonObj("version", jsVer);
		strHeartBeat	= jsRoot.getJsonString();
	}

	log << "MessageHandler: Sending Heartbeat " << strHeartBeat << std::endl;
	Utils::sendPacket(HEART_BEAT_PORT, strHeartBeat);
}

void MessageHandler:: uploadLogs() {
    log << "Message Handler: Uploading Logs" << std::endl;
}

void MessageHandler:: enableLogLevel() {
    log << "Message Handler: Enabling log level" << std::endl;
}

void MessageHandler:: reboot() {
    log << "Message Handler: Rebooting system" << std::endl;
}

MessageHandler:: ~MessageHandler() {}

MessageHandler *MessageHandler::getInstance() {
    if(NULL == pMsgH) {
            pMsgH = new MessageHandler();
    }
    return pMsgH;
}

void MessageHandler::onXmppConnect() {
    log << "JabberClient: Xmpp connected \n" << std::endl;
}

void MessageHandler::onXmppDisconnect(int iErr) {
    log << "Xmpp disconnected \n" << std::endl;
    JsonFactory jsRoot;
    jsRoot.addStringValue("command", "reconnect_jabber");
    std::string strCmd	= jsRoot.getJsonString();
    pushToQ(strCmd);
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
    Logger &log = pThis->log;

    while(1) {
        pthread_mutex_lock(&pThis->qLock);
        if(pThis->msgQ.empty()) {
            log << "MessageHandler: Waiting for messages" << std::endl;
            pthread_cond_wait(&pThis->qCond, &pThis->qLock);
        }
        strMsg = pThis->msgQ.front();
        pThis->msgQ.pop();
        log << "MessageHandler: Got a message " << strMsg << std::endl;
        pthread_mutex_unlock(&pThis->qLock);
        try {
            JsonFactory jsRoot;
            jsRoot.setJsonString(strMsg);
            jsRoot.validateJSONAndGetValue("command", strCmd);
            log << "MessageHandler: Got command " << strCmd << std::endl;
            int iCmd    = pThis->msgStruct.getCommandVal(strCmd);
            switch(iCmd) {
                case SMART_METER_UPDATE:
                    pThis->smartMeterUpdate(strMsg);
                    break;

                case XMPP_RECONNECT:
                	pThis->reconnectJabber();
                	break;

                case HEART_BEAT:
                	pThis->sendHeartBeat();
                	break;

                case UPLOAD_LOGS:
                    pThis->uploadLogs();
                    break;

                case ENABLE_LOG_LEVEL:
                    pThis->enableLogLevel();
                    break;

                case REBOOT_SYSTEM:
                    pThis->reboot();
                    break;

                default:
                	log << "MessageHandler: Error: Unknown command " << strCmd << std::endl;
                	break;
                }
            } catch(JsonException &jed) {
                log << "MessageHandler: Error: Parsing JSON" << jed.what() << std::endl;
            }
    }
    return NULL;
}

void MessageHandler::onDownloadSuccess(int iResp, int iCmdNo) {
	std::pair<std::string, int> reqPair;
    std::string dwldFile= std::string(TECHNO_SPURS_ROOT_PATH) + std::string(TECHNO_SPURS_DOWNLOAD_FILE);
    log << "MessageHandler: Triggering extract request " << dwldFile << std::endl;
    FileHandler *pFH	= FileHandler::getInstance();
    reqPair				= std::make_pair(dwldFile, iCmdNo);
    pFH->pushToQ(reqPair);
}

void MessageHandler::onDownloadFailure(int iResp, int iCmdNo) {
    log << "MessageHandler: Download reqeust failed for CmdNo " << iCmdNo << std::endl;
}
















