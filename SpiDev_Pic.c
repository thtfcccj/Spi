/******************************************************************************

           ͨ��SPI�����豸��������ӿ�-��PIC 16/18��Ƭ���е�ʵ��
//��PICϵ�е�Ƭ��SPIֻ��һ��Ӳ���豸����ֱ�Ӳ����Ĵ���ʵ��
******************************************************************************/

#include "SpiDev.h"
#include <string.h>

#include <pic.h>
#include "PicBit.h"

/***************************************************************************
                        �û�������ʹ��˵��
//�밴˵��������Ӧ����
***************************************************************************/

//����(����Ԥ��������)ʱ�ò�ѯ��ʽʹ���жϣ�����ʹ���ж�ģʽ
//#define  SUPPORT_SPI_DEV_QUREY

//����SPI��ʱֵ�����ڷ�ֹSPI����  ��ֵ��SpiDev_Task���������й�,������ʱΪ2
//#define  SPI_DEV_TIMER_OV     2

//�жϴ�����,�����жϺ�����(�ж�ģʽʱ)����ڿ���������ɨ��(��ѯ��ʽʱ)
extern void SpiDev_IRQ(void); 

//�������ⲿʵ�ֵ�struct _SpiDevʵ������ֻ��һ����ֱ�Ӷ�����,�ⲿ��ʵ��
extern struct _SpiDev SpiDev; 

/***************************************************************************
                             �ڲ���ת��
***************************************************************************/

#ifndef SPI_DEV_TIMER_OV
  #define SPI_DEV_TIMER_OV  2
#endif

/***************************************************************************
                           ��غ���ʵ��
***************************************************************************/
//----------------------Spi����Ӳ������------------------------------
static void _CfgHw(void)
{
  SSPCON1 = 0;//��λ,��SSPCON1 &= ~PICB_SSPEN;,ֻ���ڽ�ֹSSPENʱ���ܽ�������
  //����Ϊ�½�����ʼ�������ز���,CLKƽʱΪ�ߵ�ƽ
  SSPCON1 |= PICB_CKP;   //CLK�����ڼ�ߵ�ƽ
  SSPSTAT = 0; //SSPSTAT &= ~PICB_CKE;  //��2���仯�ز���
  //��Ϊ����ģʽ
  //SSPCON1 |= PICB_SSPM1;  //0b0010= SPI Master mode, clock = FOSC/64
  SSPADD = 127;//����ʱ��,���ǵ�ʱ��Ϊ32M,ʹ��32/4/128��Ƶ
  SSPCON1 |= PICB_SSPM3 | PICB_SSPM1;  //0b1010= SPI��ģʽ��ʱ��= FOSC/(4 * (SSPADD+1))
  SSPCON1 |= PICB_SSPEN; //������ɺ��������
  PIR1 &= ~PICB_SSPIF; //���ж�
}  

//----------------------Spi�豸��ʼ������------------------------------
void SpiDev_Init(struct _SpiDev *pDev,
               const void *pSpiHw)  //�ҽӵ�SpiӲ���豸,������Ч
{
  memset(pDev, 0, sizeof(struct _SpiDev));
  //pDev->pSpiHw = pSpiHw;
  //��ʼ��Ӳ���豸,(IO���������ⲿʵ��)
  //SpiSlv_cbCfgIo();//����IO 
   
  //��ʼ��Ӳ���ṹ
  _CfgHw();
  
  //����ж�
  #ifndef  SUPPORT_SPI_DEV_QUREY
    PIE1 |= PICB_SSPIE; //�ж�����
  #endif
}

//----------------------Spi�豸��������------------------------------
//��������Spi�ӿ�
//���ض���Ϊ:-1:�豸��,0:�ɹ�
signed char SpiDev_Restart(struct _SpiDev *pDev,
                        struct _SpiDevCmd *pCmd)   //�ҽӵ�����
{
  if(pDev->Flag & SPI_DEV_BUSY) return -1;//Spiռ����

  //��ǰ��ʼSPI���������Ч��
  //pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_CS_VALID); //Ƭѡѡ��
  //pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_CMD_CFG); //��������͸�ʽ
  SSPBUF = *(pCmd->pCmd); //��������
  
  //��ʼ�����
  pDev->pCmd = pCmd;
  pDev->Timer = SPI_DEV_TIMER_OV;
  pDev->Index = 0;
  pDev->Flag = SPI_DEV_BUSY;
  //����ж�
  #ifndef  SUPPORT_SPI_DEV_QUREY
    PIE1 |= PICB_SSPIE; //�ж����� 
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
  //����SPIʵ��ֹͣ����λ
  _CfgHw(); //��λ��ֹͣ�����¿�ʼ
  //if(pDev->pCmd)
  //  pDev->pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_CS_INVALID); //Ƭѡȡ��
  pDev->Flag = 0;
};

//----------------------Spi�жϴ�����------------------------------
void SpiDev_IRQ(void)
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
      *(pCmd->pCmd + Index) = SSPBUF;
    }
    Index++;

    //����δ���
    if(Index < pCmd->CmdSize){
      SSPBUF = *(pCmd->pCmd + Index); //��������
      pDev->Index = Index;
      goto End;
    }
    //����д�����
    Index = 0;
    Flag |= SPI_DEV_CMD_FINAL; //�������ݴ���״̬
    pDev->Flag = Flag;
    //pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_DATA_CFG); //�������ݷ��͸�ʽ
  }
  else{
    //�����ѷ������,�쳣����
    //if(Flag & SPI_DEV_DATA_FINAL){
    //  SSPBUF = 0xff;  //���ڻָ��ж�
    //  goto End;
    //}
  
    //�����������ʱ,��������
    if(!(Cfg & SPI_DEV_CMD_DIS_RX)){
      *(pCmd->pData + Index) = SSPBUF;
    }
  
    //���ͻ�׼��������һ������  
    Index++;
    pDev->Index = Index;
  }
  
  //������������ʱ,���ͻ��������δ���
  if((pCmd->DataSize) && (Index < pCmd->DataSize)){
    if (!(Cfg & SPI_DEV_CMD_DIS_TX))
      SSPBUF = *(pCmd->pData + Index); //��������
    else //��˫������ʱ���ߵ�ƽ���ڽ�������
      SSPBUF = 0xff;
    //�ж�ģʽʱ���ú�������
    //if(Cfg & SPI_DEV_CMD_INTERPOS) 
    //  pCmd->cbEndInt(pDev); 
  }
  else{//�������������ݷ������
    Flag &= ~SPI_DEV_BUSY;//ȡ��æ��־
    pDev->Flag = Flag | SPI_DEV_DATA_FINAL;//���
    //pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_CS_INVALID); //Ƭѡȡ��
    //pCmd->cbEndInt(pDev);
    PIE1 &= ~PICB_SSPIE; //���жϽ���
  }

  End: //��������,���жϵ�
  //���ж�: SPIӲ����ȡ״̬������ʱ�Զ����ж�
  PIR1 &= ~PICB_SSPIF; //���ж�
  return;  
}















