/*sgn sgn
 * FileLogger.h
 *
 *  Created on: 20-Mar-2020
 *      Author: tstone10
 */

#ifndef FILELOGGER_H_
#define FILELOGGER_H_

#include <pthread.h>
#include <sstream>
#include <iostream>
#include <stdio.h>
#include <queue>

#define LOG_FILE_NAME	"log_file_"

class Logger {
    pthread_mutex_t qLock;
    pthread_cond_t qCond;
    bool bTime;
    pthread_mutex_t writeLock;
	std::queue<std::string> logQ;

	std::stringstream ss_log;
	Logger();
	void stampTime();
	static Logger *pLogger;
	static void *run(void *);

public:

	virtual ~Logger() {}
	void uploadLogs();
	void pushToQ(std::string log_data);

	Logger &operator<<(const std::string strLog);
	Logger &operator<<(const int val);
//	Logger &operator<<(const char *log);

    // this is the type of std::cout
    typedef std::basic_ostream<char, std::char_traits<char> > CoutType;
    // this is the function signature of std::endl
    typedef CoutType& (*StandardEndLine)(CoutType&);
    Logger &operator << (StandardEndLine manip);

	static Logger &getInstance();
};


#endif /* FILELOGGER_H_ */
