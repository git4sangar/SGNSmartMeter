/*
 * Constants.h
 *
 *  Created on: 16-Feb-2020
 *      Author: tstone10
 */

#ifndef CONSTANTS_H_
#define CONSTANTS_H_

#include <openssl/rsa.h>
#include <openssl/aes.h>
#include <openssl/opensslconf.h>
#include <openssl/engine.h>
#include <openssl/pem.h>
#include <openssl/rc4.h>
#include <openssl/evp.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <errno.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>
#include "FileLogger.h"
#include "JsonFactory.h"
#include "JsonException.h"
#include "Utils.h"

#define JABBER_CLIENT_VERSION		(1)

#define ENCRYPT_KEY     "01234567890123456789012345678901"
#define ENCRYPT_SALT    "0123456789012345"

#define ONE_KB  		(1024)
#define ONE_MB  		(ONE_KB * ONE_KB)
#define WAIT_TIME_SECs	(2 * 60)
#define WDOG_Tx_PORT	(4951)
#define WDOG_Rx_PORT	(4952)

//	Let all paths be suffixed with "/"
//	Treat it as folders otherwise
#define TECHNO_SPURS_ROOT_PATH      "/home/pi/technospurs/"
//#define TECHNO_SPURS_ROOT_PATH      "/home/tstone10/sgn/bkup/private/projs/SGNBarc/technospurs/"
#define TECHNO_SPURS_CFG_FILE       "cfg/config_file.bin"
#define TECHNO_SPURS_VERSIONS       "wdog/versions.bin"
#define TECHNO_SPURS_CERT_FILE      "certs/cacert.pem"
#define TECHNO_SPURS_DOWNLOAD_FILE  "downloads/new_update.zip"
#define TECHNO_SPURS_DOWNLOAD_PATH	"downloads/"
#define TECHNO_SPURS_TEMP_DWLD_PATH	"downloads/SmartMeter"
#define TECHNO_SPURS_APP_FOLDER		"SmartMeter"
#define TECHNO_SPURS_LOG_FILE		"logs/jabber_logs_"
#define TECHNO_SPURS_CLIENT_FOLDER	"JabberClient"
#define TECHNO_SPURS_CLIENT_FILE	"JabberClient/JabberClient"

#define MAX_BUFF_SIZE			(10 * ONE_KB)
#define MAX_FILE_SIZE           ONE_MB
#define MAX_RETRY_COUNT			(3)
#define MAX_LOG_SIZE			ONE_MB
#define MODULE_NAME				"JabberClient"

class XmppDetails {
    std::string client_jid,
                client_pwd,
                cpanel_jid;
public:
    void setClientJid(std::string _client_jid) { client_jid = _client_jid;}
    void setClientPwd(std::string _client_pwd) { client_pwd = _client_pwd;}
    void setCPanelJid(std::string _server_jid) { cpanel_jid = _server_jid;}

    std::string getClientJid() {return client_jid;}
    std::string getClientPwd() {return client_pwd;}
    std::string getCPanelJid() {return cpanel_jid;}

    std::string toString();
};

class Version {
    int ver;
    std::string procName;

public:
    Version();
    virtual ~Version();

    void setProcName(std::string name) { procName = name; }
    std::string getProcName() { return procName; }
    int getVer() { return ver; }
    std::string getVerInfo();

    void parseFromString(std::string strJson);
    void parseFromJsonFactory(JsonFactory jsRoot);
};

class Config {
    Config();
    unsigned char *encrypt_key;     // 256 bit key
    unsigned char *encrypt_salt;    // 128 bit IV
    static Config *pConfig;
    std::string strXmppDetails, rpi_uniqId, strCurVer;
    std::vector<Version> curVersions;
    XmppDetails xmpp_details;
    Logger &info_log;

public:
    virtual ~Config();
    static Config *getInstance();
    bool readEncryptedFile(std::string strFileName, std::string &strContent);
    bool parseXmppDetails();
    bool parseCurVersions(std::string strCurVersions);
    std::string getCurVersions() { return strCurVer; }
    int getVerForProc(std::string strProcName);

    void setRPiUniqId(std::string _rpi_uniqId) { rpi_uniqId = _rpi_uniqId;}
    std::string getRPiUniqId() {return rpi_uniqId;}

    XmppDetails getXmppDetails() { return xmpp_details;}
    std::vector<Version> getCurrentVersions() { return curVersions; }
};
#endif /* CONSTANTS_H_ */
