#include <time.h>

#if defined(_MSC_VER)
#include <windows.h>
#include <stdio.h>
#else
#include <sys/time.h>
#include <stdlib.h>
#endif


static unsigned long state[16] =
{
	0x8C12D566, 0x00C2793E, 0x5016EECF, 0x234869FA,
	0x6E4C8852, 0x32574386, 0x3573DC34, 0x7E4C00F0,
	0xC830516B, 0x085437E9, 0x32478EAC, 0xDEFCDCD0,
	0xFE09BBC2, 0x34EA087D, 0x112F367C, 0x0324EFAA
};

static unsigned long index = 0;

void winitseed(unsigned int seed)
{
	int i;

	if (!seed) seed = (unsigned int)time(NULL);

	for (i = 0; i < 16; ++i)
	{
		state[i] ^= seed;
	}
}


static unsigned long WELLRNG512()
{
	unsigned long a, b, c, d;
	a = state[index];
	c = state[(index + 13) & 15];
	b = a ^ c ^ (a << 16) ^ (c << 15);
	c = state[(index + 9) & 15];
	c ^= (c >> 11);
	a = state[index] = b ^ c;
	d = a ^ ((a << 5) & 0xDA442D20UL);
	index = (index + 15) & 15;
	a = state[index];
	state[index] = a ^ b ^ d ^ (a << 2) ^ (b << 18) ^ (c << 28);
	return state[index];
}


//返回一个随机数
unsigned long wrandvalue()
{
	//srand虽然比rand慢，但是每秒都可以运行几千万次
#if defined(_MSC_VER)
	srand(WELLRNG512());
	unsigned long hi16 = rand();
	unsigned long lo16 = rand();
	unsigned long n = hi16 << 16 | lo16;
#else
	struct timeval tv ;
	gettimeofday( &tv, NULL ) ;
	srand( tv.tv_usec ) ;
	unsigned long n = rand();
#endif
	
	/*
	if (sizeof(n) == 8)
	{
		n <<= 32;
		n |= rand();
	}*/
	return n;
	//return WELLRNG512();
}


unsigned long wrand(unsigned long max)
{
	//return WELLRNG512() % max;
	return wrandvalue() %max;
}
