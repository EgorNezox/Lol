#include "packagemanager.h"
#include "string.h"
#include "ui/texts.h"

PackageManager::PackageManager()
{
    makeTable(CRC_POLY1);
//    koi_sim.insert(std::pair<char,int>(ch_key1[0],46));
//    koi_sim.insert(ch_key1[1],44);
//    koi_sim.insert(ch_key1[2],33);
//    koi_sim.insert(ch_key1[3],63);
//    koi_sim.insert(ch_key1[4],34);
//    koi_sim.insert(ch_key1[5],58);
//    koi_sim.insert(ch_key2[0],97);
//    koi_sim.insert(ch_key2[1],98);
//    koi_sim.insert(ch_key2[2],119);
//    koi_sim.insert(ch_key2[3],103);
//    koi_sim.insert(ch_key3[0],100);
//    koi_sim.insert(ch_key3[1],101);
//    koi_sim.insert(ch_key3[2],118);
//    koi_sim.insert(ch_key3[3],122);
//    koi_sim.insert(ch_key4[0],105);
//    koi_sim.insert(ch_key4[1],106);
//    koi_sim.insert(ch_key4[2],107);
//    koi_sim.insert(ch_key4[3],108);
//    koi_sim.insert(ch_key5[0],109);
//    koi_sim.insert(ch_key5[1],110);
//    koi_sim.insert(ch_key5[2],111);
//    koi_sim.insert(ch_key5[3],112);
//    koi_sim.insert(ch_key6[0],114);
//    koi_sim.insert(ch_key6[1],115);
//    koi_sim.insert(ch_key6[2],116);
//    koi_sim.insert(ch_key6[3],117);
//    koi_sim.insert(ch_key7[0],102);
//    koi_sim.insert(ch_key7[1],104);
//    koi_sim.insert(ch_key7[2],99);
//    koi_sim.insert(ch_key7[3],126);
//    koi_sim.insert(ch_key8[0],123);
//    koi_sim.insert(ch_key8[1],125);
//    koi_sim.insert(ch_key8[2],-1);
//    koi_sim.insert(ch_key8[3],121);
//    koi_sim.insert(ch_key9[0],120);
//    koi_sim.insert(ch_key9[1],124);
//    koi_sim.insert(ch_key9[2],96);
//    koi_sim.insert(ch_key9[3],113);
//    koi_sim.insert(ch_key0[0],48);
//    koi_sim.insert('1',49);
//    koi_sim.insert('2',50);
//    koi_sim.insert('3',51);
//    koi_sim.insert('4',52);
//    koi_sim.insert('5',53);
//    koi_sim.insert('6',54);
//    koi_sim.insert('7',55);
//    koi_sim.insert('8',56);
//    koi_sim.insert('9',57);
//    koi_sim.insert(ch_key0[1],32);

}

void PackageManager::ConvertBit(uint8_t src, bool dest[7])
{
    for(int i = 0; i < 7; i++) {
        uint8_t temp = (src & (1 << i)) >> i;
        dest[i] = (bool)temp;
//        dest[7 - i] = (bool)((src & (1 << i)) >> i);
    }
}

void PackageManager::ConvertByte(bool* data, uint8_t& sum)
{
    for(int i = 0; i < 7; i++)
        if (data[i])
            sum += (uint8_t)pow(2, 6-i);
}

// 700 simbols
void PackageManager::Text(uint8_t *message, uint8_t* sms,int index)
{

    bool mes[704];

    for(int i = 0; i<index;i++)
    {
        bool rc[7] = {0,0,0,0,0,0,0};
        ConvertBit(message[i], rc);
        memcpy(&mes[i*7], rc, 7); // по 7 символов
    }

    if ((7*index) < 700)
        for(int i = 7*index; i<700;i++)
            mes[i] = 0;

    for(int i = 700; i<704;i++)
        mes[i] = 0; // последние нули


    for(int i = 0; i < 88; i++)
    {
        bool str[8];
        memcpy(&str,&mes[i*8] ,8);
        ConvertByte(str, sms[i]);
    }
}

void PackageManager::UnpackText(uint8_t *message,uint8_t *source)
{
	int cnt = 7;

	for(int i = 0; i<100;i++)
	 {
		source[i] = source[i] & 0x7F;

		for(int j = 0; j < 7;j++)
		{
		  source[cnt] = message[i] & (1 >> j);
		  ++cnt;
		}
	 }
}

