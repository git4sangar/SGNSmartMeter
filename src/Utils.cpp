/*
 * Utils.cpp
 *
 *  Created on: 16-Feb-2020
 *      Author: tstone10
 */

#include "Utils.h"
#include "Constants.h"
#include <jansson.h>
#include <iostream>
#include <map>
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

pthread_mutex_t Utils::mtxRunningNo = PTHREAD_MUTEX_INITIALIZER;
int Utils::iRunningNo = MAX_RESERVED_REQ_IDS;

int Utils::getUniqueRunningNo() {
    int iRet = 0;
    pthread_mutex_lock(&mtxRunningNo);
    iRet = ++iRunningNo;
    pthread_mutex_unlock(&mtxRunningNo);
    return iRet;
}


ReqPacket *Utils::getReqPacketForReservedId(int iReqId) {
    if(MAX_RESERVED_REQ_IDS < iReqId) {
        return NULL;
    }

    ReqPacket *pReq = NULL;
    switch(iReqId) {
    case REQ_ID_HTTP_CLIENT_THREAD_READY:
        pReq            = new ReqPacket();
        pReq->iReqId    = REQ_ID_HTTP_CLIENT_THREAD_READY;
        pReq->iType     = HTTP_CLIENT_THREAD_READY;
        pReq->pObject   = NULL;
        pReq->isAlive   = 0;
        break;
    }
    return pReq;
}

std::string Utils::calculateSHA256String(std::string &data) {
    std::string sha256Str = "";
    unsigned char md[SHA256_DIGEST_LENGTH];

    if(SHA256((unsigned char *)(data.c_str()),data.length(),md)){
        int i;
        char oBuff[(SHA256_DIGEST_LENGTH*2)+1];
        for(i = 0; i < SHA256_DIGEST_LENGTH; i++) {
            sprintf(oBuff + (i * 2), "%02x", md[i]);
        }
        oBuff[SHA256_DIGEST_LENGTH*2] = '\0';
        sha256Str = oBuff;
    }

    return sha256Str;
}

std::string Utils::get4BitRShiftDateInMDYYYY(){
    std::string strDate = "";
    unsigned int date =0 ;
    char outStr[16];
    time_t t;
    struct tm *tmp;

    t = time(NULL);
    tmp = gmtime(&t);
    if(NULL != tmp) {
        //printf("Current UTC time and date: %s", asctime(tmp));
        sprintf(outStr, "%d%d%d", (tmp->tm_mon + 1), tmp->tm_mday, (tmp->tm_year + 1900));
        date = atoi(outStr);
        date = (date >> 4);
    }
    sprintf(outStr,"%u",date);
    strDate = outStr;
    //printf("4 Shifted Date Int: %u String:%s\n",date,strDate.c_str());

    return strDate;
}


/**
* Create an 256 bit key and IV using the supplied key_data. salt can be added for taste.
* Fills in the encryption and decryption ctx objects and returns 0 on success
**/
int Utils::aesInit(unsigned char *key_data, int key_data_len, unsigned char *salt,
                        EVP_CIPHER_CTX *e_ctx, EVP_CIPHER_CTX *d_ctx) {
    int i, nrounds = 5;
    unsigned char key[32], iv[32];
    /*
    * Gen key & IV for AES 256 CBC mode. A SHA1 digest is used to hash the supplied key material.
    * nrounds is the number of times the we hash the material. More rounds are more secure but
    * slower.
    */
    i = EVP_BytesToKey(EVP_aes_256_cbc(), EVP_sha1(), salt, key_data, key_data_len, nrounds, key, iv);
    if (i != 32) {
        printf("Key size is %d bits - should be 256 bitsn", i);
        return -1;
    }
    EVP_CIPHER_CTX_init(e_ctx);
    EVP_EncryptInit_ex(e_ctx, EVP_aes_256_cbc(), NULL, key, iv);
    EVP_CIPHER_CTX_init(d_ctx);
    EVP_DecryptInit_ex(d_ctx, EVP_aes_256_cbc(), NULL, key, iv);
    return 0;
}

/*
* Encrypt *len bytes of data
* All data going in & out is considered binary (unsigned char[])
*/
unsigned char *Utils::aesEncrypt(EVP_CIPHER_CTX *e, unsigned char *plaintext, int *len) {
    /* max ciphertext len for a n bytes of plaintext is n + AES_BLOCK_SIZE -1 bytes */
    int c_len = *len + AES_BLOCK_SIZE, f_len = 0;
    unsigned char *ciphertext = (unsigned char *)malloc(c_len);
    /* allows reusing of 'e' for multiple encryption cycles */
    EVP_EncryptInit_ex(e, NULL, NULL, NULL, NULL);
    /* update ciphertext, c_len is filled with the length of ciphertext generated,
    *len is the size of plaintext in bytes */
    EVP_EncryptUpdate(e, ciphertext, &c_len, plaintext, *len);
    /* update ciphertext with the final remaining bytes */
    EVP_EncryptFinal_ex(e, ciphertext+c_len, &f_len);
    *len = c_len + f_len;
    return ciphertext;
}


