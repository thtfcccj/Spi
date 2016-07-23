/******************************************************************************

      ͨ��SPI�����豸��������ӿ�->��Spi�豸ʵ��+��Spi�ӻ�ʱ��ʵ���ӿ�
//�˽ӿ�ֱ��ʵ����SpiDev��Ƭѡ(SpiCs����)�Ͳ����ʵ�,�����ڼ�SpiͨѶ

//�˽ӿڲ���ΪӦ����أ����������Ӧ�ã��Դ˽ӿڽ��������������޸�
//�˽ӿڿ�Ϊģ��,������Spi�豸ʵ��,����ж��Spi�ӻ�ʱ,��ʹ�ô�ģ���������
******************************************************************************/
#ifndef __SPI_CS1_H
#define __SPI_CS1_H

/******************************************************************************
                             �������
******************************************************************************/

#define SPI_DATA_SPACE             16       //�������ݼ�ļ����ӻ���������

#define SPI_CMD_BUF_SIZE           1        //�������������С
#define SPI_DATA_BUF_SIZE          8        //�������ݻ�������С

/******************************************************************************
                             ��ؽṹ
******************************************************************************/
#include"IOCtrl.h"    //��ʵ�ֵĽӿڼ��� ��ػص�������˵��
#include"SpiDev.h"    //Spi�豸����
struct _Spi{
  //SPI�������
  struct _SpiDevCmd Cmd;  //SPI�豸������
  unsigned char CmdBuf[SPI_CMD_BUF_SIZE];   //�豸�����ֵ��������
  unsigned char DataBuf[SPI_DATA_BUF_SIZE];  //���ڶ�д�����������ڲ�ʹ��
  unsigned char Timer;       //�ֽڼ���ʱ��ʱ��
};

extern struct _SpiDev SpiDev;   //ʵ���豸
extern struct _Spi Spi;

/******************************************************************************
                             ��غ���
******************************************************************************/

//---------------------------��ʼ������---------------------------
void Spi_Init (void);

//------------------------------����������----------------------
//����ϵͳ���������в�ѯ
void Spi_FastTask(void);

//---------------------------���SPI�Ƿ������----------------------
//unsigned char Spi_IsIdie(void);
//ֱ�Ӽ������Ƿ�SPI CS��Ϊ�ߵ�ƽ
#define Spi_IsIdie()  (IsOutSpiCs() && IsSetSpiCs())

//-------------------------�ж�Cs�Ƿ�Ϊ�ߵ�ƽ----------------------
//unsigned char Spi_IsCs(void);
#define Spi_IsCs()   IsSetSpiCs()

/******************************************************************************
                             ��ػص�����
******************************************************************************/
//IOCtrl.h�еı�׼IO�ӿ�,��������ʵ��Ƭѡ�ĺ꺯����:
//����(Ϊ���ӻ�����ʱ�Ķ���)
//#define InSpiCs()			do{DDR_SpiCs &= ~BIT_SpiCs;}while(0)
//#define OutSpiCs()		do{DDR_SpiCs |= BIT_SpiCs;}while(0)
//#define IsOutSpiCs()		(DDR_SpiCs & BIT_SpiCs)
//���ʱ,�ߵ͵�ƽ
//#define SetSpiCs()		do{PORT_SpiCs |= BIT_SpiCs;}while(0)
//#define ClrSpiCs()		do{PORT_SpiCs &= ~BIT_SpiCs;}while(0)
//#define IsSetSpiCs()		(PORT_SpiCs & BIT_SpiCs)
//����ʱ,�Ƿ�Ϊ�ߵ�ƽ
//#define IsSpiCs()		        (PIN_SpiCs & BIT_SpiCs)
//����ʱ,�Ͻӵ���(���ӻ��ɻ���ʱ����)
//#define SetPullUpSpiCs()		do{PORT_SpiCs |= BIT_SpiCs;}while(0)
//#define ClrPullUpSpiCs()		do{PORT_SpiCs &= ~BIT_SpiCs;}while(0)
//#define IsPullUpSpiCs()	    (PORT_SpiCs & BIT_SpiCs)

//��ʼ��������Ϊ:,����ߵ�ƽ״̬(���ӻ��ɻ���ʱ,����ʱӦ��ǰ������״̬)
//#define CfgSpiCs() do{SetSpiCs();OutSpiCs();}while(0)


#endif //#define __SPI_H

