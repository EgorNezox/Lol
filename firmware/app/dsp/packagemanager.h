#ifndef PACKAGEMANAGER_H
#define PACKAGEMANAGER_H
#include <stdint.h>
#include <stdlib.h>
#include <math.h>

class PackageManager
{
public:
    PackageManager();

    //---- List of functinal
    void ConvertBit(uint8_t src, bool dest[]);
    void ConvertByte(bool* data, uint8_t &sum);
    void Text(uint8_t *message, uint8_t* sms,int index);
    void UnpackText(uint8_t *message);
    unsigned int CRC32(unsigned char* pData,  int len);

private:
    const unsigned int CRC_POLY1 = 0xEDB88320;

    unsigned int table[256];

    void makeTable(int polynomChoosen);


};

#endif // PACKAGEMANAGER_H
