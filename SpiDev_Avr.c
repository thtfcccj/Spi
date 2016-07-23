/******************************************************************************

           通用SPI主机设备驱动程序接口-在AVR单片机中的实现
//因PIC系列单片机SPI只有一个硬件设备，故直接操作寄存器实现
******************************************************************************/

#include "SpiDev.h"
#include <string.h>

/***************************************************************************
                        用户配置与使用说明
//请按说明进行相应处理
***************************************************************************/

//定义开发环境，定义(请在预编译中做)时用AVRGCC使用中断，否则使用IAR环境
//#define  SUPPORT_AVRGCC

//定义(请在预编译中做)时用查询方式使用中断，否则使用中断模式
//#define  SUPPORT_SPI_DEV_QUREY

//定义SPI超时值，用于防止SPI死掉  此值与SpiDev_Task调用周期有关,不定义时为2
//#define  SPI_DEV_TIMER_OV     2

//查询模式时: 中断处理函数，放在快速任务中扫描
extern void SpiDev_IRQ(void); 

//依靠于外部实现的struct _SpiDev实例，因只有一个，直接定义死,外部需实现
extern struct _SpiDev SpiDev; 

/***************************************************************************
                             内部宏转义
***************************************************************************/

#ifndef SPI_DEV_TIMER_OV
  #define SPI_DEV_TIMER_OV  2
#endif

#ifdef SUPPORT_AVRGCC //AVRGCC环境时
  #include <avr/IO.h> 
#else
  #include <ioavr.h>//IAR环境时
#endif

//_FUN_IRQ()的转义:
#ifndef  SUPPORT_SPI_DEV_QUREY  //中断模式时,为中断入口
  #ifdef SUPPORT_AVRGCC //AVRGCC环境时
    #define _FUN_IRQ()   ISR(SPI_STC_vect)
  #else //IAR中断写法,未研究！！！
    #define _FUN_IRQ()   void SpiDev_IRQ(void)
  #endif
#else //查询模式时
  #define _FUN_IRQ()     void SpiDev_IRQ(void)
#endif

/***************************************************************************
                           相关函数实现
***************************************************************************/

//----------------------Spi设备初始化函数------------------------------
void SpiDev_Init(struct _SpiDev *pDev,
               const void *pSpiHw)  //挂接的Spi硬件设备,这里无效
{
  memset(pDev, 0, sizeof(struct _SpiDev));
  //pDev->pSpiHw = pSpiHw;
  //初始化硬件设备,(IO口配置在外部实现)
  //使能SPI并置为主机模式
  SPCR |= (1<<SPE) | (1<<MSTR);
}

//----------------------Spi设备启动函数------------------------------
//用于重启Spi接口
//返回定义为:-1:设备用,0:成功
signed char SpiDev_Restart(struct _SpiDev *pDev,
                        struct _SpiDevCmd *pCmd)   //挂接的命令
{
  if(pDev->Flag & SPI_DEV_BUSY) return -1;//Spi占用中
  //因存在被拉为从机可能性,强制置为主机
  SPSR |= (1<<SPIF);  //清置为从机的中断
  SPCR |= (1<<MSTR);  //强制为主机
  //提前开始SPI周期以提高效率
  pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_CS_VALID); //片选选中
  pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_CMD_CFG); //配置命令发送格式
  SPDR = *(pCmd->pCmd); //启动发送
  
  //初始化相关
  pDev->pCmd = pCmd;
  pDev->Timer = SPI_DEV_TIMER_OV;
  pDev->Index = 0;
  pDev->Flag = SPI_DEV_BUSY;
  //最后开中断
  #ifndef  SUPPORT_SPI_DEV_QUREY
    SPCR |= (1<<SPIE); 
  #endif
  return 0;
};

//----------------------Spi设备任务函数------------------------------
//用于强制停止Spi设备,必要时(程序强制等待)需在硬件定时器Tick中断里调用
void SpiDev_Task(struct _SpiDev *pDev)
{
  if(!pDev->Timer) return;  //没有工作
  pDev->Timer--;
  if(pDev->Timer) return;  //时间未到
  //超时处理
  SpiDev_Stop(pDev);
  pDev->Flag = SPI_DEV_OV_ERR; //超时故障
};

//----------------------Spi设备停止函数------------------------------
//强制停止Spi设备
void SpiDev_Stop(struct _SpiDev *pDev)
{
  //先读SPSR，紧接着访问SPDR来对SPIF清零
  //unsigned char Dump = SPSR;
  
  //重启SPI实现停止并复位
  SPCR &= ~((1<<SPE) | (1<<MSTR)); //关闭SPI
  if(pDev->pCmd)
    pDev->pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_CS_INVALID); //片选取消
  //使能SPI并置为主机模式
  SPCR |= (1<<SPE) | (1<<MSTR);
  pDev->Flag = 0;
};

//----------------------Spi中断处理函数------------------------------
_FUN_IRQ()
{
  struct _SpiDev *pDev = &SpiDev;
  struct _SpiDevCmd *pCmd = pDev->pCmd;
  unsigned char Flag = pDev->Flag;
  unsigned char Index = pDev->Index;
  unsigned char Cfg = pCmd->Cfg;
  
  //发送命令模式处理
  if(!(Flag & SPI_DEV_CMD_FINAL)){
    //发送命令同时允许接收返回信息时,先收发送命令字同时返回的数据
    if(Cfg & SPI_DEV_CMD_EN_CMD_RCV){
      *(pCmd->pCmd + Index) = SPDR;
    }
    Index++;

    //命令未完成
    if(Index < pCmd->CmdSize){
      SPDR = *(pCmd->pCmd + Index); //启动发送
      pDev->Index = Index;
      goto End;
    }
    //命令写完成了
    Index = 0;
    Flag |= SPI_DEV_CMD_FINAL; //进入数据处理状态
    pDev->Flag = Flag;
    pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_DATA_CFG); //配置数据发送格式
  }
  else{
    //数据已发送完成,异常处理
    //if(Flag & SPI_DEV_DATA_FINAL){
    //  SPDR = 0xff;  //用于恢复中断
    //  goto End;
    //}
  
    //允许接收数据时,先收数据
    if(!(Cfg & SPI_DEV_CMD_DIS_RX)){
      *(pCmd->pData + Index) = SPDR;
    }
  
    //发送或准备接收下一个数据  
    Index++;
    pDev->Index = Index;
  }
  
  //有数据区发送时,发送或接收数据未完成
  if((pCmd->DataSize) && (Index < pCmd->DataSize)){
    if (!(Cfg & SPI_DEV_CMD_DIS_TX))
      SPDR = *(pCmd->pData + Index); //启动发送
    else //半双工接收时发高电平用于接收数据
      SPDR = 0xff;
    //中断模式时调用函数处理
    if(Cfg & SPI_DEV_CMD_INTERPOS) 
      pCmd->cbEndInt(pDev); 
  }
  else{//无数据区或数据发送完成
    Flag &= ~SPI_DEV_BUSY;//取消忙标志
    pDev->Flag = Flag | SPI_DEV_DATA_FINAL;//完成
    pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_CS_INVALID); //片选取消
    pCmd->cbEndInt(pDev);
    SPCR &= ~(1<<SPIE);   //关中断结束
  }

  End: //结束处理,清中断等
  return;  
}















