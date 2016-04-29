#ifndef RS_TMS_H_
#define RS_TMS_H_


//#define	GALUA_FIELD_SIZE	6								// MM
//#define	INFO_SYMBOLS_NUM	64								// KK
//#define	CODE_LENGTH			128								// NN
//#define	MAX_CODE_LENGTH		((1 << GALUA_FIELD_SIZE) - 1)	// NN_MAX

namespace Galua
{

#define MAX_GALUA_FIELD_SIZE	12
#define	MAX_CODE_LENGTH			1<<MAX_GALUA_FIELD_SIZE
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




int erase_pos[MAX_CODE_LENGTH];
int recd[MAX_CODE_LENGTH];
int lambda[MAX_CODE_LENGTH+1];
int s[MAX_CODE_LENGTH+1];  // Err+Eras Locator poly
								// and syndrome poly
int b[MAX_CODE_LENGTH+1];
int t[MAX_CODE_LENGTH+1];
int omega[MAX_CODE_LENGTH+1];
int root[MAX_CODE_LENGTH];
int reg[MAX_CODE_LENGTH+1];
int loc[MAX_CODE_LENGTH];

int temp_data[MAX_CODE_LENGTH+1];
int temp_data_clear[MAX_CODE_LENGTH+1];

void initrs(rs_settings &rs)
{
	rs.galua_field_size=8;
	rs.code_length=255;
	rs.info_symbols_num=93;
	rs.real_code_length=255;
	rs.real_info_num=93;
}

int encode_rs(int* data, int* bb, rs_settings* settings)
{
    static int i, j;
    static int feedback;

    //CLEAR(bb,NN-KK);
	for(i=0;i<settings->code_length-settings->info_symbols_num;i++)
		bb[i]=0;

    for (i = settings->info_symbols_num - 1; i >= 0; i--) {

        feedback = settings->index_of[data[i] ^ bb[settings->code_length - settings->info_symbols_num - 1]];
        if (feedback != (settings->max_code_length)) {   /* feedback term is non-zero */
            for (j = settings->code_length - settings->info_symbols_num - 1; j > 0; j--)
                if (settings->gg[j] !=(settings->max_code_length))
                	bb[j] = bb[j - 1] ^ settings->alpha_to[(settings->gg[j] + feedback) % (settings->max_code_length)];
                else
                    bb[j] = bb[j - 1];
            bb[0] = settings->alpha_to[(settings->gg[0] + feedback) % (settings->max_code_length)];
        } else {    /* feedback term is zero. encoder becomes a
                 * single-byte shifter */
            for (j = settings->code_length - settings->info_symbols_num - 1; j > 0; j--) {
                bb[j] = bb[j - 1];
            }
            bb[0] = 0;
        }
    }
    return 0;
}

int eras_dec_rs(int* data, int* erase, rs_settings* settings)
{
    static int deg_lambda, el, deg_omega;
    static int i, j, r;
	static int u,q,tmp,num1,num2,den,discr_r;

	static int syn_error, count, ret;
	static int Nerase = 0;
    for(i=0; i < settings->code_length; i++) {
        if(erase[i]) {
            erase_pos[Nerase++] = i;
        }
    }
    ret=0;
    if(Nerase >= (settings->code_length-settings->info_symbols_num))
        return(-1);
    /* data[] is in polynomial form, copy and convert to index form */
    for (i = 0; i < settings->code_length; i++){
        recd[i] = settings->index_of[data[i]];
    }
    /* first form the syndromes; i.e., evaluate recd(x) at roots of g(x)
     * namely @**(B0+i), i = 0, ... ,(NN-KK-1)
     */
    syn_error = 0;
    for (i = 0; i < settings->code_length-settings->info_symbols_num; i++) {
        tmp = 0;
        for (j = 0; j < settings->code_length; j++) {
            if (recd[j] != settings->max_code_length)  /* recd[j] in index form */
                tmp ^= settings->alpha_to[(recd[j] + (B0 + i)*((j < settings->info_symbols_num)? j:(j + settings->max_code_length - settings->code_length))) % settings->max_code_length];
        }
        syn_error |= tmp;   /* set flag if non-zero syndrome =>  error */
        /* store syndrome in index form  */
        s[i + 1] = settings->index_of[tmp];
    }


    /*
     * if syndrome is zero, data[] is a codeword and there are no
     * errors to correct. So return data[] unmodified
    */

    if(syn_error) {
        //CLEAR(&lambda[1], NN-KK);
		for (i=1;i<(settings->code_length-settings->info_symbols_num);i++)
			lambda[i]=0;
        lambda[0] = 1;

        if (Nerase > 0) {
            /* Init lambda to be the erasure locator polynomial */
            lambda[1] = settings->alpha_to[(erase_pos[0] < settings->info_symbols_num)? erase_pos[0]:(erase_pos[0] + settings->max_code_length - settings->code_length)];
            for (i = 1; i < Nerase; i++) {
                u = (erase_pos[i] < settings->info_symbols_num)? erase_pos[i]:(erase_pos[i] + settings->max_code_length - settings->code_length);
                for (j = i+1; j > 0; j--) {
                    tmp = settings->index_of[lambda[j - 1]];
                    if(tmp != settings->max_code_length)
                        lambda[j] ^= settings->alpha_to[(u + tmp) % settings->max_code_length];
                }
            }
            /* find roots of the erasure location polynomial */
            for(i=1; i <= Nerase; i++)
                reg[i] = settings->index_of[lambda[i]];
            count = 0;
            for (i = 1; i <= settings->max_code_length; i++) {
                q = 1;
                for (j = 1; j <= Nerase; j++)
                    if (reg[j] != settings->max_code_length) {
                        reg[j] = (reg[j] + j) % settings->max_code_length;
                        q ^= settings->alpha_to[reg[j]];
                    }
                if (!q) {
                    /* store root and error location
                     * number indices
                     */
                    root[count] = i;
                    loc[count] = settings->max_code_length - i;
                    count++;
                }
            }
            if (count != Nerase) {
                ret = -1;
            }
        }

        if(ret != -1) {

            for(i=0; i < settings->code_length-settings->info_symbols_num+1; i++)
                b[i] = settings->index_of[lambda[i]];

            /*
             * Begin Berlekamp-Massey algorithm to determine error+erasure
             * locator polynomial
             */
            r = Nerase;
            el = Nerase;
            while (++r <= settings->code_length-settings->info_symbols_num) {  /* r is the step number */
                /* Compute discrepancy at the r-th step in poly-form */
                discr_r = 0;
                for (i = 0; i < r; i++){
                    if ((lambda[i] != 0) && (s[r - i] != settings->max_code_length)) {
                        discr_r ^= settings->alpha_to[(settings->index_of[lambda[i]] + s[r - i]) % settings->max_code_length];
                    }
                }
                discr_r = settings->index_of[discr_r];    /* Index form */
                if (discr_r == settings->max_code_length) {
                    /* 2 lines below: B(x) <-- x*B(x) */
                    //COPYDOWN(&b[1], b, NN-KK);
					for(i=0;i<settings->code_length-settings->info_symbols_num;i++)
						b[settings->code_length-settings->info_symbols_num-i]=b[settings->code_length-settings->info_symbols_num-i-1];
                    b[0] = settings->max_code_length;
                } else {
                    /* 7 lines below: T(x) <-- lambda(x) - discr_r*x*b(x) */
                    t[0] = lambda[0];
                    for (i = 0 ; i < settings->code_length-settings->info_symbols_num; i++) {
                        if(b[i] != settings->max_code_length)
                            t[i+1] = lambda[i+1] ^ settings->alpha_to[(discr_r + b[i]) % settings->max_code_length];
                        else
                            t[i+1] = lambda[i+1];
                    }
                    if (2 * el <= r + Nerase - 1) {
                        el = r + Nerase - el;
                        /*
                         * 2 lines below: B(x) <-- inv(discr_r) *
                         * lambda(x)
                         */
                        for (i = 0; i <= settings->code_length-settings->info_symbols_num; i++)
                            b[i] = (lambda[i] == 0) ? settings->max_code_length : (settings->index_of[lambda[i]] - discr_r + settings->max_code_length) % settings->max_code_length;
                    } else {
                        /* 2 lines below: B(x) <-- x*B(x) */
                        //COPYDOWN(&b[1], b, NN-KK);
                        for(i=0;i<settings->code_length-settings->info_symbols_num;i++)
							b[settings->code_length-settings->info_symbols_num-i]=b[settings->code_length-settings->info_symbols_num-i-1];
						b[0] = settings->max_code_length;
                    }
                    //COPY(lambda, t, NN-KK+1);
					for(i=0;i<settings->code_length-settings->info_symbols_num+1;i++)
						lambda[i]=t[i];
                }
            }

            /* Convert lambda to index form and compute deg(lambda(x)) */
            deg_lambda = 0;
            for(i=0; i < settings->code_length-settings->info_symbols_num+1; i++)
			{
                lambda[i] = settings->index_of[lambda[i]];
                if(lambda[i] != settings->max_code_length)
                    deg_lambda = i;
            }

            if(((el-Nerase) > (settings->code_length-settings->info_symbols_num-Nerase)/2) || (deg_lambda != el) || (!deg_lambda))
                ret = -1;

            else
			{
                /*
                 * Find roots of the error+erasure locator polynomial. By Chien
                 * Search
                 */
                //COPY(&reg[1],&lambda[1],NN-KK);
				for(i=1;i<settings->code_length-settings->info_symbols_num;i++)
					reg[i]=lambda[i];
                count = 0;      /* Number of roots of lambda(x) */
                for (i = 1; i <= settings->max_code_length; i++) {
                    q = 1;
                    for (j = deg_lambda; j > 0; j--) {
                        if (reg[j] != settings->max_code_length) {
                            reg[j] = (reg[j] + j) % settings->max_code_length;
                            q ^= settings->alpha_to[reg[j]];
                        }
                    }
                    if (!q) {
                        /* store root (index-form) and error location number */
                        root[count] = i;
                        loc[count] = settings->max_code_length - i;
                        // If we've already found max possible roots,
                        // abort the search to save time
                        //
                        if(++count == deg_lambda)
                            break;
                    }
                }

                if (deg_lambda != count) {
                    /* deg(lambda) unequal to number of roots => uncorrectable error detected */
                    ret = -1;
                }
                else {
                    /*
                     * Compute err+eras evaluator poly omega(x) = s(x)*lambda(x) (modulo
                     * x**(NN-KK)). in index form. Also find deg(omega).
                     */
                    deg_omega = 0;
                    for (i = 0; i < settings->code_length-settings->info_symbols_num; i++){
                        tmp = 0;
                        j = (deg_lambda < i) ? deg_lambda : i;
                        for(;j >= 0; j--){
                            if ((s[i + 1 - j] != settings->max_code_length) && (lambda[j] != settings->max_code_length))
                                tmp ^= settings->alpha_to[(s[i + 1 - j] + lambda[j]) % settings->max_code_length];
                        }
                        if(tmp != 0)
                            deg_omega = i;
                        omega[i] = settings->index_of[tmp];
                    }
                    omega[settings->code_length-settings->info_symbols_num] = settings->max_code_length;

                    /*
                     * Compute error values in poly-form. num1 = omega(inv(X(l))), num2 =
                     * inv(X(l))**(B0-1) and den = lambda_pr(inv(X(l))) all in poly-form
                    */
                    ret = count;
                    for (j = count-1; j >=0; j--) {
                        num1 = 0;
                        for (i = deg_omega; i >= 0; i--) {
                            if (omega[i] != settings->max_code_length)
                                num1  ^= settings->alpha_to[(omega[i] + i * root[j]) % settings->max_code_length];
                        }
                        num2 = settings->alpha_to[(root[j] * (B0 - 1) + settings->max_code_length) % settings->max_code_length];
                        den = 0;

                        /* lambda[i+1] for i even is the formal derivative lambda_pr of lambda[i] */
                        /*for (i = min(deg_lambda,settings->code_length-settings->info_symbols_num-1) & ~1; i >= 0; i -=2) {
                            if(lambda[i+1] != settings->max_code_length)
                                den ^= settings->alpha_to[(lambda[i+1] + i * root[j]) % settings->max_code_length];
                        }*/
                        if(deg_lambda<(settings->code_length-settings->info_symbols_num-1))
                        {
                        	for (i = (deg_lambda & (~1)); i >= 0; i -=2) {
								if(lambda[i+1] != settings->max_code_length)
									den ^= settings->alpha_to[(lambda[i+1] + i * root[j]) % settings->max_code_length];
							}
                        }
                        else
                        {
                        	for (i = ((settings->code_length-settings->info_symbols_num-1) & (~1)); i >= 0; i -=2) {
								if(lambda[i+1] != settings->max_code_length)
									den ^= settings->alpha_to[(lambda[i+1] + i * root[j]) % settings->max_code_length];
							}
                        }

                        if (den == 0) { // ERROR: denominator = 0
                            ret = -1;
                            break;
                        }

                        /* Apply error to data */
                        if (num1 != 0) {
                            int n_loc = loc[j];
                            if(n_loc >= settings->info_symbols_num && n_loc < settings->max_code_length - (settings->code_length -settings->info_symbols_num)) {
                                ret = -1;
                                break;
                            }
                            data[(n_loc < settings->info_symbols_num)? loc[j]:(loc[j]- (settings->max_code_length - settings->code_length))] ^= settings->alpha_to[(settings->index_of[num1] + settings->index_of[num2] + settings->max_code_length - settings->index_of[den]) % settings->max_code_length];
                        }
                    }
                }
            }
        }
    }

    return ret;
}

int rs_encode(rs_settings* settings, int* input, int* output, int input_length)
// situation input==output resolved
{
	static int i,j;
	if((input_length%(settings->real_info_num))!=0)
		return 32767;
	for(i=(input_length/(settings->real_info_num))-1;i>=0;i--)
	{
		for(j=0;j<(settings->real_info_num);j++)
			temp_data[j]=input[j+i*(settings->real_info_num)];
		for(j=(settings->real_info_num);j<(settings->info_symbols_num);j++)
			temp_data[j]=0;
		encode_rs(temp_data,&(temp_data[settings->info_symbols_num]),settings);
		for(j=0;j<(settings->real_info_num);j++)
			output[i*(settings->real_code_length)+j]=temp_data[j];
		for(j=(settings->real_info_num);j<(settings->real_code_length);j++)
			output[i*(settings->real_code_length)+j]=temp_data[settings->info_symbols_num+j-settings->real_info_num];
	}
	return 0;
}

int rs_decode(rs_settings* settings, int* input, int* input_clear, int* output, int* output_clear, int input_length)
// situation input==output resolved
{
	static int i,j, clr, temp;
	if((input_length%(settings->real_code_length))!=0)
		return 32767;
	clr=0;
	for(i=0;i<(input_length/(settings->real_code_length));j++)
	{
		for(j=0;j<(settings->real_info_num);j++)
		{
			temp_data[j]=input[i*(settings->real_code_length)+j];
			temp_data_clear[j]=input_clear[i*(settings->real_code_length)+j];
		}
		for(j=(settings->real_info_num);j<(settings->info_symbols_num);j++)
		{
			temp_data[j]=0;
			temp_data_clear[j]=0;
		}
		for(j=(settings->info_symbols_num);j<(settings->code_length);j++)
		{
			temp_data[j]=input[i*(settings->real_code_length)+j+settings->real_info_num-settings->info_symbols_num];
			temp_data_clear[j]=input_clear[i*(settings->real_code_length)+j+settings->real_info_num-settings->info_symbols_num];
		}
		temp=eras_dec_rs(temp_data,temp_data_clear,settings);
		if(temp==-1)
		{
			for(j=0;j<(settings->real_info_num);j++)
				output_clear[i*(settings->real_info_num)+j]=1;
		}
		else
		{
			clr=clr+temp;
			for(j=0;j<(settings->real_info_num);j++)
				output_clear[i*(settings->real_info_num)+j]=0;
		}
		for(j=0;j<(settings->real_info_num);j++)
			output[i*(settings->real_info_num)+j]=temp_data[j];
	}
	return clr;
}
// situation input==output resolved
void GenerateGaloisField(rs_settings* settings)
{
    static int i, mask;
	mask = 1;
	settings->max_code_length=(1<<(settings->galua_field_size))-1;
    settings->alpha_to[settings->galua_field_size] = 0;
    for (i = 0; i < settings->galua_field_size; i++) {
    	settings->alpha_to[i] = mask;
    	settings->index_of[settings->alpha_to[i]] = i;
        /* If Pp[i] == 1 then, term @^i occurs in poly-repr of @^MM */
        if (/*settings->p_mas[i]*/primitive_polynom[(settings->galua_field_size)-1][i] != 0)
        	settings->alpha_to[settings->galua_field_size] ^= mask;   /* Bit-wise EXOR operation */
        mask <<= 1;                 /* single left-shift */
    }
    settings->index_of[settings->alpha_to[settings->galua_field_size]] = settings->galua_field_size;
    /*
     * Have obtained poly-repr of @^MM. Poly-repr of @^(i+1) is given by
     * poly-repr of @^i shifted left one-bit and accounting for any @^MM
     * term that may occur when poly-repr of @^i is shifted.
     */
    mask >>= 1;
    for (i = settings->galua_field_size + 1; i < settings->max_code_length; i++) {
        if (settings->alpha_to[i - 1] >= mask)
        	settings->alpha_to[i] = settings->alpha_to[settings->galua_field_size] ^ ((settings->alpha_to[i - 1] ^ mask) << 1);
        else
        	settings->alpha_to[i] = settings->alpha_to[i - 1] << 1;
        settings->index_of[settings->alpha_to[i]] = i;
    }
    settings->index_of[0] = settings->max_code_length;
    settings->alpha_to[settings->max_code_length] = 0;
}
void gen_poly(rs_settings* settings)
{
    static int i, j;
    settings->gg[0] = settings->alpha_to[B0];
    settings->gg[1] = 1;      /* g(x) = (X+@**B0) initially */
    for (i = 2; i <= settings->code_length - settings->info_symbols_num; i++) {
    	settings->gg[i] = 1;
        /*
         * Below multiply (Gg[0]+Gg[1]*x + ... +Gg[i]x^i) by
         * (@**(B0+i-1) + x)
         */
        for (j = i - 1; j > 0; j--)
            if (settings->gg[j] != 0)
            	settings->gg[j] = settings->gg[j - 1] ^ settings->alpha_to[(settings->index_of[settings->gg[j]] + B0 + i - 1) % (settings->max_code_length)];
            else
            	settings->gg[j] = settings->gg[j - 1];
        /* Gg[0] can never be zero */
        settings->gg[0] = settings->alpha_to[((settings->index_of[settings->gg[0]]) + B0 + i - 1) % (settings->max_code_length)];
    }
    /* convert Gg[] to index form for quicker encoding */
    for (i = 0; i <= settings->code_length - settings->info_symbols_num; i++)
    	settings->gg[i] = settings->index_of[settings->gg[i]];
}




}
#endif	/*	RS_TMS_H_	*/
