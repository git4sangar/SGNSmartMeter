/*
 * MessageStructure.cpp
 *
 *  Created on: 01-Mar-2020
 *      Author: tstone10
 */

//sgn

#include "MessageStructure.h"

MessageStructure::MessageStructure() {
    headers.insert(std::pair<std::string, int>("smart_meter_update",	SMART_METER_UPDATE));
    headers.insert(std::pair<std::string, int>("python_package_update",	PYTHON_PACKAGE_UPDATE));
    headers.insert(std::pair<std::string, int>("c_library_update",		C_LIBRARY_UPDATE));
    headers.insert(std::pair<std::string, int>("watch_dog_update",		WATCH_DOG_UPDATE));
    headers.insert(std::pair<std::string, int>("ca_cert_file_update",	CA_CERT_FILE_UPDATE));
    headers.insert(std::pair<std::string, int>("update_logs",       	UPLOAD_LOGS));
    headers.insert(std::pair<std::string, int>("enable_log_level",  	ENABLE_LOG_LEVEL));
    headers.insert(std::pair<std::string, int>("reboot_system",     	REBOOT_SYSTEM));
}

int MessageStructure::getCommandVal(std::string strCmd) {
    return headers[strCmd];
}
