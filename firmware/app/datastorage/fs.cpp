
#include "qm.h"
#include "qmfile.h"

#include "fs.h"


namespace DataStorage {

FS::FS(const std::string &dir) :
	dir(dir)
{
    //updateFileTree();
	fileTypeInfo[FT_SMS].fileName = "Sms";
	fileTypeInfo[FT_VM].fileName = "Voice";
	fileTypeInfo[FT_GRP].fileName = "Group";
	fileTypeInfo[FT_CND].fileName = "Cond";
	fileTypeInfo[FT_VM].maxCount = 1;
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

bool FS::getFhssKey(uint16_t& data) {
	data = 0;
	QmFile file(dir, "FhssKey");
	if (!file.open(QmFile::ReadOnly))
		return false;
	int64_t file_size = file.size();
	if (!(file_size == 2))
		return false;
	file.read((uint8_t *)&data, 2);
	return true;
}

void FS::setFhssKey(uint16_t data) {
	QmFile file(dir, "FhssKey");
	if (!file.open(QmFile::WriteOnly))
		return;
	file.write((uint8_t *)&data, 2);
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
	return true;
}

void FS::setVoiceFrequency(uint32_t data) {
	QmFile file(dir, "VoiceFrequency");
	if (!file.open(QmFile::WriteOnly))
		return;
	file.write((uint8_t *)&data, 4);
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
	return true;
}

void FS::setVoiceEmissionType(Multiradio::voice_emission_t data) {
	QmFile file(dir, "VoiceEmissionType");
	if (!file.open(QmFile::WriteOnly))
		return;
	file.write((uint8_t *)&data, 1);
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

//-----------------------------------------------------

bool FS::getCondCommand(std::vector<uint8_t>* data, uint8_t number)
{
    if (number > maxFilesCount - 1) return false;
    std::string fileName = generateFileNameByNumber(FT_CND, number);
    if (fileName != errorFileName){
    	QmFile file(dir, fileName);
    	if (!file.open(QmFile::ReadOnly))
    		return false;
    	int64_t file_size = file.size();
    	if (!(file_size == 1))
    		return false;
    	data->resize(file_size);
    	file.read(data->data(), file_size);
    	return true;
    } else
    	return false;
}

void FS::setCondCommand(uint8_t data)
{
    std::string fileName = prepareFileStorageToWriting(FT_CND);
    if (fileName != errorFileName){
    	QmFile file(dir, fileName);
    	if (!file.open(QmFile::WriteOnly))
    		return;
    	int64_t writeSize = file.write((uint8_t*)&data, sizeof(int));
    	if (writeSize)
    		fileTypeInfo[FT_CND].count++;
    }
}

bool FS::getGroupCondCommand(std::vector<uint8_t>* data, uint8_t number)
{
    if (number > maxFilesCount - 1) return false;

    std::string fileName = generateFileNameByNumber(FT_GRP, number);
    if (fileName != errorFileName){
    	QmFile file(dir, fileName);
    	if (!file.open(QmFile::ReadOnly))
    		return false;
    	int64_t file_size = file.size();
    	data->resize(file_size);
    	file.read(data->data(), file_size);
    	return true;
    } else
    	return false;
}

void FS::setGroupCondCommand(uint8_t* data, uint16_t size)
{
    std::string fileName = prepareFileStorageToWriting(FT_GRP);
    if (fileName != errorFileName){
    	QmFile file(dir, fileName);
    	if (!file.open(QmFile::WriteOnly))
    		return;
    	int64_t writeSize = file.write(data, size);
    	if (writeSize)
    		fileTypeInfo[FT_GRP].count++;
    }
}

bool FS::getSms(std::vector<uint8_t>* data, uint8_t number)
{
    if (number > maxFilesCount - 1) return false;

    std::string fileName = generateFileNameByNumber(FT_SMS, number);
    if (fileName != errorFileName){
    	QmFile file(dir, fileName);
    	if (!file.open(QmFile::ReadOnly))
    		return false;
    	int64_t file_size = file.size();
    	if (file_size > 100)
    		return false;
    	data->resize(file_size);
    	file.read(data->data(), file_size);
    	return true;
    } else
    	return false;
}

void FS::setSms(uint8_t* data, uint16_t size)
{
    std::string fileName = prepareFileStorageToWriting(FT_SMS);
    if (fileName != errorFileName){
    	QmFile file(dir, fileName);
    	if (!file.open(QmFile::WriteOnly))
    		return;
    	int64_t writeSize = file.write(data, size);
    	if (writeSize)
    		fileTypeInfo[FT_SMS].count++;
    }
}

bool FS::getVoiceMail(std::vector<uint8_t>* data, uint8_t number)
{
    if (number > maxFilesCount - 1) return false;

    std::string fileName = generateFileNameByNumber(FT_VM, number);
    if (fileName != errorFileName){
    	QmFile file(dir, fileName);
    	if (!file.open(QmFile::ReadOnly))
    		return false;
    	int64_t file_size = file.size();
    	if (file_size == 0)
    		return false;
    	data->resize(file_size);
    	file.read(data->data(), file_size);
    	return true;
    } else
    	return false;
}

void FS::setVoiceMail(std::vector<uint8_t>* data)
{
    std::string fileName = prepareFileStorageToWriting(FT_VM);
    if (fileName != errorFileName){
    	QmFile file(dir, fileName);
    	if (!file.open(QmFile::WriteOnly))
    		return;
    	int64_t writeSize = file.write(data->data(), data->size());
    	if (writeSize)
    		fileTypeInfo[FT_VM].count++;
    }
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
    for (uint8_t fileType = 0; fileType < 4; fileType++)
    for (uint8_t fileNum = 0; fileNum < 10; fileNum++)
    {
        fileName = generateFileNameByNumber((FileType)fileType, fileNum);
        if (existFile(fileName))
            files.push_back(fileName);
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
    for (uint8_t fileNum = 0; fileNum < 10; fileNum++)
    {
        fileName = generateFileNameByNumber(fileType, fileNum);
        if (existFile(fileName))
            typeFiles->push_back(fileName);
        else
            break;
    }
}

uint8_t FS::getFreeFileSlotCount()
{
    uint8_t sum = 0;
    for (uint8_t ftype = 0; ftype < 4; ftype++)
       sum += fileTypeInfo[ftype].count;
    return maxFilesCount - sum;
}

std::string FS::generateFileNameByNumber(FS::FileType fileType, uint8_t number)
{
    char n[1] = {0};
    std::string name = fileTypeInfo[fileType].fileName;
    sprintf(n,"%d", number);
    return name.append(n);
}

std::string FS::prepareFreeFileSlot(FS::FileType fileType)
{
    deleteFile(generateFileNameByNumber(fileType, 0));

    //cyclic rename file n to n-1
    for (uint8_t fileNumber = 1; fileNumber < fileTypeInfo[fileType].count; fileNumber++)
        renameFile(generateFileNameByNumber(fileType, fileNumber),
                   generateFileNameByNumber(fileType, fileNumber-1));
}

std::string FS::prepareFileStorageToWriting(FS::FileType fileType)
{
    //is limit file count of this type
    if (fileTypeInfo[fileType].count == fileTypeInfo[fileType].maxCount)
    {
        prepareFreeFileSlot(fileType);
        return generateFileNameByNumber(fileType, fileTypeInfo[fileType].maxCount-1);
    }
    else if (getFreeFileSlotCount() > 0)
    {
        //define type of file to delete (is type with max file count)
        FS::FileType typeToDelete = fileType;
        for (uint8_t typeID = 0; typeID < 4; typeID++)
            if (fileTypeInfo[typeID].count > fileTypeInfo[fileType].count)
                typeToDelete = (FileType)typeID;

        prepareFreeFileSlot(typeToDelete);
        return generateFileNameByNumber(fileType, fileTypeInfo[fileType].count);
    } else
        return errorFileName;
}

} /* namespace DataStorage */
