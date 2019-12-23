#ifndef _ENCRYPT_H_
#define _ENCRYPT_H_


#include "wrand.h"
#include "CRC.h"
class  Encrypt
{
public:
	Encrypt()
	{
		m_nSelfSalt = 0;
		m_nTargetSalt = 0;
		m_nKey = 0;
		m_nSelfSalt = 0;

	}
	void SetSelfSalt(int salt); //�����Լ��������
	void SetTargetSalt(int salt);  //����ͨ�ŷ��������,ͬʱ������key

	int GetSelfSalt()
	{
		return m_nSelfSalt;   //��ȡ�����������
	}

	bool Encode(char* pInBuff, size_t len, char* pOutBuff); //��������

	bool Decode(char* pInBuff, size_t len, char* pOutBuff); //��������

	inline bool IsKeyCrcCorrect(unsigned short nKeyCRC)  //
	{
		return (GetKeyCRC() == nKeyCRC);
	}

	inline static unsigned short CRC16(const char* lpBuffer, size_t len)  //����CRC-16��Checksum
	{
		return (unsigned short)CRC16Calc((unsigned char*)lpBuffer, len);
	}
	inline int GetKey()
	{
		return m_nKey;
	}
	unsigned int GetKeyCRC() //��ȡkey��crcֵ
	{
		return (CRC16((const char*)m_sKeybuff, 4));
	}
private:
	void  GenKey();      //������Կ

	int   GenSalt(); // ����salt
private:
	int m_nSelfSalt;    //�Լ������������
	int m_nTargetSalt;  //ͨ�ŷ��������
	int m_nKey;         //��Կ
	unsigned char m_sKeybuff[4] ;  //��Կ��buff���������Ϊ�˷������
};


#endif
