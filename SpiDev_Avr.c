/******************************************************************************

           ͨ��SPI�����豸��������ӿ�-��AVR��Ƭ���е�ʵ��
//��PICϵ�е�Ƭ��SPIֻ��һ��Ӳ���豸����ֱ�Ӳ����Ĵ���ʵ��
******************************************************************************/

#include "SpiDev.h"
#include <string.h>

/***************************************************************************
                        �û�������ʹ��˵��
//�밴˵��������Ӧ����
***************************************************************************/

//���忪������������(����Ԥ��������)ʱ��AVRGCCʹ���жϣ�����ʹ��IAR����
//#define  SUPPORT_AVRGCC

//����(����Ԥ��������)ʱ�ò�ѯ��ʽʹ���жϣ�����ʹ���ж�ģʽ
//#define  SUPPORT_SPI_DEV_QUREY

//����SPI��ʱֵ�����ڷ�ֹSPI����  ��ֵ��SpiDev_Task���������й�,������ʱΪ2
//#define  SPI_DEV_TIMER_OV     2

//��ѯģʽʱ: �жϴ����������ڿ���������ɨ��
extern void SpiDev_IRQ(void); 

//�������ⲿʵ�ֵ�struct _SpiDevʵ������ֻ��һ����ֱ�Ӷ�����,�ⲿ��ʵ��
extern struct _SpiDev SpiDev; 

/***************************************************************************
                             �ڲ���ת��
***************************************************************************/

#ifndef SPI_DEV_TIMER_OV
  #define SPI_DEV_TIMER_OV  2
#endif

#ifdef SUPPORT_AVRGCC //AVRGCC����ʱ
  #include <avr/IO.h> 
#else
  #include <ioavr.h>//IAR����ʱ
#endif

//_FUN_IRQ()��ת��:
#ifndef  SUPPORT_SPI_DEV_QUREY  //�ж�ģʽʱ,Ϊ�ж����
  #ifdef SUPPORT_AVRGCC //AVRGCC����ʱ
    #define _FUN_IRQ()   ISR(SPI_STC_vect)
  #else //IAR�ж�д��,δ�о�������
    #define _FUN_IRQ()   void SpiDev_IRQ(void)
  #endif
#else //��ѯģʽʱ
  #define _FUN_IRQ()     void SpiDev_IRQ(void)
#endif

/***************************************************************************
                           ��غ���ʵ��
***************************************************************************/

//----------------------Spi�豸��ʼ������------------------------------
void SpiDev_Init(struct _SpiDev *pDev,
               const void *pSpiHw)  //�ҽӵ�SpiӲ���豸,������Ч
{
  memset(pDev, 0, sizeof(struct _SpiDev));
  //pDev->pSpiHw = pSpiHw;
  //��ʼ��Ӳ���豸,(IO���������ⲿʵ��)
  //ʹ��SPI����Ϊ����ģʽ
  SPCR |= (1<<SPE) | (1<<MSTR);
}

//----------------------Spi�豸��������------------------------------
//��������Spi�ӿ�
//���ض���Ϊ:-1:�豸��,0:�ɹ�
signed char SpiDev_Restart(struct _SpiDev *pDev,
                        struct _SpiDevCmd *pCmd)   //�ҽӵ�����
{
  if(pDev->Flag & SPI_DEV_BUSY) return -1;//Spiռ����
  //����ڱ���Ϊ�ӻ�������,ǿ����Ϊ����
  SPSR |= (1<<SPIF);  //����Ϊ�ӻ����ж�
  SPCR |= (1<<MSTR);  //ǿ��Ϊ����
  //��ǰ��ʼSPI���������Ч��
  pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_CS_VALID); //Ƭѡѡ��
  pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_CMD_CFG); //��������͸�ʽ
  SPDR = *(pCmd->pCmd); //��������
  
  //��ʼ�����
  pDev->pCmd = pCmd;
  pDev->Timer = SPI_DEV_TIMER_OV;
  pDev->Index = 0;
  pDev->Flag = SPI_DEV_BUSY;
  //����ж�
  #ifndef  SUPPORT_SPI_DEV_QUREY
    SPCR |= (1<<SPIE); 
  #endif
  return 0;
};

