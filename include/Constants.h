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

#include "JsonFactory.h"
#include "JsonException.h"
#include "Utils.h"

#define ENCRYPT_KEY     "01234567890123456789012345678901"
#define ENCRYPT_SALT    "0123456789012345"

#define ONE_KB  		(1024)
#define ONE_MB  		(1024 * 1024)
#define WAIT_TIME_SECs	(2 * 60)

//	Let all paths be suffixed with "/"
//	Treat it as folders otherwise
//#define TECHNO_SPURS_ROOT_PATH      "/home/pi/technospurs/"
#define TECHNO_SPURS_ROOT_PATH      "/home/tstone10/sgn/bkup/private/projs/SGNBarc/technospurs/"
#define TECHNO_SPURS_CFG_FILE       "cfg/config_file.bin"
#define TECHNO_SPURS_VERSIONS       "wdog/versions.bin"
#define TECHNO_SPURS_CERT_FILE      "certs/cacert.pem"
#define TECHNO_SPURS_DOWNLOAD_FILE  "downloads/new_update.zip"
#define TECHNO_SPURS_DOWNLOAD_PATH	"downloads/"
#define TECHNO_SPURS_TEMP_DWLD_PATH	"downloads/SmartMeter"
#define TECHNO_SPURS_APP_FOLDER		"SmartMeter"
#define TECHNO_SPURS_LOG_PATH		"logs/"

#define MAX_FILE_SIZE           ONE_MB
#define MAX_RETRY_COUNT             (3)

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
};

class Version {
    int mjr, mnr, patch;
    std::string procName;

public:
    Version();
    virtual ~Version();

    void setProcName(std::string name) { procName = name; }
    std::string getProcName() { return procName; }
    void parseFromString(std::string strJson);
    void parseFromJsonFactory(JsonFactory jsRoot);
    bool operator < (const Version &other);
    bool operator > (const Version &other);
    bool operator == (const Version &other);
};

class Config {
    Config();
    unsigned char *encrypt_key;     // 256 bit key
    unsigned char *encrypt_salt;    // 128 bit IV
    static Config *pConfig;
    std::string strXmppDetails, strCurVersions;
    std::vector<Version> curVersions;
    XmppDetails xmpp_details;

public:
    virtual ~Config();
    static Config *getInstance();
    bool readEncryptedFile(std::string strFileName, std::string &strContent);
    bool parseXmppDetails();
    bool parseCurVersions();
    Version getVerForProc(std::string strProcName);

    XmppDetails getXmppDetails() { return xmpp_details;}
    std::vector<Version> getCurrentVersions() { return curVersions; }
};
#endif /* CONSTANTS_H_ */
