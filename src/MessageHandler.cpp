/*
 * MessageHandler.cpp
 *
 *  Created on: 29-Feb-2020
 *      Author: tstone10
 */


#include <iostream>
#include <queue>
#include <pthread.h>

//  socket
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netdb.h>

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

void *heartBeat(void *);
void *wdogRespThread(void *);

MessageHandler *MessageHandler::pMsgH;

MessageHandler::MessageHandler() : info_log(Logger::getInstance()) {
    qLock   = PTHREAD_MUTEX_INITIALIZER;
    qCond   = PTHREAD_COND_INITIALIZER;
    pthread_t msg_handler_thread;
    pthread_create(&msg_handler_thread, NULL, &run, this);
    pthread_detach(msg_handler_thread);
}

/*  {
		"command":"smart_meter_update",
		"command_no" : 10,
		"url":"https://technospurs.com/imageSets.zip",
		"processes" :
        [
          {"process_name" : "process1","version":1},
          {"process_name" : "process2","version":1},
          {"process_name" : "process3","version":1}
        ]
	}*/
void MessageHandler:: smartMeterUpdate(std::string strPkt) {
    std::vector<Version> newVersions;
    bool isUpdateRequired = false;

    Config *pConfig = Config::getInstance();
    newVersions     = getNewVersions(strPkt);

    //  Compare if current version is lesser than new
    for(Version newVer : newVersions) {
    	if(!newVer.getProcName().compare(WDOG_PROC_NAME)) continue;
        int curVer  = pConfig->getVerForProc(newVer.getProcName());
        info_log << "MessageHandler: Cur Ver " << curVer << ", New Ver " << newVer.getVer() << std::endl;
        if(curVer < newVer.getVer()) {
        	info_log << "Message Handler: Cur Ver < New Ver" << std::endl;
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
            info_log << "MessageHandler: Triggering software update" << std::endl;
            pHttpClient->smartMeterUpdate(cmdNo, strUrl);
        } catch(JsonException &jed) {
        	info_log << jed.what() << std::endl;
        }
    } else {
        info_log << "Message Handler: Current Version >= New Version" << std::endl;
    }
}

/*{
	"command"		: "jabber_client_update",
	"command_no"	: 11,
	"url"			: "https://technospurs.com/imageSets.zip",
	"version"		: 2
}*/
void MessageHandler::jabberClientUpdate(std::string strPkt) {
	if(strPkt.empty()) return;

	int cmdNo, ver;
	std::string strUrl;
	JsonFactory jsRoot;
	try {
		jsRoot.setJsonString(strPkt);
		jsRoot.validateJSONAndGetValue("url", strUrl);
		jsRoot.validateJSONAndGetValue("command_no", cmdNo);
		jsRoot.validateJSONAndGetValue("version", ver);
	} catch(JsonException &je) {}

	if(JABBER_CLIENT_VERSION < ver) {
		HttpClient *pHttpClient = HttpClient::getInstance();
		pHttpClient->jabberClientUpdate(cmdNo, strUrl);
	}
}

void MessageHandler::watchDogUpdate(std::string strPkt) {
	if(strPkt.empty()) return;

	int cmdNo, ver;
	std::string strUrl;
	JsonFactory jsRoot;
	try {
		jsRoot.setJsonString(strPkt);
		jsRoot.validateJSONAndGetValue("url", strUrl);
		jsRoot.validateJSONAndGetValue("command_no", cmdNo);
		jsRoot.validateJSONAndGetValue("version", ver);
	} catch(JsonException &je) {}

	Config *pConfig = Config::getInstance();
	if(pConfig->getVerForProc(WDOG_PROC_NAME) < ver) {
		HttpClient *pHttpClient = HttpClient::getInstance();
		pHttpClient->watchDogUpdate(cmdNo, strUrl);
	}
}

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
        info_log << jed.what() << std::endl;
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
	info_log << "Returned " << iRet << std::endl;
	while(0 != iRet) {
		pJabberClient->xmppShutDown();
		sleep(2);
		info_log << "Connecting again in loop " << iRet << std::endl;
		iRet	= pJabberClient->connect("", xmppDetails.getClientJid().c_str(), xmppDetails.getClientPwd().c_str());
	}
}

void MessageHandler::sendHeartBeat() {
	std::string strBinFile	= std::string(TECHNO_SPURS_ROOT_PATH) + std::string(TECHNO_SPURS_JABBER_FILE);

	if(strHeartBeat.empty()) {
		JsonFactory jsRoot;
		jsRoot.addStringValue("command", "heart_beat");
		jsRoot.addStringValue("process_name", "JabberClient");
		jsRoot.addIntValue("pid_of_process", getpid());
		jsRoot.addStringValue("run_command", strBinFile);
		jsRoot.addIntValue("version", JABBER_CLIENT_VERSION);
		strHeartBeat	= jsRoot.getJsonString();
	}

	info_log << "MessageHandler: Sending Heartbeat " << strHeartBeat << std::endl;
	Utils::sendPacket(WDOG_Tx_PORT, strHeartBeat);
}

