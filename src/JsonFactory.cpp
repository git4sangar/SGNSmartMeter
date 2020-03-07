/*
 * JsonFactory.cpp
 *
 *  Created on: 16-Feb-2020
 *      Author: tstone10
 */


#include <jansson.h>
#include <stdio.h>
#include <string>
#include <sstream>
#include <stack>
#include <iostream>
#include "JsonFactory.h"
#include "JsonException.h"
#include "Constants.h"

using namespace std;

JsonFactory::JsonFactory() {
    pJRoot      = NULL;
    iNoOfPkts   = 0;
    iCurPkt     = 0;
}

JsonFactory::JsonFactory(JsonFactory &je) {
    pJRoot      = je.pJRoot;
    iNoOfPkts   = je.iNoOfPkts;
    iCurPkt     = je.iCurPkt;

    // without this, the guy who invokes this will decref and crash
    if(1 < iNoOfPkts) {
        vector<json_t *>::iterator iter;
        for(iter = pJRoots.begin(); iter != pJRoots.end(); iter++) {
            json_t *tempRoot = *iter;
            json_incref(tempRoot);
        }
    } else {
        json_incref(pJRoot);
    }
}

void JsonFactory::clear() {
    json_t *tempRoot = NULL;
    if(1 < iNoOfPkts) {
        vector<json_t *>::iterator iter;
        for(iter = pJRoots.begin(); iter != pJRoots.end(); iter++) {
            tempRoot = *iter;
            json_decref(tempRoot);
        }
        pJRoots.clear();
    } else {
        if(pJRoot) json_decref(pJRoot);
    }
    iNoOfPkts   = 0;
    iCurPkt     = 0;
}

JsonFactory::~JsonFactory() {
    clear();
}

unsigned char JsonFactory::isEnd() {
    return (0 > iCurPkt);
}

string JsonFactory::getJsonString() throw(JsonException) {
    string strJPkt;
    if(NULL == pJRoot) {
        throw JsonException("No content");
    }

    char *pRet  = json_dumps(pJRoot, JSON_COMPACT);
    if(pRet) {
        strJPkt     = pRet;
        free(pRet);
    }
    return strJPkt;
}

json_t *JsonFactory::getRoot() {
    return pJRoot;
}

unsigned char JsonFactory::isMultiPktJson(string strJson) {
    json_error_t error = {0};
    unsigned int iLoop = 0, iRet = 0, iNewPkts = 0;
    stack<char> braces;
    json_t *pTemp = NULL, *pFirst = NULL;
    stringstream ss;
    char ch, isInQuotes = 0;
    vector<json_t *> newVector;

    /* keep pushing the '{' braces until a '}' is got.
     * If so, then pop and check if stack is empty.
     * When stack is empty that is when one full json packet is got
     */
    unsigned int iLen   = strJson.length();
    for(iLoop = 0, isInQuotes = 0; iLoop < iLen; iLoop++) {
        ch = strJson.at(iLoop);
        ss << ch;
        if('"' == ch && (0 == iLoop || strJson.at(iLoop-1) != '\\')) {
            isInQuotes = 1 - isInQuotes;
        }
        if(!isInQuotes) {
            if('{' == ch) {
                braces.push('{');
            } else if('}' == ch) {
                braces.pop();
                if(0 == braces.size()) {
                    pTemp = json_loads(ss.str().c_str(), 0, &error);
                    if(pTemp) {
                        newVector.push_back(pTemp);
                        iNewPkts++;
                        pFirst  = (pFirst) ? pFirst : pTemp;
                        iRet = 1;
                    }
                    ss.str("");
                }
            }
        }
    }

    //  If we are setting a string to existing valid packet then this is will execute
    if(iRet) {
        clear();
        pJRoot      = pFirst;
        pJRoots     = newVector;
        iCurPkt     = 0;
        iNoOfPkts   = iNewPkts;
    }
    return iRet;
}

int JsonFactory::getArraySize(json_t *jsArrayObj) {
    if(json_is_array(jsArrayObj)) {
        return json_array_size(jsArrayObj);
    }
    return 0;
}

JsonFactory JsonFactory::getObjAt(json_t* &jsArrayObj, int iIndex) throw (JsonException) {
    JsonFactory jsRoot;
    json_t *pTemp = NULL;

    if(getArraySize(jsArrayObj) > iIndex) {
        throw JsonException("&& JsonFactory: Index out of bounds");
    }
    pTemp   = json_array_get(jsArrayObj, iIndex);
    jsRoot.iNoOfPkts    = 1;
    jsRoot.iCurPkt      = 0;
    jsRoot.pJRoot       = pTemp;
    return jsRoot;
}

void JsonFactory::increment() {
    iCurPkt = (0 > iCurPkt) ? 0 : iCurPkt+1;
    if(iCurPkt >= iNoOfPkts) {
        iCurPkt = -1;
    } else {
        pJRoot = pJRoots[iCurPkt];
    }
}

void JsonFactory::decrement() {
    iCurPkt = (iCurPkt >= iNoOfPkts) ? (iNoOfPkts-1) : iCurPkt-1;
    if(0 > iCurPkt) {
        iCurPkt = -1;
    } else {
        pJRoot = pJRoots[iCurPkt];
    }
}

void JsonFactory::operator++() { increment(); }
void JsonFactory::operator++(int) { increment(); }
void JsonFactory::operator--() { decrement(); }
void JsonFactory::operator--(int) { decrement(); }
void JsonFactory::gotoFirst() { pJRoot = (1 < iNoOfPkts) ? pJRoots[0] : pJRoot; }
void JsonFactory::gotoLast() { pJRoot = (1 < iNoOfPkts) ? pJRoots[iNoOfPkts-1] : pJRoot; }

