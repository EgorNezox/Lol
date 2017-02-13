
#include "ale_com.h"

 AleCom::AleCom()
 {

 }

void AleCom::generate_call(int8s* data, bool line_type, int8s cycle_num, int8s resp_addr)
{
	data[0]=((cycle_num&15)<<3)|((resp_addr&0x1C)>>2);
	data[1]=((resp_addr&3)<<6);
	if(line_type)
		data[0]=data[0]|0x80;
}

void AleCom::generate_resp_call_qual(int8s* data, int8s err_num, int8s snr)
{
	data[0]=((err_num&0x1F)<<3)|((snr&0x38)>>3);
	data[1]=(snr&0x07)<<5;
}

void AleCom::generate_trans_mode(int8s* data, int8s sound_type, int8s work_mode, int8s param_mode, bool schedule, int8s call_addr)
{
	data[0]=((sound_type&3)<<6)|((work_mode&15)<<2)|((param_mode&0x30)>>4);
	data[1]=((param_mode&15)<<4)|((call_addr&0x1C)>>2);
	data[2]=(call_addr&3)<<6;
    if(schedule)
		data[1]=data[1]|0x08;
}

void AleCom::generate_msg_head(int8s* data, int16s msg_size)
{
	data[0]=(msg_size&0x7F8)>>3;
	data[1]=(msg_size&7)<<5;
}

void AleCom::generate_resp_pack_qual(int8s* data, bool pack_result, int8s snr)
{
	data[0]=(snr&0x3F)<<1;
	if(pack_result)
		data[0]=data[0]|0x80;
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
	return ((data[0]&0x78)>>3);
}

int8s AleCom::get_call_resp_addr(int8s* data)
{
	return (((data[0]&0x07)<<2)|((data[1]&0xC0)>>6));
}

int16s AleCom::get_msg_head_msg_size(int8s* data)
{
    return (((((int16s)(data[0]))&0xFF)<<3)|((((int16s)(data[1]))&0xE0)>>5));
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
		counter++;
		if((out_sign_form[i]&0x0F)==0x0F)
			out_sign_form[i]=0;
		else if((out_sign_form[i]&0x0E)==0x0E)
			out_sign_form[i]=1;	
		else if((out_sign_form[i]&0x04)==0x04)
			out_sign_form[i]=2;	
		else if((out_sign_form[i]&0x08)==0x08)
			out_sign_form[i]=3;	
		else if((out_sign_form[i]&0x70)==0x70)
			out_sign_form[i]=4;	
		else if((out_sign_form[i]&0x60)==0x60)
			out_sign_form[i]=5;	
		else if((out_sign_form[i]&0x40)==0x40)
			out_sign_form[i]=6;	
		else if(out_sign_form[i]==0)
			counter--;
		else
			out_sign_form[i]=7;	
	}
	return counter;
}

int8s AleCom::get_short_sound_snr(int8s* data)
{

}

int8s AleCom::get_short_sound_sign_forms(int8s* data)
{

}
