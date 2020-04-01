/*
 * JabberClient.h
 *
 *  Created on: 28-Feb-2020
 *      Author: tstone10
 */

#ifndef JABBERCLIENT_H_
#define JABBERCLIENT_H_

#include <JabberResponse.h>
#include <iostream>
#include <string>
#include <pthread.h>
#include <strophe.h>
#include "FileLogger.h"

#define KA_TIMEOUT 60
#define KA_INTERVAL 1

class JabberClient {
    JabberClient();
    static JabberClient *pJabberClient;
    Logger &info_log;

    xmpp_ctx_t *ctx;
    xmpp_conn_t *conn;
    xmpp_log_t *xmpp_log;
    char *pFrom;

public:
        virtual ~JabberClient();
        static JabberClient *getJabberClient();
        int connect(std::string strServer, std::string strFullJid, std::string strPswd);
        void subscribeNotification(JabberResponse *pListener){ pXmppListener = pListener; }
        void stopXmpp();
        void startXmpp() { if(NULL != ctx) xmpp_run(ctx);}
        int sendMsgTo(std::string strMsg, std::string toAddress);
        void sendPresence(std::string toAddress);
        void xmppShutDown();
        xmpp_ctx_t *getConext() {return ctx;}

        JabberResponse *pXmppListener;
// STATIC
        static void conn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status,
                                 const int error, xmpp_stream_error_t * const stream_error,
                                 void * const userdata);
        static int message_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza,
                                   void * const userdata);
        static int version_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata);
};



#endif /* JABBERCLIENT_H_ */
