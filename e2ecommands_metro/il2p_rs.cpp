//
//    This file is part of Dire Wolf, an amateur radio packet TNC.
//
//    Copyright (C) 2021  John Langner, WB2OSZ
//
//    This program is free software: you can redistribute it and/or modify
//    it under the terms of the GNU General Public License as published by
//    the Free Software Foundation, either version 2 of the License, or
//    (at your option) any later version.
//
//    This program is distributed in the hope that it will be useful,
//    but WITHOUT ANY WARRANTY; without even the implied warranty of
//    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//    GNU General Public License for more details.
//
//    You should have received a copy of the GNU General Public License
//    along with this program.  If not, see <http://www.gnu.org/licenses/>.
//

#include <Arduino.h>
#include <ArduinoLog.h>
#include "il2p_rs.h"

/* Reed-Solomon codec control block */
struct RS {
  unsigned int mm;              /* Bits per symbol */
  unsigned int nn;              /* Symbols per block (= (1<<mm)-1) */
  unsigned char *alpha_to;      /* log lookup table */
  unsigned char *index_of;      /* Antilog lookup table */
  unsigned char *genpoly;       /* Generator polynomial */
  unsigned int nroots;     /* Number of generator roots = number of parity symbols */
  unsigned char fcr;        /* First consecutive root, index form */
  unsigned char prim;       /* Primitive element, index form */
  unsigned char iprim;      /* prim-th root of 1, index form */
};


#define DTYPE unsigned char
#define MAX_NROOTS 16
#define NTAB 5

#define MM (rs->mm)
#define NN (rs->nn)
#define ALPHA_TO (rs->alpha_to) 
#define INDEX_OF (rs->index_of)
#define GENPOLY (rs->genpoly)
#define NROOTS (rs->nroots)
#define FCR (rs->fcr)
#define PRIM (rs->prim)
#define IPRIM (rs->iprim)
#define A0 (NN)


int modnn(RS* rs, int x){
  while (x >= rs->nn) {
    x -= rs->nn;
    x = (x >> rs->mm) + (x & rs->nn);
  }
  return x;
}

struct Tab
{
	int symsize;          // Symbol size, bits (1-8).  Always 8 for this application.
	int genpoly;          // Field generator polynomial coefficients.
	int fcs;              // First root of RS code generator polynomial, index form.
				// FX.25 uses 1 but IL2P uses 0.
	int prim;             // Primitive element to generate polynomial roots.
	int nroots;           // RS code generator polynomial degree (number of roots).
                        // Same as number of check bytes added.
	RS *rs;        // Pointer to RS codec control block.  Filled in at init time.
};

Tab tab[NTAB]= {{8, 0x11d,   0,   1, 2, nullptr },  // 2 parity
				{8, 0x11d,   0,   1, 4, nullptr },  // 4 parity
				{8, 0x11d,   0,   1, 6, nullptr },  // 6 parity
				{8, 0x11d,   0,   1, 8, nullptr },  // 8 parity
				{8, 0x11d,   0,   1, 16, nullptr }};  // 16 parity

void encode_rs_char(RS *rs, DTYPE *data, DTYPE *bb);
int decode_rs_char(RS * rs, DTYPE * data, int *eras_pos, int no_eras);

/* Initialize a Reed-Solomon codec
 *   symsize = symbol size, bits (1-8) - always 8 for this application.
 *   gfpoly = Field generator polynomial coefficients
 *   fcr = first root of RS code generator polynomial, index form
 *   prim = primitive element to generate polynomial roots
 *   nroots = RS code generator polynomial degree (number of roots)
 */

