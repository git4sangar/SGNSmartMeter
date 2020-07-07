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
#include <unistd.h>
#include "JabberClient.h"
#include "FileLogger.h"
#include "Constants.h"

HttpClient *HttpClient::pHttpClient = NULL;

HttpClient :: HttpClient() : info_log(Logger::getInstance()) {
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

HttpReqPkt *HttpClient::genericUpdate(std::string strUrl, std::string strFolder, std::string strCmd, int cmdNo) {
	HttpReqPkt *pRqPkt	= new HttpReqPkt();
	pRqPkt->setReqType(HTTP_REQ_TYPE_DWLD);
	pRqPkt->setUrl(strUrl);
	pRqPkt->setTgtFile(strFolder);
	pRqPkt->setCmdNo(cmdNo);
	pRqPkt->setCmd(strCmd);
	pRqPkt->addHeader(std::string("Content-Type: application/zip"));
	return pRqPkt;
}

void HttpClient::smartMeterUpdate(int cmdNo, std::string strUrl) {
	if(strUrl.empty() || 0 > cmdNo) {
		info_log << "HttpClient: Smart meter update URL empty" << std::endl;
		return;
	}
	HttpReqPkt *pRqPkt	= genericUpdate(strUrl, std::string(TECHNO_SPURS_APP_FOLDER), "smart_meter_update", cmdNo);
	info_log << "HttpClient: Making Smart meter update request with CmdNo: " << cmdNo << std::endl;
	pushToQ(pRqPkt);
}

void HttpClient::jabberClientUpdate(int cmdNo, std::string strUrl) {
	if(strUrl.empty() || 0 > cmdNo) {
		info_log << "HttpClient: Jabber client update URL empty" << std::endl;
		return;
	}
	HttpReqPkt *pRqPkt	= genericUpdate(strUrl, std::string(TECHNO_SPURS_JABBER_FOLDER), "jabber_client_update", cmdNo);
	info_log << "HttpClient: Making Jabber client update request with CmdNo: " << cmdNo << std::endl;
	pushToQ(pRqPkt);
}

void HttpClient::watchDogUpdate(int cmdNo, std::string strUrl) {
	if(strUrl.empty() || 0 > cmdNo) {
		info_log << "HttpClient: Watchdog update update URL empty" << std::endl;
		return;
	}
	HttpReqPkt *pRqPkt	= genericUpdate(strUrl, std::string(TECHNO_SPURS_WDOG_FOLDER), "watchdog_update", cmdNo);
	info_log << "HttpClient: Making Watchdog update request with CmdNo: " << cmdNo << std::endl;
	pushToQ(pRqPkt);
}

void HttpClient::uploadLogs(int cmdNo, std::string fileName, std::string strLogData) {
	if(strLogData.empty() || 0 > cmdNo) {
		info_log << "HttpClient: Log content empty" << std::endl;
		return;
	}

	HttpReqPkt *pRqPkt	= new HttpReqPkt();
	pRqPkt->setReqType(HTTP_REQ_TYPE_UPLD);
	pRqPkt->setUrl(Config::getInstance()->getLogUploadURL());
	std::string strFile	= std::string(TECHNO_SPURS_ROOT_PATH) + fileName +
							Utils::getYYYYMMDD_HHMMSS();
	pRqPkt->setTgtFile(strFile);
	pRqPkt->setCmdNo(cmdNo);
	pRqPkt->setCmd("upload_logs");

	//	Prepare the headers
	std::string strUniqID	= Config::getInstance()->getRPiUniqId();
	pRqPkt->addHeader(std::string("Content-Type: multipart/form-data"));
	pRqPkt->addHeader(std::string("module:") + std::string(MODULE_NAME));
	pRqPkt->addHeader(std::string("panel:") + strUniqID);

	pRqPkt->setUserData(strLogData);

	info_log << "HttpClient: Making log upload request" << std::endl;
	pushToQ(pRqPkt);
}

void HttpClient::postReq(std::string strUrl, std::string strPLoad) {
	if(strPLoad.empty()) {
		info_log << "HttpClient: Invalid post payload" << std::endl;
		return;
	}
	HttpReqPkt *pRqPkt	= new HttpReqPkt();
	pRqPkt->setReqType(HTTP_REQ_TYPE_POST);
	pRqPkt->setUrl(strUrl);
	pRqPkt->setUserData(strPLoad);

	pRqPkt->addHeader(std::string("Accept: application/json"));
	pRqPkt->addHeader(std::string("Content-Type: application/json"));

	info_log << "HttpClient: Making POST request with payload: " << strPLoad << std::endl;
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
    	info_log << "HttpClient: Waiting for http requests" << endl;
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
    char pPostPayLoad[MAX_BUFF_SIZE];

    curl_mime *form = NULL;
	curl_mimepart *field = NULL;

    HttpClient *pThis		= reinterpret_cast<HttpClient *>(pHttpClient);
    Logger &info_log		= Logger::getInstance();
	Config *pCfg			= Config::getInstance();
	JabberClient *pJbrCli	= JabberClient::getJabberClient();
	XmppDetails xmpp		= pCfg->getXmppDetails();
	std::string cPanelJid	= xmpp.getCPanelJid();
	std::string strUnqId	= pCfg->getRPiUniqId();
	info_log << "HttpClient: cPanel " << cPanelJid << ", UniqId " << strUnqId << std::endl;

    info_log << "HttpClient: Curl thread started" << endl;

    std::string strCAFile		= std::string(TECHNO_SPURS_ROOT_PATH) + std::string(TECHNO_SPURS_CERT_FILE);
    while(1) {
    	write_data = NULL; form = NULL; field = NULL; curl_hdrs	= NULL;
    	pReqPkt	= pThis->readFromQ();
    	info_log << "HttpClient: Got HTTP req " << pReqPkt->getUrl() << endl;

        CURL *curl      = curl_easy_init();
        if(curl && pThis->pListener) {
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

				case HTTP_REQ_TYPE_POST:
					bzero(pPostPayLoad, MAX_BUFF_SIZE);
					//	Need to copy to a buffer & make POST request. Otherwise not working
					strncpy(pPostPayLoad, pReqPkt->getUserData().c_str(), (MAX_BUFF_SIZE-1));
					curl_easy_setopt(curl, CURLOPT_POST, 1L);
					curl_easy_setopt(curl, CURLOPT_POSTFIELDS, pPostPayLoad);
					break;
            }

            res = curl_easy_perform(curl);
            if(write_data) { fclose(write_data); write_data = NULL; }

            CURLcode infoResp = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &respCode);
            if(CURLE_OK == res && CURLE_OK == infoResp && respCode >= 200 && respCode <= 299) {
            	info_log << "HttpClient: Command No: " << pReqPkt->getCmdNo() << " success" << std::endl;

            	//	Inform the listener to unzip it
            	if(HTTP_REQ_TYPE_DWLD == pReqPkt->getReqType())
            		pThis->pListener->onDownloadSuccess(respCode, pReqPkt->getCmdNo(), pReqPkt->getTgtFile());

            	//	We don't need the file anymore. So deleting the same
            	if(HTTP_REQ_TYPE_UPLD == pReqPkt->getReqType()) {
            		unlink(file_name.c_str());
            		std::stringstream ss;
            		ss << "{ \"from\" : \"" << strUnqId << "\" ,\"command\" : \""
            				<< pReqPkt->getCmd() << "\", \"success\" : true, \"command_no\" : "
							<< pReqPkt->getCmdNo() << " }" << std::endl;
            		pJbrCli->sendMsgTo(ss.str(), cPanelJid);
            	}

            	//	For POST request, nothing needs to be handled. So be quiet
            	if(HTTP_REQ_TYPE_POST	== pReqPkt->getReqType()) {
            		info_log << "HttpClient: POST response: " << respCode << std::endl;
            	}
            } else {
            	std::stringstream ss;
            	ss << "{ \"from\" : \"" << strUnqId << "\" ,\"command\" : \""
					<< pReqPkt->getCmd() << "\", \"success\" : false, \"command_no\" : "
					<< pReqPkt->getCmdNo() << ", \"http_resp_code\" : " << respCode << " }" << std::endl;
				pJbrCli->sendMsgTo(ss.str(), cPanelJid);
            	info_log << "HttpClient: Error: Command No: " << pReqPkt->getCmdNo()
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

/*curl_easy_setopt(curl, CURLOPT_PROXY, "http://proxyvipfmcc.nb.ford.com");
curl_easy_setopt(curl, CURLOPT_PROXY, "https://proxyvipfmcc.nb.ford.com");
curl_easy_setopt(curl, CURLOPT_PROXY, "ftp://proxyvipfmcc.nb.ford.com");
curl_easy_setopt(curl, CURLOPT_PROXYPORT, 83);*/
