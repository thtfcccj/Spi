/******************************************************************************

   通用SPI主机设备驱动程序接口->单Spi设备实例+单Spi从机时在AVR中的实例实现	

//此实现使用查询模式
//支持主从cs互换
//此文件可为模板,可根据应用进行修改
******************************************************************************/

#include "Spi_Cs1.h"
#include <string.h>

struct _SpiDev SpiDev;   //实例设备
struct _Spi Spi;

//---------------------SpiDevCmd配置函数实现--------------------------
static void _cbCfg(void *pSpiDev, //struct _SpiDev *pSpiDev,   //挂接设备
                   unsigned char CallPos)    //调用位置
{
  //使用硬件直接实现
  switch(CallPos){
    case SPI_DEV_CALL_POS_CS_VALID: //置为输出低电平状态
	    ClrSpiCs();
	    OutSpiCs();
      Spi.Timer = SPI_DATA_SPACE;   //设置间隔
	  break;
    case SPI_DEV_CALL_POS_CS_INVALID://置为输入上拉状态
	    SetPullUpSpiCs();
      InSpiCs();
	  break;
    //case SPI_DEV_CALL_POS_CMD_CFG:
    //case SPI_DEV_CALL_POS_DATA_CFG:
    default: //命令与发送数据格式相同
      //SPCR |= (1<<SPR1); //设置时钟速率固定为fck/64
    break;
  }
}

//------------------------------SpiDevCmd中断完成回调函数实现-----------------
static void _cbEndInt(void *pSpiDev)
{
  
};

//------------------------SPI初始化函数------------------------
void Spi_Init(void)
{
  CfgSpiCs();
  SpiDev_Init(&SpiDev, NULL);
  memset(&Spi, 0, sizeof(struct _Spi));
  
  //Cmd内部初始化
  //Spi.Cmd.Cs = 0;  
  Spi.Cmd.pCmd = Spi.CmdBuf;
  Spi.Cmd.pData = Spi.DataBuf;
  Spi.Cmd.cbCfg = _cbCfg;
  Spi.Cmd.cbEndInt = _cbEndInt;
  
  SPCR |= (1<<SPR1) | (1<<SPR0) | (1<<CPOL) | (1<<CPHA); //设置时钟速率固定为fck/128
}

//----------------------------快速任务函数--------------------------------
extern void SpiDev_IRQ(void); //查询模式时:中断处理函数,在SpiDev_Avr.c里实现
//放入系统快速任务中查询
void Spi_FastTask(void)
{
  if(Spi.Timer){//延时
    Spi.Timer--;
    return;
  }
  
  //用查询方式实现中断调用防止影响通讯
  if(SpiDev.Flag & SPI_DEV_BUSY){//SPI工作过程中时
    if(SPSR & (1<<SPIF)){//中断标志置位
      SpiDev_IRQ(); //进行中断处理
      Spi.Timer = SPI_DATA_SPACE; //字节间延时
      //注：中断标志在读取SPSR及SPDR后将自动复位
    }
  }
}

