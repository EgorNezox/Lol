/**
 ******************************************************************************
 * @file    fs.cpp
 * @author  Artem Pisarenko, PMR dept. software team, ONIIP, PJSC
 * @date    29.10.2015
 *
 ******************************************************************************
 */

#include "qm.h"

#include "fs.h"

namespace DataStorage {

FS::FS(int flash_spi_resource) {
	QM_UNUSED(flash_spi_resource);
	//...
}

FS::~FS() {
	//...
}

void FS::init() {
	//...
}

bool FS::getVoiceChannelsTable(Multiradio::voice_channels_table_t& data) {
	QM_UNUSED(data);
	//...
	return false;
}

} /* namespace DataStorage */
