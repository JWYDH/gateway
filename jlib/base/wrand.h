#ifndef _WELL_RNG_H_
#define _WELL_RNG_H_

/******************************************************************
 *
 *	$ ����WELL�㷨ʵ�ֵĿ������������ $
 *  
 *  - ��Ҫ���� - 
 *
 *	ʵ�ֿ��ٵĲ������޹��ɵ�������㷨
 *
 *****************************************************************/

/*
* Comments: ʹ��WELL�㷨����һ����������������������0��max���ڡ�
* Param unsigned long max: ���������
* @Return unsigned long:
*/
//#ifdef __cplusplus
//extern "C" {
//#endif

//��Χһ��[0,max)֮��������
unsigned long wrand(unsigned long max);
/*
* Comments: ��ʼ��������ӱ�
* Param unsigned int seed: ����ֵ�����ֵΪ0���Զ�ʹ��time()ֵ��Ϊ����
* �ú��������������е���һ�μ��ɡ�Ҳ���Բ����ã�ֻ��ÿ�����е�N�����������ֵ��һ����
*/
void winitseed(unsigned int seed);

//����һ�������
unsigned long wrandvalue();

//#ifdef __cplusplus
//}
//#endif

#endif
