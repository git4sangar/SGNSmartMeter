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
}

JsonFactory::JsonFactory(const JsonFactory &je) {
    pJRoot      = je.pJRoot;
    json_incref(pJRoot);
}

void JsonFactory::clear() {
    if(pJRoot) json_decref(pJRoot);
}

JsonFactory::~JsonFactory() {
    clear();
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

int JsonFactory::getArraySize(json_t *jsArrayObj) {
    if(json_is_array(jsArrayObj)) {
        return json_array_size(jsArrayObj);
    }
    return 0;
}

JsonFactory JsonFactory::getObjAt(json_t* jsArrayObj, int iIndex) throw (JsonException) {
    JsonFactory jsRoot;
    json_t *pTemp = NULL;

    if(getArraySize(jsArrayObj) < iIndex) {
        throw JsonException("&& JsonFactory: Index out of bounds");
    }
    pTemp   = json_array_get(jsArrayObj, iIndex);
    jsRoot.pJRoot   = pTemp;
    return jsRoot;
}

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
        pJRoot      = pTemp;
    }
}

void JsonFactory::addStringValue(string strKey, string strVal) {
    if(!strKey.empty() && !strVal.empty()) {
        if(NULL == pJRoot) {
            pJRoot      = json_object();
        }
        json_object_set_new(pJRoot, strKey.c_str(), json_string(strVal.c_str()));
    }
}

void JsonFactory::addIntValue(string strKey, int iVal) {
    if(!strKey.empty()) {
        if(NULL == pJRoot) {
            pJRoot      = json_object();
        }
        json_object_set_new(pJRoot, strKey.c_str(), json_integer(iVal));
    }
}

void JsonFactory::addJsonObj(string strKey, JsonFactory jsObj) {
    if(!strKey.empty()) {
        if(NULL == pJRoot) {
            pJRoot      = json_object();
        }
        if(jsObj.pJRoot) {
            json_object_set_new(pJRoot, strKey.c_str(), jsObj.pJRoot);
        }
    }
}

unsigned char JsonFactory::isKeyAvailable(string strKey) {
    json_t *pJsonObj= NULL;
    if(NULL == pJRoot) {
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
    if(iType != JSON_OBJECT && iType != JSON_ARRAY) {
        throw JsonException("&& JsonFactory: JSON data type for key " + key + " mismatch");
    }
    val = pJsonObj;
}
