/*
 * HttpResponse.h
 *
 *  Created on: 16-Feb-2020
 *      Author: tstone10
 */

#ifndef HTTPRESPONSE_H_
#define HTTPRESPONSE_H_

#include <iostream>

class HttpResponse {
public:
    HttpResponse(){};
    virtual ~HttpResponse(){};
    virtual void onDownloadSuccess(int iRespCode, int iCmdNo, std::string strDstPath) = 0;
    virtual void onDownloadFailure(int iRespCode, int iCmdNo) = 0;
};


#endif /* HTTPRESPONSE_H_ */
