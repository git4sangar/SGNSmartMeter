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

int main() {

	//	First and foremost, wait for 2 mins.
	//	Let the environment get settled & let all the Smartmeter processes get up & running
	//	sleep(WAIT_TIME_SECs);

    JabberClient *pJabberClient;
    pJabberClient   = JabberClient::getJabberClient();

    //	Start all the threads by getting the instances
    MessageHandler *pMsgH	= MessageHandler::getInstance();
    pJabberClient->subscribeNotification(pMsgH);

    HttpClient *pHttpClient	= HttpClient::getInstance();
    pHttpClient->subscribeListener(pMsgH);

    FileHandler::getInstance();

    Config *pConfig	= Config::getInstance();
    pConfig->parseCurVersions();

    if(pConfig->parseXmppDetails()) {
    	XmppDetails xmppDetails	= pConfig->getXmppDetails();
    	pJabberClient->connect("", xmppDetails.getClientJid().c_str(), xmppDetails.getClientPwd().c_str());
    	pJabberClient->startXmpp();
    }

    return 0;
}
