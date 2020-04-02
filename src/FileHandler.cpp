/*sgn
 * FileHandler.cpp
 *
 *  Created on: 14-Mar-2020
 *      Author: tstone10
 */

#include <pthread.h>
#include <stdio.h>
#include <archive.h>
#include <archive_entry.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <dirent.h>
#include "Constants.h"
#include "JsonFactory.h"
#include "JabberClient.h"
#include "FileLogger.h"
#include "FileHandler.h"

FileHandler *FileHandler::pFileHandler = NULL;


FileHandler :: FileHandler() : log(Logger::getInstance()){
	qLock   = PTHREAD_MUTEX_INITIALIZER;
	qCond   = PTHREAD_COND_INITIALIZER;
	pthread_t file_handler_thread;
	pthread_create(&file_handler_thread, NULL, &run, this);
	pthread_detach(file_handler_thread);
}

FileHandler :: ~FileHandler() {

}

FileHandler *FileHandler::getInstance() {
	if(NULL == pFileHandler) {
		pFileHandler = new FileHandler();
	}
	return pFileHandler;
}

bool FileHandler::isDirPresent(std::string strBaseDir, std::string strDirName) {
	const char *baseDir	= strBaseDir.c_str();
	const char *dir2Chk	= strDirName.c_str();

	DIR *pDir;
	struct dirent *pDirEntry;

	pDir = opendir(baseDir);
	if (pDir == NULL) {
		return false;
	}

	while( NULL != (pDirEntry = readdir(pDir)) ) {
		if (!strcmp(pDirEntry->d_name, ".") || !strcmp(pDirEntry->d_name, "..")) {
			continue;
		}

		if (DT_DIR == pDirEntry->d_type && !strcmp(dir2Chk, pDirEntry->d_name)) {
			closedir(pDir);
			return true;
		}
	}
	closedir(pDir);
	return false;
}

bool FileHandler::rmdir(std::string str_dirname, bool bRemoveBase) {
    const char *dirname	= str_dirname.c_str();
    DIR *dir;
    struct dirent *entry;
    char path[PATH_MAX];

    dir		= opendir(dirname);
    if(dir == NULL) return false;

    while (NULL != (entry = readdir(dir)) ) {
        if (!strcmp(entry->d_name, ".") || !strcmp(entry->d_name, "..")) {
            continue;
	}

	snprintf(path, (size_t) PATH_MAX, "%s/%s", dirname, entry->d_name);
	if (entry->d_type == DT_DIR) {
            rmdir(path);
	}
	remove(path);
    }

    closedir(dir);
    if(bRemoveBase) remove(dirname);

    return true;
}

int FileHandler::copy_data(struct archive * ar, struct archive * aw) {
    int iRetVal;
    const void * buff;
    size_t size;
    la_int64_t offset;

    for (;;) {
        iRetVal = archive_read_data_block(ar, & buff, & size, & offset);
        if (iRetVal == ARCHIVE_EOF)
            return (ARCHIVE_OK);
        if (iRetVal < ARCHIVE_OK)
            return (iRetVal);
        iRetVal = archive_write_data_block(aw, buff, size, offset);
        if (iRetVal < ARCHIVE_OK) {
            fprintf(stderr, "%s\n", archive_error_string(aw));
            return (iRetVal);
        }
    }
    return 0;
}

bool FileHandler::extract(std::string filename) {
    struct archive *archive_a;
    struct archive *extract_a;
    struct archive_entry * entry;
    int flags;
    int iRetVal;

    /* Select which attributes we want to restore. */
    flags = ARCHIVE_EXTRACT_TIME;
    flags |= ARCHIVE_EXTRACT_PERM;
    flags |= ARCHIVE_EXTRACT_ACL;
    flags |= ARCHIVE_EXTRACT_FFLAGS;

    archive_a = archive_read_new();
    archive_read_support_format_all(archive_a);

    extract_a = archive_write_disk_new();
    archive_write_disk_set_options(extract_a, flags);
    archive_write_disk_set_standard_lookup(extract_a);

    if ((iRetVal = archive_read_open_filename(archive_a, filename.c_str(), 10240)))
        return false;
    for (;;) {
        iRetVal = archive_read_next_header(archive_a, &entry);

        if (iRetVal == ARCHIVE_EOF)
            break;
        if (iRetVal < ARCHIVE_OK)
            fprintf(stderr, "%s\n", archive_error_string(archive_a));
        if (iRetVal < ARCHIVE_WARN)
            return false;

        iRetVal = archive_write_header(extract_a, entry);
        if (iRetVal < ARCHIVE_OK) {
            fprintf(stderr, "%s\n", archive_error_string(extract_a));
        } else if (archive_entry_size(entry) > 0) {
            iRetVal = copy_data(archive_a, extract_a);
            if (iRetVal < ARCHIVE_OK)
                fprintf(stderr, "%s\n", archive_error_string(extract_a));
            if (iRetVal < ARCHIVE_WARN)
                return false;
        }
        iRetVal = archive_write_finish_entry(extract_a);
        if (iRetVal < ARCHIVE_OK)
            fprintf(stderr, "%s\n", archive_error_string(extract_a));
        if (iRetVal < ARCHIVE_WARN)
            return false;
    }
    archive_read_close(archive_a);
    archive_read_free(archive_a);

    archive_write_close(extract_a);
    archive_write_free(extract_a);

    return true;
}

