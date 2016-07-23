/******************************************************************************

                   通用SPI主机设备驱动程序接口-软件IO模拟实现
//此模块模拟在AVR单片机普通IO中测试通过
//此模块通过辅助接口SpiDev_Soft实现进一步抽像
//通过配置SpiDev_Soft中的扩展函数，可实现按位按位长度发送功能(如HT1621)
//此实现仅模拟1个SpiDev硬件(因直接使用了硬件回调),但可以挂多个Spi从设备
******************************************************************************/

#include "SpiDev.h"
#include "SpiDev_Soft.h"
#include <string.h>

//----------------------Spi设备初始化函数------------------------------
void SpiDev_Init(struct _SpiDev *pDev,
                 const void *pSpiHw)  //挂接的Spi硬件设备,这里无效
{
  struct _SpiDev_Soft *pSoft = (struct _SpiDev_Soft *)pSpiHw;
  memset(pDev, 0, sizeof(struct _SpiDev));

  SpiDev_Soft_Init(pSoft);      //结构初始化
  SpiDev_Soft_cbIoInit(pSoft);  //IO初始化
  pDev->pSpiHw = pSpiHw;
  //置默认电平
  if(pSoft->Cfg & SPI_DEV_SOFT_CPOL_H) SpiDev_Soft_cbSetClk(pSoft);
  else  SpiDev_Soft_cbClrClk(pSoft);
}

//-------------------------移位函数------------------------------
//此函数将数据从MOSI移出,同时从MISO读数
unsigned char _DataShift(struct _SpiDev_Soft *pSoft,
                         unsigned char BitLen,//位长度,支持按位长发送
                         unsigned char Data)//数据
{
  unsigned char Cfg = pSoft->Cfg; //配置
  unsigned char Shift;
  if(Cfg & SPI_DEV_SOFT_DORD_LSB) Shift = 0x01;//低位在前
  else  Shift = 0x80; //高位在前
  while(BitLen > 0){
    //准备好需移出的数据
    if(Data & Shift) SpiDev_Soft_cbSetMosi(pSoft);
    else SpiDev_Soft_cbClrMosi(pSoft);
    //时钟起始沿
    if(Cfg & SPI_DEV_SOFT_CPOL_H) SpiDev_Soft_cbClrClk(pSoft);
    else  SpiDev_Soft_cbSetClk(pSoft);
    //超始沿采样时读取数据
    if(!(Cfg & SPI_DEV_SOFT_CPHA_END)){
      if(SpiDev_Soft_cbIsSetMiso(pSoft)) Data |= Shift;
      else Data &= ~Shift;
    }
    //数据保持
    SpiDev_Soft_cbDelay(pSoft->BitDelay);
    //时钟结束沿
    if(Cfg & SPI_DEV_SOFT_CPOL_H) SpiDev_Soft_cbSetClk(pSoft);
    else SpiDev_Soft_cbClrClk(pSoft);
    //结束沿采样时读取数据
    if(Cfg & SPI_DEV_SOFT_CPHA_END){
      if(SpiDev_Soft_cbIsSetMiso(pSoft)) Data |= Shift;
      else Data &= ~Shift;
    }
    //数据保持
    SpiDev_Soft_cbDelay(pSoft->BitDelay);

    //准备下一个需发送的数据
    BitLen--;
    if(Cfg & SPI_DEV_SOFT_DORD_LSB) Shift <<= 1;//低位在前
    else Shift >>= 1; //高位在前
  }//end while
  return Data;
}

//----------------------Spi设备启动函数------------------------------
//用于重启Spi接口
//返回定义为:-1:设备用,0:成功
signed char SpiDev_Restart(struct _SpiDev *pDev,
                           struct _SpiDevCmd *pCmd)   //挂接的命令
{
  unsigned short Count;
  unsigned char Data;
  unsigned char BitLen;//位长
  unsigned char *pData;
  struct _SpiDev_Soft *pSoft = (struct _SpiDev_Soft *)pDev->pSpiHw;
  pDev->pCmd = pCmd;

  //软件SPI直接强制发送
  SpiDev_Soft_cbEnterCritical();//进入临界区
  pCmd->cbCfg(pDev, SPI_DEV2_CALL_POS_CS_VALID);//片选
  //发送命令
  pCmd->cbCfg(pDev, SPI_DEV2_CALL_POS_CMD_CFG);//命令配置
  pData = pCmd->pCmd;
  for(Count = pCmd->CmdSize; Count > 0; Count--, pData++){
    //最后发送位长度判断
    if((Count != 1)) BitLen = 8;
    else BitLen = pSoft->LastBitLen;
    Data = _DataShift(pSoft,BitLen, *pData);
    if(pCmd->Cfg & SPI_DEV2_CMD_EN_CMD_RCV)//命令时收数据
      *pData = Data;
    if(pCmd->Cfg & SPI_DEV2_CMD_INTERPOS)//干预模式回调用户
      pCmd->cbEndInt(pDev);
    SpiDev_Soft_cbDelay(pSoft->BitDelay); //字节间延时
  }
  //发送与接收数据
  pCmd->cbCfg(pDev, SPI_DEV2_CALL_POS_DATA_CFG);//数据配置
  pData = pCmd->pData;
  for(Count = pCmd->DataSize; Count > 0; Count--, pData++){
    if(pCmd->Cfg & SPI_DEV2_CMD_DIS_TX) Data = 0xff;//不发送数据
    else Data = *pData;
    //最后发送位长度判断
    if((Count != 1)) BitLen = 8;
    else BitLen = pSoft->LastBitLen;
    Data = _DataShift(pSoft, BitLen, *pData);
    if(!(pCmd->Cfg & SPI_DEV2_CMD_DIS_RX)) //允许接收数据时
      *pData = Data;
    if(pCmd->Cfg & SPI_DEV2_CMD_INTERPOS)//干预模式回调用户
      pCmd->cbEndInt(pDev);
    SpiDev_Soft_cbDelay(pSoft->BitDelay); //字节间延时
  }
  pCmd->cbCfg(pDev, SPI_DEV2_CALL_POS_CS_INVALID);//取消片选
  SpiDev_Soft_cbExitCritical();//退出临界区
  pCmd->cbEndInt(pDev);//最后回调用户
  return 0;
}

//----------------------Spi设备任务函数------------------------------
//用于强制停止Spi设备,必要时(程序强制等待)需在硬件定时器Tick中断里调用
void SpiDev_Task(struct _SpiDev *pDev)
{
  //因软件SPI直接强制发送,这里不等待
}

//----------------------Spi设备停止函数------------------------------
//强制停止Spi设备
void SpiDev_Stop(struct _SpiDev *pDev)
{
  //因软件SPI直接强制发送,这里不处理
}













