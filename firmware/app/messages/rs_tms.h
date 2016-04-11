#ifndef RS_TMS_H_
#define RS_TMS_H_

//#define	GALUA_FIELD_SIZE	6								// MM
//#define	INFO_SYMBOLS_NUM	64								// KK
//#define	CODE_LENGTH			128								// NN
//#define	MAX_CODE_LENGTH		((1 << GALUA_FIELD_SIZE) - 1)	// NN_MAX

#define MAX_GALUA_FIELD_SIZE	12
#define	MAX_CODE_LENGTH			4096
//(1 << MAX_GALUA_FIELD_SIZE)
//#define	MIN_INFO_SYMBOLS_NUM	64

#define B0					1

typedef struct	{
	int galua_field_size;				// (MM)
	int max_code_length;				// (NN_MAX) = 2 ^ galua_field_size
	int code_length;					// (NN)
	int info_symbols_num;				// (KK)
	int alpha_to[MAX_CODE_LENGTH+1];	// index -> poly
	int index_of[MAX_CODE_LENGTH+1];	// poly -> index
	int gg[MAX_CODE_LENGTH+1];
	int real_info_num;
	int real_code_length;
}	rs_settings;

rs_settings setting;

static int primitive_polynom[14][15]=	{
		0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,
		1,	1,	1,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,
		1,	1,	0,	1,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,
		1,	1,	0,	0,	1,	0,	0,	0,	0,	0,	0,	0,	0,	0,	0,
		1,	0,	1,	0,	0,	1,	0,	0,	0,	0,	0,	0,	0,	0,	0,
		1,	1,	0,	0,	0,	0,	1,	0,	0,	0,	0,	0,	0,	0,	0,
		1,	0,	0,	1,	0,	0,	0,	1,	0,	0,	0,	0,	0,	0,	0,
		1,	0,	1,	1,	1,	0,	0,	0,	1,	0,	0,	0,	0,	0,	0,
		1,	0,	0,	0,	1,	0,	0,	0,	0,	1,	0,	0,	0,	0,	0,
		1,	0,	0,	1,	0,	0,	0,	0,	0,	0,	1,	0,	0,	0,	0,
		1,	0,	1,	0,	0,	0,	0,	0,	0,	0,	0,	1,	0,	0,	0,
		1,	1,	0,	0,	1,	0,	1,	0,	0,	0,	0,	0,	1,	0,	0,
		1,	1,	0,	1,	1,	0,	0,	0,	0,	0,	0,	0,	0,	1,	0,
		1,	1,	0,	0,	0,	0,	1,	0,	0,	0,	1,	0,	0,	0,	1
};

int rs_encode(rs_settings* settings, int* input, int* output, int input_length);
// situation input==output resolved
int rs_decode(rs_settings* settings, int* input, int* input_clear, int* output, int* output_clear, int input_length);
// situation input==output resolved
void GenerateGaloisField(rs_settings* settings);
void gen_poly(rs_settings* settings);
int eras_dec_rs(int* data, int* erase, rs_settings* settings);
int encode_rs(int* data, int* bb, rs_settings* settings);

#endif	/*	RS_TMS_H_	*/
