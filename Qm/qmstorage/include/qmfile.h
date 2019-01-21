/**
 ******************************************************************************
 * @file    qmfile.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    24.02.2016
 *
 ******************************************************************************
 */

#ifndef QMFILE_H_
#define QMFILE_H_

#include <stdint.h>
#include <string>
#include "qmflags.h"

class QmFilePrivate;

class QmFile {
public:
	enum OpenModeFlag {
		NotOpen = 0x0000,
		ReadOnly = 0x0001,
		WriteOnly = 0x0002,
		ReadWrite = (ReadOnly | WriteOnly),
		Append = 0x0004,
		Truncate = 0x0008
	};
	QM_DECLARE_FLAGS(OpenMode, OpenModeFlag)

	QmFile(const std::string &dir, const std::string &name);
	~QmFile();

	bool open(OpenMode mode);
	bool close();

	int64_t read(uint8_t *data, int64_t size);
	int64_t write(const uint8_t *data, int64_t size);
	int64_t pos() const;
	bool seek(uint32_t pos);
	bool atEnd() const;
	int64_t size() const;

	static int checkSystem(const std::string& dir);

	static bool exists(const std::string &dir, const std::string &name);
	static bool create(const std::string &dir, const std::string &name);
	static bool remove(const std::string &dir, const std::string &name);
	static bool rename(const std::string &dir, const std::string &old_name, const std::string &new_name);

private:
	QmFilePrivate *d_ptr;
};
QM_DECLARE_OPERATORS_FOR_FLAGS(QmFile::OpenMode)

#endif /* QMFILE_H_ */
