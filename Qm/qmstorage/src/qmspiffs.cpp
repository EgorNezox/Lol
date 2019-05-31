/**
 ******************************************************************************
 * @file    qmspiffs.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    16.02.2016
 *
 ******************************************************************************
 */

#include <type_traits>
#include <limits>
#include <map>
#include "spiffs.h"

#include "qm.h"
#include "qmdebug.h"
#include "qmmutex.h"
#include "qmmutexlocker.h"
#include "qmserialnorflashdevice.h"
#include "qmfilesystem_p.h"

#include "qmspiffs.h"

#if !((SPIFFS_SINGLETON == 0) && (SPIFFS_HAL_CALLBACK_EXTRA == 1) && (SPIFFS_FILEHDL_OFFSET == 0) && (SPIFFS_CACHE == 0))
#error "Unsupported SPIFFS configuration"
#endif

#define DEFINE_PCB_FROM_FS(var_pcb, fs) \
	QmSpiffsFsPcb *var_pcb = static_cast<QmSpiffsFsPcb *>(fs->user_data);

class QmSpiffsFsPcb : public QmFileSystemPcb {
public:
	struct spiffs_t fs;
	QmMutex mutex;
	int ref_counter;
	QmSerialNORFlashDevice *device;
	uint32_t physical_address;
	uint32_t physical_size;
	uint8_t *work_buffer;
	uint8_t *fd_buffer;
private:
	uintmax_t openFile(const std::string &name, QmFile::OpenMode mode);
	bool closeFile(uintmax_t handle);
	int64_t readFile(uintmax_t handle, uint8_t *data, int64_t size);
	int64_t writeFile(uintmax_t handle, const uint8_t *data, int64_t size);
	int64_t getFilePosition(uintmax_t handle);
	bool seekFilePosition(uintmax_t handle, uint32_t pos);
	bool isEndOfFile(uintmax_t handle);
	int64_t getFileSize(uintmax_t handle);
	bool isFileExist(const std::string &name);
	bool createFile(const std::string &name);
	bool removeFile(const std::string &name);
	bool renameFile(const std::string &old_name, const std::string &new_name);
	int checkFileSystem();
};

static bool qmstorageSpiffsCheckMountsOverlaps(const QmSpiffs::Config &config);
static bool qmstorageSpiffsMount(const QmSpiffs::Config &config, QmSpiffsFsPcb **pcb);
static void qmstorageSpiffsUnmount(QmSpiffsFsPcb *pcb);
static QmSpiffsFsPcb* qmstorageSpiffsAllocatePcb(const spiffs_config &config, uint32_t fd_buffer_size);
static void qmstorageSpiffsFreePcb(QmSpiffsFsPcb *pcb);
static QmSpiffsFsPcb* qmstorageSpiffsGetMountedPcb(const std::string &dir);
static int32_t qmstorageSpiffsHalReadCallback(struct spiffs_t *fs, uint32_t addr, uint32_t size, uint8_t *dst);
static int32_t qmstorageSpiffsHalWriteCallback(struct spiffs_t *fs, uint32_t addr, uint32_t size, uint8_t *src);
static int32_t qmstorageSpiffsHalEraseCallback(struct spiffs_t *fs, uint32_t addr, uint32_t size);
static bool isPowerOfTwo(unsigned int value);

static std::map<std::string,QmSpiffsFsPcb*> qmstorage_fs_mounts;
static QmMutex qmstorage_fs_mutex;

static_assert(std::is_integral<spiffs_block_ix>::value, "");
static_assert(!std::numeric_limits<spiffs_block_ix>::is_signed, "");
static_assert(std::is_integral<spiffs_page_ix>::value, "");
static_assert(!std::numeric_limits<spiffs_page_ix>::is_signed, "");
static_assert(std::is_integral<spiffs_obj_id>::value, "");
static_assert(!std::numeric_limits<spiffs_obj_id>::is_signed, "");
static_assert(std::is_integral<spiffs_span_ix>::value, "");
static_assert(!std::numeric_limits<spiffs_span_ix>::is_signed, "");
static_assert(sizeof(uintmax_t) >= sizeof(spiffs_file), "");

bool QmSpiffs::mount(const std::string &dir, const Config &config) {
	QmMutexLocker locker(&qmstorage_fs_mutex);

	if (qmstorage_fs_mounts.find(dir) != qmstorage_fs_mounts.end())
		return false;
	if (!qmstorageSpiffsCheckMountsOverlaps(config))
		return false;

	QmSpiffsFsPcb *fs_pcb = 0;
	if (!qmstorageSpiffsMount(config, &fs_pcb)) {
		if (fs_pcb)
			qmstorageSpiffsFreePcb(fs_pcb);
		return false;
	}

	qmstorage_fs_mounts[dir] = fs_pcb;
	fs_pcb->ref_counter = 0;

	return true;
}