//----------------------Spi�豸������------------------------------
//����ǿ��ֹͣSpi�豸,��Ҫʱ(����ǿ�Ƶȴ�)����Ӳ����ʱ��Tick�ж������
void SpiDev_Task(struct _SpiDev *pDev)
{
  if(!pDev->Timer) return;  //û�й���
  pDev->Timer--;
  if(pDev->Timer) return;  //ʱ��δ��
  //��ʱ����
  SpiDev_Stop(pDev);
  pDev->Flag = SPI_DEV_OV_ERR; //��ʱ����
};

//----------------------Spi�豸ֹͣ����------------------------------
//ǿ��ֹͣSpi�豸
void SpiDev_Stop(struct _SpiDev *pDev)
{
  //�ȶ�SPSR�������ŷ���SPDR����SPIF����
  //unsigned char Dump = SPSR;
  
  //����SPIʵ��ֹͣ����λ
  SPCR &= ~((1<<SPE) | (1<<MSTR)); //�ر�SPI
  if(pDev->pCmd)
    pDev->pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_CS_INVALID); //Ƭѡȡ��
  //ʹ��SPI����Ϊ����ģʽ
  SPCR |= (1<<SPE) | (1<<MSTR);
  pDev->Flag = 0;
};

//----------------------Spi�жϴ�����------------------------------
_FUN_IRQ()
{
  struct _SpiDev *pDev = &SpiDev;
  struct _SpiDevCmd *pCmd = pDev->pCmd;
  unsigned char Flag = pDev->Flag;
  unsigned char Index = pDev->Index;
  unsigned char Cfg = pCmd->Cfg;
  
  //��������ģʽ����
  if(!(Flag & SPI_DEV_CMD_FINAL)){
    //��������ͬʱ������շ�����Ϣʱ,���շ���������ͬʱ���ص�����
    if(Cfg & SPI_DEV_CMD_EN_CMD_RCV){
      *(pCmd->pCmd + Index) = SPDR;
    }
    Index++;

    //����δ���
    if(Index < pCmd->CmdSize){
      SPDR = *(pCmd->pCmd + Index); //��������
      pDev->Index = Index;
      goto End;
    }
    //����д�����
    Index = 0;
    Flag |= SPI_DEV_CMD_FINAL; //�������ݴ���״̬
    pDev->Flag = Flag;
    pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_DATA_CFG); //�������ݷ��͸�ʽ
  }
  else{
    //�����ѷ������,�쳣����
    //if(Flag & SPI_DEV_DATA_FINAL){
    //  SPDR = 0xff;  //���ڻָ��ж�
    //  goto End;
    //}
  
    //�����������ʱ,��������
    if(!(Cfg & SPI_DEV_CMD_DIS_RX)){
      *(pCmd->pData + Index) = SPDR;
    }
  
    //���ͻ�׼��������һ������  
    Index++;
    pDev->Index = Index;
  }
  
  //������������ʱ,���ͻ��������δ���
  if((pCmd->DataSize) && (Index < pCmd->DataSize)){
    if (!(Cfg & SPI_DEV_CMD_DIS_TX))
      SPDR = *(pCmd->pData + Index); //��������
    else //��˫������ʱ���ߵ�ƽ���ڽ�������
      SPDR = 0xff;
    //�ж�ģʽʱ���ú�������
    if(Cfg & SPI_DEV_CMD_INTERPOS) 
      pCmd->cbEndInt(pDev); 
  }
  else{//�������������ݷ������
    Flag &= ~SPI_DEV_BUSY;//ȡ��æ��־
    pDev->Flag = Flag | SPI_DEV_DATA_FINAL;//���
    pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_CS_INVALID); //Ƭѡȡ��
    pCmd->cbEndInt(pDev);
    SPCR &= ~(1<<SPIE);   //���жϽ���
  }

  End: //��������,���жϵ�
  return;  
}















