/*
 * JsonException.h
 *
 *  Created on: 16-Feb-2020
 *      Author: tstone10
 */

#ifndef JSONEXCEPTION_H_
#define JSONEXCEPTION_H_

#include <string>

class JsonException {
    std::string strExcept;

public:
    JsonException(std::string str) : strExcept(str){}
    ~JsonException() {}

    const char *what() const throw() {
        return "JSON Exception";
    }
};



#endif /* JSONEXCEPTION_H_ */
