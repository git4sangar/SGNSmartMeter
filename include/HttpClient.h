/*
 * HttpClient.h
 *
 *  Created on: 16-Feb-2020
 *      Author: tstone10
 */

#ifndef HTTPCLIENT_H_
#define HTTPCLIENT_H_

#include <pthread.h>
#include "HttpResponse.h"
#include <iostream>
#include <vector>
#include <map>
#include <queue>
#include <utility>
#include <curl/curl.h>
#include "FileLogger.h"

#define	LOG_UPLOAD_URL	"http://ec2-3-135-62-120.us-east-2.compute.amazonaws.com:3000/api/logupload"

#define	HTTP_REQ_TYPE_DWLD	(1)
#define	HTTP_REQ_TYPE_UPLD	(2)

class HttpReqPkt {
	std::vector<std::string> headers;
	std::string strUrl, strFile, strData, strCmd;
	int reqType, cmdNo;

public:
	HttpReqPkt() : reqType {HTTP_REQ_TYPE_DWLD}, cmdNo{0} {}
	virtual ~HttpReqPkt() {}

	std::vector<std::string> getHeaders() { return headers; }
	std::string getUrl() { return strUrl; }
	int getReqType() { return reqType; }
	int getCmdNo() { return cmdNo; }
	std::string getUserData() { return strData; }
	std::string getTgtFile() { return strFile; }
	std::string getCmd() { return strCmd; }

	void addHeader(std::string strHdr) { headers.push_back(strHdr); }
	void setUrl(std::string url) { strUrl = url; }
	void setCmd(std::string cmd) { strCmd = cmd; }
	void setReqType(int iReqType) { reqType = iReqType; }
	void setCmdNo(int iCmdNo) { cmdNo = iCmdNo; }
	void setUserData(std::string strCnt) { strData = strCnt; }
	void setTgtFile(std::string file_name) { strFile = file_name; }
};

class HttpClient {
    HttpResponse *pListener;
    Logger &info_log;

    std::queue<HttpReqPkt *> reqQ;
    pthread_mutex_t mtxgQ;
    pthread_cond_t mtxgCond;
    static HttpClient *pHttpClient;
    static void *run(void *pThis);
    HttpReqPkt *genericUpdate(std::string strUrl, std::string strFolder, std::string strCmd, int cmdNo);
    HttpClient();

public:
    virtual ~HttpClient();

    void uploadLog(int cmdNo, std::string);
    void smartMeterUpdate(int cmdNo, std::string strUrl);
    void jabberClientUpdate(int cmdNo, std::string strUrl);

    void subscribeListener(HttpResponse *pObj) { pListener = pObj; }
    void pushToQ(HttpReqPkt *pReqPkt);
    HttpReqPkt *readFromQ();

    static HttpClient *getInstance();
    static size_t write_function(char *ptr, size_t size, size_t nmemb, void *userdata);
};



#endif /* HTTPCLIENT_H_ */
