/******************************************************************************

  ͨ��SPI�����豸��������ӿ�-��LPC Armϵ��(arm7,m3,m0)�е�ʵ��
//ע:��ָ��SSPģ���е�ʵ��,ͬʱ����FIFO(������DMA)
//ʹ���ж�ģʽʱ,��ʹ��FIFO,�����ó�ʱ����RTRIS�ж�,ÿ��ֻ�շ�һ������
//��ʹ���ж�ģʽʱ,��������ݷֱ�ʹ��FIFO
//Indexָʾ�����˶��ٸ�,���ӵ�RxIndex��ʾ�����˶��ٸ�
//��û������ʱ��FIFO�����ж���������ʱ������RTRIS
//ע:�����ʱFIFO���ڷ�����,���ڵ���ʱ���ܶ�ʧ��������!!!!
******************************************************************************/

#include "SpiDev.h"
#include <string.h>
#include "LPC12xx.h"//��ͬоƬ�����
#include "LPC12xxbit.h"//��ͬоƬ�����

/***************************************************************************
                        �û�������ʹ��˵��
//�밴˵��������Ӧ����
***************************************************************************/

//����(����Ԥ��������)ʱ�ò�ѯ��ʽʹ���жϣ�����ʹ���ж�ģʽ
//#define  SUPPORT_SPI_DEV_QUREY

//����SPI��ʱֵ�����ڷ�ֹSPI����  ��ֵ��SpiDev_Task���������й�,������ʱΪ2
//#define  SPI_DEV_TIMER_OV     2

//ʹ��FIFOʱ������Ӳ���ض���(����Ԥ�����ϵͳ��������ļ�����)FIFO�Ĵ�С
//#define  SPI_DEV_HW_FIFO_SIZE  8  //������ʱΪ8

//�жϴ�����,�����жϺ�����(�ж�ģʽʱ)����ڿ���������ɨ��(��ѯ��ʽʱ)
extern void SpiDev_IRQ(struct _SpiDev *pDev); 

/***************************************************************************
                             �ڲ���ת��
***************************************************************************/

#ifndef SPI_DEV_TIMER_OV
  #define SPI_DEV_TIMER_OV  2
#endif

#ifndef SPI_DEV_HW_FIFO_SIZE
  #define  SPI_DEV_HW_FIFO_SIZE     8  //Ĭ�ϴ�С
#endif

/***************************************************************************
                           ��غ���ʵ��
***************************************************************************/

//----------------------������FIFO����------------------------------
//�����¸�����λ��,Index = Count��ʾ����
static unsigned short _PushFIFO(unsigned short Index,
                               unsigned short Count,
                               const unsigned char *pBuf,
                               LPC_SSP_TypeDef *pHw)
{
  unsigned char EnCount = SPI_DEV_HW_FIFO_SIZE; //��ֹһ�߷�һ���ж�ʱ,��ʧ���յ�����
  pBuf += Index;
  while(pHw->SR & LPC_TNF){//δ��ʱ
    pHw->DR = *pBuf;
    Index++;    
    if(Index >= Count) break;//�����
    EnCount--;
    if(!EnCount) break;//�����
    pBuf++;
  }
  return Index;
}

//----------------------���ݴ�FIFO��������------------------------------
//�����¸�����λ��,Index = Count��ʾ����
static unsigned short _PopFIFO(unsigned short Index,
                               unsigned short Count,
                               unsigned char *pBuf,
                               LPC_SSP_TypeDef *pHw)
{
  pBuf += Index;
  while(pHw->SR & LPC_RNE){//����δ��ʱ
    *pBuf = pHw->DR;
    Index++;    
    if(Index >= Count) break;//�����
    pBuf++;
  }
  return Index;
}

//------------------------���FIFO����--------------------------------
//�����¸�����λ��,Index = Count��ʾ����
static unsigned short _ClrRxFIFO(unsigned short Index,
                               unsigned short Count, //��Ҫ������ٸ�,>=1;
                               LPC_SSP_TypeDef *pHw)
{
  unsigned short Data;
  while(pHw->SR & LPC_RNE){
    Data = pHw->DR; //δ��ʱ
    Index++;
    if(Index >= Count) break;//�����
  }
  Data = Data;
  return Index;
}

