#ifndef PACKAGEMANAGER_H
#define PACKAGEMANAGER_H
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <map>
#include <vector>

class PackageManager
{
public:
    PackageManager();

    //---- List of functinal
    void ConvertBit(uint8_t src, bool dest[]);
    void ConvertByte(bool* data, uint8_t &sum);
    void Text(uint8_t *message, uint8_t* sms,int index);
    void UnpackText(uint8_t *message,uint8_t *source);
    unsigned int CRC32(unsigned char* pData,  int len);
    void PackText(uint8_t *message, uint8_t* source);

    int decompressMass(uint8_t *srcBuf,uint16_t srcLen,uint8_t *dstBuf,uint16_t  dstLen, uint16_t  numBits);
    int compressMass(uint8_t *srcBuf,int srcLen,int numBits);

    void to_Koi7(uint8_t *message);
    void to_Win1251(uint8_t *message);
    void shiftMasTo7Bit(uint8_t *input, uint8_t *output,  int out_shift, int len);
    void addBytetoBitsArray(uint8_t value, std::vector<bool> &data, int len);
    void getArrayByteFromBit(std::vector<bool> &data, uint8_t* dest);

private:
    const unsigned int CRC_POLY1 = 0xEDB88320;

    unsigned int table[256];

    void makeTable(int polynomChoosen);

    std::map<char,int> koi_sim;


};

#endif // PACKAGEMANAGER_H
