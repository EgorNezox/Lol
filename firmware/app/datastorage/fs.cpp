
#include "qm.h"
#include "qmfile.h"

#include "fs.h"

#include <string.h>

#define FAKE_CALL_FREQS 1

namespace DataStorage {

FS::FS(const std::string &dir) :
	dir(dir)
{

    trans[FTT_RX] = "r";
    trans[FTT_TX] = "t";

	fileTypeInfo[FT_SMS].fileName = "Sms";
	fileTypeInfo[FT_VM].fileName = "Voice";
	fileTypeInfo[FT_GRP].fileName = "Group";
	fileTypeInfo[FT_CND].fileName = "Cond";
    fileTypeInfo[FT_SP].fileName = "Speach";

    fileTypeInfo[FT_SMS].counter[FTT_RX].maxCount = 10;
    fileTypeInfo[FT_SMS].counter[FTT_TX].maxCount = 10;
    fileTypeInfo[FT_GRP].counter[FTT_RX].maxCount = 10;
    fileTypeInfo[FT_GRP].counter[FTT_TX].maxCount = 10;
    fileTypeInfo[FT_CND].counter[FTT_RX].maxCount = 10;
    fileTypeInfo[FT_CND].counter[FTT_TX].maxCount = 10;
    fileTypeInfo[FT_VM].counter[FTT_RX].maxCount = 1;
    fileTypeInfo[FT_VM].counter[FTT_TX].maxCount = 1;
    fileTypeInfo[FT_SP].counter[FTT_RX].maxCount = 1;
    fileTypeInfo[FT_SP].counter[FTT_TX].maxCount = 1;

    updateFileTree();

//    deleteFile("AleDefaultCallFreqs");

//    QmFile file(dir, "AleDefaultCallFreqs");
//    	file.open(QmFile::ReadOnly);
//    		uint32_t f = 4;
//    		uint8_t fw[4];
//    		memcpy(&fw, &f, 4);
//    		file.write((uint8_t*)&fw, 4);
//    file.close();

//	QmFile file(dir, "AleStationAddress");
//
//	uint8_t data1 = 3;
//	if (!file.open(QmFile::WriteOnly))
//		return;
//	file.write((uint8_t *)&data1, 1);
//	file.close();

}

FS::~FS()
{
}

bool FS::getGeneratorFreq(uint16_t* data, uint16_t defaultFreq) {
    QmFile file(dir, "GeneratorFreq");
    if (!file.open(QmFile::ReadOnly))
        return false;
    int64_t file_size = file.size();
    if (file_size > 2){
        *data = defaultFreq;
        return false;
    }
    int32_t len = file.read((uint8_t *)data, 2);
    if (len != 2){
        *data = defaultFreq;
        file.close();
        return false;
    }
    file.close();
    return true;
}

bool FS::setGeneratorFreq(uint16_t data)
{
    QmFile file(dir, "GeneratorFreq");
    if (!file.open(QmFile::WriteOnly))
        return false;
    int32_t len = file.write((uint8_t*)&data, 2);
    if (len != 2){
      file.close();
      return false;
    }
    return true;
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
	file.close();
	return true;
}

bool FS::getAleDefaultCallFreqs(Multiradio::ale_call_freqs_t &data)
{
#if FAKE_CALL_FREQS

	data.push_back(5786000);
	data.push_back(4517000);
	data.push_back(6197000);
	data.push_back(4889000);
	return true;

#else

	data.clear();
	QmFile file(dir, "AleDefaultCallFreqs");
	if (!file.open(QmFile::ReadOnly))
		return false;
	int64_t file_size = file.size();
	if (!(file_size > 4))
		return false;
	uint32_t count = 0;
	file.read((uint8_t *)&count, 4);
//	if (!(file_size == (4 + 4*count)))
//		return false;
	for (unsigned int i = 0; i < count; i++) {
//	for (unsigned int i = 0; i < 5; i++) {
		uint32_t entry;
		file.read((uint8_t *)&entry, 4);
		data.push_back(entry);
	}
//	uint32_t f = 6779000;
//	uint8_t fw[4];
//	memcpy(&fw, &f, 4);
//	file.write((uint8_t*)&fw, 4);
	file.close();
	return true;
#endif
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
	file.close();
	return true;
}

bool FS::getFhssKey(uint16_t& data) {
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
	QmFile file(dir, "FhssKey");
	if (!file.open(QmFile::WriteOnly))
		return;
	file.write((uint8_t *)&data, 2);
	file.close();
}

bool FS::getVoiceFrequency(uint32_t &data) {
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

void FS::setVoiceFrequency(uint32_t data) {
	QmFile file(dir, "VoiceFrequency");
	if (!file.open(QmFile::WriteOnly))
		return;
	file.write((uint8_t *)&data, 4);
	file.close();
}

bool FS::getVoiceEmissionType(Multiradio::voice_emission_t &data) {
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
	QmFile file(dir, "VoiceEmissionType");
	if (!file.open(QmFile::WriteOnly))
		return;
	file.write((uint8_t *)&data, 1);
	file.close();
}

bool FS::getVoiceChannelSpeed(Multiradio::voice_channel_speed_t &data) {
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
	QmFile file(dir, "VoiceChannelSpeed");
	if (!file.open(QmFile::WriteOnly))
		return;
	file.write((uint8_t *)&data, 1);
}

bool FS::getAnalogHeadsetChannel(uint8_t& data) {
	data = 0;
	QmFile file(dir, "AnalogHeadsetChannel");
	if (!file.open(QmFile::ReadOnly))
		return false;
	int64_t file_size = file.size();
	if (!(file_size == 1))
		return false;
	file.read((uint8_t *)&data, 1);
	return true;
}

void FS::setAnalogHeadsetChannel(uint8_t data) {
	QmFile file(dir, "AnalogHeadsetChannel");
	if (!file.open(QmFile::WriteOnly))
		return;
	file.write((uint8_t *)&data, 1);
}

bool FS::getGpsSynchroMode(uint8_t* data) {
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
    QmFile file(dir, "VoiceMode");
    if(!file.open(QmFile::ReadOnly))
        return false;
    file.read((uint8_t*)data,1);
    return true;
}

bool FS::getSheldure(uint8_t* data)
{
    QmFile file(dir, "Sheldure");
    if (file.open(QmFile::ReadOnly)){
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
    deleteFile("Sheldure");
    QmFile file(dir, "Sheldure");
    if(file.open(QmFile::WriteOnly)){
       uint64_t res = file.write((uint8_t*)data, size);
       file.close();
       if (res > 0) return true;
    }
    return false;
}

//-----------------------------------------------------

bool FS::getCondCommand(std::vector<uint8_t>* data, uint8_t number, TransitionFileType transFileType)
{
    if (number > maxFilesCount - 1) return false;
    std::string fileName = generateFileNameByNumber(FT_CND, transFileType, number);
    if (fileName != errorFileName){
    	QmFile file(dir, fileName);
    	if (!file.open(QmFile::ReadOnly))
    		return false;
    	int64_t file_size = file.size();
    	if (file_size > 3)
    		return false;
    	data->resize(file_size);
    	file.read(data->data(), file_size);
    	file.close();
    	return true;
    } else
    	return false;
}

void FS::setCondCommand(std::vector<uint8_t> *data, TransitionFileType transFileType)
{
    std::string fileName = prepareFileStorageToWriting(FT_CND, transFileType);
    if (fileName != errorFileName){
    	QmFile file(dir, fileName);
    	if (!file.open(QmFile::WriteOnly))
    		return;
    	uint64_t writeSize = file.write(data->data(), data->size());
    	file.close();
    	if (writeSize)
            fileTypeInfo[FT_CND].counter[transFileType].count++;
    }
}

bool FS::getGroupCondCommand(std::vector<uint8_t>* data, uint8_t number, TransitionFileType transFileType)
{
    if (number > maxFilesCount - 1) return false;

    std::string fileName = generateFileNameByNumber(FT_GRP, transFileType, number);
    if (fileName != errorFileName){
    	QmFile file(dir, fileName);
    	if (!file.open(QmFile::ReadOnly))
    		return false;
    	int64_t file_size = file.size();
    	data->resize(file_size);
    	file.read(data->data(), file_size);
    	file.close();
    	return true;
    } else
    	return false;
}

void FS::setGroupCondCommand(uint8_t* data, uint16_t size, TransitionFileType transFileType)
{
    std::string fileName = prepareFileStorageToWriting(FT_GRP, transFileType);
    if (fileName != errorFileName){
    	QmFile file(dir, fileName);
    	if (!file.open(QmFile::WriteOnly))
    		return;
    	int64_t writeSize = file.write(data, size);
    	file.close();
    	if (writeSize)
            fileTypeInfo[FT_GRP].counter[transFileType].count++;
    }
}

bool FS::getSms(std::vector<uint8_t>* data, uint8_t number, TransitionFileType transFileType)
{
    if (number > maxFilesCount - 1) return false;

    std::string fileName = generateFileNameByNumber(FT_SMS, transFileType, number);
    if (fileName != errorFileName){
    	QmFile file(dir, fileName);
    	if (!file.open(QmFile::ReadOnly))
    		return false;
    	int64_t file_size = file.size();
    	if (file_size > 100)
    		return false;
    	data->resize(file_size);
    	file.read(data->data(), file_size);
    	file.close();
    	return true;
    } else
    	return false;
}

void FS::setSms(uint8_t* data, uint16_t size, TransitionFileType transFileType)
{
    std::string fileName = prepareFileStorageToWriting(FT_SMS, transFileType);
    if (fileName != errorFileName){
    	QmFile file(dir, fileName);
    	if (!file.open(QmFile::WriteOnly))
    		return;
    	int64_t writeSize = file.write(data, size);
    	file.close();
    	if (writeSize)
            fileTypeInfo[FT_SMS].counter[transFileType].count++;
    }
}

bool FS::getVoiceMail(std::vector<uint8_t>* data, uint8_t number, TransitionFileType transFileType)
{
    if (number > maxFilesCount - 1) return false;

    std::string fileName = generateFileNameByNumber(FT_VM, transFileType, number);
    if (fileName != errorFileName){
    	QmFile file(dir, fileName);
    	if (!file.open(QmFile::ReadOnly))
    		return false;
    	int64_t file_size = file.size();
    	if (file_size == 0)
    		return false;
    	data->resize(file_size);
    	file.read(data->data(), file_size);
    	file.close();
    	return true;
    } else
    	return false;
}

void FS::setVoiceMail(std::vector<uint8_t>* data, TransitionFileType transFileType)
{
    std::string fileName = prepareFileStorageToWriting(FT_VM, transFileType);
    if (fileName != errorFileName){
    	QmFile file(dir, fileName);
    	if (!file.open(QmFile::WriteOnly))
    		return;
    	int64_t writeSize = file.write(data->data(), data->size());
    	file.close();
    	if (writeSize)
            fileTypeInfo[FT_VM].counter[transFileType].count++;
    }
}



void FS::setGpsSynchroMode(uint8_t data) {
    QmFile file(dir, "GpsSynchroMode");
    if (!file.open(QmFile::WriteOnly))
        return;
    file.write((uint8_t*)&data, 1);
	file.close();
}

bool FS::renameFile(std::string oldfileName, std::string newFileName)
{
    return QmFile::rename(dir, oldfileName, newFileName);
}

bool FS::deleteFile(std::string fileName)
{
    return QmFile::remove(dir, fileName);
}

bool FS::existFile(std::string fileName)
{
    return QmFile::exists(dir, fileName);
}

void FS::updateFileTree()
{
    files.clear();

    std::string fileName;
    for (uint8_t fileType = 0; fileType < 5; fileType++)
    for (uint8_t fileTransType = 0; fileTransType < 2; fileTransType++)
    for (uint8_t fileNum = 0; fileNum < 10; fileNum++)
    {
        fileName = generateFileNameByNumber((FileType)fileType, (TransitionFileType)fileTransType, fileNum);
        if (existFile(fileName)){
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

uint8_t FS::getFreeFileSlotCount()
{
//    uint8_t sum = 0;
//    for (uint8_t ftype = 0; ftype < 4; ftype++){
//       sum += fileTypeInfo[ftype].counter[transFileType].count;
//       if (sum > maxFilesCount) return 0;
//    }
//    uint8_t res = maxFilesCount - sum;
//    return res;
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
    if (fileTreeTypeFocus >= fileTypeInfo[fileType].counter[FTT_RX].count)
        return FTT_TX;
    else
        return FTT_RX;
}

uint8_t FS::getFileNumber(FS::FileType fileType, uint8_t fileTreeTypeFocus){

	if (fileTreeTypeFocus < fileTypeInfo[fileType].counter[FTT_RX].count)
		return fileTreeTypeFocus;
	else
		return fileTreeTypeFocus - fileTypeInfo[fileType].counter[FTT_RX].count;

}
} /* namespace DataStorage */
