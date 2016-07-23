/******************************************************************************

   通用SPI主机设备驱动程序接口->单Spi设备-在Lpc_Arm中的实例实现
//此接口不含SpiDev中回调函数实现(由各设备自已实现,如:CS553X驱动)

//使用中断模式
//此文件为模板,可根据应用进行修改
******************************************************************************/

#include "Spi.h"
#include "LPC12XX.h"
#include "LPC12XXbit.h"

struct _SpiDev SpiDev[SPI_DEV_COUNT];

/***************************************************************************
                           相关函数实现
***************************************************************************/

//------------------------SPI初始化函数------------------------
void Spi_Init (void)
{
  //1.初始化电源控制及时钟
  //LPC_SYSCON->PDRUNCFG &= ~XXX_PD;    //禁止掉电
  LPC_SYSCON->PRESETCTRL |= SSP_RST_N; //集中复位
  LPC_SYSCON->SYSAHBCLKCTRL |= SSP_ACC;//开启时钟
  
  //2.初始化管脚配置:SPI硬件模式,注:驱动不管片选,即这里不用写
  LPC_IOCON->PIO0_14 = 0x2 | 0x90;
  //LPC_IOCON->PIO0_15 = 0x0 | 0x90; //IO输出上拉模式,外部配置
  LPC_IOCON->PIO0_16 = 0x2 | 0x90;
  LPC_IOCON->PIO0_17 = 0x2 | 0x90;

  //3.初始化其它:

  //4.初始化变量及模块
  SpiDev_Init(&SpiDev[0], LPC_SSP);
  
  //5.最后允许中断
  NVIC_EnableIRQ(SSP_IRQn);
}

//---------------------中断处理程序-----------------------
void SSP0_IRQHandler (void)
{
  SpiDev_IRQ(&SpiDev[0]); 
  //LPC_BASE_VIC->VectAddr = 0;  //清中断,LPC21XX有效
}