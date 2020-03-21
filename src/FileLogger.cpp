/*sgn
 * FileLogger.cpp
 *
 *  Created on: 20-Mar-2020
 *      Author: tstone10
 */

#include <sstream>
#include <iostream>
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "Constants.h"
#include "FileLogger.h"

Logger *Logger::pLogger = NULL;

Logger &Logger:: getInstance() {
	if(NULL == pLogger) {
		pLogger = new Logger();
	}
	return *pLogger;
}

Logger::Logger() : bTime(true) {
	/*time_t now;
	time(&now);
	char suffix[32];
	struct tm *local	= localtime(&now);
	sprintf(suffix, "%02d_%02d_%02d_%02d_%02d_%02d",
			local->tm_year+1900, local->tm_mon+1, local->tm_mday,
			local->tm_hour, local->tm_min, local->tm_sec);
	std::string strFile	= std::string(TECHNO_SPURS_ROOT_PATH) + std::string(TECHNO_SPURS_LOG_PATH) +
							std::string(LOG_FILE_NAME) + std::string(suffix);
	fp = fopen(strFile.c_str(), "w");*/
}

void Logger::stampTime() {
	struct timeval st;
	gettimeofday(&st,NULL);

	//	why do i need secs since epoch? get secs from now
	//	1584718500 => secs since epoch till now (20-Mar-20 21:05)
	unsigned long secs	= st.tv_sec - 1584718500;
	secs = secs % 36000;	// reset secs every 10 hours
	unsigned long msecs	= st.tv_usec / 1000;
	unsigned long usecs	= st.tv_usec % 1000;
	std::cout << secs << ":" << msecs << ":" << usecs << ": ";
	ss_log << secs << ":" << msecs << ":" << usecs << ": ";
}

Logger &Logger::operator << (StandardEndLine manip) {
	ss_log << std::endl; std::cout << std::endl; bTime = true;

	ss_log.seekg(0, std::ios::end);
	if(ss_log.tellg() > MAX_LOG_SIZE) {
		pushToQ(ss_log.str());
		ss_log.str(""); ss_log.clear();
	}
	return *this;
}

Logger &Logger::operator <<(const std::string strMsg) {
	if(bTime) { stampTime(); bTime = false; }
	ss_log << strMsg; std::cout << strMsg;
	return *this;
}

Logger &Logger::operator <<(int iVal) {
	if(bTime) { stampTime(); bTime = false; }
	ss_log << iVal; std::cout << iVal;
	return *this;
}

//Logger &Logger::operator<<(const char *log) {
//	if(bTime) { stampTime(); bTime = false; }
//	std::string strMsg = std::string(log);
//	ss_log << strMsg; std::cout << strMsg;
//	return *this;
//}

void Logger::pushToQ(std::string strLogData) {
    pthread_mutex_lock(&qLock);
    logQ.push(strLogData);
    pthread_cond_signal(&qCond);
    pthread_mutex_unlock(&qLock);
}

void *Logger::run(void *pUserData) {
	Logger *pThis = reinterpret_cast<Logger *>(pUserData);
	std::string toUpload;

	while(1) {
		pthread_mutex_lock(&pThis->qLock);
		if(pThis->logQ.empty()) {
			std::cout << "LogUploader: Waiting for logs to upload" << std::endl;
			pthread_cond_wait(&pThis->qCond, &pThis->qLock);
		}
		toUpload = pThis->logQ.front();
		pThis->logQ.pop();
		pthread_mutex_unlock(&pThis->qLock);

		//	Write the logic to upload
	}
	return NULL;
}
