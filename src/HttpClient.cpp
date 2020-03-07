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

#include "Constants.h"

HttpClient *HttpClient::pHttpClient = NULL;

HttpClient :: HttpClient(HttpResponse *pObs) {
    pListener   = pObs;
    mtxgQ       = PTHREAD_MUTEX_INITIALIZER;
    mtxgCond    = PTHREAD_COND_INITIALIZER;

    cout << "&& Entering HttpClient constructor" << endl;

    curl_global_init(CURL_GLOBAL_ALL);

    pthread_t tCurThread;
    pthread_create(&tCurThread, NULL, &genericCurlThread, this);
    pthread_detach(tCurThread);
}

HttpClient *HttpClient::getInstance(HttpResponse *pListener) {
    if(NULL == pHttpClient) {
        pHttpClient = new HttpClient(pListener);
    }
    return pHttpClient;
}

HttpClient :: ~HttpClient() {
    curl_global_cleanup();
}

void HttpClient :: pushToQ(std::string strUrl) {
    pthread_mutex_lock(&mtxgQ);
    genericQ.push(strUrl);
    pthread_cond_signal(&mtxgCond);
    pthread_mutex_unlock(&mtxgQ);
}


std::string HttpClient :: readFromQ() {
    std::string strUrl;
    pthread_mutex_lock(&mtxgQ);
    if(genericQ.empty()) {
        cout << "&& Waiting for http requests" << endl;
        pthread_cond_wait(&mtxgCond, &mtxgQ);
    }
    strUrl = genericQ.front();
    std::cout << "&& Got a request for " << strUrl << std::endl;
    genericQ.pop();
    pthread_mutex_unlock(&mtxgQ);
    return strUrl;
}

void *HttpClient :: genericCurlThread(void *pHttpClient) {
    std::string strUrl;
    long respCode;
    FILE    *write_data;
    CURLcode res = CURLE_OK;
    HttpClient *pThis = reinterpret_cast<HttpClient *>(pHttpClient);

    cout << "&& Entering genericCurlThread" << endl;

    struct curl_slist* headers = NULL;
    while(1) {
        strUrl          = pThis->readFromQ();

        CURL *curl      = curl_easy_init();
        if(curl) {
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 0L);
            curl_easy_setopt(curl, CURLOPT_URL, strUrl);

            //  Set the header
            headers = curl_slist_append(headers, "Content-Type: application/zip");
            curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);

                //  For verifying server
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
            curl_easy_setopt(curl, CURLOPT_CAINFO, "/etc/tls/ca-bundle.pem");

            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_function);
            write_data  = fopen("download.zip", "wb");
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)write_data);

            res = curl_easy_perform(curl);
            CURLcode infoResp = curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &respCode);
            if(CURLE_OK == res && CURLE_OK == infoResp && respCode >= 200 && respCode <= 299) {
                pThis->pListener->onDownloadSuccess(respCode);
            } else {
                pThis->pListener->onDownloadFailure(respCode);
            }
            if(write_data) fclose(write_data);
        }

        curl_easy_cleanup(curl);
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

