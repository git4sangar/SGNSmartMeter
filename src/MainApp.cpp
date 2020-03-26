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

int main() {
	Logger &log = Logger::getInstance(); sleep(1);
	log << "Starting Application" << std::endl;

	Config *pConfig	= Config::getInstance(); sleep(1);
	if(!pConfig->parseXmppDetails()) {
		info_log << "Main: Error: Could not parse xmpp details" << std::endl;
		return 0;
	}

	JabberClient *pJabberClient	= JabberClient::getJabberClient();	sleep(1);
	MessageHandler *pMsgH		= MessageHandler::getInstance();	sleep(1);
	HttpClient *pHttpClient		= HttpClient::getInstance();		sleep(1);
	FileHandler *pFH		= FileHandler::getInstance();		sleep(1);

	//	All threads are started. Now connect Jabber
	XmppDetails xmppDetails	= pConfig->getXmppDetails();

	//	Subscribe for notifications
	pJabberClient->subscribeNotification(pMsgH);
	pHttpClient->subscribeListener(pMsgH);

	//	Now all set to launch the Jabber client
	int iRet = pJabberClient->connect("", xmppDetails.getClientJid().c_str(), xmppDetails.getClientPwd().c_str());
	while(0 != iRet) {
		info_log << "Main: Error: XMPP connection to server failed with error " << iRet << std::endl;
		pJabberClient->xmppShutDown();
		sleep(5);
		iRet	= pJabberClient->connect("", xmppDetails.getClientJid().c_str(), xmppDetails.getClientPwd().c_str());
	}
	info_log << "Main: XMPP connection to server through" << std::endl;

	pJabberClient->startXmpp();

    return 0;
}


