#include "qm.h"
#include "qmfile.h"

#include "fs.h"

namespace DataStorage {

FS::FS(const std::string &dir) :
	dir(dir)
{
}

FS::~FS()
{
}

bool FS::getVoiceChannelsTable(Multiradio::voice_channels_table_t& data) {
	data.clear();
	QmFile file(dir, "VoiceChannelsTable");
	if (!file.open(QmFile::ReadOnly))
		return false;
	int64_t file_size = file.size();
	if (!(file_size > 4))
		return false;
	uint32_t count = 0;
	file.read((uint8_t *)&count, 4);
	if (!(file_size == (4 + 6*count)))
		return false;
	for (unsigned int i = 0; i < count; i++) {
		Multiradio::voice_channel_entry_t entry;
		file.read((uint8_t *)&(entry.frequency), 4);
		file.read((uint8_t *)&(entry.type), 1);
		file.read((uint8_t *)&(entry.speed), 1);
		data.push_back(entry);
	}
	return true;
}

bool FS::getAleDefaultCallFreqs(Multiradio::ale_call_freqs_t &data) {
	data.clear();
	QmFile file(dir, "AleDefaultCallFreqs");
	if (!file.open(QmFile::ReadOnly))
		return false;
	int64_t file_size = file.size();
	if (!(file_size > 4))
		return false;
	uint32_t count = 0;
	file.read((uint8_t *)&count, 4);
	if (!(file_size == (4 + 4*count)))
		return false;
	for (unsigned int i = 0; i < count; i++) {
		uint32_t entry;
		file.read((uint8_t *)&entry, 4);
		data.push_back(entry);
	}
	return true;
}

bool FS::getAleStationAddress(uint8_t& data) {
	data = 0;
	QmFile file(dir, "AleStationAddress");
	if (!file.open(QmFile::ReadOnly))
		return false;
	int64_t file_size = file.size();
	if (!(file_size == 1))
		return false;
	file.read((uint8_t *)&data, 1);
	return true;
}

bool FS::getFhssKey(uint8_t& data) {
	data = 0;
	QmFile file(dir, "FhssKey");
	if (!file.open(QmFile::ReadOnly))
		return false;
	int64_t file_size = file.size();
	if (!(file_size == 1))
		return false;
	file.read((uint8_t *)&data, 1);
	return true;
}

void FS::setFhssKey(uint8_t data) {
	QmFile file(dir, "FhssKey");
	if (!file.open(QmFile::WriteOnly | QmFile::Append))
		return;
	file.write((uint8_t *)&data, 1);
}

void FS::setChannelStation(uint8_t number)
{
    QmFile file(dir,"ChanVal");
    if (file.open(QmFile::WriteOnly))
    	return;
    file.write((uint8_t*)&number,1);
}

bool FS::getChannelStation(uint8_t &number)
{
    QmFile file(dir, "ChanVal");
    if (!file.open(QmFile::ReadOnly))
        return false;

    int64_t file_size = file.size();
    if (!(file_size == 1))
        return false;
    file.read((uint8_t *)&number, 1);
    return true;
}

} /* namespace DataStorage */
