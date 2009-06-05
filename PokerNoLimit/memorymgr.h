#ifndef _memorymgr_h_
#define _memorymgr_h_

#include "../PokerLibrary/constants.h"

//algorithm strategy data for N-membered nodes
//used once per cardsi per N-membered node.
template <int N> struct dataN_t
{
	dataN_t() //initialize all to zero
	{
		for(int a=0; a<N-1; a++)
			regret[a] = stratn[a] = 0;
		regret[N-1] = stratd = 0;
	}
	fp_type regret[N];
	fp_type stratn[N-1];
	fp_type stratd;
};

//resulting strategy data for N-membered nodes
//used once per cardsi per N-membered node.
//probability 0 is mapped to 0
//probability 1 is mapped to 256
//then is rounded down, to yeild number 0-255, with all number used equally.
template <int N> struct stratN_t
{

	//this constructor is used to convert from algorithm data types (above) to strategy 
	//probabilities, packed as 8-bit chars. we perform the safe division here.

	stratN_t(const dataN_t<N> &data)
	{
		for(int a=0; a<N-1; a++)
		{
			//if we just didn't get there, then the numerator would also be zero
			if (data.stratd==0 && data.stratn[a]!=0) 
				REPORT("zero denominator!");
			//could have just never gotten there, will happen for short iteration runs
			else if(data.stratd==0 && data.stratn[a]==0) 
				strat[a] = 0; //would evaluate to infinity otherwise
			else
			{
				int temp = int(256.0 * data.stratn[a] / data.stratd);
				if(temp == 256) temp = 255; //will happen if ratio is exactly one
				if(temp < 0 || temp > 255)
					REPORT("failure to divide");
				strat[a] = (unsigned char)temp;
			}
		}
	}

	//the actual data.

	unsigned char strat[N-1];
};

//each is a pointer to a large array allocated on the heap
//one such array per gameround per type of node.
//used as 2-dimensional arrays below.
//alocated individually in initmem(). 
//i supposed it is a ragged array
extern dataN_t<9> * data9[4];
extern dataN_t<8> * data8[4];
extern dataN_t<7> * data7[4];
extern dataN_t<6> * data6[4];
extern dataN_t<5> * data5[4];
extern dataN_t<4> * data4[4];
extern dataN_t<3> * data3[4];
extern dataN_t<2> * data2[4];

//used to obtain the correct pointers to the data from the above arrays, 
//so that other code doesn't need to worry about it.
void dataindexing(int gr, int nacts, int actioni, int cardsi, fp_type* &stratn, fp_type* &stratd, fp_type* &regret);
void initmem();
void closemem();
void savestratresult(const char * const filename);

#endif