//----------------------Spi�豸��ʼ������------------------------------
void SpiDev_Init(struct _SpiDev *pDev,
                 void *pSpiHw)  //�ҽӵ�SpiӲ���豸,������Ч
{
  memset(pDev, 0, sizeof(struct _SpiDev));
  pDev->pSpiHw = pSpiHw;
  LPC_SSP_TypeDef *pHw = pSpiHw;
  //��ʼ��Ӳ���豸,(IO���������ⲿʵ��)
  //ʹ��SPI����Ϊ����ģʽ
  pHw->CR1 = LPC_SSE;// ע:CR0��������CPSR�����ⲿ���� �ػ�����ʱ| LPC_LBM;
}

//----------------------Spi�豸��������------------------------------
//��������Spi�ӿ�
//���ض���Ϊ:-1:�豸��,0:�ɹ�
signed char SpiDev_Restart(struct _SpiDev *pDev,
                           struct _SpiDevCmd *pCmd)   //�ҽӵ�����
{
  if(pDev->Flag & SPI_DEV_BUSY) return -1;//Spiռ����
  
  LPC_SSP_TypeDef *pHw = pDev->pSpiHw;  
  if((pHw->SR & (LPC_TFE | LPC_RNE)) != LPC_TFE){//SPI�쳣,��ֹͣ
    SpiDev_Stop(pDev);
  }
  
  //��ǰ��ʼSPI���������Ч��
  pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_CS_VALID); //Ƭѡѡ��
  pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_CMD_CFG); //��������͸�ʽ
  
  signed char FinalFlag;
  //�ж�ģʽʱ,����һ����
  if(pCmd->Cfg & SPI_DEV_CMD_INTERPOS){
    pHw->DR = *pCmd->pCmd;
    pDev->Index = 1;
    FinalFlag = 1;//���
  }
  else{//���ж�ģʽʱ,һ�����Ϳ��ܵ�����
    pDev->Index = _PushFIFO(0, pCmd->CmdSize, pCmd->pCmd, pHw);//������
    if(pDev->Index < pCmd->CmdSize)//δ���
      FinalFlag = 0;
    else FinalFlag = 1;//�������
  }

  //��ʼ�����
  pDev->RxIndex = 0;
  pDev->pCmd = pCmd;
  pDev->Timer = SPI_DEV_TIMER_OV;
  pDev->Flag = SPI_DEV_BUSY;

  //����ж�
  #ifndef  SUPPORT_SPI_DEV_QUREY
    if(FinalFlag) pHw->IMSC = LPC_RTIM; //������ɳ�ʱʱ�����ж�
    else pHw->IMSC =LPC_TXIM; //���ݷ��͹�����
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
  LPC_SSP_TypeDef *pHw = pDev->pSpiHw;  
  //��дFIFOʹ�������
  //for(unsigned char i = 0; i < 16; i++){
  //  while(pHw->SR & LPC_LPC_TFE)break;//Ϊ����
    //pHw->DR = 0xff;//��ô�����д��FIFO�е�����???
  //}
  //for(unsigned char i = 0; i < 16; i++){
  //  while(pHw->SR & LPC_RNE)break;//Ϊ����
  //  volatile int dump = pHw->DR;//�ٶ����
  //}
  //�ٶ����
  while(pHw->SR & LPC_RNE){ volatile int Dump = pHw->DR;};
  
  //д1����������������ж��������
  pHw->ICR = LPC_RORIC | LPC_RTIC;  
  
  //����SPIʵ��ֹͣ����λ
  pHw->CR1 &= ~LPC_SSE;//�ر�SSP
  pDev->pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_CS_INVALID); //Ƭѡȡ��
  //ʹ��SPI����Ϊ����ģʽ
  pHw->CR1 |= LPC_SSE;
  pDev->Flag = 0;
};

