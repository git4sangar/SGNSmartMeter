/*sgn
 * Constants.cpp
 *
 *  Created on: 06-Mar-2020
 *      Author: tstone10
 */

#include <stdio.h>
#include <iostream>
#include <string.h>
#include <unistd.h>
#include "JsonFactory.h"
#include "JsonException.h"
#include "Utils.h"
#include "Constants.h"

Config *Config::pConfig = NULL;

Config::Config() {
    encrypt_key     = (unsigned char *) strdup(ENCRYPT_KEY);  // 256 bit key
    encrypt_salt    = (unsigned char *) strdup(ENCRYPT_SALT); // 128 bit IV
}

Config::~Config() {
    if(encrypt_key)     free(encrypt_key);
    if(encrypt_salt)    free(encrypt_salt);
}

Config *Config::getInstance() {
    if(NULL == pConfig) {
        pConfig = new Config();
    }
    return pConfig;
}

bool Config::readEncryptedFile(std::string file_name, std::string &strContent) {
    FILE *fp = NULL;
    unsigned char *cipher_text  = NULL;
    unsigned char *plain_text   = NULL;
    int cipher_len = 0, iRetryCount = 0;
    bool bRet       = false;

    fp = NULL;
    while(iRetryCount < MAX_RETRY_COUNT && NULL == fp) {
        fp = fopen(file_name.c_str(), "rb");
        iRetryCount++;
        sleep(1);
    }

    if(NULL != fp) {
        cipher_text = (unsigned char *) malloc(MAX_FILE_SIZE);
        plain_text  = (unsigned char *) malloc(MAX_FILE_SIZE);
        cipher_len  = fread(cipher_text, 1, MAX_FILE_SIZE, fp);
        fclose(fp);

        //  Decrypt
        int plain_len   = Utils::aesDecrypt(cipher_text, cipher_len, encrypt_key, encrypt_salt, plain_text);
        plain_text[plain_len]   = '\0';
        strContent  = std::string(reinterpret_cast<char *>(plain_text));

        free(cipher_text);
        free(plain_text);
        bRet = true;
    }

    return bRet;
}


/*{
  "xmpp_client" : {
    "client_jid" : "altimeter_0001@im.koderoot.net",
    "client_password" : "abcd1234",
    "cpanel_jid" : "technospurs@im.koderoot.net"
  }
}*/
bool Config::parseXmppDetails() {
    JsonFactory jsRoot;
    json_t      *jsXmpp;
    bool        bRet = false;

    std::string strCfgFile	= std::string(TECHNO_SPURS_ROOT_PATH) + std::string(TECHNO_SPURS_CFG_FILE);
    if(!readEncryptedFile(strCfgFile, strXmppDetails)) {
        std::cout << "Failed reading XMPP Configurations" << std::endl;
        return false;
    }

    if(!strXmppDetails.empty()) {
        std::string client_jid, client_pwd, cpanel_jid;
        try {
            jsRoot.setJsonString(strXmppDetails);
            jsRoot.validateJSONAndGetValue("xmpp_client", jsXmpp);
            jsRoot.validateJSONAndGetValue("client_jid", client_jid, jsXmpp);
            jsRoot.validateJSONAndGetValue("client_password", client_pwd, jsXmpp);
            jsRoot.validateJSONAndGetValue("cpanel_jid", cpanel_jid, jsXmpp);

            xmpp_details.setClientJid(client_jid);
            xmpp_details.setClientPwd(client_pwd);
            xmpp_details.setCPanelJid(cpanel_jid);
            bRet = true;
        } catch(JsonException &jed) {
            std::cout << "Initialization: Config json parsing failed" << std::endl;
            std::cout << jed.what() << std::endl;
            bRet = false;
        }
    }
    return bRet;
}


/*{
  "processes" :
    [
      {"name" : "process1", "pid":213, "version":{"major":1, "minor":0, "patch":0}},
      {"name" : "process2", "pid":812, "version":{"major":1, "minor":0, "patch":0}}
    ]
}*/

bool Config::parseCurVersions() {
    std::string strProcName;
    Version procVer;
    std::string strVerFile	= std::string(TECHNO_SPURS_ROOT_PATH) + std::string(TECHNO_SPURS_VERSIONS);
    if(!readEncryptedFile(strVerFile, strCurVersions)) {
        std::cout << "Failed reading Versions" << std::endl;
        return false;
    }
    JsonFactory jsRoot;
    json_t *jsProcs;
    int iLoop = 0;
    try {
        jsRoot.setJsonString(strCurVersions);
        jsRoot.validateJSONAndGetValue("processes", jsProcs);
        for(iLoop = 0; iLoop < jsRoot.getArraySize(jsProcs); iLoop++) {
            JsonFactory jsProc  = jsRoot.getObjAt(jsProcs, iLoop);
            procVer.parseFromJsonFactory(jsProc);
            curVersions.push_back(procVer);
        }
    } catch(JsonException &jed) {
        std::cout << jed.what() << std::endl;
    }
    return true;
}

Version Config:: getVerForProc(std::string strProcName) {
    Version temp;
    for(Version ver : curVersions) {
        if (0 == ver.getProcName().compare(strProcName)) {
            temp = ver;
            break;
        }
    }
    return temp;
}

Version::Version() : mjr {0}, mnr {0}, patch {0} { }
Version::~Version() {}

bool Version::operator < (const Version &other) {
    if(mjr < other.mjr || mnr < other.mnr || patch < other.patch) {
            return true;
    }
    return false;
}

bool Version::operator > (const Version &other) {
    if(mjr > other.mjr || mnr > other.mnr || patch > other.patch) {
            return true;
    }
    return false;
}

bool Version::operator == (const Version &other) {
    if(mjr == other.mjr && mnr == other.mnr && patch == other.patch) {
            return true;
    }
    return false;
}

/*  { "name" : "process_name", "version": {"major":1,"minor":0,"patch":0} } */
void Version::parseFromJsonFactory(JsonFactory jsRoot) {
    json_t *jsVer;
    try {
            jsRoot.validateJSONAndGetValue("name", procName);
            jsRoot.validateJSONAndGetValue("version", jsVer);
            jsRoot.validateJSONAndGetValue("major", mjr, jsVer);
            jsRoot.validateJSONAndGetValue("minor", mnr, jsVer);
            jsRoot.validateJSONAndGetValue("patch", patch, jsVer);
    } catch(JsonException &jed) {
            mjr = mnr = patch = 0;
            std::cout << jed.what() << std::endl;
    }
}

/*  { "name" : "process_name", "version": {"major":1,"minor":0,"patch":0} } */
void Version::parseFromString(std::string strJson) {
    JsonFactory jsRoot;
    json_t *jsVer;

    if(!strJson.empty()) {
        try {
                jsRoot.setJsonString(strJson);
                jsRoot.validateJSONAndGetValue("name", procName);
                jsRoot.validateJSONAndGetValue("version", jsVer);
                jsRoot.validateJSONAndGetValue("major", mjr, jsVer);
                jsRoot.validateJSONAndGetValue("minor", mnr, jsVer);
                jsRoot.validateJSONAndGetValue("patch", patch, jsVer);
        } catch(JsonException &jed) {
                mjr = mnr = patch = 0;
                std::cout << jed.what() << std::endl;
        }
    }
}




