/******************************************************************************

  通用SPI主机设备驱动程序接口-在LPC Arm系列(arm7,m3,m0)中的实现
//注:仅指在SSP模块中的实现,同时启用FIFO(不启用DMA)
//使用中断模式时,不使用FIFO,仅启用超时接收RTRIS中断,每次只收发一个数据
//不使用中断模式时,命令和数据分别使用FIFO
//Index指示发送了多少个,附加的RxIndex表示接收了多少个
//在没有送完时有FIFO发送中断最后发送完成时才启用RTRIS
//注:因调试时FIFO任在发数据,故在调试时可能丢失接收数据!!!!
******************************************************************************/

#include "SpiDev.h"
#include <string.h>
#include "LPC12xx.h"//不同芯片需更改
#include "LPC12xxbit.h"//不同芯片需更改

/***************************************************************************
                        用户配置与使用说明
//请按说明进行相应处理
***************************************************************************/

//定义(请在预编译中做)时用查询方式使用中断，否则使用中断模式
//#define  SUPPORT_SPI_DEV_QUREY

//定义SPI超时值，用于防止SPI死掉  此值与SpiDev_Task调用周期有关,不定义时为2
//#define  SPI_DEV_TIMER_OV     2

//使用FIFO时，根据硬件重定义(请在预编译或系统配局配置文件里做)FIFO的大小
//#define  SPI_DEV_HW_FIFO_SIZE  8  //不定义时为8

//中断处理函数,放入中断函数内(中断模式时)或放在快速任务中扫描(查询方式时)
extern void SpiDev_IRQ(struct _SpiDev *pDev); 

/***************************************************************************
                             内部宏转义
***************************************************************************/

#ifndef SPI_DEV_TIMER_OV
  #define SPI_DEV_TIMER_OV  2
#endif

#ifndef SPI_DEV_HW_FIFO_SIZE
  #define  SPI_DEV_HW_FIFO_SIZE     8  //默认大小
#endif

/***************************************************************************
                           相关函数实现
***************************************************************************/

//----------------------数据入FIFO函数------------------------------
//返回下个数据位置,Index = Count表示入完
static unsigned short _PushFIFO(unsigned short Index,
                               unsigned short Count,
                               const unsigned char *pBuf,
                               LPC_SSP_TypeDef *pHw)
{
  unsigned char EnCount = SPI_DEV_HW_FIFO_SIZE; //防止一边发一边判断时,丢失接收的数据
  pBuf += Index;
  while(pHw->SR & LPC_TNF){//未满时
    pHw->DR = *pBuf;
    Index++;    
    if(Index >= Count) break;//完成了
    EnCount--;
    if(!EnCount) break;//完成了
    pBuf++;
  }
  return Index;
}

//----------------------数据从FIFO弹出函数------------------------------
//返回下个数据位置,Index = Count表示出完
static unsigned short _PopFIFO(unsigned short Index,
                               unsigned short Count,
                               unsigned char *pBuf,
                               LPC_SSP_TypeDef *pHw)
{
  pBuf += Index;
  while(pHw->SR & LPC_RNE){//接收未空时
    *pBuf = pHw->DR;
    Index++;    
    if(Index >= Count) break;//完成了
    pBuf++;
  }
  return Index;
}

//------------------------清空FIFO函数--------------------------------
//返回下个数据位置,Index = Count表示出完
static unsigned short _ClrRxFIFO(unsigned short Index,
                               unsigned short Count, //需要清除多少个,>=1;
                               LPC_SSP_TypeDef *pHw)
{
  unsigned short Data;
  while(pHw->SR & LPC_RNE){
    Data = pHw->DR; //未空时
    Index++;
    if(Index >= Count) break;//完成了
  }
  Data = Data;
  return Index;
}

//----------------------Spi设备初始化函数------------------------------
void SpiDev_Init(struct _SpiDev *pDev,
                 void *pSpiHw)  //挂接的Spi硬件设备,这里无效
{
  memset(pDev, 0, sizeof(struct _SpiDev));
  pDev->pSpiHw = pSpiHw;
  LPC_SSP_TypeDef *pHw = pSpiHw;
  //初始化硬件设备,(IO口配置在外部实现)
  //使能SPI并置为主机模式
  pHw->CR1 = LPC_SSE;// 注:CR0及波特率CPSR等由外部控制 回环测试时| LPC_LBM;
}

