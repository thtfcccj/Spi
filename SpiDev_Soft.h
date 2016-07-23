/******************************************************************************

                   SPI�豸����-���IOʵ�ֽṹ���ӿ�
//��֧������ģʽ
//֧��λ���ȿ��ƹ���,�������8bit�����SPI�豸ͨѶ
//�����û����������������Ӳ��IO�����������ص�����,��ǿ�ҽ��鲻�޸Ķ�ֱ��ʵ�ֺ�
//ע:�˽ӿ���SpiDev�Ľӿ��޹�,������SpiDev�����IOʵ��
******************************************************************************/

#ifndef __SPI_DEV_SOFT_H
#define __SPI_DEV_SOFT_H

/******************************************************************************
                             ��ؽṹ
******************************************************************************/

//---------------------------Spi�豸����-------------------------------------
//Spi�豸��ʾһ������SpiӲ���ӿڣ�һ��SpiӲ���ӿ���������Spi�豸(��Ƭѡ����),
struct _SpiDev_Soft{
  volatile unsigned char Cfg;        //��ر�־,������
  unsigned char LastBitLen;          //λ���ȿ��ƹ���:����ֽ�λ����
  unsigned char BitDelay;            //λ���ƽ����ʱ��,������ϵͳʱ��������ù�ϵ
  unsigned char ByteDelay;           //�ֽڼ��ƽ����ʱ��,������ϵͳʱ��������ù�ϵ
};

//����,��ر�־����Ϊ:
#define SPI_DEV_SOFT_CPOL_H     0x80     //����ʱSCKΪ�ߵ�ƽ��־,����Ϊ�͵�ƽ
#define SPI_DEV_SOFT_CPHA_END   0x40     //SCK�ڽ����ز���,��ʼ�������������෴
#define SPI_DEV_SOFT_DORD_LSB   0x20     //�ֽڷ���˳��:��λLSB�ȷ�,����֮

/******************************************************************************
                     ����û��ײ���ƺ���
******************************************************************************/

//-------------------------------��ʼ������-------------------------------
//void SpiDev_Soft_Init(struct _SpiDev_Soft *pDev);
//����ֱ�Ӷ���Ĭ��״̬
#define SpiDev_Soft_Init(pdev) \
  do{(pdev)->Cfg = SPI_DEV_SOFT_CPOL_H | SPI_DEV_SOFT_CPHA_END; \
    (pdev)->LastBitLen = 8; (pdev)->BitDelay = 10; (pdev)->ByteDelay = 50;}while(0)

//----------------------Spi���豸������λ����------------------------------
//void SpiDev_Soft_CfgSet(struct _SpiDev_Soft *pDev,
//                        unsigned char Cfg);
#define SpiDev_Soft_CfgSet(pdev, cfg) do{(pdev)->Cfg |= cfg;}while(0)

//----------------------Spi���豸�����������------------------------------
//void SpiDev_Soft_CfgClr(struct _SpiDev_Soft *pDev,
//                        unsigned char Cfg);
#define SpiDev_Soft_CfgClr(pdev, cfg) do{(pdev)->Cfg &= ~(cfg);}while(0)

//----------------------Spi���豸����ʱ����------------------------------
//void SpiDev_Soft_SetDelay(struct _SpiDev_Soft *pDev,
//                          unsigned short Delay);
#define SpiDev_Soft_SetDelay(pdev, delay) do{(pdev)->Dealy = delay;}while(0)

//----------------------Spi���豸������ֽ�λ���Ⱥ���------------------
//void SpiDev_Soft_SetLastBitLen(struct _SpiDev_Soft *pDev,
//                                  unsigned char LastBitLen);
#define SpiDev_Soft_SetLastBitLen(pdev, cmdlastbitlen) \
  do{(pdev)->LastBitLen = cmdlastbitlen;}while(0)

/******************************************************************************
                             Ӳ��IO���ƻص�����
//ע:����Ƭѡ����
******************************************************************************/
//ֱ��ʵ��:
#include "IOCtrl.h"

//--------------------------IO��ʼ��--------------------------------
//void SpiDev_Soft_cbIoInit(struct _SpiDev_Soft *pDev);
#define SpiDev_Soft_cbIoInit(pdev) do{Spi2IoCfg();}while(0)

//--------------------------��λʱ��--------------------------------
//void SpiDev_Soft_cbSetClk(struct _SpiDev_Soft *pDev);
#define SpiDev_Soft_cbSetClk(pdev) do{SetSPI2_SCK();}while(0)

//--------------------------���ʱ��--------------------------------
//void SpiDev_Soft_cbClrClk(struct _SpiDev_Soft *pDev);
#define SpiDev_Soft_cbClrClk(pdev) do{ClrSPI2_SCK();}while(0)

//--------------------------��λMOSI--------------------------------
//void SpiDev_Soft_cbSetMosi(struct _SpiDev_Soft *pDev);
#define SpiDev_Soft_cbSetMosi(pdev) do{SetSPI2_MOSI();}while(0)

//--------------------------���MOSI--------------------------------
//void SpiDev_Soft_cbClrMosi(struct _SpiDev_Soft *pDev);
#define SpiDev_Soft_cbClrMosi(pdev) do{ClrSPI2_MOSI();}while(0)

//--------------------------�ж�MISO��ƽ-----------------------------
//signed char SpiDev_Soft_cbIsSetMiso(struct _SpiDev_Soft *pDev);
#define SpiDev_Soft_cbIsSetMiso(pdev)   (IsSPI2_MISO())


/******************************************************************************
                             �����ص�����
******************************************************************************/

//------------------------��ʱ��������-------------------------------
//��ʱʱ�������õ�ʱ���Ӧ
//void SpiDev_Soft_cbDelay(unsigned short Delay);
#include "Delay.h"
#define SpiDev_Soft_cbDelay(delay) DelayUs(delay)

//------------------------�����ٽ���-------------------------------
//void SpiDev_Soft_cbEnterCritical(void);
#define SpiDev_Soft_cbEnterCritical() do{}while(0)

//------------------------�˳��ٽ���-------------------------------
//void SpiDev_Soft_cbExitCritical(void);
#define SpiDev_Soft_cbExitCritical() do{}while(0)


#endif // #define __SPI_DEV_SOFT_H

