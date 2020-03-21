/*
 * JabberClient.cpp
 *
 *  Created on: 28-Feb-2020
 *      Author: tstone10
 */

#include <iostream>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <strophe.h>
#include "JabberClient.h"
#include "FileLogger.h"

JabberClient::JabberClient() : log(Logger::getInstance()) {
    ctx     = NULL;
    conn    = NULL;
    xmpp_log= NULL;
    pFrom   = NULL;
    pXmppListener = NULL;
    //xmppThread = 0;
}
JabberClient::~JabberClient() {}

JabberClient *JabberClient::pJabberClient = NULL;

JabberClient *JabberClient::getJabberClient(void) {
    if(NULL == pJabberClient) {
            pJabberClient = new JabberClient();
    }
    return pJabberClient;
}

//	Returns 0 on success and -1 on failure
int JabberClient::connect(std::string strServer, std::string strFullJid, std::string strPswd){

    xmpp_initialize();
    xmpp_log= xmpp_get_default_logger(XMPP_LEVEL_ERROR); /* pass NULL instead to silence output */
    ctx     = xmpp_ctx_new(NULL, xmpp_log);
    conn    = xmpp_conn_new(ctx);

    xmpp_conn_set_keepalive(conn, KA_TIMEOUT, KA_INTERVAL);
    log    << "JabberClient: Connecting xmpp server with username "
                << strFullJid << ", password " << strPswd << std::endl;

    xmpp_conn_set_jid(conn, strFullJid.c_str());
    xmpp_conn_set_pass(conn, strPswd.c_str());
    return xmpp_connect_client(conn, NULL, 0, JabberClient::conn_handler, this);
}

void JabberClient::conn_handler(xmpp_conn_t * const conn, const xmpp_conn_event_t status,
                  const int error, xmpp_stream_error_t * const stream_error,
                  void * const userdata)
{
    JabberClient *pJabberClient = reinterpret_cast<JabberClient *>(userdata);
    xmpp_ctx_t *ctx = pJabberClient->getConext();

    if (status == XMPP_CONN_CONNECT) {
        xmpp_stanza_t* pres;
        pJabberClient->log << "JabberClient: Connected" << std::endl;
        xmpp_handler_add(conn, JabberClient::version_handler, "jabber:iq:version", "iq", NULL, ctx);
        xmpp_handler_add(conn, JabberClient::message_handler, NULL, "message", NULL, pJabberClient);

        /* Send initial <presence/> so that we appear online to contacts */
        pres = xmpp_presence_new(ctx);
        xmpp_send(conn, pres);
        xmpp_stanza_release(pres);
        if(pJabberClient->pXmppListener) pJabberClient->pXmppListener->onXmppConnect();
    }
    else {
    	pJabberClient->log << "JabberClient: Error: Connect to server failed" << std::endl;
    	if(pJabberClient->pXmppListener) pJabberClient->pXmppListener->onXmppDisconnect(status);
    }
}

int JabberClient::version_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata)
{
    xmpp_stanza_t *reply, *query, *name, *version, *text;
    const char *ns;
    xmpp_ctx_t *ctx = (xmpp_ctx_t*)userdata;

//    printf("Received version request from %s\n", xmpp_stanza_get_from(stanza));

    reply = xmpp_stanza_reply(stanza);
    xmpp_stanza_set_type(reply, "result");

    query = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(query, "query");
    ns = xmpp_stanza_get_ns(xmpp_stanza_get_children(stanza));
    if (ns) {
        xmpp_stanza_set_ns(query, ns);
    }

    name = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(name, "name");
    xmpp_stanza_add_child(query, name);
    xmpp_stanza_release(name);

    text = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(text, "techno spurs");
    xmpp_stanza_add_child(name, text);
    xmpp_stanza_release(text);

    version = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(version, "version");
    xmpp_stanza_add_child(query, version);
    xmpp_stanza_release(version);

    text = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(text, "1.0");
    xmpp_stanza_add_child(version, text);
    xmpp_stanza_release(text);

    xmpp_stanza_add_child(reply, query);
    xmpp_stanza_release(query);

    xmpp_send(conn, reply);
    xmpp_stanza_release(reply);
    return 1;
}


int JabberClient::message_handler(xmpp_conn_t * const conn, xmpp_stanza_t * const stanza, void * const userdata)
{
    JabberClient *pJabberClient = reinterpret_cast<JabberClient *>(userdata);
    xmpp_ctx_t *ctx             = (xmpp_ctx_t*)pJabberClient->getConext();
    xmpp_stanza_t *body;
    const char *type;
    char *intext;

    body = xmpp_stanza_get_child_by_name(stanza, "body");
    if (body == NULL)
        return 1;
    type = xmpp_stanza_get_type(stanza);
    if (type != NULL && strcmp(type, "error") == 0)
        return 1;

    intext      = xmpp_stanza_get_text(body);
    const char *pFrom = xmpp_stanza_get_attribute(stanza, "from");

    pJabberClient->log << "JabberClient: Got packet " << intext << ", from " << pFrom << std::endl;
    if(pJabberClient->pXmppListener) {
            std::string strFrom = (pFrom) ? pFrom : "";
            pJabberClient->pXmppListener->onXmppMessage(std::string(intext), std::string(strFrom));
    }
    xmpp_free(ctx, intext);
    return 1;
}

void JabberClient::xmppShutDown() {
	xmpp_conn_release(conn);
	xmpp_ctx_free(ctx);
	xmpp_shutdown();
}

int JabberClient::sendMsgTo(std::string strMsg, std::string toAddress) {
    std::cout << "&& Sending " << strMsg << " to " << toAddress << std::endl;
    xmpp_stanza_t *reply = NULL, *body = NULL, *text = NULL;

    if(toAddress.empty()) {
    	log << "JabberClient: Error: Invalid to address" << std::endl;
        return -1;
    }

    reply = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(reply, "message");
    xmpp_stanza_set_type(reply, "chat");
    xmpp_stanza_set_attribute(reply, "to", toAddress.c_str());

    body = xmpp_stanza_new(ctx);
    xmpp_stanza_set_name(body, "body");

    text = xmpp_stanza_new(ctx);
    xmpp_stanza_set_text(text, strMsg.c_str());
    xmpp_stanza_add_child(body, text);
    xmpp_stanza_add_child(reply, body);

    xmpp_send(conn, reply);
    xmpp_stanza_release(reply);
    return 0;
}

void JabberClient::stopXmpp() {
    xmpp_conn_release(conn);
    xmpp_ctx_free(ctx);
    xmpp_shutdown();
}

