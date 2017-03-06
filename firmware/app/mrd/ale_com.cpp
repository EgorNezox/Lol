
#include "ale_com.h"
#include "ale_fxn.h"

int8s data_bit[528];

AleCom::AleCom()
{

}

void AleCom::resize_symbols(int8s* data_in, int8s* data_out, int16s size_in, int16s size_out, int16s length_in)
// --- size - number of bits in symbol
// --- size out%size_in==0 || size_in%size_out==0
{
    static int8s i,j,mul,mask,temp;
    if((data_in==data_out)&&(size_in==size_out))
        return;
    if(size_in==size_out)
    {
        for(i=0;i<length_in;i++)
            data_out[i]=data_in[i];
        return;
    }
    if(size_in>size_out)
    {
        mul=size_in/size_out;
        mask=(1<<size_out)-1;
        for(i=(length_in-1);i>=0;i--)
        {
            temp=data_in[i];
            for(j=0;j<mul;j++)
                data_out[j+i*mul]=(temp>>((mul-j-1)*size_out))&mask;
        }
        return;
    }
    if(size_in<size_out)
    {
        mul=size_out/size_in;
        mask=(1<<size_in)-1;
        for(i=0;i<length_in/mul;i++)
        {
            temp=0;
            for(j=0;j<mul;j++)
                temp=temp|((data_in[i*mul+j]&mask)<<((mul-j-1)*size_in));
            data_out[i]=temp;
        }
        return;
    }
}

int8s AleCom::generate_call(int8s* data, bool line_type, int8s cycle_num, int8s resp_addr)
{
    data_bit[0]=(int8s)line_type;
    data_bit[12]=cycle_num+1;
    resize_symbols(&(data_bit[12]),&(data_bit[1]),4,1,1);
    resize_symbols(&resp_addr,&(data_bit[5]),5,1,1);
    resize_symbols(data_bit,data,1,8,16);
    return 2;
}

int8s AleCom::generate_resp_call_qual(int8s* data, int8s err_num, int8s snr)
{
    resize_symbols(&err_num,&(data_bit[0]),5,1,1);
    resize_symbols(&snr,&(data_bit[5]),6,1,1);
    resize_symbols(data_bit,data,1,8,16);
    return 2;
}

int8s AleCom::generate_trans_mode(int8s* data, int8s sound_type, int8s work_mode, int8s param_mode, bool schedule, int8s call_addr)
{
    resize_symbols(&sound_type,&(data_bit[0]),2,1,1);
    resize_symbols(&work_mode,&(data_bit[2]),4,1,1);
    resize_symbols(&param_mode,&(data_bit[6]),6,1,1);
    data_bit[12]=(int8s)schedule;
    resize_symbols(&call_addr,&(data_bit[13]),5,1,1);
    resize_symbols(data_bit,data,1,8,24);
    return 3;
}

int8s AleCom::generate_sound_qual(int8s* data, int8s* indexes, int8s* snr, int8s* sign_forms)
{
    int16s crc16;
    for(int16s i=0;i<528;i++)
    	data_bit[i]=0;
    for(int16s i=0;i<3;i++)
    {
        resize_symbols(&(indexes[i]),&(data_bit[i*(5+6+7)]),5,1,1);
        resize_symbols(&(snr[i]),&(data_bit[i*(5+6+7)+5]),6,1,1);
        resize_symbols(&(sign_forms[i]),&(data_bit[i*(5+6+7)+5+6]),7,1,1);
    }
    resize_symbols(data_bit,data,1,8,56);
    crc16=Multiradio::AleFxn::CRC16(data,7);
    data_bit[526]=(crc16>>8)&0xFF;
    data_bit[527]=(crc16>>0)&0xFF;
    resize_symbols(&(data_bit[526]),&(data_bit[(5+6+7)*3]),8,1,2);
    resize_symbols(data_bit,data,1,8,72);
    return 9;
}

int8s AleCom::generate_msg_head(int8s* data, int16s msg_size)
{
    //data_bit[526]=(msg_size>>8)&0xFF;
    //data_bit[527]=(msg_size>>0)&0xFF;
#ifndef	NEW_MSG_HEAD
    data[0]=(msg_size>>3)&0xFF;
    data[1]=(msg_size<<5)&0xE0;
#else
    data[0]=(msg_size>>4)&0xFF;
    data[1]=(msg_size<<4)&0xE0;
#endif
    //resize_symbols(&(data_bit[526]),&(data_bit[0]),8,1,2);
    //resize_symbols(&(data_bit[5]),data,1,8,16);
    return 2;
}

