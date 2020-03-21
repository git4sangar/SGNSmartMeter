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
    headers["c_library_update"]		= C_LIBRARY_UPDATE;
    headers["watch_dog_update"]		= WATCH_DOG_UPDATE;
    headers["ca_cert_file_update"]	= CA_CERT_FILE_UPDATE;
    headers["update_logs"]		= UPLOAD_LOGS;
    headers["enable_log_level"]		= ENABLE_LOG_LEVEL;
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