RS* init_rs_char(unsigned int symsize, unsigned int gfpoly, unsigned int fcr, unsigned int prim, unsigned int nroots)
{
    RS* rs;
    int i, j, sr,root,iprim;

    if(symsize > 8*sizeof(DTYPE))
        return nullptr; /* Need version with ints rather than chars */

    if(fcr >= (1<<symsize))
        return nullptr;
    if(prim == 0 || prim >= (1<<symsize))
        return nullptr;
    if(nroots >= (1<<symsize))
        return nullptr; /* Can't have more roots than symbol values! */

    rs = (RS *)calloc(1,sizeof(RS));
    if (rs == nullptr) 
    {
        Log.error("FATAL ERROR: Out of memory.\r\n");
        exit (EXIT_FAILURE);
    }
    rs->mm = symsize;
    rs->nn = (1<<symsize)-1;

    rs->alpha_to = (DTYPE *)calloc((rs->nn+1),sizeof(DTYPE));
    if(rs->alpha_to == nullptr)
    {
        Log.error("FATAL ERROR: Out of memory.\r\n");
        exit (EXIT_FAILURE);
    }
    
    rs->index_of = (DTYPE *)calloc((rs->nn+1),sizeof(DTYPE));
    if(rs->index_of == nullptr)
    {
        Log.error("FATAL ERROR: Out of memory.\r\n");
        exit (EXIT_FAILURE);
    }

    /* Generate Galois field lookup tables */
    rs->index_of[0] = A0; /* log(zero) = -inf */
    rs->alpha_to[A0] = 0; /* alpha**-inf = 0 */
    sr = 1;

    for(i=0;i<rs->nn;i++)
    {
        rs->index_of[sr] = i;
        rs->alpha_to[i] = sr;
        sr <<= 1;
        if(sr & (1<<symsize))
        sr ^= gfpoly;
        sr &= rs->nn;
    }
    
    if(sr != 1)
    {
        /* field generator polynomial is not primitive! */
        free(rs->alpha_to);
        free(rs->index_of);
        free(rs);
        return nullptr;
    }

    /* Form RS code generator polynomial from its roots */
    rs->genpoly = (DTYPE *)calloc((nroots+1),sizeof(DTYPE));
    if(rs->genpoly == nullptr)
    {
        Log.error("FATAL ERROR: Out of memory.\r\n");
        delay(3000);  //trigger the watchdog
    }
    rs->fcr = fcr;
    rs->prim = prim;
    rs->nroots = nroots;

//!!!!!!!!!!!!!
    /* Find prim-th root of 1, used in decoding */
    for(iprim=1;(iprim % prim) != 0;iprim += rs->nn)
        ;
    rs->iprim = iprim / prim;

    rs->genpoly[0] = 1;
    for (i = 0,root=fcr*prim; i < nroots; i++,root += prim) 
    {
        rs->genpoly[i+1] = 1;
        /* Multiply rs->genpoly[] by  @**(root + x) */
        for (j = i; j > 0; j--)
        {
            if (rs->genpoly[j] != 0)
            rs->genpoly[j] = rs->genpoly[j-1] ^ rs->alpha_to[modnn(rs,rs->index_of[rs->genpoly[j]] + root)];
            else
            rs->genpoly[j] = rs->genpoly[j-1];
        }
        /* rs->genpoly[0] can never be zero */
        rs->genpoly[0] = rs->alpha_to[modnn(rs,rs->index_of[rs->genpoly[0]] + root)];
    }
    /* convert rs->genpoly[] to index form for quicker encoding */
    for (i = 0; i <= nroots; i++) 
    {
        rs->genpoly[i] = rs->index_of[rs->genpoly[i]];
    }
  
    return rs;
}


/*-------------------------------------------------------------
 *
 * Name:	il2p_init
 *
 * Purpose:	This must be called at application start up time.
 *		It sets up tables for the Reed-Solomon functions.
 *
 * Inputs:	debug	- Enable debug output.
 *
 *--------------------------------------------------------------*/

void il2p_init()
{

	for (int i = 0 ; i < NTAB ; i++) 
	{
		tab[i].rs = init_rs_char(tab[i].symsize, tab[i].genpoly, tab[i].fcs,  tab[i].prim, tab[i].nroots);
	}

} // end il2p_init


// Find RS codec control block for specified number of parity symbols.

RS* il2p_find_rs(int nparity)
{
	for (int n = 0; n < NTAB; n++) 
	{
	    if (tab[n].nroots == nparity) 
		{
	        Log.verbose("found tab: %i\r\n", n);
            return (tab[n].rs);
	    }
	}
	Log.error("IL2P INTERNAL ERROR: il2p_find_rs: control block not found for nparity = %d.\r\n", nparity);
	return (tab[0].rs);
}


