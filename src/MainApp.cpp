/*
 * MainApp.cpp
 *
 *  Created on: 16-Feb-2020
 *      Author: tstone10
 */
//sgn

#include <iostream>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include "Constants.h"
#include "JabberClient.h"
#include "MessageHandler.h"
#include "HttpClient.h"
#include "FileHandler.h"
#include "FileLogger.h"

void *heartBeat(void *);
int main() {

	//	First and foremost, wait for 2 mins.
	//	Let the environment get settled & let all the Smartmeter processes get up & running
	//sleep(WAIT_TIME_SECs);

	Logger &log = Logger::getInstance();

	log << "Starting Application" << std::endl;

	pthread_t heartBeatThread;
	pthread_create(&heartBeatThread, NULL, &heartBeat, NULL);
	pthread_detach(heartBeatThread);

    Config *pConfig	= Config::getInstance();
    pConfig->parseCurVersions();

    if(pConfig->parseXmppDetails()) {
    	XmppDetails xmppDetails	= pConfig->getXmppDetails();
    	MessageHandler *pMsgH	= MessageHandler::getInstance();

    	//	Both Jabber & Http clients are dependent on Message Handler for handling responses
    	//	So this order
        JabberClient *pJabberClient   = JabberClient::getJabberClient();
        pJabberClient->subscribeNotification(pMsgH);

        HttpClient *pHttpClient	= HttpClient::getInstance();
        pHttpClient->subscribeListener(pMsgH);

        //	File handler uses xmpp details. so launch it finally
        FileHandler::getInstance();

    	//	Now all set to launch the Jabber client
        int iRet = pJabberClient->connect("", xmppDetails.getClientJid().c_str(), xmppDetails.getClientPwd().c_str());
        while(0 != iRet) {
        	log << "Main: Error: XMPP connection to server failed with error " << iRet << std::endl;
        	pJabberClient->xmppShutDown();
        	sleep(5);
        	iRet	= pJabberClient->connect("", xmppDetails.getClientJid().c_str(), xmppDetails.getClientPwd().c_str());
        }
        log << "Main: XMPP connection to server through" << std::endl;

    	pJabberClient->startXmpp();
    }

    return 0;
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
		pJabberClient->sendMsgTo(strCmd, pConfig->getXmppDetails().getClientJid());
	}
}
