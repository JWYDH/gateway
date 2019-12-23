#ifndef _ED_PASS_H_
#define _ED_PASS_H_
/**
*	���������ַ���
*	sDest			�������������
*	nDestSize		�������������(����ռ��㹻�������sDestĩβ������ֹ�ַ�)
*	sPassWord		�����ַ���
*	sKey128Bit		16�ֽڵ���Կ�ַ���
**/
const char* EncryptPassword(char* sDest, int nDestSize, const unsigned char* sPassWord, const unsigned char* sKey128Bit);

/**
*	���������ַ���
*	sDest			�������������
*	nDestSize		�������������(����ռ��㹻�������sDestĩβ������ֹ�ַ�)
*	sPassWord		���ܹ��������ַ���
*	sKey128Bit		16�ֽڵ���Կ�ַ���
**/
const char* DecryptPassword(char* sPassword, int nPasswordLen, const unsigned char* sEncrypted, const unsigned char* sKey128Bit);

#endif

