/**
  ******************************************************************************
  * @file    qmfilesystem_p.h
  * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
  * @date    24.02.2016
  *
  ******************************************************************************
 */

#ifndef QMFILESYSTEM_P_H_
#define QMFILESYSTEM_P_H_

#include <stdint.h>
#include <string>
#include "qmfile.h"

class QmFileSystemPcb {
public:
	QmFileSystemPcb() {};
	virtual ~QmFileSystemPcb() {};
	virtual uintmax_t openFile(const std::string &name, QmFile::OpenMode mode) = 0;
	virtual bool closeFile(uintmax_t handle) = 0;
	virtual int64_t readFile(uintmax_t handle, uint8_t *data, int64_t size) = 0;
	virtual int64_t writeFile(uintmax_t handle, const uint8_t *data, int64_t size) = 0;
	virtual int64_t getFilePosition(uintmax_t handle) = 0;
	virtual bool seekFilePosition(uintmax_t handle, uint32_t pos) = 0;
	virtual bool isEndOfFile(uintmax_t handle) = 0;
	virtual int64_t getFileSize(uintmax_t handle) = 0;
	virtual bool isFileExist(const std::string &name) = 0;
	virtual bool createFile(const std::string &name) = 0;
	virtual bool removeFile(const std::string &name) = 0;
	virtual bool renameFile(const std::string &old_name, const std::string &new_name) = 0;
	virtual int  checkFileSystem() = 0;
};

QmFileSystemPcb* qmstorageSpiffsAcquirePcb(const std::string &dir);
void qmstorageSpiffsReleasePcb(QmFileSystemPcb *pcb);

#endif /* QMFILESYSTEM_P_H_ */
