/******************************************************************************

   ͨ��SPI�����豸��������ӿ�->��Spi�豸ʵ��+��Spi�ӻ�ʱ�����IO�е�ʵ��ʵ��	

//��ʵ��ʹ�ò�ѯģʽ
//֧������CS����
//���ļ���Ϊģ��,�ɸ���Ӧ�ý����޸�
******************************************************************************/

#include "Spi_Cs1.h"
//#include <avr/IO.h>  //AVRGCC����ʱ
#include <string.h>  //NULL

struct _SpiDev2 SpiDev2;   //ʵ���豸
struct _Spi Spi;

//---------------------SpiDev2Cmd���ú���ʵ��--------------------------
static void _cbCfg(void *pSpiDev, //struct _SpiDev *pSpiDev,   //�ҽ��豸
                   unsigned char CallPos)    //����λ��
{
  //ʹ��Ӳ��ֱ��ʵ��
  switch(CallPos){
    case SPI_DEV2_CALL_POS_CS_VALID: //��Ϊ����͵�ƽ״̬
	    ClrSpiCs();
	    OutSpiCs();
	  break;
    case SPI_DEV2_CALL_POS_CS_INVALID://��Ϊ��������״̬
	    SetPullUpSpiCs();
      InSpiCs();
	  break;
    //case SPI_DEV_CALL_POS_CMD_CFG:
    //case SPI_DEV_CALL_POS_DATA_CFG:
    default: //�����뷢�����ݸ�ʽ��ͬ
      //SPCR |= (1<<SPR1); //����ʱ�����ʹ̶�Ϊfck/64
    break;
  }
}

//------------------------------SpiDevCmd�ж���ɻص�����ʵ��-----------------
static void _cbEndInt(void *pSpiDev) //struct _SpiDev *pSpiDev)
{
  
};



//------------------------SPI��ʼ������------------------------
void Spi_Init (void)
{
  CfgSpiCs();
  
  memset(&Spi, 0, sizeof(struct _Spi));  
  SpiDev2_Init(&SpiDev2, &Spi.SpiDev_Soft);
  //Cmd�ڲ���ʼ��
  //Spi.Cmd.Cs = 0;  
  Spi.Cmd.pCmd = Spi.CmdBuf;
  Spi.Cmd.pData = Spi.DataBuf;
  Spi.Cmd.cbCfg = _cbCfg;
  Spi.Cmd.cbEndInt = _cbEndInt;
  
  //SetSpiCs();    //ȡ��Ƭѡ
  
  //����ʱ������->��������ʵ��
  
}
