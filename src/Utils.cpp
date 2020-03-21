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
int Utils::iRunningNo = 1001;

int Utils::getUniqueRunningNo() {
    int iRet = 0;
    pthread_mutex_lock(&mtxRunningNo);
    iRet = ++iRunningNo;
    pthread_mutex_unlock(&mtxRunningNo);
    return iRet;
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

int Utils::aesEncrypt(unsigned char *plaintext, int plaintext_len, unsigned char *key,
                   unsigned char *iv, unsigned char *ciphertext) {
    EVP_CIPHER_CTX *ctx;
    int len, ciphertext_len;

    /* Create and initialize the context */
    if(!(ctx = EVP_CIPHER_CTX_new())) {
        std::cout << "Failed initializing cipher context" << std::endl;
    }

    /*
     * Initialise the encryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
        std::cout << "Failed initializing encrypt" << std::endl;
    }

    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len)) {
        std::cout << "Failed updating encrypt" << std::endl;
    }
    ciphertext_len = len;

    /*
     * Finalize the encryption. Further ciphertext bytes may be written at
     * this stage.
     */
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len)) {
        std::cout << "Failed finalizing encrypt" << std::endl;
    }
    ciphertext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int Utils::aesDecrypt(unsigned char *ciphertext, int ciphertext_len, unsigned char *key,
                      unsigned char *iv, unsigned char *plaintext) {
    EVP_CIPHER_CTX *ctx;
    int len, plaintext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new())) {
        std::cout << "Failed initializing decrypt context" << std::endl;
    }

    /*
     * Initialise the decryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_256_cbc(), NULL, key, iv)) {
        std::cout << "Failed initializing decrypt" << std::endl;
    }

    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary.
     */
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len)) {
        std::cout << "Failed updating decrypt" << std::endl;
    }
    plaintext_len = len;

    /*
     * Finalise the decryption. Further plaintext bytes may be written at
     * this stage.
     */
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len)) {
        std::cout << "Failed finalizing decrypt" << std::endl;
    }
    plaintext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
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

void Utils::sendPacket(int port, std::string strPacket) {
	struct hostent *he;

	struct sockaddr_in their_addr;
	int sockfd	= socket(AF_INET, SOCK_DGRAM, 0);
	he	= gethostbyname("localhost");
	their_addr.sin_family   = AF_INET;
	their_addr.sin_port     = htons(port);
	their_addr.sin_addr     = *((struct in_addr *)he->h_addr);
	bzero(&(their_addr.sin_zero), 8);
	sendto(sockfd, strPacket.c_str(), strPacket.length(), 0,
			 (struct sockaddr *)&their_addr, sizeof(struct sockaddr));
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