bool QmSpiffs::unmount(const std::string &dir) {
	QmMutexLocker locker(&qmstorage_fs_mutex);
	QmSpiffsFsPcb *fs_pcb = qmstorageSpiffsGetMountedPcb(dir);

	if (fs_pcb == 0)
		return false;
	if (fs_pcb->ref_counter > 0)
		return false;

	qmstorage_fs_mounts.erase(dir);
	qmstorageSpiffsUnmount(fs_pcb);
	qmstorageSpiffsFreePcb(fs_pcb);

	return true;
}

bool QmSpiffs::isMounted(const std::string &dir) {
	QmMutexLocker locker(&qmstorage_fs_mutex);
	return (qmstorage_fs_mounts.find(dir) != qmstorage_fs_mounts.end());
}

bool QmSpiffs::check(const std::string& dir) {
	QmSpiffsFsPcb *fs_pcb = 0;

	qmstorage_fs_mutex.lock();
	std::map<std::string,QmSpiffsFsPcb*>::iterator dir_mount = qmstorage_fs_mounts.find(dir);
	if (dir_mount != qmstorage_fs_mounts.end())
		fs_pcb = dir_mount->second;
	qmstorage_fs_mutex.unlock();
	if (fs_pcb == 0)
		return false;

	s32_t result = SPIFFS_check(&(fs_pcb->fs));
	return (result == 0);
}

bool QmSpiffs::format(const Config &config) {
	bool mounts_check_ok;
	qmstorage_fs_mutex.lock();
	mounts_check_ok = qmstorageSpiffsCheckMountsOverlaps(config);
	qmstorage_fs_mutex.unlock();
	if (!mounts_check_ok)
		return false;

	QmSpiffsFsPcb *fs_pcb = 0;
	if (qmstorageSpiffsMount(config, &fs_pcb))
		qmstorageSpiffsUnmount(fs_pcb);
	if (fs_pcb == 0)
		return false;

	bool success = true;
	if (SPIFFS_format(&(fs_pcb->fs)) != 0)
		success = false;
	qmstorageSpiffsFreePcb(fs_pcb);
	return success;
}

int QmSpiffs::getErrorCode()
{
	return SPIFFS_get_error_code();
}

void QmSpiffs::check_files(const std::string &d,  std::vector<std::string> &v)
{
	spiffs_DIR dir;
	struct spiffs_dirent pe;
	struct spiffs_dirent *p = &pe;

	QmSpiffsFsPcb *fs_pcb = qmstorageSpiffsGetMountedPcb(d);

	SPIFFS_opendir (&(fs_pcb->fs),d.c_str(), &dir);

	while ((p = SPIFFS_readdir(&dir,p)))
	{
		v.push_back(std::string((char*)p->name));
	}

	SPIFFS_closedir(&dir);

}

static bool qmstorageSpiffsCheckMountsOverlaps(const QmSpiffs::Config &config) {
	for (auto& i : qmstorage_fs_mounts) {
		QmSpiffsFsPcb *i_pcb = i.second;
		if ((config.device == i_pcb->device)
				&& (config.physical_address >= i_pcb->physical_address)
				&& (config.physical_address < (i_pcb->physical_address + i_pcb->physical_size))) {
			return false;
		}
	}
	return true;
}

