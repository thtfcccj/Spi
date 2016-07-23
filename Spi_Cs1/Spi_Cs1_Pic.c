/******************************************************************************

   ͨ��SPI�����豸��������ӿ�->��Spi�豸ʵ��+��Spi�ӻ�ʱ��PIC�е�ʵ��ʵ��	

//��ʵ��ʹ�ò�ѯģʽ
//���ļ���Ϊģ��,�ɸ���Ӧ�ý����޸�
******************************************************************************/

#include "Spi_Cs1.h"
#include <pic.h>
#include "PicBit.h"
#include <string.h>  //NULL

struct _SpiDev SpiDev;   //ʵ���豸
struct _Spi Spi;

//---------------------SpiDevCmd���ú���ʵ��--------------------------
static void _cbCfg(void *pSpiDev, //struct _SpiDev *pSpiDev,   //�ҽ��豸
                   unsigned char CallPos)    //����λ��
{
  //PIC��Ƭѡʹ��SPIӲ��ģ���Զ�ʵ��,���ﲻ��Ҫ����
  switch(CallPos){
    case SPI_DEV_CALL_POS_CS_VALID: //��Ϊ����͵�ƽ״̬
	    //ClrSpiCs();
	    //OutSpiCs();
      Spi.Timer = SPI_DATA_SPACE;   //���ü��
	  //break;
    //case SPI_DEV_CALL_POS_CS_INVALID://��Ϊ��������״̬
	    //SetPullUpSpiCs();
      //InSpiCs();
	  //break;
    //case SPI_DEV_CALL_POS_CMD_CFG:
    //case SPI_DEV_CALL_POS_DATA_CFG:
    default: //�����뷢�����ݸ�ʽ��ͬ
    break;
  }
}

//------------------------------SpiDevCmd�ж���ɻص�����ʵ��-----------------
static void _cbEndInt(void *pSpiDev)
{
  
}

//------------------------SPI��ʼ������------------------------
void Spi_Init(void)
{
  CfgSpiCs();
  SpiDev_Init(&SpiDev, NULL);
  memset(&Spi, 0, sizeof(struct _Spi));
  
  //Cmd�ڲ���ʼ��
  //Spi.Cmd.Cs = 0;  
  Spi.Cmd.pCmd = Spi.CmdBuf;
  Spi.Cmd.pData = Spi.DataBuf;
  Spi.Cmd.cbCfg = _cbCfg;
  Spi.Cmd.cbEndInt = _cbEndInt;
  
  //SetSpiCs();    //ȡ��Ƭѡ
}

//----------------------------����������--------------------------------
extern void SpiDev_IRQ(void); //��ѯģʽʱ:�жϴ�����,��SpiDev_Pic.c��ʵ��
//����ϵͳ���������в�ѯ
void Spi_FastTask(void)
{
  if(Spi.Timer){//��ʱ
    Spi.Timer--;
    return;
  }
  
  //�ò�ѯ��ʽʵ���жϵ��÷�ֹӰ��ͨѶ
  if(SpiDev.Flag & SPI_DEV_BUSY){//SPI����������ʱ
    if(PIR1 & PICB_SSPIF){//�жϱ�־��λ
      SpiDev_IRQ(); //�����жϴ���
      Spi.Timer = SPI_DATA_SPACE; //�ֽڼ���ʱ
      //ע���жϱ�־�ڶ�ȡSPSR��SPDR���Զ���λ
    }
  }
}

