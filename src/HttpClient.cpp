/*
 * HttpClient.cpp
 *
 *  Created on: 16-Feb-2020
 *      Author: tstone10
 */


#include <pthread.h>
#include <queue>
#include <string.h>
#include <curl/curl.h>
#include "HttpClient.h"
#include <sstream>
#include "FileLogger.h"
#include "Constants.h"

HttpClient *HttpClient::pHttpClient = NULL;

HttpClient :: HttpClient() : log(Logger::getInstance()) {
    mtxgQ       = PTHREAD_MUTEX_INITIALIZER;
    mtxgCond    = PTHREAD_COND_INITIALIZER;

    curl_global_init(CURL_GLOBAL_ALL);

    pthread_t tCurThread;
    pthread_create(&tCurThread, NULL, &run, this);
    pthread_detach(tCurThread);
}

HttpClient *HttpClient::getInstance() {
    if(NULL == pHttpClient) {
        pHttpClient = new HttpClient();
    }
    return pHttpClient;
}

HttpClient :: ~HttpClient() {
    curl_global_cleanup();
}

void HttpClient::softwareUpdate(int cmdNo, std::string strUrl) {
	if(strUrl.empty() || 0 > cmdNo) {
		log << "HttpClient: Upload URL empty" << std::endl;
		return;
	}
	HttpReqPkt *pRqPkt	= new HttpReqPkt();
	pRqPkt->setReqType(HTTP_REQ_TYPE_DWLD);
	pRqPkt->setUrl(strUrl);
	pRqPkt->setTgtFile(std::string(TECHNO_SPURS_APP_FOLDER));
	pRqPkt->setCmdNo(cmdNo);

	//	Prepare the headers
	pRqPkt->addHeader(std::string("Content-Type: application/zip"));

	log << "HttpClient: Making software update request with CmdNo: " << cmdNo << std::endl;
	pushToQ(pRqPkt);
}

void HttpClient::uploadLog(int cmdNo, std::string strLog) {
	if(strLog.empty() || 0 > cmdNo) {
		log << "HttpClient: Log content empty" << std::endl;
		return;
	}

	HttpReqPkt *pRqPkt	= new HttpReqPkt();
	pRqPkt->setReqType(HTTP_REQ_TYPE_UPLD);
	pRqPkt->setUrl(LOG_UPLOAD_URL);
	pRqPkt->setTgtFile(std::string(TECHNO_SPURS_ROOT_PATH) + std::string(TECHNO_SPURS_LOG_FILE));
	pRqPkt->setCmdNo(cmdNo);

	//	Prepare the headers
	std::string strUniqID	= Config::getInstance()->getRPiUniqId();
	pRqPkt->addHeader(std::string("Content-Type: multipart/form-data"));
	pRqPkt->addHeader(std::string("module:") + std::string(MODULE_NAME));
	pRqPkt->addHeader(std::string("panel:") + strUniqID);

	pRqPkt->setUserData(strLog);

	log << "HttpClient: Making log upload request" << std::endl;
	pushToQ(pRqPkt);
}

void HttpClient :: pushToQ(HttpReqPkt *pReqPkt) {
	if(pReqPkt) {
		pthread_mutex_lock(&mtxgQ);
		reqQ.push(pReqPkt);
		pthread_cond_signal(&mtxgCond);
		pthread_mutex_unlock(&mtxgQ);
	}
}

HttpReqPkt *HttpClient :: readFromQ() {
	HttpReqPkt *pReqPkt;
    pthread_mutex_lock(&mtxgQ);
    if(reqQ.empty()) {
    	log << "HttpClient: Waiting for http requests" << endl;
    	pthread_cond_wait(&mtxgCond, &mtxgQ);
    }
    pReqPkt	= reqQ.front();
    reqQ.pop();
    pthread_mutex_unlock(&mtxgQ);
    return pReqPkt;
}

