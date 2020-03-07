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
#include "JabberClient.h"
#include "XmppRespHandler.h"

int main() {
    JabberClient *pJabberClient;
    pJabberClient   = JabberClient::getJabberClient();

    XmppRespHandler *pXmppRespHndlr   = new XmppRespHandler();
    pJabberClient->subscribeNotification(pXmppRespHndlr);

    pJabberClient->connect("", "altimeter_0001@im.koderoot.net", "abcd1234");
    pJabberClient->startXmpp();
    return 0;
}
