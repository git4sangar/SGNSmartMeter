/*
 * MsgParser.h
 *
 *  Created on: 29-Feb-2020
 *      Author: tstone10
 */

#ifndef MESSAGESTRUCTURE_H_
#define MESSAGESTRUCTURE_H_

#include <iostream>
#include <map>

#define SMART_METER_UPDATE	(0)
#define PYTHON_PACKAGE_UPDATE	(1)
#define JABBER_CLIENT_UPDATE	(2)
#define WATCH_DOG_UPDATE	(3)
#define CA_CERT_FILE_UPDATE	(4)
#define UPLOAD_LOGS         	(5)
#define GET_VERSION_INFO    	(6)
#define REBOOT_SYSTEM       	(7)
#define	BLANK_CMD		(9)
#define XMPP_RECONNECT		(10)
#define HEART_BEAT		(11)
#define MAX_COMMANDS        	(12)

class MessageStructure {
    std::map<std::string, int> headers;

public:
    MessageStructure();
    int getCommandVal(std::string strCmd);
};

#endif /* MESSAGESTRUCTURE_H_ */