void *HttpClient :: run(void *pHttpClient) {
	HttpReqPkt *pReqPkt	= NULL;
    long respCode = 0;
    std::string file_name;
    FILE    *write_data;
    CURLcode res = CURLE_OK;
    struct curl_slist* curl_hdrs;

    curl_mime *form = NULL;
	curl_mimepart *field = NULL;

    HttpClient *pThis	= reinterpret_cast<HttpClient *>(pHttpClient);
    Logger &log		= pThis->log;

    log << "HttpClient: Curl thread started" << endl;

    std::string strCAFile		= std::string(TECHNO_SPURS_ROOT_PATH) + std::string(TECHNO_SPURS_CERT_FILE);
    while(1) {
    	write_data = NULL; form = NULL; field = NULL; curl_hdrs	= NULL;
    	pReqPkt	= pThis->readFromQ();
    	log << "HttpClient: Got download req" << pReqPkt->getUrl() << endl;

        CURL *curl      = curl_easy_init();
        if(curl && pThis->pListener) {
			/*curl_easy_setopt(curl, CURLOPT_PROXY, "http://proxyvipfmcc.nb.ford.com");
			curl_easy_setopt(curl, CURLOPT_PROXY, "https://proxyvipfmcc.nb.ford.com");
			curl_easy_setopt(curl, CURLOPT_PROXY, "ftp://proxyvipfmcc.nb.ford.com");
			curl_easy_setopt(curl, CURLOPT_PROXYPORT, 83);*/

            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 0L);
            curl_easy_setopt(curl, CURLOPT_URL, pReqPkt->getUrl().c_str());

            //  Set the header
            std::vector<std::string> headers	= pReqPkt->getHeaders();
            for(std::string header : headers) {
            	curl_hdrs = curl_slist_append(curl_hdrs, header.c_str());
            }
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, curl_hdrs);

            //  For verifying server
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
            curl_easy_setopt(curl, CURLOPT_CAINFO, strCAFile.c_str());

            switch(pReqPkt->getReqType()) {
				case HTTP_REQ_TYPE_DWLD:
					file_name	= std::string(TECHNO_SPURS_ROOT_PATH) + std::string(TECHNO_SPURS_DOWNLOAD_FILE);
					curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_function);
					write_data  = fopen(file_name.c_str(), "wb");
					curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)write_data);
					break;

				case HTTP_REQ_TYPE_UPLD:
					file_name	= pReqPkt->getTgtFile();
					write_data  = fopen(file_name.c_str(), "w");
					if(write_data) {
						fwrite(pReqPkt->getUserData().c_str(), 1, pReqPkt->getUserData().length(), write_data);
						fclose(write_data);
						write_data = NULL;
					}
					form	= curl_mime_init(curl);
					field	= curl_mime_addpart(form);
					curl_mime_name(field, "file");
					curl_mime_filedata(field, file_name.c_str());
					curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
					break;
            }

            res = curl_easy_perform(curl);
            if(write_data) fclose(write_data);

            CURLcode infoResp = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &respCode);
            if(CURLE_OK == res && CURLE_OK == infoResp && respCode >= 200 && respCode <= 299) {
            	log << "HttpClient: Command No: " << pReqPkt->getCmdNo() << " success" << std::endl;
            	if(HTTP_REQ_TYPE_DWLD == pReqPkt->getReqType())
            		pThis->pListener->onDownloadSuccess(respCode, pReqPkt->getCmdNo(), pReqPkt->getTgtFile());
            } else {
            	log << "HttpClient: Error: Command No: " << pReqPkt->getCmdNo()
            			<< ", res: " << res
            			<< ", infoResp: "<< infoResp
						<< ", respCode :" << respCode << std::endl;
                pThis->pListener->onDownloadFailure(respCode, pReqPkt->getCmdNo());
            }
        }

        if(curl) curl_easy_cleanup(curl);
        delete pReqPkt;
    }
    return NULL;
}

size_t HttpClient :: write_function(char *ptr, size_t size, size_t nmemb, void *userdata) {
    size_t ret  = 0;
    FILE *fp    = reinterpret_cast<FILE *>(userdata);

    size_t iTotal = (size * nmemb);
    if(0 < iTotal) {
        ret = fwrite(ptr, 1, iTotal, fp);
    }
    return ret;
}

