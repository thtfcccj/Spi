/******************************************************************************

			        通用SPI主机设备驱动程序接口
 此接口支持多路相同类型的SPI设备
 此接口支持同一SPI设备上,通过普通IO作片选的多个从设备的通讯
******************************************************************************/

#ifndef __SPI_DEV_H
#define __SPI_DEV_H

/******************************************************************************
                             相关结构
******************************************************************************/

//---------------------------Spi命令字定义-------------------------------------
//一个命令字表示一路Spi总线上的一个物理设备,每个物理设备用片选线区分,各设备
//允许有不同的波特效率，时钟极性等不同配置
//命令结构定义
struct _SpiDevCmd{
	unsigned char *pCmd;        //命令字
	unsigned char *pData;       //数据
	unsigned short CmdSize;     //命令大小,>= 1;
	unsigned short DataSize;    //数据大小
	unsigned char  Cs;         //用于区分Spi设备
	unsigned char  Cfg;        //相关标志,见定义
  //配置函数，后跟调用所在位置标志，见定义,中断内调用
  void(*cbCfg)(void*, unsigned char);
  void(*cbEndInt)(void*); //定义的结束处理函数,中断内调用
};

//其中,相关标志定义为:
//工作方式:
#define     SPI_DEV_CMD_MODE_MASK  0x03 //具体定义为:
#define     SPI_DEV_CMD_DUPLEX    0x00 //全双工工作机制,发送时即接收,会覆盖接收缓冲区
#define     SPI_DEV_CMD_DIS_RX    0x01 //不接收数据,即半双工发送,将数据点数据发出
#define     SPI_DEV_CMD_DIS_TX    0x02 //不发送数据,即半双工接收,数据点数据为接收

//人工干预模式,否则为自动工作模式
//自动工作模式在Spi启动后自动完成Spi周期处理,结束时才调用cbEndInt函数
//人工干预模式每次处理完一个命令或数据后均调用cbEndInt交由用户处理，可用于慢速器件工作、
//设备调试等应用
#define     SPI_DEV_CMD_INTERPOS 0x04
//发送命令同时允许接收返回信息,注意此位置位时，返回的信息将覆盖命令字!
#define     SPI_DEV_CMD_EN_CMD_RCV 0x08

//后跟调用所在位置标志cbCfg定义为:
//片选有效，此时用户需将该设备选中
#define SPI_DEV_CALL_POS_CS_VALID   0
//片选无效，此时用户需将该设备选中
#define SPI_DEV_CALL_POS_CS_INVALID 1
//发送命令时调用配置,此时用户需配置发送命令所需的相关寄存器(如:波特率,数据个数,触发方式等)
#define SPI_DEV_CALL_POS_CMD_CFG    2
//发送命令时调用配置,此时用户需配置发送数据所需的相关寄存器(如:波特率,数据个数,触发方式等)
#define SPI_DEV_CALL_POS_DATA_CFG   3

//---------------------------Spi设备定义-------------------------------------
//Spi设备表示一个物理Spi硬件接口，一个Spi硬件接口允许带多个Spi设备(以片选区分),
struct _SpiDev{
  void *pSpiHw;       //挂接的Spi硬件设备
  struct _SpiDevCmd *pCmd;  //Spi命令
  unsigned char Timer;      //定时器，用于防止Spi故障
  unsigned char Index;      //标识当位状态位置
  unsigned char RxIndex;    //接收位置,部分芯片需要
  volatile unsigned char Flag;       //相关标志,见定义
};

//其中,相关标志定义为:
#define SPI_DEV_BUSY       0x80    //Spi忙标志
#define SPI_DEV_CMD_FINAL  0x40   //Spi命令发送完标志,置此标志表示在发送数据状态
#define SPI_DEV_DATA_FINAL 0x20   //Spi数据发送完标志,置此标志表示SPI数据已处理完成

#define SPI_DEV_ERR        0x08   //Spi故障标志
#define SPI_DEV_OV_ERR     0x04   //Spi超时故障标志

/******************************************************************************
                             相关函数
******************************************************************************/

//----------------------Spi设备初始化函数------------------------------
void SpiDev_Init(struct _SpiDev *pDev,
                 void *pSpiHw);  //挂接的Spi硬件设备


//----------------------Spi设备启动函数------------------------------
//用于重启Spi接口
//返回定义为:-1:设备被占用,0:成功
signed char SpiDev_Restart(struct _SpiDev *pDev,
                        struct _SpiDevCmd *pCmd);   //挂接的命令

//----------------------Spi设备任务函数------------------------------
//用于强制停止Spi设备,必要时(程序强制等待)需在硬件定时器Tick中断里调用
void SpiDev_Task(struct _SpiDev *pDev);

//----------------------Spi设备停止函数------------------------------
//强制停止Spi设备
void SpiDev_Stop(struct _SpiDev *pDev);

//---------------------判断是否忙------------------------------
//void SpiDev_IsFinal(const struct _SpiDev *pDev);
#define SpiDev_IsFinal(pdev) ((pdev)->Flag & SPI_DEV_BUSY)

#endif // #define __SPI_DEV_H