/*-------------------------------------------------------------
 *
 * Name:	void il2p_encode_rs
 *
 * Purpose:	Add parity symbols to a block of data.
 *
 * Inputs:	tx_data		Header or other data to transmit.
 *		data_size	Number of data bytes in above.
 *		num_parity	Number of parity symbols to add.
 *				Maximum of IL2P_MAX_PARITY_SYMBOLS.
 *
 * Outputs:	parity_out	Specified number of parity symbols
 *
 * Restriction:	data_size + num_parity <= 255 which is the RS block size.
 *		The caller must ensure this.
 *
 *--------------------------------------------------------------*/

void il2p_encode_rs (unsigned char *tx_data, int data_size, int num_parity, unsigned char *parity_out)
{
	unsigned char rs_block[255];
	memset (rs_block, 0, sizeof(rs_block));
	memcpy (rs_block + sizeof(rs_block) - data_size - num_parity, tx_data, data_size);
	encode_rs_char(il2p_find_rs(num_parity), rs_block, parity_out);
}

/*-------------------------------------------------------------
 *
 * Name:	void il2p_decode_rs
 *
 * Purpose:	Check and attempt to fix block with FEC.
 *
 * Inputs:	rec_block	Received block composed of data and parity.
 *				Total size is sum of following two parameters.
 *		data_size	Number of data bytes in above.
 *		num_parity	Number of parity symbols (bytes) in above.
 *
 * Outputs:	out		Original with possible corrections applied.
 *				data_size bytes.
 *
 * Returns:	-1 for unrecoverable.
 *		>= 0 for success.  Number of symbols corrected.
 *
 *--------------------------------------------------------------*/

int il2p_decode_rs (unsigned char *rec_block, int data_size, int num_parity, unsigned char *out)
{
	//  Use zero padding in front if data size is too small.

	int n = data_size + num_parity;		// total size in.

	unsigned char rs_block[255];

	// We could probably do this more efficiently by skipping the
	// processing of the bytes known to be zero.  Good enough for now.

	memset (rs_block, 0, sizeof(rs_block) - n);
	memcpy (rs_block + sizeof(rs_block) - n, rec_block, n);

	Log.verbose("==============================  il2p_decode_rs  ==============================\r\n");

	int derrlocs[64];	// Half would probably be OK.

	int derrors = decode_rs_char(il2p_find_rs(num_parity), rs_block, derrlocs, 0);
	memcpy (out, rs_block + sizeof(rs_block) - n, data_size);

	// It is possible to have a situation where too many errors are
	// present but the algorithm could get a good code block by "fixing"
	// one of the padding bytes that should be 0.

	for (int i = 0; i < derrors; i++) 
	{
	    if (derrlocs[i] < sizeof(rs_block) - n) 
		{
	        derrors = -1;
	        break;
	    }
	}

	Log.verbose("==============================  il2p_decode_rs  returns %d  ==============================\r\n", derrors);
	
	return (derrors);
}


void encode_rs_char(RS *rs, DTYPE *data, DTYPE *bb)
{
    int i, j;
    DTYPE feedback;
    Log.verbose("starting encoder \r\n");
    memset(bb,0,NROOTS*sizeof(DTYPE)); // clear out the FEC data area
    Log.verbose("bb = %X\r\n", *bb);
    Log.verbose("NROOTS = %X\r\n", rs->nroots);
    Log.verbose("NN = %X\r\n", rs->nn);
    for(i=0;i<NN-NROOTS;i++){
        feedback = INDEX_OF[data[i] ^ bb[0]];
        if(feedback != A0){      /* feedback term is non-zero */
        for(j=1;j<NROOTS;j++)
            bb[j] ^= ALPHA_TO[modnn(rs,feedback + GENPOLY[NROOTS-j])];
        }
        /* Shift */
        memmove(&bb[0],&bb[1],sizeof(DTYPE)*(NROOTS-1));
        if(feedback != A0)
        bb[NROOTS-1] = ALPHA_TO[modnn(rs,feedback + GENPOLY[0])];
        else
        bb[NROOTS-1] = 0;
    }
    Log.verbose("encoding complete \r\n");
}

