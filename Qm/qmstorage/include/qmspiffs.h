/**
 ******************************************************************************
 * @file    qmspiffs.h
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    16.02.2016
 *
 ******************************************************************************
 */

#ifndef QMSPIFFS_H_
#define QMSPIFFS_H_

#include <stdint.h>
#include <string>
#include <vector>

class QmSerialNORFlashDevice;

class QmSpiffs {
public:
	struct Config {
		QmSerialNORFlashDevice *device;
		uint32_t physical_address;
		uint32_t physical_size;
		uint32_t logical_block_size;
		uint32_t logical_page_size;
		uint32_t max_opened_files;
	};

	static bool mount(const std::string &dir, const Config &config);
	static bool unmount(const std::string &dir);
	static bool isMounted(const std::string &dir);
	static bool check(const std::string &dir);
	static bool format(const Config &config);

	static void check_files(const std::string &d,  std::vector<std::string> &v);
};

#endif /* QMSPIFFS_H_ */