//----------------------Spi设备启动函数------------------------------
//用于重启Spi接口
//返回定义为:-1:设备用,0:成功
signed char SpiDev_Restart(struct _SpiDev *pDev,
                           struct _SpiDevCmd *pCmd)   //挂接的命令
{
  if(pDev->Flag & SPI_DEV_BUSY) return -1;//Spi占用中
  
  LPC_SSP_TypeDef *pHw = pDev->pSpiHw;  
  if((pHw->SR & (LPC_TFE | LPC_RNE)) != LPC_TFE){//SPI异常,先停止
    SpiDev_Stop(pDev);
  }
  
  //提前开始SPI周期以提高效率
  pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_CS_VALID); //片选选中
  pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_CMD_CFG); //配置命令发送格式
  
  signed char FinalFlag;
  //中断模式时,发送一个数
  if(pCmd->Cfg & SPI_DEV_CMD_INTERPOS){
    pHw->DR = *pCmd->pCmd;
    pDev->Index = 1;
    FinalFlag = 1;//完成
  }
  else{//非中断模式时,一次性送可能的数据
    pDev->Index = _PushFIFO(0, pCmd->CmdSize, pCmd->pCmd, pHw);//命令入
    if(pDev->Index < pCmd->CmdSize)//未完成
      FinalFlag = 0;
    else FinalFlag = 1;//发送完成
  }

  //初始化相关
  pDev->RxIndex = 0;
  pDev->pCmd = pCmd;
  pDev->Timer = SPI_DEV_TIMER_OV;
  pDev->Flag = SPI_DEV_BUSY;

  //最后开中断
  #ifndef  SUPPORT_SPI_DEV_QUREY
    if(FinalFlag) pHw->IMSC = LPC_RTIM; //命令完成超时时产生中断
    else pHw->IMSC =LPC_TXIM; //数据发送过程中
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
  LPC_SSP_TypeDef *pHw = pDev->pSpiHw;  
  //读写FIFO使数据清空
  //for(unsigned char i = 0; i < 16; i++){
  //  while(pHw->SR & LPC_LPC_TFE)break;//为空了
    //pHw->DR = 0xff;//怎么样清除写入FIFO中的数据???
  //}
  //for(unsigned char i = 0; i < 16; i++){
  //  while(pHw->SR & LPC_RNE)break;//为空了
  //  volatile int dump = pHw->DR;//假读清空
  //}
  //假读清空
  while(pHw->SR & LPC_RNE){ volatile int Dump = pHw->DR;};
  
  //写1将其它可能引起的中断条件清空
  pHw->ICR = LPC_RORIC | LPC_RTIC;  
  
  //重启SPI实现停止并复位
  pHw->CR1 &= ~LPC_SSE;//关闭SSP
  pDev->pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_CS_INVALID); //片选取消
  //使能SPI并置为主机模式
  pHw->CR1 |= LPC_SSE;
  pDev->Flag = 0;
};

