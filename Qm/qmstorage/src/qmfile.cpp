/**
 ******************************************************************************
 * @file    qmfile.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    24.02.2016
 *
 ******************************************************************************
 */

#include "qmdebug.h"
#include "qmfilesystem_p.h"

#include "qmfile.h"

class QmFilePrivate {
public:
	std::string name;
	QmFileSystemPcb *fs_pcb;
	bool is_open;
	uintmax_t handle;
};

QmFile::QmFile(const std::string &dir, const std::string &name) :
	d_ptr(new QmFilePrivate())
{
	d_ptr->name = name;
	d_ptr->fs_pcb = qmstorageSpiffsAcquirePcb(dir);
	QM_ASSERT(d_ptr->fs_pcb);
	d_ptr->is_open = false;
}

QmFile::~QmFile()
{
	if (d_ptr->is_open)
		d_ptr->fs_pcb->closeFile(d_ptr->handle);
	qmstorageSpiffsReleasePcb(d_ptr->fs_pcb);
	delete d_ptr;
}

bool QmFile::open(OpenMode mode) {
	if (d_ptr->is_open)
		return false;
	d_ptr->handle = d_ptr->fs_pcb->openFile(d_ptr->name, mode);
	if (d_ptr->handle == 0)
		return false;
	d_ptr->is_open = true;
	return true;
}

bool QmFile::close() {
	if (!d_ptr->is_open)
		return false;
	if (!d_ptr->fs_pcb->closeFile(d_ptr->handle))
		return false;
	d_ptr->is_open = false;
	return true;
}

int64_t QmFile::read(uint8_t *data, int64_t size) {
	if (!d_ptr->is_open)
		return -1;
	return d_ptr->fs_pcb->readFile(d_ptr->handle, data, size);
}

int64_t QmFile::write(const uint8_t *data, int64_t size) {
	if (!d_ptr->is_open)
		return -1;
	return d_ptr->fs_pcb->writeFile(d_ptr->handle, data, size);
}

int64_t QmFile::pos() const {
	if (!d_ptr->is_open)
		return -1;
	return d_ptr->fs_pcb->getFilePosition(d_ptr->handle);
}

bool QmFile::seek(uint32_t pos) {
	if (!d_ptr->is_open)
		return false;
	return d_ptr->fs_pcb->seekFilePosition(d_ptr->handle, pos);
}

bool QmFile::atEnd() const {
	if (!d_ptr->is_open)
		return false;
	return d_ptr->fs_pcb->isEndOfFile(d_ptr->handle);
}

int64_t QmFile::size() const {
	if (!d_ptr->is_open)
		return false;
	return d_ptr->fs_pcb->getFileSize(d_ptr->handle);
}

bool QmFile::exists(const std::string& dir, const std::string& name) {
	QmFileSystemPcb* pcb = qmstorageSpiffsAcquirePcb(dir);
	if (pcb == 0)
		return false;
	bool result = pcb->isFileExist(name);
	qmstorageSpiffsReleasePcb(pcb);
	return result;
}

bool QmFile::create(const std::string& dir, const std::string& name) {
	QmFileSystemPcb* pcb = qmstorageSpiffsAcquirePcb(dir);
	if (pcb == 0)
		return false;
	bool result = pcb->createFile(name);
	qmstorageSpiffsReleasePcb(pcb);
	return result;
}

bool QmFile::remove(const std::string& dir, const std::string& name) {
	QmFileSystemPcb* pcb = qmstorageSpiffsAcquirePcb(dir);
	if (pcb == 0)
		return false;
	bool result = pcb->removeFile(name);
	qmstorageSpiffsReleasePcb(pcb);
	return result;
}

bool QmFile::rename(const std::string& dir, const std::string& old_name,
		const std::string& new_name) {
	QmFileSystemPcb* pcb = qmstorageSpiffsAcquirePcb(dir);
	if (pcb == 0)
		return false;
	bool result = pcb->renameFile(old_name, new_name);
	qmstorageSpiffsReleasePcb(pcb);
	return result;
}