void MessageHandler:: uploadLogs() {
    info_log << "Message Handler: Uploading Logs" << std::endl;
    info_log.uploadLogs();
    // Upload watchdog logs too
    Utils::sendPacket(WDOG_Tx_PORT, "{\"command\":\"upload_logs\"}");
}

void MessageHandler:: getVerReq() {
    info_log << "Message Handler: Requesting WDog all processes' version info" << std::endl;
    Utils::sendPacket(WDOG_Tx_PORT, "{\"command\":\"version_req\"}");
}

void MessageHandler:: reboot() {
    uploadLogs();
    info_log << "Message Handler: Going to reboot in 30 secs..." << std::endl;
    sleep(30);
    Utils::sendPacket(WDOG_Tx_PORT, "{\"command\":\"reboot\"}");
}

MessageHandler:: ~MessageHandler() {}

MessageHandler *MessageHandler::getInstance() {
    if(NULL == pMsgH) {
        pMsgH = new MessageHandler();
    }
    return pMsgH;
}

void MessageHandler::onXmppConnect() {
    info_log << "JabberClient: Xmpp connected \n" << std::endl;
    static bool bThreadsCreated = false;
    if(!bThreadsCreated) {
    	bThreadsCreated	= true;
		pthread_t heartBeatThread, recvWDogThread;

		pthread_create(&heartBeatThread, NULL, &heartBeat, NULL);
		pthread_detach(heartBeatThread);

		pthread_create(&recvWDogThread, NULL, &wdogRespThread, NULL);
		pthread_detach(recvWDogThread);
		Utils::sendPacket(WDOG_Tx_PORT, "{\"command\":\"version_req\"}");
    }
}

void MessageHandler::onXmppDisconnect(int iErr) {
    info_log << "Xmpp disconnected with stauts " << iErr << std::endl;
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
    Logger &log = pThis->info_log;

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

                case JABBER_CLIENT_UPDATE:
                	pThis->jabberClientUpdate(strMsg);
                	break;

                case WATCH_DOG_UPDATE:
                	pThis->watchDogUpdate(strMsg);
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

                case VERSION_REQUEST:
                    pThis->getVerReq();
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

void MessageHandler::onDownloadSuccess(int iResp, int iCmdNo, std::string strDstPath) {
	std::pair<std::string, int> reqPair;
	FileHandler *pFH	= FileHandler::getInstance();

    info_log << "MessageHandler: Triggering extract request " << std::endl;
    reqPair				= std::make_pair(strDstPath, iCmdNo);
    pFH->pushToQ(reqPair);
}

void MessageHandler::onDownloadFailure(int iResp, int iCmdNo) {
    info_log << "MessageHandler: Download request failed for CmdNo " << iCmdNo << std::endl;
}

void *heartBeat(void *pUserData) {
	std::string strCmd	= "{ \"command\" : \"heart_beat\" }";
	Logger &log = Logger::getInstance();

	JabberClient *pJabberClient	= JabberClient::getJabberClient();
	Config *pConfig	= Config::getInstance();

	log << "Starting Heartbeat thread" << std::endl;
	while(true) {
		sleep(25);
		log << "Main: Sending HeartBeat msg to self JID" << std::endl;
		pJabberClient->sendPresence(pConfig->getXmppDetails().getCPanelJid());
		pJabberClient->sendMsgTo(strCmd, pConfig->getXmppDetails().getClientJid());
	}
	return NULL;
}

void *wdogRespThread(void *pUserData) {
    struct sockaddr_in clientaddr;
    int clientlen, recvd;
    char buf[MAX_BUFF_SIZE];
    std::string strCmd, strData;

    JabberClient *pJabberClient	= JabberClient::getJabberClient();
    Config *pConfig	= Config::getInstance();
    Logger &log = Logger::getInstance();
    MessageStructure msgStruct;

    int sockfd	= Utils::prepareRecvSock(WDOG_Rx_PORT);
    clientlen   = sizeof(struct sockaddr_in);
    while(true) {
		JsonFactory jsRoot;
		recvd       = recvfrom(sockfd, buf, MAX_BUFF_SIZE, 0, (struct sockaddr *) &clientaddr, (socklen_t*)&clientlen);
		buf[recvd]  = '\0';
		log << "Main: Got WatchDog response " << buf << std::endl;
		try {
			jsRoot.setJsonString(std::string(buf));
			jsRoot.validateJSONAndGetValue("command", strCmd);

			//	Process command
			int iCmd    = msgStruct.getCommandVal(strCmd);
			switch(iCmd) {
			case VERSION_REQUEST:
				pConfig->parseCurVersions(std::string(buf));
				pJabberClient->sendMsgTo(pConfig->getCurVersions(), pConfig->getXmppDetails().getCPanelJid());
				break;

			case UPLOAD_LOGS:
				jsRoot.validateJSONAndGetValue("log_data", strData);
				HttpClient::getInstance()->uploadLogs(0, TECHNO_SPURS_WDOG_LOG, strData);
				break;
			}
		} catch(JsonException &je) { log << "Json Exception" << std::endl;}
    }
	return NULL;
}














