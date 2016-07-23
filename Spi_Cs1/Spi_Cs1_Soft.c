/******************************************************************************

   通用SPI主机设备驱动程序接口->单Spi设备实例+单Spi从机时在软件IO中的实例实现	

//此实现使用查询模式
//支持主从CS互换
//此文件可为模板,可根据应用进行修改
******************************************************************************/

#include "Spi_Cs1.h"
//#include <avr/IO.h>  //AVRGCC环境时
#include <string.h>  //NULL

struct _SpiDev2 SpiDev2;   //实例设备
struct _Spi Spi;

//---------------------SpiDev2Cmd配置函数实现--------------------------
static void _cbCfg(void *pSpiDev, //struct _SpiDev *pSpiDev,   //挂接设备
                   unsigned char CallPos)    //调用位置
{
  //使用硬件直接实现
  switch(CallPos){
    case SPI_DEV2_CALL_POS_CS_VALID: //置为输出低电平状态
	    ClrSpiCs();
	    OutSpiCs();
	  break;
    case SPI_DEV2_CALL_POS_CS_INVALID://置为输入上拉状态
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
static void _cbEndInt(void *pSpiDev) //struct _SpiDev *pSpiDev)
{
  
};



//------------------------SPI初始化函数------------------------
void Spi_Init (void)
{
  CfgSpiCs();
  
  memset(&Spi, 0, sizeof(struct _Spi));  
  SpiDev2_Init(&SpiDev2, &Spi.SpiDev_Soft);
  //Cmd内部初始化
  //Spi.Cmd.Cs = 0;  
  Spi.Cmd.pCmd = Spi.CmdBuf;
  Spi.Cmd.pData = Spi.DataBuf;
  Spi.Cmd.cbCfg = _cbCfg;
  Spi.Cmd.cbEndInt = _cbEndInt;
  
  //SetSpiCs();    //取消片选
  
  //设置时钟速率->已由驱动实现
  
}
