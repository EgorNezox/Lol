
#include "qm.h"
#include "qmfile.h"

#include "fs.h"
#include "qmspiffs.h"
#include <string.h>

#define QMDEBUGDOMAIN	data_storage
#include "qmdebug.h"

#define FAKE_CALL_FREQS 0
#define FAKE_WORK_FREQS 0

#define DEFAULT_GEN_FREG 3000

#define DEFAUL_STATION_ADDRESS 31
#define DEFAULT_FHSS_KEY       631

namespace DataStorage {

FS::FS(const std::string &dir) :
	dir(dir)
{

    trans[TFT_RX] = "r";
    trans[TFT_TX] = "t";

	fileTypeInfo[FT_SMS].fileName = "Sms";
	fileTypeInfo[FT_VM].fileName = "Voice";
	fileTypeInfo[FT_GRP].fileName = "Group";
	fileTypeInfo[FT_CND].fileName = "Cond";

    fileTypeInfo[FT_SMS].counter[TFT_RX].maxCount = 10;
    fileTypeInfo[FT_SMS].counter[TFT_TX].maxCount = 10;
    fileTypeInfo[FT_GRP].counter[TFT_RX].maxCount = 10;
    fileTypeInfo[FT_GRP].counter[TFT_TX].maxCount = 10;
    fileTypeInfo[FT_CND].counter[TFT_RX].maxCount = 10;
    fileTypeInfo[FT_CND].counter[TFT_TX].maxCount = 10;
    fileTypeInfo[FT_VM].counter[TFT_RX].maxCount = 1;
    fileTypeInfo[FT_VM].counter[TFT_TX].maxCount = 1;

    updateFileTree();

}

bool FS::getDiagnsticInfo()
{
	 return (QmFile::checkSystem("data") == true);
}

FS::~FS()
{
}

void FS::setBugDetect()
{
	isBugDetect = true;
}

void FS::resetBug()
{
	isBugDetect = false;
}

bool FS::getBugState()
{
	return isBugDetect;
}

bool FS::readFilterCoeff(float* data)
{
	if (getBugState())
		return false;

	QmFile file(dir, "GeneratorFreq");
	if (!file.open(QmFile::ReadOnly))
		return false;

	uint8_t data_len = sizeof(float);

	int64_t file_size = file.size();

	if (file_size > data_len)
	{
		*data = DEFAULT_GEN_FREG;
		return false;
	}

	int32_t len = file.read((uint8_t *)data, data_len);

	if (len != data_len)
	{
		*data = DEFAULT_GEN_FREG;
		file.close();
		return false;
	}

	file.close();
	return true;
}

bool FS::writeFilterCoeff(float data)
{
	if (getBugState())
	    return false;

    QmFile file(dir, "GeneratorFreq");

    if (!file.open(QmFile::WriteOnly))
        return false;

    uint8_t data_len = sizeof(float);
    qmDebugMessage(QmDebug::Info, "len of write coeff = %i",  data_len);

    int32_t len = file.write((uint8_t*)&data, data_len);

    QmFile::checkSystem("data");

    if (len != data_len)
    {
      deleteFile("GeneratorFreq");
      return false;
    }

    bool res = file.close();
    return res;
}

bool FS::getGenDacValue(int *data)
{
	if (getBugState())
	    return false;

	*data = 0;
	QmFile file(dir, "GenDacValue");
	if (!file.open(QmFile::ReadOnly))
		return false;
	int64_t file_size = file.size();
	if (!(file_size == 1))
		return false;
	file.read((uint8_t *)data, sizeof(int));
	file.close();
	return true;
}


bool FS::setGenDacValue(int data)
{
	if (getBugState())
	    return false;

    QmFile file(dir, "GenDacValue");
    if (!file.open(QmFile::WriteOnly))
        return false;
    file.write((uint8_t *)&data, sizeof(int));
    file.close();
    return true;
}

bool FS::getVoiceChannelsTable(Multiradio::voice_channels_table_t& data, uint8_t &count) {

if (getBugState())
{
	Multiradio::voice_channel_entry_t entry;
	entry.frequency = 3288000;
	entry.type 	    = Multiradio::channelOpen;
	entry.speed     = Multiradio::voicespeed1200;
	data.push_back(entry);

	entry.frequency = 3288000;
	entry.type 	    = Multiradio::channelClose;
	entry.speed     = Multiradio::voicespeed1200;
	data.push_back(entry);

	entry.frequency = 3810000;
	entry.type 	    = Multiradio::channelOpen;
	entry.speed     = Multiradio::voicespeed1200;
	data.push_back(entry);

	entry.frequency = 3810000;
	entry.type 	    = Multiradio::channelClose;
	entry.speed     = Multiradio::voicespeed1200;
	data.push_back(entry);

	entry.frequency = 4517000;
	entry.type 	    = Multiradio::channelOpen;
	entry.speed     = Multiradio::voicespeed1200;
	data.push_back(entry);

	entry.frequency = 4517000;
	entry.type 	    = Multiradio::channelClose;
	entry.speed     = Multiradio::voicespeed1200;
	data.push_back(entry);

	entry.frequency = 5786000;
	entry.type 	    = Multiradio::channelOpen;
	entry.speed     = Multiradio::voicespeed1200;
	data.push_back(entry);

	entry.frequency = 5786000;
	entry.type 	    = Multiradio::channelClose;
	entry.speed     = Multiradio::voicespeed1200;
	data.push_back(entry);

	entry.frequency = 6779000;
	entry.type 	    = Multiradio::channelOpen;
	entry.speed     = Multiradio::voicespeed1200;
	data.push_back(entry);

	entry.frequency = 6779000;
	entry.type 	    = Multiradio::channelClose;
	entry.speed     = Multiradio::voicespeed1200;
	data.push_back(entry);

	entry.frequency = 7011000;
	entry.type 	    = Multiradio::channelOpen;
	entry.speed     = Multiradio::voicespeed1200;
	data.push_back(entry);

	entry.frequency = 7011000;
	entry.type 	    = Multiradio::channelClose;
	entry.speed     = Multiradio::voicespeed1200;
	data.push_back(entry);

	entry.frequency = 8667000;
	entry.type 	    = Multiradio::channelOpen;
	entry.speed     = Multiradio::voicespeed1200;
	data.push_back(entry);

	entry.frequency = 8667000;
	entry.type 	    = Multiradio::channelClose;
	entry.speed     = Multiradio::voicespeed1200;
	data.push_back(entry);

	entry.frequency = 10227000;
	entry.type 	    = Multiradio::channelOpen;
	entry.speed     = Multiradio::voicespeed1200;
	data.push_back(entry);

	entry.frequency = 10227000;
	entry.type 	    = Multiradio::channelClose;
	entry.speed     = Multiradio::voicespeed1200;
	data.push_back(entry);

	entry.frequency = 12143000;
	entry.type 	    = Multiradio::channelOpen;
	entry.speed     = Multiradio::voicespeed1200;
	data.push_back(entry);

	entry.frequency = 12143000;
	entry.type 	    = Multiradio::channelClose;
	entry.speed     = Multiradio::voicespeed1200;
	data.push_back(entry);

	entry.frequency = 12204000;
	entry.type 	    = Multiradio::channelOpen;
	entry.speed     = Multiradio::voicespeed1200;
	data.push_back(entry);

	entry.frequency = 12204000;
	entry.type 	    = Multiradio::channelClose;
	entry.speed     = Multiradio::voicespeed1200;
	data.push_back(entry);

	entry.frequency = 14557000;
	entry.type 	    = Multiradio::channelOpen;
	entry.speed     = Multiradio::voicespeed1200;
	data.push_back(entry);

	entry.frequency = 14557000;
	entry.type 	    = Multiradio::channelClose;
	entry.speed     = Multiradio::voicespeed1200;
	data.push_back(entry);

	entry.frequency = 26000000;
	entry.type 	    = Multiradio::channelOpen;
	entry.speed     = Multiradio::voicespeed1200;
	data.push_back(entry);

	entry.frequency = 26000000;
	entry.type 	    = Multiradio::channelClose;
	entry.speed     = Multiradio::voicespeed1200;
	data.push_back(entry);
    return true;
}
else
{
	data.clear();
	QmFile file(dir, "VoiceChannelsTable");
	if (!file.open(QmFile::ReadOnly))
		return false;
	int64_t file_size = file.size();
	if (!(file_size > 4))
		return false;
	count = 0;
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
	file.close();
	return true;
}
}

bool FS::addVoiceChannelTable(uint8_t position, Multiradio::voice_channel_entry_t &entry)
{
	Multiradio::voice_channels_table_t channelTable;
	uint8_t countChannel;
	getVoiceChannelsTable(channelTable,countChannel);

	if (countChannel <= position)
		return false;

	QmFile file(dir, "VoiceChannelsTable");
	if (!file.open(QmFile::WriteOnly))
		return false;

	// start of entry which we change
	uint16_t pos = 4 + (position - 1) * 6;

	if (!file.seek(pos))
		return false;

	int64_t res = 0;

	if (file.pos() == pos)
	{
		res = file.write((uint8_t*)&(entry.frequency),4);
		if (res != 4)
			return false;

		res = file.write((uint8_t*)&(entry.type),     1);
		if (res != 1)
			return false;

		res = file.write((uint8_t*)&(entry.speed),    1);
		if (res != 1)
			return false;
	}

	return true;
}

bool FS::getAleDefaultCallFreqs(Multiradio::ale_call_freqs_t &data)
{
if (getBugState())
{
	data.push_back(4517000);
	data.push_back(3288000);
	data.push_back(6779000);
	data.push_back(4889000);
	data.push_back(7000000);
	return true;
}
else
{
	data.clear();
	QmFile file(dir, "AleDefaultCallFreqs");
	if (!file.open(QmFile::ReadOnly))
		return false;
	int64_t file_size = file.size();
	if (!(file_size > 4) || (file_size % 4 != 0))
		return false;
	uint32_t count = 0;
	file.read((uint8_t *)&count, 4);
	for (unsigned int i = 0; i < count; i++)
	{
		uint32_t entry;
		file.read((uint8_t *)&entry, 4);
		data.push_back(entry);
	}
	file.close();
	return true;
}
}

int FS::getMessageCount(FileType mode, TransitionFileType txrx)
{
	 return fileTypeInfo[mode].counter[txrx].count;
}

bool FS::getAleDefaultWorkFreqs(Multiradio::ale_work_freqs_t &data)
{
if (getBugState())
{
    data.push_back(4517000);
    data.push_back(3288000);
    data.push_back(6779000);
    data.push_back(4889000);
    data.push_back(8000000);
    return true;
}
else
{
    data.clear();
    QmFile file(dir, "AleSessionFreqs");
    if (!file.open(QmFile::ReadOnly))
        return false;
    int64_t file_size = file.size();
    if (!(file_size > 4) || (file_size % 4 != 0))
        return false;
    uint32_t count = 0;
    file.read((uint8_t *)&count, 4);
    for (unsigned int i = 0; i < count; i++) {
        uint32_t entry;
        file.read((uint8_t *)&entry, 4);
        data.push_back(entry);
    }
    file.close();
    return true;
}
}



bool FS::getAleStationAddress(uint8_t& data) {

	if (getBugState())
	{
		data = DEFAUL_STATION_ADDRESS;
	}
	else
	{
		data = 0;
		QmFile file(dir, "AleStationAddress");
		if (!file.open(QmFile::ReadOnly))
			return false;
		int64_t file_size = file.size();
		if (!(file_size == 1))
			return false;
		file.read((uint8_t *)&data, 1);
		file.close();
		return true;
	}
}


bool FS::setAleStationAddress(uint8_t data)
{
	if (getBugState())
		return false;

    QmFile file(dir, "AleStationAddress");
    if (!file.open(QmFile::WriteOnly))
        return false;
    int64_t file_size = file.size();
    if (!(file_size == 1))
        return false;
    file.write((uint8_t *)&data, 1);
    file.close();
    return true;
}

bool FS::getFhssKey(uint16_t& data) {

	if (getBugState())
	{
		data = DEFAULT_FHSS_KEY;
		return false;
	}

	data = 0;
	QmFile file(dir, "FhssKey");
	if (!file.open(QmFile::ReadOnly))
		return false;
	int64_t file_size = file.size();
	if (!(file_size == 2))
		return false;
	file.read((uint8_t *)&data, 2);
	file.close();
	return true;
}

void FS::setFhssKey(uint16_t data) {

	if (getBugState())
		return;

	QmFile file(dir, "FhssKey");
	if (!file.open(QmFile::WriteOnly))
		return;
	file.write((uint8_t *)&data, 2);
	file.close();
}

bool FS::getVoiceFrequency(uint32_t &data) {

	if (getBugState())
		return false;

	data = 0;
	QmFile file(dir, "VoiceFrequency");
	if (!file.open(QmFile::ReadOnly))
		return false;
	int64_t file_size = file.size();
	if (!(file_size == 4))
		return false;
	file.read((uint8_t *)&data, 4);
	file.close();
	return true;
}

void FS::setVoiceFrequency(uint32_t data)
{
	if (getBugState())
		return;

	QmFile file(dir, "VoiceFrequency");
	if (!file.open(QmFile::WriteOnly))
		return;
	file.write((uint8_t *)&data, 4);
	file.close();
}

bool FS::getVoiceEmissionType(Multiradio::voice_emission_t &data) {

	if (getBugState())
	{
		data = Multiradio::voiceemissionUSB;
		return true;
	}

	data = Multiradio::voiceemissionInvalid;
	QmFile file(dir, "VoiceEmissionType");
	if (!file.open(QmFile::ReadOnly))
		return false;
	int64_t file_size = file.size();
	if (!(file_size == 1))
		return false;
	file.read((uint8_t *)&data, 1);
	file.close();
	return true;
}

void FS::setVoiceEmissionType(Multiradio::voice_emission_t data) {

	if (getBugState())
		return;

	QmFile file(dir, "VoiceEmissionType");
	if (!file.open(QmFile::WriteOnly))
		return;
	file.write((uint8_t *)&data, 1);
	file.close();
}

bool FS::getVoiceChannelSpeed(Multiradio::voice_channel_speed_t &data) {

	if (getBugState())
	{
		data = Multiradio::voicespeed1200;
		return true;
	}

	data = Multiradio::voicespeedInvalid;
	QmFile file(dir, "VoiceChannelSpeed");
	if (!file.open(QmFile::ReadOnly))
		return false;
	int64_t file_size = file.size();
	if (!(file_size == 1))
		return false;
	file.read((uint8_t *)&data, 1);
	return true;
}

void FS::setVoiceChannelSpeed(Multiradio::voice_channel_speed_t data) {

	if (getBugState())
		return;

	QmFile file(dir, "VoiceChannelSpeed");
	if (!file.open(QmFile::WriteOnly))
		return;
	file.write((uint8_t *)&data, 1);
}

bool FS::getAnalogHeadsetChannel(uint8_t& data) {

	if (getBugState())
	{
		data = 1;
		return true;
	}

	data = 0;
	QmFile file(dir, "");
	if (!file.open(QmFile::ReadOnly))
		return false;
	int64_t file_size = file.size();
	if (!(file_size == 1))
		return false;
	file.read((uint8_t *)&data, 1);
	return true;
}

void FS::setAnalogHeadsetChannel(uint8_t data) {

	if (getBugState())
		return;

	QmFile file(dir, "AnalogHeadsetChannel");
	if (!file.open(QmFile::WriteOnly))
		return;
	file.write((uint8_t *)&data, 1);
}

bool FS::getGpsSynchroMode(uint8_t* data) {

	if (getBugState())
	{
		bool gpsSynchronization   = true;
		*data = gpsSynchronization;
		return true;
	}

    QmFile file(dir, "GpsSynchroMode");
    if (!file.open(QmFile::ReadOnly))
        return false;
    int64_t file_size = file.size();
    if (!(file_size == 1))
        return false;
    file.read((uint8_t*)data, 1);
    return true;
}

/** @brief setVoiceMode Функция для записи речевого режима на флешку
 *  @details Данная функция записывает речевой режим на флешку
 *  @param true (1) VoiceModeAuto
 *         false (0) VoiceModeManual
 *  @return void.
 */
void FS::setVoiceMode(bool data)
{
	if (getBugState())
		return;

	if (data > 0)
		data = true;
	else
		data = false;
    QmFile file(dir, "VoiceMode");
    if(!file.open(QmFile::WriteOnly))
        return;
    file.write((uint8_t*)&data,1);
}
/** @brief getVoiceMode Функция для чтения речевого режима из флешки
 *  @details Данная функция считывает речевой режим из флешки
 *  @param true (1) VoiceModeAuto
 *         false (0) VoiceModeManual
 *  @return data Речевой режим.
 */
bool FS::getVoiceMode(bool *data)
{
	if (getBugState())
	{
		// manual mode
		bool useMode = false;
		*data = useMode;
		return true;
	}

    QmFile file(dir, "VoiceMode");
    if(!file.open(QmFile::ReadOnly))
        return false;
    file.read((uint8_t*)data,1);
	if (*data > 0)
		*data = true;
	else
		*data = false;
    return true;
}

bool FS::getSheldure(uint8_t* data)
{
	if (getBugState())
		return false;

	QmFile file(dir, "Sheldure");
    if (file.open(QmFile::ReadOnly))
    {
        uint32_t fileSize = file.size();
        if (fileSize >= 14){
            uint64_t res =  file.read(data, fileSize);
            file.close();
            if (res > 0) return true;
        }
    }
    return false;
}

bool FS::setSheldure(uint8_t* data, uint16_t size)
{
	if (getBugState())
		return false;

    deleteFile("Sheldure");
    QmFile file(dir, "Sheldure");
    if(file.open(QmFile::WriteOnly)){
       uint64_t res = file.write((uint8_t*)data, size);
       file.close();
       if (res > 0) return true;
    }
    return false;
}

//! @brief setTimeZone Функция для записи Временной зоны в Flash память
//! @param zone [uint8_t] временная зона [ 0 ... 11 ]
//! @return bool true если запись прошла успешно
bool FS::setTimeZone(uint8_t &zone)
{
	if (getBugState())
		return false;

	uint64_t res;
	deleteFile("TimeZone");
	QmFile file(dir, "TimeZone");
	if(file.open(QmFile::WriteOnly))
	{
		res = file.write(&zone, 1);
		file.close();
		if (res > 0)
			return true;
	}
	return false;
}

//! @brief setTimeZone Функция для чтения Временной зоны из Flash памяти
//! @param zone [uint8_t] временная зона [ 0 ... 11 ]
//! @return bool true если чтение прошло успешно
bool FS::getTimeZone(uint8_t &zone)
{
	if (getBugState())
		return false;

	uint64_t res;
	QmFile file(dir, "TimeZone");
	if (file.open(QmFile::ReadOnly))
	{
		res = file.read(&zone, 1);
		file.close();
		if (res > 0)
			return true;
	}
	return false;
}

bool FS::readMessage(FileType fileType, TransitionFileType transFileType, std::vector<uint8_t>* data, uint8_t fileNumber)
{
	if (getBugState())
		return false;

    std::string fileName = generateFileNameByNumber(fileType, transFileType, fileNumber);
    if (fileName != errorFileName){
        QmFile file(dir, fileName);
        if (file.open(QmFile::ReadOnly)){
            uint32_t file_size = file.size();
            if (file_size > 0){
                data->resize(file_size);
                if (file.read(data->data(), file_size))
                    return true;
            }
        }
    }
    return false;
}

bool FS::writeMessage(FileType fileType, TransitionFileType transFileType, std::vector<uint8_t> *data)
{
	if (getBugState())
		return false;

    if (data->size() > 0){
        std::string fileName = prepareFileStorageToWriting(fileType, transFileType);
        if (fileName != errorFileName){
            QmFile file(dir, fileName);
            if (file.open(QmFile::WriteOnly)){
                uint32_t writeSize = file.write(data->data(), data->size());
                if (writeSize){
                    fileTypeInfo[fileType].counter[transFileType].count++;
                    return true;
                }
            }
        }
    }
    return false;
}

void FS::setGpsSynchroMode(uint8_t data)
{
	if (getBugState())
		return;

    QmFile file(dir, "GpsSynchroMode");
    if (!file.open(QmFile::WriteOnly))
        return;
    file.write((uint8_t*)&data, 1);
	file.close();
}

bool FS::renameFile(std::string oldfileName, std::string newFileName)
{
	if (getBugState()) return false;

    return QmFile::rename(dir, oldfileName, newFileName);
}

bool FS::deleteFile(std::string fileName)
{
	if (getBugState()) return false;

    return QmFile::remove(dir, fileName);
}

bool FS::existFile(std::string fileName)
{
	if (getBugState()) return false;
    return QmFile::exists(dir, fileName);
}

void FS::updateFileTree()
{
	if (getBugState()) return;

    files.clear();

    std::string fileName;
    for (uint8_t fileType = 0; fileType < 5; fileType++)
    for (uint8_t fileTransType = 0; fileTransType < 2; fileTransType++)
    for (uint8_t fileNum = 0; fileNum < 10; fileNum++)
    {
        fileName = generateFileNameByNumber((FileType)fileType, (TransitionFileType)fileTransType, fileNum);
        if (existFile(fileName))
        {
        	transmitFileTypeCount[fileType][fileTransType]++;
            files.push_back(fileName);
            fileTypeInfo[fileType].counter[fileTransType].count++;
        }
        else
            break;
    }
}

std::vector<std::string>* FS::getFileTree()
{
    return &files;
}

void FS::getFileNamesByType(std::vector<std::string> *typeFiles, FS::FileType fileType)
{

    typeFiles->clear();
    std::string fileName;
    for (uint8_t fileTransType = 0; fileTransType < 2; fileTransType++){
        fileTypeInfo[fileType].counter[fileTransType].count = 0;
        for (uint8_t fileNum = 0; fileNum < 10; fileNum++)
        {
            fileName = generateFileNameByNumber(fileType, (TransitionFileType)fileTransType, fileNum);
            if (existFile(fileName)){
                typeFiles->push_back(fileName);
                fileTypeInfo[fileType].counter[fileTransType].count++;
            }
            else
                break;
        }
    }
}

std::string FS::generateFileNameByNumber(FS::FileType fileType, TransitionFileType transFileType, uint8_t number)
{
    char n[1] = {0};
    std::string name = fileTypeInfo[fileType].fileName;
    sprintf(n,"%d", number);
    name += n;
    name.append(trans[transFileType]);
    return name;
}

void FS::prepareFreeFileSlot(FS::FileType fileType, TransitionFileType transFileType)
{
	if (getBugState()) return;

    deleteFile(generateFileNameByNumber(fileType,transFileType, 0));

    //cyclic rename file n to n-1
    for (uint8_t fileNumber = 1; fileNumber < fileTypeInfo[fileType].counter[transFileType].count; fileNumber++)
        renameFile(generateFileNameByNumber(fileType, transFileType, fileNumber),
                   generateFileNameByNumber(fileType, transFileType, fileNumber-1));
    fileTypeInfo[fileType].counter[transFileType].count--;
}

std::string FS::prepareFileStorageToWriting(FS::FileType fileType, TransitionFileType transFileType)
{
    if (fileTypeInfo[fileType].counter[transFileType].count == fileTypeInfo[fileType].counter[transFileType].maxCount)
    {
        prepareFreeFileSlot(fileType, transFileType);
        return generateFileNameByNumber(fileType, transFileType, fileTypeInfo[fileType].counter[transFileType].maxCount-1);
    }
    else
        return generateFileNameByNumber(fileType, transFileType, fileTypeInfo[fileType].counter[transFileType].count);
}

DataStorage::FS::TransitionFileType FS::getTransmitType(FS::FileType fileType, uint8_t fileTreeTypeFocus){
    // fileTreeTypeFocus - focus in one type file list(sms or vm)
    // first loaded files is rx, second is tx =>
    if (fileTreeTypeFocus >= fileTypeInfo[fileType].counter[TFT_RX].count)
        return TFT_TX;
    else
        return TFT_RX;
}

uint8_t FS::getFileNumber(FS::FileType fileType, uint8_t fileTreeTypeFocus){

    if (fileTreeTypeFocus < fileTypeInfo[fileType].counter[TFT_RX].count)
        return fileTreeTypeFocus;
    else
        return fileTreeTypeFocus - fileTypeInfo[fileType].counter[TFT_RX].count;

}

void FS::findFilesToFiletree()
{
	std::vector<std::string> files;
	isFileTreeReady = false;

	// read all files from dir
	QmSpiffs::check_files("data",files);

	isFileTreeReady = true;

}

uint8_t FS::getTransmitFileTypeCount(FS::FileType fileType, FS::TransitionFileType transFileType)
{
	return  getMessageCount(fileType, transFileType);  //transmitFileTypeCount[fileType][transFileType];
}
} /* namespace DataStorage */

#include "qmdebug_domains_start.h"
QMDEBUG_DEFINE_DOMAIN(data_storage, LevelVerbose)
#include "qmdebug_domains_end.h"
