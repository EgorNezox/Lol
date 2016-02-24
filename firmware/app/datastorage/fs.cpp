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
	Multiradio::voice_channel_entry_t ch_entry;
	ch_entry.type = Multiradio::channelOpen;
	ch_entry.speed = Multiradio::voicespeedInvalid;

	ch_entry.frequency = 1500000;
	data.push_back(ch_entry);
	ch_entry.frequency = 3000000;
	data.push_back(ch_entry);
	ch_entry.frequency = 5000000;
	data.push_back(ch_entry);
	ch_entry.frequency = 10000000;
	data.push_back(ch_entry);
	ch_entry.frequency = 15000000;
	data.push_back(ch_entry);
	ch_entry.frequency = 15000010;
	data.push_back(ch_entry);
	ch_entry.frequency = 27000000;
	data.push_back(ch_entry);
	ch_entry.frequency = 50000000;
	data.push_back(ch_entry);

	return true;
}

} /* namespace DataStorage */