void PackageManager::PackText(uint8_t *message, uint8_t* source)
{
	int cnt = 0;
	int index = 0;

	for(int i = 0; i<100;i++)
	{
		for(int j  = 0; j<8;j++)
		{
			int shift  = index % 7;
			source[index/8] = (source[index/8] | (message[i] & (1 << shift))); // 1-7 ��� ����� �
			++index;
		}
	}
}



int PackageManager::decompressMass(uint8_t *srcBuf,
                                 uint16_t srcLen,
								 uint8_t *dstBuf,
								 uint16_t  dstLen,
								 uint16_t  numBits)
{
    // ��������������� ���������� ��� ������������ ������� ������
	uint8_t   byte, b1;
    // ��������������� ���������� ����� ���� � ������� �����
	uint8_t   nBit;

	uint16_t lenRet;

    if (dstLen < ((srcLen*8/numBits)))
        return -1;

    // �������� ���������� �������� ����� �������
    lenRet = 0;
    nBit = 0;
    byte = 0;
    uint16_t  nByte;
    uint16_t  bit;

    for (nByte = 0; nByte < srcLen; nByte++)
    {
        b1 = srcBuf[nByte];
        // ������� ���������� ������ ���� ������� � 0 �� 7 ����
        for (bit = 0; bit < 8; bit++)
        {
            // ���� �� ������ ����, � ������� �������� 0 ��� 1
            // � �������� �� ������ ������� � �������� �����
            if (b1 & (1 << bit))
                byte |= (1 << nBit);
            // �������������� ����� ����
            nBit++;
            // ���� ������� ���� ������� �� ������� �������� ����� �� ���� �����
            if (nBit >= numBits)
            {
                // ���������� ��� � ��� �� ������
                dstBuf[lenRet] = byte;
                // �������������� ����� �������� ����� (����� ��������� �������)
                lenRet++;
                byte = 0;
                nBit = 0;
            }
        }
    }

    // ���������� ��������� ���� �� � ��� �� ���������� �������
    if (nBit)
    {
        // ���������� ��� � ��� �� ������
        dstBuf[lenRet] = byte;
        // �������������� ����� �������� ����� (����� ��������� �������)
        lenRet++;
    }

    return lenRet;
}


int PackageManager::compressMass(uint8_t *srcBuf,int srcLen,int numBits)
{
    // ��������������� ���������� ��� ������������ ������� ������
	uint8_t  byte, b1;
    // ��������������� ���������� ����� ���� � ������� �����
	uint8_t  nBit;

	uint16_t lenDst;

    // �������� ���������� �������� ����� �������
    lenDst = 0;
    nBit = 0;
    byte = 0;
    uint16_t nByte;
    uint16_t bit;
    // ����������� ������� ���� �� ��� �������
    for (nByte = 0; nByte < srcLen; nByte++)
    {
        b1 = srcBuf[nByte];
        // ������� ���������� ������ ���� ������� � 0 �� numBits ����
        for (bit = 0; bit < numBits; bit++)
        {
            // ���� �� ������ ����, � ������� �������� 0 ��� 1
            // � �������� �� ������ ������� � �������� �����
            if (b1 & (1 << bit))
                byte |= (1 << nBit);
            // �������������� ����� ����
            nBit++;
            // ���� ������� ���� ������� �� ������� ����� �� ���� �����
            if (nBit >= 8)
            {
                // ���������� ��� � ��� �� ������
                srcBuf[lenDst] = byte;
                // �������������� ����� �������� ����� (����� ��������� �������)
                lenDst++;
                byte = 0;
                nBit = 0;
            }
        }
    }

    // ���������� ��������� ���� �� � ��� �� ���������� �������
    if (nBit)
    {
        // ���������� ��� � ��� �� ������
        srcBuf[lenDst] = byte;
        // �������������� ����� �������� ����� (����� ��������� �������)
        lenDst++;
    }

    return lenDst;
}

void PackageManager::to_Koi7(uint8_t *message)
{

}

void PackageManager::makeTable(int polynomChoosen)
{
    const unsigned int CRC_POLY1 = 0xEDB88320; //is used in Total Commander
    unsigned int i, j, r;

    for (i = 0; i < 256; i++)
    {
        for (r = i, j = 8; j; j--)
            r = r & 1? (r >> 1) ^ CRC_POLY1: r >> 1;
        table[i] = r;
    }

}


unsigned int PackageManager::CRC32(unsigned char* pData,  int len)
{
    const unsigned int CRC_MASK = 0xD202EF8D;
    unsigned int crc = 0;
    while (len--)
    {
        crc = table[(unsigned char)crc ^ (* pData ++)] ^ crc >> 8;
        crc ^= CRC_MASK;
    }
    return crc;
}
