/* sgn
 * FileHandler.h
 *
 *  Created on: 14-Mar-2020
 *      Author: tstone10
 */

#ifndef FILEHANDLER_H_
#define FILEHANDLER_H_

#include <archive.h>
#include <archive_entry.h>
#include <iostream>
#include <queue>
#include <pthread.h>

class FileHandler {
    std::queue<std::string> msgQ;
    pthread_mutex_t qLock;
    pthread_cond_t qCond;

    int copy_data(struct archive * ar, struct archive * aw);
    bool rmdir(std::string dirname);

	static FileHandler *pFileHandler;
	FileHandler();

public:
	virtual ~FileHandler();

	void pushToQ(std::string strMsg);
	bool extract(std::string filename);

	static FileHandler *getInstance();
	static void *run(void *pUserData);
};



#endif /* FILEHANDLER_H_ */
