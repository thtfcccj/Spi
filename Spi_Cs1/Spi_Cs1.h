/******************************************************************************

      通用SPI主机设备驱动程序接口->单Spi设备实例+单Spi从机时的实例接口
//此接口直接实现了SpiDev的片选(SpiCs引脚)和波特率等,可用于简单Spi通讯

//此接口部分为应用相关，即允许根据应用，对此接口进行少量配置与修改
//此接口可为模板,当含多Spi设备实例,或挂有多个Spi从机时,可使用此模板重新设计
******************************************************************************/
#ifndef __SPI_CS1_H
#define __SPI_CS1_H

/******************************************************************************
                             相关配置
******************************************************************************/

#define SPI_DATA_SPACE             16       //定义数据间的间隔组从机处理数据

#define SPI_CMD_BUF_SIZE           1        //定义命令缓冲区大小
#define SPI_DATA_BUF_SIZE          8        //定义数据缓冲区大小

/******************************************************************************
                             相关结构
******************************************************************************/
#include"IOCtrl.h"    //需实现的接口见“ 相关回调函数”说明
#include"SpiDev.h"    //Spi设备定义
struct _Spi{
  //SPI驱动相关
  struct _SpiDevCmd Cmd;  //SPI设备命令字
  unsigned char CmdBuf[SPI_CMD_BUF_SIZE];   //设备命令字的命令缓冲区
  unsigned char DataBuf[SPI_DATA_BUF_SIZE];  //用于读写长整理数据内部使用
  unsigned char Timer;       //字节间延时定时器
};

extern struct _SpiDev SpiDev;   //实例设备
extern struct _Spi Spi;

/******************************************************************************
                             相关函数
******************************************************************************/

//---------------------------初始化函数---------------------------
void Spi_Init (void);

//------------------------------快速任务函数----------------------
//放入系统快速任务中查询
void Spi_FastTask(void);

//---------------------------检查SPI是否空闲中----------------------
//unsigned char Spi_IsIdie(void);
//直接检查程序是否将SPI CS置为高电平
#define Spi_IsIdie()  (IsOutSpiCs() && IsSetSpiCs())

//-------------------------判断Cs是否为高电平----------------------
//unsigned char Spi_IsCs(void);
#define Spi_IsCs()   IsSetSpiCs()

/******************************************************************************
                             相关回调函数
******************************************************************************/
//IOCtrl.h中的标准IO接口,需在其中实现片选的宏函数有:
//方向(为主从机互换时的定义)
//#define InSpiCs()			do{DDR_SpiCs &= ~BIT_SpiCs;}while(0)
//#define OutSpiCs()		do{DDR_SpiCs |= BIT_SpiCs;}while(0)
//#define IsOutSpiCs()		(DDR_SpiCs & BIT_SpiCs)
//输出时,高低电平
//#define SetSpiCs()		do{PORT_SpiCs |= BIT_SpiCs;}while(0)
//#define ClrSpiCs()		do{PORT_SpiCs &= ~BIT_SpiCs;}while(0)
//#define IsSetSpiCs()		(PORT_SpiCs & BIT_SpiCs)
//输入时,是否为高电平
//#define IsSpiCs()		        (PIN_SpiCs & BIT_SpiCs)
//输入时,上接电阻(主从机可互换时定义)
//#define SetPullUpSpiCs()		do{PORT_SpiCs |= BIT_SpiCs;}while(0)
//#define ClrPullUpSpiCs()		do{PORT_SpiCs &= ~BIT_SpiCs;}while(0)
//#define IsPullUpSpiCs()	    (PORT_SpiCs & BIT_SpiCs)

//初始化及配置为:,输出高电平状态(主从机可互换时,输入时应提前置上拉状态)
//#define CfgSpiCs() do{SetSpiCs();OutSpiCs();}while(0)


#endif //#define __SPI_H