int8s AleCom::generate_resp_pack_qual(int8s* data, bool pack_result, int8s snr)
{
	data[0]=(snr&0x3F)<<1;
	if(pack_result)
		data[0]=data[0]|0x80;
    return 2;
}

bool AleCom::check_resp_pack_qual(int8s* data)
// TRUE - DATA RECEIVE OK, FALSE - DATA RECEIVING ERROR
{
	if((data[0]&128)==128)
		return true;
	return false;
}

int8s AleCom::get_resp_pack_qual_snr(int8s* data)
{
	return ((data[0]&0x7E)>>1);
}

int8s AleCom::get_resp_call_qual_err_num(int8s* data)
{
	return ((data[0]&0xF8)>>3);
}

int8s AleCom::get_resp_call_qual_snr(int8s* data)
{
	return (((data[0]&0x7)<<3)|((data[1]&0xE0)>>5));
}

int8s AleCom::get_trans_mode_sound_type(int8s* data)
{
	return ((data[0]&0xC0)>>6);
}

int8s AleCom::get_trans_mode_work_mode(int8s* data)
{
	return ((data[0]&0x3C)>>2);
}

int8s AleCom::get_trans_mode_param_mode(int8s* data)
{
	return (((data[0]&0x03)<<4)|((data[1]&0xF0)>>4));
}

bool AleCom::get_trans_mode_schedule(int8s* data)
{
	if((data[1]&8)==8)
		return true;
	return false;
}

int8s AleCom::get_trans_mode_caller(int8s* data)
{
	return (((data[1]&7)<<2)|((data[2]&0xC0)>>6));
}

bool AleCom::get_call_line_type(int8s* data)
{
	if((data[0]&0x80)==0x80)
		return true;
	return false;
}

int8s AleCom::get_call_cycle_num(int8s* data)
{
	return (((data[0]&0x78)>>3)+1);
}

int8s AleCom::get_call_resp_addr(int8s* data)
{
	return (((data[0]&0x07)<<2)|((data[1]&0xC0)>>6));
}

int16s AleCom::get_msg_head_msg_size(int8s* data)
{
#ifndef	NEW_MSG_HEAD
    return (((((int16s)(data[0]))&0xFF)<<3)|((((int16s)(data[1]))&0xE0)>>5));
#else
    return (((((int16s)(data[0]))&0xFF)<<4)|((((int16s)(data[1]))&0xE0)>>4));
#endif
}

int8s AleCom::get_pack_head_sign_form(int8s* data)
{
	return (data[0]&7);
}

int8s AleCom::get_sound_qual_freq_info(int8s* data, int8s* out_indexes, int8s* out_sign_form)
{
	int8s counter;
	counter=0;
	out_indexes[0]=(data[0]&0xF8)>>3;
	out_indexes[1]=(data[2]&0x3E)>>1;
	out_indexes[2]=((data[4]&0x0F)<<1)|((data[5]&0x80)>>7);
	out_sign_form[0]=((data[1]&0x1F)<<2)|((data[2]&0xC0)>>6);
	out_sign_form[1]=((data[3]&0x07)<<4)|((data[4]&0xF0)>>4);
    out_sign_form[2]=((data[5]&0x01)<<6)|((data[6]&0xFC)>>2);
    for(int8s i=0;i<3;i++)
    {
    	if(out_sign_form[i]!=0)
    		counter++;
    }
	return counter;
}

int16s AleCom::get_sound_qual_crc16(int8s* data)
{
    data[8]=((data[8]&0xFC)>>2)|((data[7]&0x03)<<6);
    data[7]=((data[7]&0xFC)>>2)|((data[6]&0x03)<<6);
    data[6]=data[6]&0xFC;
    return ((((int16s)data[7])<<8)|(((int16s)data[8])<<0));
}

int8s AleCom::get_packet_num(int8s* data)
{
    return ((data[0]&0xFC)>>2);
}

int8s AleCom::get_short_sound_snr(int8s* data)
{

}

int8s AleCom::get_short_sound_sign_forms(int8s* data)
{

}