/*
* Decrypt *len bytes of ciphertext
*/
unsigned char *Utils::aesDecrypt(EVP_CIPHER_CTX *e, unsigned char *ciphertext, int *len) {
    /* plaintext will always be equal to or lesser than length of ciphertext*/
    int p_len = *len, f_len = 0;
    unsigned char *plaintext = (unsigned char *)malloc(p_len);
    EVP_DecryptInit_ex(e, NULL, NULL, NULL, NULL);
    EVP_DecryptUpdate(e, plaintext, &p_len, ciphertext, *len);
    EVP_DecryptFinal_ex(e, plaintext+p_len, &f_len);
    *len = p_len + f_len;
    return plaintext;
}


std::string Utils::getHostToIp(std::string strHost) {
    std::string strIp, serv1 = "https://", serv2 = "http://", serv = "http";
    struct addrinfo hints, *servinfo = NULL, *p = NULL;
    struct sockaddr_in *h = NULL;
    int rv;

    if(!strHost.empty()) {
        memset(&hints, 0, sizeof hints);
        hints.ai_family     = AF_INET;
        hints.ai_socktype   = SOCK_DGRAM;
        hints.ai_protocol   = 0;    // Let it search for all protocols

        //  Trim the service name by searching "https://" first and "http://" next
        size_t pos1 = strHost.find(serv1);
        size_t pos2 = strHost.find(serv2);
        serv        = "http";
        if(pos1 != std::string::npos) { strHost.erase(pos1, serv1.size()); serv = "https";}
        else if(pos2 != std::string::npos) { strHost.erase(pos2, serv2.size()); serv = "http";}

        //  Let the service be NULL. Becoz VMS doesn't have entries for "http" or "https" in /etc/services.
        if((rv = getaddrinfo(strHost.c_str(), NULL, &hints, &servinfo)) != 0) {
            return strIp;
        }
        for(p = servinfo; p != NULL; p = p->ai_next) {
            h = (struct sockaddr_in *)p->ai_addr;
            char *pIp = inet_ntoa(h->sin_addr);
            if(NULL != pIp) {
                strIp = inet_ntoa(h->sin_addr);
                break;
            } else {
                continue;
            }
        }
    }
    return strIp;
}

in_addr_t Utils::getIpv4IpOfEthIF() {
    struct ifaddrs *ifAddrs = NULL, *ifIter = NULL;
    std::string ifName;
    in_addr_t myip = 0;

    //  Gets the ip address of all the interfaces
    getifaddrs(&ifAddrs);

    for(ifIter  = ifAddrs; NULL != ifIter; ifIter = ifIter->ifa_next) {
        ifName  = ifIter->ifa_name;
        //  skip interfaces other than ipv4 and eth
        if(ifIter->ifa_addr->sa_family == AF_INET && std::string::npos != ifName.find("eth")) {
            myip    = ((struct sockaddr_in *)ifIter->ifa_addr)->sin_addr.s_addr;
            break;
        }
    }
    if(NULL != ifAddrs) freeifaddrs(ifAddrs);
    return myip;
}

in_addr_t Utils::getIpv4BroadcastIpOfEthIF() {
    struct ifaddrs *ifAddrs = NULL, *ifIter = NULL;
    std::string strBroadCastIp, ifName;
    //struct in_addr tAddr = {0};
    //char szAddr[INET_ADDRSTRLEN];
    in_addr_t mybc = 0;

    //  Gets the ip address of all the interfaces
    getifaddrs(&ifAddrs);

    for(ifIter  = ifAddrs; NULL != ifIter; ifIter = ifIter->ifa_next) {
        ifName  = ifIter->ifa_name;
        //  skip interfaces other than ipv4 and eth
        if(ifIter->ifa_addr->sa_family != AF_INET || std::string::npos == ifName.find("eth")) {
            continue;
        }

        //  Calculate the broadcast using the netmask
        in_addr_t myip  = ((struct sockaddr_in *)ifIter->ifa_addr)->sin_addr.s_addr;
        in_addr_t nMask = ((struct sockaddr_in *)ifIter->ifa_netmask)->sin_addr.s_addr;
        mybc    = myip | (~nMask);
        //tAddr.s_addr      = mybc;
        //inet_ntop(AF_INET, (void *)&tAddr, szAddr, INET_ADDRSTRLEN);
        //strBroadCastIp    = szAddr;
        break;
    }

    if(NULL != ifAddrs) freeifaddrs(ifAddrs);

    return mybc;
}

int Utils::sendUDPPacket(in_addr_t toIp, int iPort, std::string strPayload, unsigned char isBroadCast) {
    int iSock = 0 ,bcEnable = 1;

    //  Sanity check
    if(0 > toIp || strPayload.empty() || 0 > iPort) {
        return -1;
    }

    iSock   = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if(isBroadCast) {
        setsockopt(iSock, SOL_SOCKET, SO_BROADCAST, &bcEnable, sizeof(bcEnable));
    }
    struct sockaddr_in ipAddr;
    int iLen = sizeof(ipAddr);
    memset((char *) &ipAddr, 0, sizeof(ipAddr));
    ipAddr.sin_family       = AF_INET;
    ipAddr.sin_port         = htons(iPort);
    ipAddr.sin_addr.s_addr  = toIp;

    int iRet = sendto(iSock, strPayload.c_str(), strPayload.length()+1, 0, (struct sockaddr *)&ipAddr, iLen);
    return iRet;
}

std::string Utils::getDotFormattedIp(in_addr_t ip) {
    char szAddr[INET_ADDRSTRLEN];
    struct in_addr ipAddr;
    ipAddr.s_addr = ip;
    inet_ntop(AF_INET, &ipAddr, szAddr, INET_ADDRSTRLEN);
    return szAddr;
}
