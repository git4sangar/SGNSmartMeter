/*
 * JsonFactory.h
 *
 *  Created on: 16-Feb-2020
 *      Author: tstone10
 */

#ifndef JSONFACTORY_H_
#define JSONFACTORY_H_


#include <string>
#include <jansson.h>
#include <vector>
#include "JsonException.h"

using namespace std;

class JsonFactory {
    //  As this is a pointer, wherever this object is passed as value pJRoot is copied.
    //  If the new object decrefs this pointer, the object invoked this will be holding an invalid pointer
    //  If that too tries to decrefs, BANG...
    json_t *pJRoot;

public:
    JsonFactory();
    JsonFactory(const JsonFactory &je);
    virtual ~JsonFactory();


    json_t *getRoot();
    void clear();

    bool isArray(json_t *jsObj) { return json_is_array(jsObj); }
    int getArraySize(json_t *jsArrayObj);
    JsonFactory getObjAt(json_t* jsArrayObj, int iIndex) throw (JsonException) ;

    void setJsonString(string jsonStr) throw(JsonException);
    void addStringValue(string strKey, string strVal);
    void addIntValue(string strKey, int iVal);
    void addBoolValue(string strKey, bool bVal);
    void addJsonObj(string strKey, JsonFactory jsObj);

    string getJsonString() throw(JsonException);

    void validateJSONAndGetValue(string key, string &val, json_t *pJObj = NULL) throw(JsonException);
    void validateJSONAndGetValue(string key, int &val, json_t *pJObj = NULL) throw(JsonException);
    void validateJSONAndGetValue(string key, json_t* &val, json_t *pJObj = NULL) throw(JsonException);
    unsigned char isKeyAvailable(string strKey);
};


#endif /* JSONFACTORY_H_ */