static bool qmstorageSpiffsMount(const QmSpiffs::Config &config, QmSpiffsFsPcb **pcb) {
	uint32_t physical_block_size = 0;

	if (!((config.physical_address + config.physical_size) <= config.device->getTotalSize()))
		return false;
	if (!((config.physical_size/config.logical_block_size) <= std::numeric_limits<spiffs_block_ix>::max()))
		return false;
	if (!((config.physical_size/config.logical_page_size) <= std::numeric_limits<spiffs_page_ix>::max()))
		return false;
	if (!((2 + (config.physical_size/(2*config.logical_page_size))*2) <= std::numeric_limits<spiffs_obj_id>::max()))
		return false;
	if (!((config.physical_size/config.logical_page_size - 1) <= std::numeric_limits<spiffs_span_ix>::max()))
		return false;
	bool physical_address_is_aligned = false;
	std::list<QmSerialNORFlashDevice::Block> physical_blocks = config.device->getBlocks();
	for (auto& i : physical_blocks) {
		if (config.physical_address == i.address)
			physical_address_is_aligned = true;
		if ((config.physical_address <= i.address) && (i.address < (config.physical_address + config.physical_size))) {
			if ((physical_block_size == 0))
				physical_block_size = i.size;
			else if (i.size != physical_block_size)
				return false;
		}
	}
	if (!(physical_address_is_aligned && (physical_block_size > 0) && ((config.physical_size % config.logical_block_size) == 0)))
		return false;
	if (!((config.logical_block_size >= physical_block_size)
			&& ((config.logical_block_size % physical_block_size) == 0)
			&& (isPowerOfTwo(config.logical_block_size/physical_block_size))))
		return false;
	if (!((config.logical_block_size/config.logical_page_size >= 8)
			&& ((config.logical_block_size % config.logical_page_size) == 0)
			&& (isPowerOfTwo(config.logical_block_size/config.logical_page_size))))
		return false;

	spiffs_config fs_config;
	fs_config.hal_read_f = qmstorageSpiffsHalReadCallback;
	fs_config.hal_write_f = qmstorageSpiffsHalWriteCallback;
	fs_config.hal_erase_f = qmstorageSpiffsHalEraseCallback;
	fs_config.phys_size = config.physical_size;
	fs_config.phys_addr = config.physical_address;
	fs_config.phys_erase_block = physical_block_size;
	fs_config.log_block_size = config.logical_block_size;
	fs_config.log_page_size = config.logical_page_size;
	uint32_t fd_buffer_size = SPIFFS_buffer_bytes_for_filedescs(0, config.max_opened_files);

	*pcb = qmstorageSpiffsAllocatePcb(fs_config, fd_buffer_size);
	(*pcb)->fs.user_data = static_cast<void *>(*pcb);
	(*pcb)->device = config.device;
	(*pcb)->physical_address = config.physical_address;
	(*pcb)->physical_size = config.physical_size;

	s32_t result = SPIFFS_mount(&((*pcb)->fs), &fs_config, (*pcb)->work_buffer, (*pcb)->fd_buffer, fd_buffer_size, 0, 0, 0);

	return (result == 0);
}

static void qmstorageSpiffsUnmount(QmSpiffsFsPcb *pcb) {
	SPIFFS_unmount(&(pcb->fs));
}

static QmSpiffsFsPcb* qmstorageSpiffsAllocatePcb(const spiffs_config &config, uint32_t fd_buffer_size) {
	QmSpiffsFsPcb *pcb = new QmSpiffsFsPcb();
	pcb->work_buffer = new uint8_t[2*config.log_page_size];
	pcb->fd_buffer = new uint8_t[fd_buffer_size];
	return pcb;
}

static void qmstorageSpiffsFreePcb(QmSpiffsFsPcb *pcb) {
	delete[] pcb->fd_buffer;
	delete[] pcb->work_buffer;
	delete pcb;
}

static QmSpiffsFsPcb* qmstorageSpiffsGetMountedPcb(const std::string &dir) {
	std::map<std::string,QmSpiffsFsPcb*>::iterator dir_mount = qmstorage_fs_mounts.find(dir);
	if (dir_mount == qmstorage_fs_mounts.end())
		return 0;
	return dir_mount->second;
}

int32_t qmstorageSpiffsHalReadCallback(struct spiffs_t *fs, uint32_t addr, uint32_t size, uint8_t *dst) {
	DEFINE_PCB_FROM_FS(fs_pcb, fs);
	if (!fs_pcb->device->read(addr, dst, size))
		return -1;
	return SPIFFS_OK;
}

int32_t qmstorageSpiffsHalWriteCallback(struct spiffs_t *fs, uint32_t addr, uint32_t size, uint8_t *src) {
	DEFINE_PCB_FROM_FS(fs_pcb, fs);
	if (!fs_pcb->device->write(addr, src, size))
		return -1;
	return SPIFFS_OK;
}

int32_t qmstorageSpiffsHalEraseCallback(struct spiffs_t *fs, uint32_t addr, uint32_t size) {
	DEFINE_PCB_FROM_FS(fs_pcb, fs);
	if (!fs_pcb->device->erase(addr, size))
		return -1;
	return SPIFFS_OK;
}

static bool isPowerOfTwo(unsigned int value) {
	return (value != 0) && ((value & (value - 1)) == 0);
}

QmFileSystemPcb* qmstorageSpiffsAcquirePcb(const std::string &dir) {
	QmMutexLocker locker(&qmstorage_fs_mutex);
	QmSpiffsFsPcb *pcb = qmstorageSpiffsGetMountedPcb(dir);
	if (pcb != 0)
		pcb->ref_counter++;
	return pcb;
}