void JsonFactory::setJsonString(string jsonStr) throw(JsonException) {
    json_error_t error = {0};
    json_t *pTemp = NULL;

    if(jsonStr.empty()) {
        throw JsonException("&& JsonFactory: Invalid JSON string");
    }

    //  Load the json
    pTemp = json_loads(jsonStr.c_str(), 0, &error);
    if(pTemp) {
        clear();
        iNoOfPkts   = 1;
        iCurPkt     = 0;
        pJRoot      = pTemp;
    } else if(!isMultiPktJson(jsonStr) ) {
        throw JsonException("&& JsonFactory: Invalid JSON string");
    }
}

void JsonFactory::addStringValue(string strKey, string strVal) {
    if(1 < iNoOfPkts && (0 > iCurPkt || iCurPkt >= iNoOfPkts)) {
        return;
    }
    if(!strKey.empty() && !strVal.empty()) {
        if(NULL == pJRoot) {
            pJRoot      = json_object();
            iNoOfPkts   = 1;
        }
        json_object_set_new(pJRoot, strKey.c_str(), json_string(strVal.c_str()));
    }
}

void JsonFactory::addIntValue(string strKey, int iVal) {
    if(1 < iNoOfPkts && (0 > iCurPkt || iCurPkt >= iNoOfPkts)) {
        return;
    }
    if(!strKey.empty()) {
        if(NULL == pJRoot) {
            pJRoot      = json_object();
            iNoOfPkts   = 1;
        }
        json_object_set_new(pJRoot, strKey.c_str(), json_integer(iVal));
    }
}

void JsonFactory::addJsonObj(string strKey, JsonFactory jsObj) {
    if(1 < iNoOfPkts && (0 > iCurPkt || iCurPkt >= iNoOfPkts)) {
        return;
    }
    if(!strKey.empty()) {
        if(NULL == pJRoot) {
            pJRoot      = json_object();
            iNoOfPkts   = 1;
        }
        if(jsObj.pJRoot) {
            json_object_set_new(pJRoot, strKey.c_str(), jsObj.pJRoot);
        }
    }
}

unsigned char JsonFactory::isKeyAvailable(string strKey) {
    json_t *pJsonObj= NULL;
    if(1 < iNoOfPkts && (0 > iCurPkt || iCurPkt >= iNoOfPkts)) {
        return 0;
    } else if(NULL == pJRoot) {
        return 0;
    }
    pJsonObj    = json_object_get(pJRoot, strKey.c_str());
    return (NULL != pJsonObj);
}

void JsonFactory::validateJSONAndGetValue(string key, string &val, json_t *pJObj) throw(JsonException) {
    json_t *pJsonObj= NULL;
    json_t *pMyRoot = pJRoot;

    if(NULL != pJObj) {
        pMyRoot = pJObj;
    } else if(1 < iNoOfPkts && (0 > iCurPkt || iCurPkt >= iNoOfPkts)) {
        return;
    }

    if(NULL == pMyRoot) {
        throw JsonException("&& JsonFactory: No content");
    }
    if(key.empty()) {
        throw JsonException("&& JsonFactory: Invalid key");
    }
    pJsonObj        = json_object_get(pMyRoot, key.c_str());
    if(NULL == pJsonObj) {
        throw JsonException("&& JsonFactory: Key \"" + key + "\" not found");
    }
    int iType   = json_typeof(pJsonObj);
    if(iType != JSON_STRING) {
        throw JsonException("&& JsonFactory: JSON data type for key " + key + " mismatch");
    }
    val = json_string_value(pJsonObj);
}

void JsonFactory::validateJSONAndGetValue(string key, int &val, json_t *pJObj) throw(JsonException) {
    json_t *pJsonObj= NULL;
    json_t *pMyRoot = pJRoot;
    if(NULL != pJObj) {
        pMyRoot = pJObj;
    } else if(1 < iNoOfPkts && (0 > iCurPkt || iCurPkt >= iNoOfPkts)) {
        return;
    }

    if(NULL == pMyRoot) {
        throw JsonException("&& JsonFactory: No content");
    }
    if(key.empty()) {
        throw JsonException("&& JsonFactory: Invalid key");
    }
    pJsonObj        = json_object_get(pMyRoot, key.c_str());
    if(NULL == pJsonObj) {
        throw JsonException("&& JsonFactory: Key \"" + key + "\" not found");
    }
    int iType   = json_typeof(pJsonObj);
    if(iType != JSON_INTEGER) {
        throw JsonException("&& JsonFactory: JSON data type for key " + key + " mismatch");
    }
    val = json_integer_value(pJsonObj);
}

void JsonFactory::validateJSONAndGetValue(string key, json_t* &val, json_t *pJObj) throw(JsonException) {
    json_t *pJsonObj= NULL;
    json_t *pMyRoot = pJRoot;
    if(NULL != pJObj) {
        pMyRoot = pJObj;
    } else if(1 < iNoOfPkts && (0 > iCurPkt || iCurPkt >= iNoOfPkts)) {
        return;
    }

    if(NULL == pMyRoot) {
        throw JsonException("&& JsonFactory: No content");
    }
    if(key.empty()) {
        throw JsonException("&& JsonFactory: Invalid key");
    }
    pJsonObj        = json_object_get(pMyRoot, key.c_str());
    if(NULL == pJsonObj) {
        throw JsonException("&& JsonFactory: Key \"" + key + "\" not found");
    }
    int iType   = json_typeof(pJsonObj);
    if(iType != JSON_OBJECT) {
        throw JsonException("&& JsonFactory: JSON data type for key " + key + " mismatch");
    }
    val = pJsonObj;
}

