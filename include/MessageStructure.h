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
#include <array>

#define UPDATE_SOFTWARE     (0)
#define UPLOAD_LOGS         (1)
#define ENABLE_LOG_LEVEL    (2)
#define REBOOT_SYSTEM       (3)
#define MAX_COMMANDS        (4)

class MessageStructure {
    std::map<std::string, int> headers;

public:
    MessageStructure();
    int getCommandVal(std::string strCmd);
};

#endif /* MESSAGESTRUCTURE_H_ */
