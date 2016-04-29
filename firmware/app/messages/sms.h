#ifndef SMS_H
#define SMS_H



class sms
{
public:
    sms();

    char* Crc32(); // расчет crc32
    char* get8Byte(); // получить посылку

    void  formPacket(int *input);
    int*  getRandomBits(int len); // добавить рандомные биты до 744 некодированных битов
    int   getCALL_LCODE(int CYC_N,char *date); // расчет L_CODE

    int* output_post[7]; // массив под 37 посылок
};

#endif // SMS_H
