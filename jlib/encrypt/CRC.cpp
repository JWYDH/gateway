/************************************************************
*	 crc16��checksum����
*    ���g_ulTableû�г�ʼ������һ��ʹ�õ�ʱ�򽫳�ʼ��
***********************************************************/
#include <stddef.h>

#define CRC16_POLYNOMIAL 0x1021 // CRC_16У�鷽ʽ�Ķ���ʽ.

typedef unsigned char uchar;
typedef unsigned int uint;
typedef unsigned long ulong;
static ulong  g_ulTable[256];

static bool has_inited = false;

// CRC_16��ʽУ��ĳ�ʼ������, ����CRC_16������.
void CRC16Init(void)
{
	if (has_inited) return;

	uint   nRemainder;
	int    n, m;
	ulong*   pulTable = g_ulTable;

	for (n = 0; n < 256; n ++)
	{
		nRemainder = (uint)n << 8;

		for (m = 8; m > 0; m --)
		{
			if (nRemainder & 0x8000)
			{
				nRemainder = (nRemainder << 1) ^ CRC16_POLYNOMIAL;
			}
			else
			{
				nRemainder = (nRemainder << 1);
			}
		}

		*(pulTable + n) = nRemainder;
	}

	has_inited = true;
}



// ��ת���ݵı���λ, ��ת��MSBΪ1.
// ��תǰ: 1110100011101110 0010100111100000
// ��ת��: 1111001010001110 1110001011100000
unsigned int CRCBitReflect(ulong ulData, int nBits)
{
	ulong   ulResult = 0x00000000L;
	int    n;

	for (n = 0; n < nBits; n ++)
	{
		if (ulData & 0x00000001L)
		{
			ulResult |= (ulong)(1L << ((nBits - 1) - n));
		}

		ulData = (ulData >> 1);
	}

	return(ulResult);
}

// ��CRC_16��ʽ����һ�����ݿ��CRCֵ.
// pucData - ��У������ݿ�ָ��.
// nBytes - ���ݿ��С, ��λ���ֽ�.
// ����ֵ���޷��ŵĳ�����, ���е�16λ��Ч.
unsigned int CRC16Calc(unsigned char* pucData, size_t nBytes)
{
	uint   nRemainder, nRet;

	uchar   index;

	if (has_inited == false)
	{
		CRC16Init(); //���û�г�ʼ���ͳ�ʼ������
	}

	ulong*   pulTable = g_ulTable;
	nRemainder = 0x0000;

	for (size_t n = 0; n < nBytes; n ++)
	{
		index = (uchar)((uchar)CRCBitReflect(*(pucData + n), 8) ^ (nRemainder >> 8));
		nRemainder = (uint) * (pulTable + index) ^ (nRemainder << 8);
	}

	nRet = (uint)CRCBitReflect(nRemainder, 16) ^ 0x0000;
	return(nRet);
}

