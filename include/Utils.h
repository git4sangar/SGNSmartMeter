/*
 * Utils.h
 *
 *  Created on: 16-Feb-2020
 *      Author: tstone10
 */

#ifndef _UTILS_H_
#define _UTILS_H_

#include <jansson.h>
#include <pthread.h>
#include <string>
#include <map>
#include <openssl/rsa.h>
#include <openssl/aes.h>
#include <openssl/opensslconf.h>
#include <openssl/engine.h>
#include <openssl/pem.h>
#include <openssl/rc4.h>
#include <openssl/evp.h>
#include "JsonException.h"
#include <netdb.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <ifaddrs.h>

typedef struct {
    int iReqId;
    int iType;
    std::string pReqFrom;
    void *pObject;
    unsigned char isAlive;
} ReqPacket;

class Utils {
public:

    static pthread_mutex_t mtxRunningNo;
    static int iRunningNo;
    //static map<string, int> mapFeatureToIds;

    static int getUniqueRunningNo();
    static std::string calculateSHA256String(std::string &data);
    static std::string get4BitRShiftDateInMDYYYY();
    static int aesInit(unsigned char *key_data,
                        int key_data_len,
                        unsigned char *salt,
                        EVP_CIPHER_CTX *e_ctx,
                        EVP_CIPHER_CTX *d_ctx);
    static std::string getHostToIp(std::string strHost);
    static int aesEncrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
                          unsigned char *iv, unsigned char *ciphertext);
    static int aesDecrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
                          unsigned char *iv, unsigned char *plaintext);
    static in_addr_t getIpv4BroadcastIpOfEthIF();
    static in_addr_t getIpv4IpOfEthIF();
    static int sendUDPPacket(in_addr_t toIp, int iPort, std::string strPayload, unsigned char isBroadCast = 0);
    static std::string getDotFormattedIp(in_addr_t ip);
};

#endif


