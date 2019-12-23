#ifndef	_CRC_H_
#define	_CRC_H_

// ��CRC_16��ʽ����һ�����ݿ��CRCֵ.
// pData - ��У������ݿ�ָ��.
// nBytes - ���ݿ��С, ��λ���ֽ�.
// ����ֵ���޷��ŵĳ�����, ���е�16λ��Ч.
#include <stddef.h>

unsigned int CRC16Calc(unsigned char *pData, size_t nBytes);

#endif

