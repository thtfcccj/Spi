/******************************************************************************

   通用SPI主机设备驱动程序接口->Spi设备实例接口
//此接口不含SpiDev中回调函数实现(由各设备自已实现,如:CS553X驱动)

******************************************************************************/
#ifndef __SPI_H
#define __SPI_H

/**********************************************************************
                        用户配置说明
**********************************************************************/

//定义I2C接口数量,在系统配置里预定义,未定义是为1
//#define SPI_DEV_COUNT   2  //或更多,但需在SPI_???里具体实现

/**********************************************************************
                        相关结构
**********************************************************************/

#ifndef SPI_DEV_COUNT
  #define SPI_DEV_COUNT    1
#endif

#include"SpiDev.h"    //Spi设备定义
extern struct _SpiDev SpiDev[SPI_DEV_COUNT];

/******************************************************************************
                             相关函数
******************************************************************************/

//------------------------初始化函数------------------------
void Spi_Init (void);

#endif //#define __SPI_H

