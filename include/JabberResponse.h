/*
 * XmppResponse.h
 *
 *  Created on: 16-Feb-2020
 *      Author: tstone10
 */

#ifndef JABBERRESPONSE_H_
#define JABBERRESPONSE_H_

#include <iostream>

class JabberResponse {
public:
    virtual void onXmppMessage(std::string strResp, std::string strFrom) = 0;
    virtual void onXmppConnect() = 0;
    virtual void onXmppDisconnect(int iErr) = 0;
};


#endif /* JABBERRESPONSE_H_ */