void FileHandler::pushToQ(std::pair<std::string, int> qCntnt) {
    pthread_mutex_lock(&qLock);
    msgQ.push(qCntnt);
    pthread_cond_signal(&qCond);
    pthread_mutex_unlock(&qLock);
}

std::string FileHandler::makeRespPkt(int cmdNo, std::string strFrom, bool isSucces, std::string strRemarks) {
	JsonFactory jsRoot;
	jsRoot.addIntValue("command_no", cmdNo);
	jsRoot.addStringValue("from", strFrom);
	jsRoot.addBoolValue("success", isSucces);
	jsRoot.addStringValue("remarks", strRemarks);
	return jsRoot.getJsonString();
}

void *FileHandler :: run(void *pUserData) {
	FileHandler *pThis	= reinterpret_cast<FileHandler *>(pUserData);
	std::string strDstPath, strResp, strFolder;
	int cmdNo = 0;
	std::pair<std::string, int> qVal;
	Logger &info_log = Logger::getInstance();

	Config *pCfg				= Config::getInstance();
	XmppDetails xmpp			= pCfg->getXmppDetails();
	std::string cPanelJid		= xmpp.getCPanelJid();
	std::string strUnqId		= pCfg->getRPiUniqId();
	JabberClient *pJabberClient	= JabberClient::getJabberClient();

	while(true) {
		pthread_mutex_lock(&pThis->qLock);
		if(pThis->msgQ.empty()) {
			info_log << "FileHandler: Waiting for messages" << std::endl;
			pthread_cond_wait(&pThis->qCond, &pThis->qLock);
		}
		qVal = pThis->msgQ.front();
		pThis->msgQ.pop();
		pthread_mutex_unlock(&pThis->qLock);

		strFolder	= qVal.first;
		strDstPath	= std::string(TECHNO_SPURS_ROOT_PATH) + qVal.first;
		cmdNo		= qVal.second;

		info_log << "FileHandler: Got a extract request. CmdNo: " << cmdNo << ", Folder: " << strFolder << std::endl;

		std::string dwld_path	= std::string(TECHNO_SPURS_ROOT_PATH) + std::string(TECHNO_SPURS_DOWNLOAD_FOLDER);
		chdir(dwld_path.c_str());

		std::string strZipFile	= std::string(TECHNO_SPURS_ROOT_PATH) + std::string(TECHNO_SPURS_DOWNLOAD_FILE);
		if(pThis->extract(strZipFile)) {
			info_log << "FileHandler: File Extract Check: Through" << std::endl;

			//	now delete the actual folder
			info_log << "Now deleting the actual folder" << std::endl;
			pThis->rmdir(strDstPath);

			//	Create a folder if it is not already
			if(!pThis->isDirPresent(dwld_path, strFolder)) {
				mkdir(strDstPath.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
				chdir(strDstPath.c_str());
			} else {
				chdir(TECHNO_SPURS_ROOT_PATH);
			}

			if(pThis->extract(strZipFile)) {
				info_log << "FileHandler: Extracted zip file: " << strZipFile << " for command no " << cmdNo << std::endl;

				//	Extraction is succeeded. Delete all files at Download folder
				info_log << "FileHandler: Deleting files at " << dwld_path << "/*" << std::endl;
				pThis->rmdir(dwld_path, false);

				//	Send a success message
				strResp	= pThis->makeRespPkt(cmdNo, strUnqId, true, strFolder);
				pJabberClient->sendMsgTo(strResp, cPanelJid);

				Utils::sendPacket(WDOG_Tx_PORT, "{\"command\":\"upload_logs\"}");
				info_log << "FileHandler: Going to reboot in 30 secs..." << std::endl;
				info_log.uploadLogs();
				sleep(30);	// Let us wait for 30 secs

				Utils::sendPacket(WDOG_Tx_PORT, "{\"command\":\"reboot\"}");
			}
		} else {
			//	Control is not supposed to reach here. We are in bad shape now.
			info_log << "FileHandler: Error: Failed extracting zip file with command no " << cmdNo << std::endl;
			strResp	= pThis->makeRespPkt(cmdNo, strUnqId, false, "failed extracting zip");
			pJabberClient->sendMsgTo(strResp, cPanelJid);
		}
	}
	return NULL;
}
