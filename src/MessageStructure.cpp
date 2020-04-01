/*
 * MessageStructure.cpp
 *
 *  Created on: 01-Mar-2020
 *      Author: tstone10
 */

//sgn

#include "MessageStructure.h"

MessageStructure::MessageStructure() {
    headers["smart_meter_update"]	= SMART_METER_UPDATE;
    headers["python_package_update"]	= PYTHON_PACKAGE_UPDATE;
    headers["jabber_client_update"]	= JABBER_CLIENT_UPDATE;
    headers["watchdog_update"]		= WATCH_DOG_UPDATE;
    headers["ca_cert_file_update"]	= CA_CERT_FILE_UPDATE;
    headers["upload_logs"]		= UPLOAD_LOGS;
    headers["version_req"]		= VERSION_REQUEST;
    headers["reconnect_jabber"]		= XMPP_RECONNECT;
    headers["reboot_system"]		= REBOOT_SYSTEM;
    headers["heart_beat"]		= HEART_BEAT;
}

int MessageStructure::getCommandVal(std::string strCmd) {
	int iVal = MAX_COMMANDS;

	try {
		iVal = headers.at(strCmd);
	} catch (const std::out_of_range& oor) {}
    return iVal;
}