//-----------------------------------------------------------------------
// Revision History
//-----------------------------------------------------------------------
// 0.0.1  - initial release
//          Modifications from Phil Karn's GPL source code.
//          Initially added code to run with PC file 
//          I/O and use the (255,239) decoder exclusively.  Confirmed that the
//          code produces the correct results.
//  
//-----------------------------------------------------------------------
// 0.0.2  - 

#define	min(a,b)	((a) < (b) ? (a) : (b))

int decode_rs_char(RS * rs, DTYPE * data, int *eras_pos, int no_eras) 
{
    int deg_lambda, el, deg_omega;
    int i, j, r,k;
    DTYPE u,q,tmp,num1,num2,den,discr_r;
    //  DTYPE lambda[NROOTS+1], s[NROOTS];	/* Err+Eras Locator poly and syndrome poly */
    //  DTYPE b[NROOTS+1], t[NROOTS+1], omega[NROOTS+1];
    //  DTYPE root[NROOTS], reg[NROOTS+1], loc[NROOTS];
    DTYPE lambda[64+1], s[64];	/* Err+Eras Locator poly and syndrome poly */
    DTYPE b[64+1], t[64+1], omega[64+1];
    DTYPE root[64], reg[64+1], loc[64];
    int syn_error, count;

    /* form the syndromes; i.e., evaluate data(x) at roots of g(x) */
    for(i=0;i<NROOTS;i++)
        s[i] = data[0];

    for(j=1;j<NN;j++)
    {
        for(i=0;i<NROOTS;i++)
        {
            if(s[i] == 0)
            {
                s[i] = data[j];
            } 
            else 
            {
                s[i] = data[j] ^ ALPHA_TO[modnn(rs,INDEX_OF[s[i]] + (FCR+i)*PRIM)];
            }
        }
    }

    /* Convert syndromes to index form, checking for nonzero condition */
    syn_error = 0;
    for(i=0;i<NROOTS;i++)
    {
        syn_error |= s[i];
        s[i] = INDEX_OF[s[i]];
    }

    if (!syn_error) 
    {
        /* if syndrome is zero, data[] is a codeword and there are no
        * errors to correct. So return data[] unmodified
        */
        count = 0;
        goto finish;
    }

    memset(&lambda[1],0,(NROOTS*sizeof(lambda[0])));
    lambda[0] = 1;

    if (no_eras > 0) 
    {
        /* Init lambda to be the erasure locator polynomial */
        lambda[1] = ALPHA_TO[modnn(rs,PRIM*(NN-1-eras_pos[0]))];
        for (i = 1; i < no_eras; i++) 
        {
            u = modnn(rs,PRIM*(NN-1-eras_pos[i]));
            for (j = i+1; j > 0; j--) 
            {
                tmp = INDEX_OF[lambda[j - 1]];
                if(tmp != A0)
                lambda[j] ^= ALPHA_TO[modnn(rs,u + tmp)];
            }
        }
    }
    for(i=0;i<NROOTS+1;i++)
        b[i] = INDEX_OF[lambda[i]];
  
    /*
    * Begin Berlekamp-Massey algorithm to determine error+erasure
    * locator polynomial
    */
    r = no_eras;
    el = no_eras;
    while (++r <= NROOTS) 
    {	/* r is the step number */
        /* Compute discrepancy at the r-th step in poly-form */
        discr_r = 0;
        for (i = 0; i < r; i++)
        {
            if ((lambda[i] != 0) && (s[r-i-1] != A0)) 
            {
                discr_r ^= ALPHA_TO[modnn(rs,INDEX_OF[lambda[i]] + s[r-i-1])];
            }
        }
        discr_r = INDEX_OF[discr_r];	/* Index form */
        if (discr_r == A0) 
        {
            /* 2 lines below: B(x) <-- x*B(x) */
            memmove(&b[1],b,NROOTS*sizeof(b[0]));
            b[0] = A0;
        } 
        else 
        {
            /* 7 lines below: T(x) <-- lambda(x) - discr_r*x*b(x) */
            t[0] = lambda[0];
            for (i = 0 ; i < NROOTS; i++) 
            {
                if(b[i] != A0)
                t[i+1] = lambda[i+1] ^ ALPHA_TO[modnn(rs,discr_r + b[i])];
                else
                t[i+1] = lambda[i+1];
            }
            if (2 * el <= r + no_eras - 1) 
            {
                el = r + no_eras - el;
                /*
                * 2 lines below: B(x) <-- inv(discr_r) *
                * lambda(x)
                */
                for (i = 0; i <= NROOTS; i++)
                b[i] = (lambda[i] == 0) ? A0 : modnn(rs,INDEX_OF[lambda[i]] - discr_r + NN);
            } 
            else 
            {
                /* 2 lines below: B(x) <-- x*B(x) */
                memmove(&b[1],b,NROOTS*sizeof(b[0]));
                b[0] = A0;
            }
            memcpy(lambda,t,(NROOTS+1)*sizeof(t[0]));
        }
    }

    /* Convert lambda to index form and compute deg(lambda(x)) */
    deg_lambda = 0;
    for(i=0;i<NROOTS+1;i++)
    {
        lambda[i] = INDEX_OF[lambda[i]];
        if(lambda[i] != A0)
        deg_lambda = i;
    }
    /* Find roots of the error+erasure locator polynomial by Chien search */
    memcpy(&reg[1],&lambda[1],NROOTS*sizeof(reg[0]));
    count = 0;		/* Number of roots of lambda(x) */
    for (i = 1,k=IPRIM-1; i <= NN; i++,k = modnn(rs,k+IPRIM)) 
    {
        q = 1; /* lambda[0] is always 0 */
        for (j = deg_lambda; j > 0; j--){
        if (reg[j] != A0) 
            {
                reg[j] = modnn(rs,reg[j] + j);
                q ^= ALPHA_TO[reg[j]];
            }
        }
        if (q != 0)
        continue; /* Not a root */
        /* store root (index-form) and error location number */
        root[count] = i;
        loc[count] = k;
        /* If we've already found max possible roots,
        * abort the search to save time
        */
        if(++count == deg_lambda)
        break;
    }

    if (deg_lambda != count) 
    {
        /*
        * deg(lambda) unequal to number of roots => uncorrectable
        * error detected
        */
        count = -1;
        goto finish;
    }
    /*
    * Compute err+eras evaluator poly omega(x) = s(x)*lambda(x) (modulo
    * x**NROOTS). in index form. Also find deg(omega).
    */
    deg_omega = 0;
    for (i = 0; i < NROOTS;i++)
    {
        tmp = 0;
        j = (deg_lambda < i) ? deg_lambda : i;
        for(;j >= 0; j--)
        {
            if ((s[i - j] != A0) && (lambda[j] != A0))
            tmp ^= ALPHA_TO[modnn(rs,s[i - j] + lambda[j])];
        }
        if(tmp != 0)
        deg_omega = i;
        omega[i] = INDEX_OF[tmp];
    }
    omega[NROOTS] = A0;
    
    /*
    * Compute error values in poly-form. num1 = omega(inv(X(l))), num2 =
    * inv(X(l))**(FCR-1) and den = lambda_pr(inv(X(l))) all in poly-form
    */
    for (j = count-1; j >=0; j--) 
    {
        num1 = 0;
        for (i = deg_omega; i >= 0; i--) 
        {
            if (omega[i] != A0)
            num1  ^= ALPHA_TO[modnn(rs,omega[i] + i * root[j])];
        }
        num2 = ALPHA_TO[modnn(rs,root[j] * (FCR - 1) + NN)];
        den = 0;
        
        // lambda[i+1] for i even is the formal derivative lambda_pr of lambda[i]
        for (i = min(deg_lambda,NROOTS-1) & ~1; i >= 0; i -=2) 
        {
            if(lambda[i+1] != A0)
            den ^= ALPHA_TO[modnn(rs,lambda[i+1] + i * root[j])];
        }
        if (den == 0) 
        {
            count = -1;
            goto finish;
        }
        // Apply error to data 
        if (num1 != 0) 
        {
            data[loc[j]] ^= ALPHA_TO[modnn(rs,INDEX_OF[num1] + INDEX_OF[num2] + NN - INDEX_OF[den])];
        }
    }
    finish:
    if(eras_pos != nullptr)
    {
        for(i=0;i<count;i++)
        eras_pos[i] = loc[i];
    }
    return count;
}

// end il2p_rs.c