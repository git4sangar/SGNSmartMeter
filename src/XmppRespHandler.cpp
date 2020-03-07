/*sgn
 * XmppRespHandler.cpp
 *
 *  Created on: 01-Mar-2020
 *      Author: tstone10
 */

#include <iostream>
#include "XmppRespHandler.h"

XmppRespHandler::XmppRespHandler() {
    pMsgHandler = MessageHandler::getInstance();
    pthread_create(&msgHandlerThread, NULL, &MessageHandler::run, pMsgHandler);
}

XmppRespHandler::~XmppRespHandler() {

}

void XmppRespHandler::onXmppConnect() {
    std::cout << "Xmpp connected \n" << std::endl;
}

void XmppRespHandler::onXmppDisconnect(int iErr) {
    std::cout << "Xmpp disconnected \n" << std::endl;
}

void XmppRespHandler::onXmppMessage(std::string strResp, std::string strFrom) {
    std::cout << "Got from XMPP " << strResp << std::endl;
    pMsgHandler->pushToQ(strResp);
}
