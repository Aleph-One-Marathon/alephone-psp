#ifndef _RANDOM_
#define _RANDOM_
/*
	Random-number generator
	by Loren Petrich,
	May 14, 2000

	It uses random-number generators created by Dr. George Marsaglia,
	and communicated to me by Joe McMahon (mcmahon@metalab.unc.edu).
	I've rewritten the code communicated to me into a C++ class,
	so that separate instances can be more easily handled.
	
	This is from code that is presumably GNU-compatible.
*/


struct GM_Random
{
	uint32 z, w, jsr, jcong, t[256], x, y;
	unsigned char c;
	
	uint32 znew() {return (z=36969*(z&65535)+(z>>16));}
	uint32 wnew() {return (w=18000*(w&65535)+(w>>16));}
	uint32 MWC() {return ((znew()<<16)+wnew());}
	uint32 SHR3() {return (jsr=(jsr=(jsr=jsr^(jsr<<17))^(jsr>>13))^(jsr<<5));}
	uint32 CONG() {return (jcong=69069*jcong+1234567);}
	uint32 KISS() {return ((MWC()^CONG())+SHR3());}
	uint32 LFIB4() {t[c]=t[c]+t[c+58]+t[c+119]+t[c+178]; return t[++c];}
	uint32 SWB() {t[c+237]=(x=t[c+15])-(y=t[c]+(x<y)); return t[++c];}
	float UNI() {return (float)(KISS()*2.328306e-10);}
	float VNI() {return (float)(((int32) KISS())*4.656613e-10);}

	/*
		Random seeds must be used to reset z, w, jsr, jcong and
		the table t[256].  Here is an example procedure, using KISS:
	*/
	void SetTable() {for (int i=0;i<256;i++) t[i]=KISS();}

	/*
		Any one of KISS, MWC, LFIB4, SWB, SHR3, or CONG  can be used in
		an expression to provide a random 32-bit integer, while UNI
		provides a real in (0,1) and VNI a real in (-1,1).
	*/
	
	GM_Random(): z(362436069), w(521288629), jsr(123456789), jcong(380116160),
		x(0), y(0), c(0) {SetTable();}
};

#endif