//----------------------Spi中断处理函数------------------------------
void SpiDev_IRQ(struct _SpiDev *pDev)
{
  struct _SpiDevCmd *pCmd = pDev->pCmd;
  LPC_SSP_TypeDef *pHw = pDev->pSpiHw; 
  
  unsigned char Flag = pDev->Flag;
  unsigned char Cfg = pCmd->Cfg;
  unsigned short Index = pDev->Index;
  
  //====================发送命令模式处理===========================
  if(!(Flag & SPI_DEV_CMD_FINAL)){
    //发送命令同时允许接收返回信息时,先收发送命令字同时返回的数据
    unsigned short RxIndex = pDev->RxIndex;
    if(Cfg & SPI_DEV_CMD_EN_CMD_RCV)
      RxIndex = _PopFIFO(RxIndex, pCmd->CmdSize, pCmd->pCmd, pHw);
    else RxIndex = _ClrRxFIFO(RxIndex, pCmd->CmdSize, pHw);
    if(RxIndex < pCmd->CmdSize){ //命令接收未完成
      pDev->RxIndex = RxIndex;
      if(Index < pCmd->CmdSize){//发送没完成
        if(Cfg & SPI_DEV_CMD_INTERPOS){//中断模式
          pHw->DR = *(pCmd->pCmd + Index);
          pDev->Index++;
        }
        else{//FIFO模式
          Index = _PushFIFO(Index, pCmd->CmdSize, pCmd->pCmd, pHw);//命令入
          if(Index >= pCmd->CmdSize) ////命令发送完成,但未接收完,等待收完再中断
            pHw->IMSC = LPC_RTIM; //命令完成超时时产生中断完整发送完数据
          pDev->Index = Index;
        }
      }
      else{//接收未完,但没数送了
        pHw->IMSC = LPC_RTIM; //命令完成超时时产生中断完整发送完数据
      }
      //中断模式时调用函数处理
      if(Cfg & SPI_DEV_CMD_INTERPOS) pCmd->cbEndInt(pDev);
      goto End;      
    }
    else _ClrRxFIFO(0,255, pHw);//命令接收完成,强制清空接收FIFO
    
    //命令读写完成了
    Index = 0;
    pDev->Index = Index;
    pDev->RxIndex = Index;
    Flag |= SPI_DEV_CMD_FINAL;//进入数据处理状态
    pDev->Flag = Flag;
    pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_DATA_CFG); //配置数据发送格式
  }
  //====================收数据模式处理===========================
  else{
    //数据已发送完成,异常处理
    //if(Flag & SPI_DEV_DATA_FINAL){
    //  //用于恢复中断
    //  goto End;
    //}
    unsigned short RxIndex = pDev->RxIndex;
    if(!(Cfg & SPI_DEV_CMD_DIS_RX))
      RxIndex = _PopFIFO(RxIndex, pCmd->DataSize, pCmd->pData, pHw);
    else RxIndex = _ClrRxFIFO(RxIndex, pCmd->DataSize, pHw);
    if(RxIndex >= pCmd->DataSize) //数据接收完成
      _ClrRxFIFO(0,255, pHw);//数据接收完成,强制清空接收FIFO
    pDev->RxIndex = RxIndex;
  }
  //有数据区发送时,发送或接收数据未完成
  if((pCmd->DataSize) && (Index < pCmd->DataSize)){
    if(Cfg & SPI_DEV_CMD_INTERPOS){//中断模式
      pHw->DR = *(pCmd->pData + Index);
      pDev->Index++;
    }
    else{//FIFO模式
      Index = _PushFIFO(Index, pCmd->DataSize, pCmd->pData, pHw);//数据入
      if(Index < pCmd->DataSize)//数据发送未完成
        pHw->IMSC =LPC_TXIM; //数据发送过程中
      else //数据发送完成,但未接收完,等待收完再中断
        pHw->IMSC = LPC_RTIM; //数据完成超时时产生中断完整发送完数据
      pDev->Index = Index;
    }
    //中断模式时调用函数处理
    if(Cfg & SPI_DEV_CMD_INTERPOS) pCmd->cbEndInt(pDev);
  }
  else{//无数据区或数据发送完成
    Flag &= ~SPI_DEV_BUSY;//取消忙标志
    pDev->Flag = Flag | SPI_DEV_DATA_FINAL;//完成
    pCmd->cbCfg(pDev, SPI_DEV_CALL_POS_CS_INVALID); //片选取消
    pHw->ICR = LPC_RORIC | LPC_RTIC;  //清除中断
    pHw->IMSC = 0;// clr LPC_TXIM | LPC_RXIM;   //关中断结束
    pDev->Timer = 0;//时间结束
    pCmd->cbEndInt(pDev);//最后回调
    return;
  }

  End: //结束处理,清中断等
  pHw->ICR = LPC_RORIC | LPC_RTIC;  //清除中断 
  return;
}















