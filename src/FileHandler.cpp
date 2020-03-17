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
#include "FileHandler.h"

FileHandler *FileHandler::pFileHandler = NULL;


FileHandler :: FileHandler() {
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

bool FileHandler::rmdir(std::string str_dirname) {
	const char *dirname	= str_dirname.c_str();
    DIR *dir;
    struct dirent *entry;
    char path[PATH_MAX];

    dir = opendir(dirname);
    if (dir == NULL) {
        return false;
    }

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
	remove(dirname);

	return true;
}

int FileHandler::copy_data(struct archive * ar, struct archive * aw) {
    int iRetVal;
    const void * buff;
    size_t size;
    off_t offset;

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

void FileHandler::pushToQ(std::string strFileName) {
    pthread_mutex_lock(&qLock);
    msgQ.push(strFileName);
    pthread_cond_signal(&qCond);
    pthread_mutex_unlock(&qLock);
}

void *FileHandler :: run(void *pUserData) {
	FileHandler *pThis	= reinterpret_cast<FileHandler *>(pUserData);
	std::string strFileName;

	while(true) {
		pthread_mutex_lock(&pThis->qLock);
		if(pThis->msgQ.empty()) {
			std::cout << "FileHandler: Waiting for messages" << std::endl;
			pthread_cond_wait(&pThis->qCond, &pThis->qLock);
		}
		strFileName = pThis->msgQ.front();
		pThis->msgQ.pop();
		std::cout << "FileHandler: Got a file, " << strFileName << ", to extract" << std::endl;
		pthread_mutex_unlock(&pThis->qLock);

		//	delete the temporary download folder where previous downloads may exist
		std::string tmp_dwld_path	= std::string(TECHNO_SPURS_ROOT_PATH) + std::string(TECHNO_SPURS_TEMP_DWLD_PATH);
		pThis->rmdir(tmp_dwld_path);

		//	extract the downloaded file in temp path and make sure it is extracting properly
		std::string dwld_path	= std::string(TECHNO_SPURS_ROOT_PATH) + std::string(TECHNO_SPURS_DOWNLOAD_PATH);

		//	check if extract is working
		chdir(dwld_path.c_str());
		if(pThis->extract(strFileName)) {

			//	now delete the actual folder
			std::string app_path	= std::string(TECHNO_SPURS_ROOT_PATH) + std::string(TECHNO_SPURS_APP_FOLDER);
			pThis->rmdir(app_path);

			//	extract the downloaded file to app path
			chdir(TECHNO_SPURS_ROOT_PATH);
			if(pThis->extract(strFileName)) {
				//	Send a success message, probably with a version
			}
		} else {
			//	Send a message to techno spurs control-panel saying failed extracting
		}
	}
	return NULL;
}