void qmstorageSpiffsReleasePcb(QmFileSystemPcb *pcb) {
	QmMutexLocker locker(&qmstorage_fs_mutex);
	static_cast<QmSpiffsFsPcb*>(pcb)->ref_counter--;
}

uintmax_t QmSpiffsFsPcb::openFile(const std::string &name, QmFile::OpenMode mode) {
	spiffs_flags flags = 0;
	if (mode.testFlag(QmFile::ReadOnly))
		flags |= SPIFFS_RDONLY;
	if (mode.testFlag(QmFile::WriteOnly))
		flags |= (SPIFFS_WRONLY | SPIFFS_CREAT);
	if (mode.testFlag(QmFile::Append))
		flags |= SPIFFS_APPEND;
	if (mode.testFlag(QmFile::Truncate))
		flags |= SPIFFS_TRUNC;
	spiffs_file fh = SPIFFS_open(&fs, name.c_str(), flags, 0);
	if (fh < 0)
		return 0;
	return (uintmax_t)fh;
}

bool QmSpiffsFsPcb::closeFile(uintmax_t handle) {
	s32_t result = SPIFFS_close(&fs, (spiffs_file)handle);
	return (result == 0);
}

int64_t QmSpiffsFsPcb::readFile(uintmax_t handle, uint8_t *data, int64_t size) {
	return SPIFFS_read(&fs, (spiffs_file)handle, (void *)data, size);
}

int64_t QmSpiffsFsPcb::writeFile(uintmax_t handle, const uint8_t *data, int64_t size) {
	return SPIFFS_write(&fs, (spiffs_file)handle, (void *)data, size);
}

int64_t QmSpiffsFsPcb::getFilePosition(uintmax_t handle) {
	return SPIFFS_tell(&fs, (spiffs_file)handle);
}

bool QmSpiffsFsPcb::seekFilePosition(uintmax_t handle, uint32_t pos) {
	int64_t file_size = getFileSize(handle);
	if (file_size < 0)
		return false;
	if (pos > file_size) {
		uint32_t appended_space_size = pos - file_size;
		uint8_t *space_data = new uint8_t[appended_space_size];
		for (uint32_t i = 0; i < appended_space_size; i++)
			space_data[i] = 0;
		SPIFFS_lseek(&fs, (spiffs_file)handle, 0, SPIFFS_SEEK_END);
		SPIFFS_write(&fs, (spiffs_file)handle, (void *)space_data, appended_space_size);
		delete[] space_data;
	}
	return (SPIFFS_lseek(&fs, (spiffs_file)handle, pos, SPIFFS_SEEK_SET) == (s32_t)pos);
}

bool QmSpiffsFsPcb::isEndOfFile(uintmax_t handle) {
#if 0
	return (SPIFFS_eof(&fs, (spiffs_file)handle) > 0); // see bug https://github.com/pellepl/spiffs/issues/72
#else
	return (getFilePosition(handle) == getFileSize(handle));
#endif
}

int64_t QmSpiffsFsPcb::getFileSize(uintmax_t handle) {
	spiffs_stat s;
	if (SPIFFS_fstat(&fs, (spiffs_file)handle, &s) != 0)
		return -1;
	return s.size;
}

bool QmSpiffsFsPcb::isFileExist(const std::string &name) {
	spiffs_stat s;
	return (SPIFFS_stat(&fs, name.c_str(), &s) == 0);
}

bool QmSpiffsFsPcb::createFile(const std::string &name) {
	return (SPIFFS_creat(&fs, name.c_str(), 0) == 0);
}

bool QmSpiffsFsPcb::removeFile(const std::string &name) {
	return (SPIFFS_remove(&fs, name.c_str()) == 0);
}

bool QmSpiffsFsPcb::renameFile(const std::string &old_name, const std::string &new_name) {
	return (SPIFFS_rename(&fs, old_name.c_str(), new_name.c_str()) == 0);
}

int QmSpiffsFsPcb::checkFileSystem()
{
	uint32_t total, used;
	int res = SPIFFS_info(&fs, &total, &used);
	if (used > total && res == 0)
	{
		res = SPIFFS_check(&fs);
		return res;
	}
	return 0;
}

void qmstorageSpiffsLock(struct spiffs_t *fs) {
	DEFINE_PCB_FROM_FS(fs_pcb, fs);
	fs_pcb->mutex.lock();
}

void qmstorageSpiffsUnlock(struct spiffs_t *fs) {
	DEFINE_PCB_FROM_FS(fs_pcb, fs);
	fs_pcb->mutex.unlock();
}
