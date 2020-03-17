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
#include <curl/curl.h>

using namespace std;

class HttpClient {
    HttpResponse *pListener;

    queue<std::string> genericQ;
    pthread_mutex_t mtxgQ;
    pthread_cond_t mtxgCond;
    static HttpClient *pHttpClient;
    HttpClient();

public:
    virtual ~HttpClient();

    void subscribeListener(HttpResponse *pObj) { pListener = pObj; }
    void pushToQ(std::string strUrl);
    std::string readFromQ();

    static HttpClient *getInstance();
    static size_t write_function(char *ptr, size_t size, size_t nmemb, void *userdata);
    static void *run(void *pThis);
};



#endif /* HTTPCLIENT_H_ */
