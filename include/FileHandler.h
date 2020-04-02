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
#include <utility>
#include <pthread.h>

class FileHandler {
    std::queue<std::pair<std::string, int> > msgQ;
    pthread_mutex_t qLock;
    pthread_cond_t qCond;
    Logger &log;

    int copy_data(struct archive * ar, struct archive * aw);
    bool rmdir(std::string dirname, bool bRemoveBase = true);
    std::string makeRespPkt(int cmdNo, std::string strFrom, bool isSucces, std::string strRemarks);
    bool isDirPresent(std::string strBase, std::string strDirName);

	static FileHandler *pFileHandler;
	static void *run(void *pUserData);
	FileHandler();

public:
	virtual ~FileHandler();

	void pushToQ(std::pair<std::string, int> qCntnt);
	bool extract(std::string filename);

	static FileHandler *getInstance();
};



#endif /* FILEHANDLER_H_ */
