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

using namespace std;

class HttpClient {
    HttpResponse *pListener;
    Logger &log;

    queue<std::pair<std::string, int> > genericQ;
    pthread_mutex_t mtxgQ;
    pthread_cond_t mtxgCond;
    static HttpClient *pHttpClient;
    HttpClient();

public:
    virtual ~HttpClient();

    void subscribeListener(HttpResponse *pObj) { pListener = pObj; }
    void pushToQ(std::pair<std::string, int> qCntnt);
    std::pair<std::string, int> readFromQ();

    static HttpClient *getInstance();
    static size_t write_function(char *ptr, size_t size, size_t nmemb, void *userdata);
    static void *run(void *pThis);
};



#endif /* HTTPCLIENT_H_ */
