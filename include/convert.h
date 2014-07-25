#ifndef __FUNC_H__
#define __FUNC_H__

#include "stdio.h"

#define MINV(a, b)	(((a) < (b)) ? (a) : (b))
#define DBUF 4096
#define READ_BYTES 512

#ifndef _WIN32
	#define SHRT_MAX 32767
	#define SHRT_MIN (-SHRT_MAX-1)
	#define USHRT_MAX 0xffff
#endif

int Char2Float (float Dbuff[], char Buf[], int Nval );
int Float2Short (short Buf[], const float Dbuff[], int Nval );

#endif