/*
 * MessageStructure.cpp
 *
 *  Created on: 01-Mar-2020
 *      Author: tstone10
 */

//sgn

#include "MessageStructure.h"

MessageStructure::MessageStructure() {
    headers.insert(std::pair<std::string, int>("update_software",   UPDATE_SOFTWARE));
    headers.insert(std::pair<std::string, int>("update_logs",       UPLOAD_LOGS));
    headers.insert(std::pair<std::string, int>("enable_log_level",  ENABLE_LOG_LEVEL));
    headers.insert(std::pair<std::string, int>("reboot_system",     REBOOT_SYSTEM));
}

int MessageStructure::getCommandVal(std::string strCmd) {
    return headers[strCmd];
}