//----------------------Spi�жϴ�����------------------------------
void SpiDev_IRQ(struct _SpiDev *pDev)
{
  struct _SpiDevCmd *pCmd = pDev->pCmd;
  LPC_SSP_TypeDef *pHw = pDev->pSpiHw; 
  
  unsigned char Flag = pDev->Flag;
  unsigned char Cfg = pCmd->Cfg;
  unsigned short Index = pDev->Index;
  
  //====================��������ģʽ����===========================
  if(!(Flag & SPI_DEV_CMD_FINAL)){
    //��������ͬʱ������շ�����Ϣʱ,���շ���������ͬʱ���ص�����
    unsigned short RxIndex = pDev->RxIndex;
    if(Cfg & SPI_DEV_CMD_EN_CMD_RCV)
      RxIndex = _PopFIFO(RxIndex, pCmd->CmdSize, pCmd->pCmd, pHw);
    else RxIndex = _ClrRxFIFO(RxIndex, pCmd->CmdSize, pHw);
    if(RxIndex < pCmd->CmdSize){ //�������δ���
      pDev->RxIndex = RxIndex;
      if(Index < pCmd->CmdSize){//����û���
        if(Cfg & SPI_DEV_CMD_INTERPOS){//�ж�ģʽ
          pHw->DR = *(pCmd->pCmd + Index);
          pDev->Index++;
        }
        else{//FIFOģʽ
          Index = _PushFIFO(Index, pCmd->CmdSize, pCmd->pCmd, pHw);//������
          if(Index >= pCmd->CmdSize) ////��������,��δ������,�ȴ��������ж�
            pHw->IMSC = LPC_RTIM; //������ɳ�ʱʱ�����ж���������������
          pDev->Index = Index;
        }
      }
      else{//����δ��,��û������
        pHw->IMSC = LPC_RTIM; //������ɳ�ʱʱ�����ж���������������
      }
      //�ж�ģʽʱ���ú�������
      if(Cfg & SPI_DEV_CMD_INTERPOS) pCmd->cbEndInt(pDev);
      goto End;      
    }
    else _ClrRxFIFO(0,255, pHw);//����������,ǿ����ս���FIFO
    
    //�����д�����
    Index = 0;
    pDev->Index = Index;
    pDev->RxIndex = Index;
    Flag |= SPI_DEV_CMD_FINAL;//�������ݴ���״̬
    pDev->Flag = Flag;
    pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_DATA_CFG); //�������ݷ��͸�ʽ
  }
  //====================������ģʽ����===========================
  else{
    //�����ѷ������,�쳣����
    //if(Flag & SPI_DEV_DATA_FINAL){
    //  //���ڻָ��ж�
    //  goto End;
    //}
    unsigned short RxIndex = pDev->RxIndex;
    if(!(Cfg & SPI_DEV_CMD_DIS_RX))
      RxIndex = _PopFIFO(RxIndex, pCmd->DataSize, pCmd->pData, pHw);
    else RxIndex = _ClrRxFIFO(RxIndex, pCmd->DataSize, pHw);
    if(RxIndex >= pCmd->DataSize) //���ݽ������
      _ClrRxFIFO(0,255, pHw);//���ݽ������,ǿ����ս���FIFO
    pDev->RxIndex = RxIndex;
  }
  //������������ʱ,���ͻ��������δ���
  if((pCmd->DataSize) && (Index < pCmd->DataSize)){
    if(Cfg & SPI_DEV_CMD_INTERPOS){//�ж�ģʽ
      pHw->DR = *(pCmd->pData + Index);
      pDev->Index++;
    }
    else{//FIFOģʽ
      Index = _PushFIFO(Index, pCmd->DataSize, pCmd->pData, pHw);//������
      if(Index < pCmd->DataSize)//���ݷ���δ���
        pHw->IMSC =LPC_TXIM; //���ݷ��͹�����
      else //���ݷ������,��δ������,�ȴ��������ж�
        pHw->IMSC = LPC_RTIM; //������ɳ�ʱʱ�����ж���������������
      pDev->Index = Index;
    }
    //�ж�ģʽʱ���ú�������
    if(Cfg & SPI_DEV_CMD_INTERPOS) pCmd->cbEndInt(pDev);
  }
  else{//�������������ݷ������
    Flag &= ~SPI_DEV_BUSY;//ȡ��æ��־
    pDev->Flag = Flag | SPI_DEV_DATA_FINAL;//���
    pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_CS_INVALID); //Ƭѡȡ��
    pHw->ICR = LPC_RORIC | LPC_RTIC;  //����ж�
    pHw->IMSC = 0;// clr LPC_TXIM | LPC_RXIM;   //���жϽ���
    pDev->Timer = 0;//ʱ�����
    pCmd->cbEndInt(pDev);//���ص�
    return;
  }

  End: //��������,���жϵ�
  pHw->ICR = LPC_RORIC | LPC_RTIC;  //����ж� 
  return;
}















