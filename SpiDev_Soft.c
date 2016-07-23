/******************************************************************************

                   ͨ��SPI�����豸��������ӿ�-���IOģ��ʵ��
//��ģ��ģ����AVR��Ƭ����ͨIO�в���ͨ��
//��ģ��ͨ�������ӿ�SpiDev_Softʵ�ֽ�һ������
//ͨ������SpiDev_Soft�е���չ��������ʵ�ְ�λ��λ���ȷ��͹���(��HT1621)
//��ʵ�ֽ�ģ��1��SpiDevӲ��(��ֱ��ʹ����Ӳ���ص�),�����ԹҶ��Spi���豸
******************************************************************************/

#include "SpiDev.h"
#include "SpiDev_Soft.h"
#include <string.h>

//----------------------Spi�豸��ʼ������------------------------------
void SpiDev_Init(struct _SpiDev *pDev,
                 const void *pSpiHw)  //�ҽӵ�SpiӲ���豸,������Ч
{
  struct _SpiDev_Soft *pSoft = (struct _SpiDev_Soft *)pSpiHw;
  memset(pDev, 0, sizeof(struct _SpiDev));

  SpiDev_Soft_Init(pSoft);      //�ṹ��ʼ��
  SpiDev_Soft_cbIoInit(pSoft);  //IO��ʼ��
  pDev->pSpiHw = pSpiHw;
  //��Ĭ�ϵ�ƽ
  if(pSoft->Cfg & SPI_DEV_SOFT_CPOL_H) SpiDev_Soft_cbSetClk(pSoft);
  else  SpiDev_Soft_cbClrClk(pSoft);
}

//-------------------------��λ����------------------------------
//�˺��������ݴ�MOSI�Ƴ�,ͬʱ��MISO����
unsigned char _DataShift(struct _SpiDev_Soft *pSoft,
                         unsigned char BitLen,//λ����,֧�ְ�λ������
                         unsigned char Data)//����
{
  unsigned char Cfg = pSoft->Cfg; //����
  unsigned char Shift;
  if(Cfg & SPI_DEV_SOFT_DORD_LSB) Shift = 0x01;//��λ��ǰ
  else  Shift = 0x80; //��λ��ǰ
  while(BitLen > 0){
    //׼�������Ƴ�������
    if(Data & Shift) SpiDev_Soft_cbSetMosi(pSoft);
    else SpiDev_Soft_cbClrMosi(pSoft);
    //ʱ����ʼ��
    if(Cfg & SPI_DEV_SOFT_CPOL_H) SpiDev_Soft_cbClrClk(pSoft);
    else  SpiDev_Soft_cbSetClk(pSoft);
    //��ʼ�ز���ʱ��ȡ����
    if(!(Cfg & SPI_DEV_SOFT_CPHA_END)){
      if(SpiDev_Soft_cbIsSetMiso(pSoft)) Data |= Shift;
      else Data &= ~Shift;
    }
    //���ݱ���
    SpiDev_Soft_cbDelay(pSoft->BitDelay);
    //ʱ�ӽ�����
    if(Cfg & SPI_DEV_SOFT_CPOL_H) SpiDev_Soft_cbSetClk(pSoft);
    else SpiDev_Soft_cbClrClk(pSoft);
    //�����ز���ʱ��ȡ����
    if(Cfg & SPI_DEV_SOFT_CPHA_END){
      if(SpiDev_Soft_cbIsSetMiso(pSoft)) Data |= Shift;
      else Data &= ~Shift;
    }
    //���ݱ���
    SpiDev_Soft_cbDelay(pSoft->BitDelay);

    //׼����һ���跢�͵�����
    BitLen--;
    if(Cfg & SPI_DEV_SOFT_DORD_LSB) Shift <<= 1;//��λ��ǰ
    else Shift >>= 1; //��λ��ǰ
  }//end while
  return Data;
}

//----------------------Spi�豸��������------------------------------
//��������Spi�ӿ�
//���ض���Ϊ:-1:�豸��,0:�ɹ�
signed char SpiDev_Restart(struct _SpiDev *pDev,
                           struct _SpiDevCmd *pCmd)   //�ҽӵ�����
{
  unsigned short Count;
  unsigned char Data;
  unsigned char BitLen;//λ��
  unsigned char *pData;
  struct _SpiDev_Soft *pSoft = (struct _SpiDev_Soft *)pDev->pSpiHw;
  pDev->pCmd = pCmd;

  //���SPIֱ��ǿ�Ʒ���
  SpiDev_Soft_cbEnterCritical();//�����ٽ���
  pCmd->cbCfg(pDev, SPI_DEV2_CALL_POS_CS_VALID);//Ƭѡ
  //��������
  pCmd->cbCfg(pDev, SPI_DEV2_CALL_POS_CMD_CFG);//��������
  pData = pCmd->pCmd;
  for(Count = pCmd->CmdSize; Count > 0; Count--, pData++){
    //�����λ�����ж�
    if((Count != 1)) BitLen = 8;
    else BitLen = pSoft->LastBitLen;
    Data = _DataShift(pSoft,BitLen, *pData);
    if(pCmd->Cfg & SPI_DEV2_CMD_EN_CMD_RCV)//����ʱ������
      *pData = Data;
    if(pCmd->Cfg & SPI_DEV2_CMD_INTERPOS)//��Ԥģʽ�ص��û�
      pCmd->cbEndInt(pDev);
    SpiDev_Soft_cbDelay(pSoft->BitDelay); //�ֽڼ���ʱ
  }
  //�������������
  pCmd->cbCfg(pDev, SPI_DEV2_CALL_POS_DATA_CFG);//��������
  pData = pCmd->pData;
  for(Count = pCmd->DataSize; Count > 0; Count--, pData++){
    if(pCmd->Cfg & SPI_DEV2_CMD_DIS_TX) Data = 0xff;//����������
    else Data = *pData;
    //�����λ�����ж�
    if((Count != 1)) BitLen = 8;
    else BitLen = pSoft->LastBitLen;
    Data = _DataShift(pSoft, BitLen, *pData);
    if(!(pCmd->Cfg & SPI_DEV2_CMD_DIS_RX)) //�����������ʱ
      *pData = Data;
    if(pCmd->Cfg & SPI_DEV2_CMD_INTERPOS)//��Ԥģʽ�ص��û�
      pCmd->cbEndInt(pDev);
    SpiDev_Soft_cbDelay(pSoft->BitDelay); //�ֽڼ���ʱ
  }
  pCmd->cbCfg(pDev, SPI_DEV2_CALL_POS_CS_INVALID);//ȡ��Ƭѡ
  SpiDev_Soft_cbExitCritical();//�˳��ٽ���
  pCmd->cbEndInt(pDev);//���ص��û�
  return 0;
}

//----------------------Spi�豸������------------------------------
//����ǿ��ֹͣSpi�豸,��Ҫʱ(����ǿ�Ƶȴ�)����Ӳ����ʱ��Tick�ж������
void SpiDev_Task(struct _SpiDev *pDev)
{
  //�����SPIֱ��ǿ�Ʒ���,���ﲻ�ȴ�
}

//----------------------Spi�豸ֹͣ����------------------------------
//ǿ��ֹͣSpi�豸
void SpiDev_Stop(struct _SpiDev *pDev)
{
  //�����SPIֱ��ǿ�Ʒ���,���ﲻ����
}













