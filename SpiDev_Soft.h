/******************************************************************************

                   SPI设备抽像-软件IO实现结构及接口
//仅支持主机模式
//支持位长度控制功能,用于与非8bit对齐的SPI设备通讯
//允许用户根据情况重新配置硬件IO控制与其它回调函数,但强烈建议不修改而直接实现宏
//注:此接口与SpiDev的接口无关,但用于SpiDev的软件IO实现
******************************************************************************/

#ifndef __SPI_DEV_SOFT_H
#define __SPI_DEV_SOFT_H

/******************************************************************************
                             相关结构
******************************************************************************/

//---------------------------Spi设备定义-------------------------------------
//Spi设备表示一个物理Spi硬件接口，一个Spi硬件接口允许带多个Spi设备(以片选区分),
struct _SpiDev_Soft{
  volatile unsigned char Cfg;        //相关标志,见定义
  unsigned char LastBitLen;          //位长度控制功能:最后字节位长度
  unsigned char BitDelay;            //位间电平保持时间,具体与系统时钟与调用用关系
  unsigned char ByteDelay;           //字节间电平保持时间,具体与系统时钟与调用用关系
};

//其中,相关标志定义为:
#define SPI_DEV_SOFT_CPOL_H     0x80     //空闲时SCK为高电平标志,否则为低电平
#define SPI_DEV_SOFT_CPHA_END   0x40     //SCK在结束沿采样,起始沿置数，否则相反
#define SPI_DEV_SOFT_DORD_LSB   0x20     //字节发送顺序:置位LSB先发,否则反之

/******************************************************************************
                     相关用户底层控制函数
******************************************************************************/

//-------------------------------初始化函数-------------------------------
//void SpiDev_Soft_Init(struct _SpiDev_Soft *pDev);
//这里直接定义默认状态
#define SpiDev_Soft_Init(pdev) \
  do{(pdev)->Cfg = SPI_DEV_SOFT_CPOL_H | SPI_DEV_SOFT_CPHA_END; \
    (pdev)->LastBitLen = 8; (pdev)->BitDelay = 10; (pdev)->ByteDelay = 50;}while(0)

//----------------------Spi软设备配置置位函数------------------------------
//void SpiDev_Soft_CfgSet(struct _SpiDev_Soft *pDev,
//                        unsigned char Cfg);
#define SpiDev_Soft_CfgSet(pdev, cfg) do{(pdev)->Cfg |= cfg;}while(0)

//----------------------Spi软设备配置清除函数------------------------------
//void SpiDev_Soft_CfgClr(struct _SpiDev_Soft *pDev,
//                        unsigned char Cfg);
#define SpiDev_Soft_CfgClr(pdev, cfg) do{(pdev)->Cfg &= ~(cfg);}while(0)

//----------------------Spi软设备置延时函数------------------------------
//void SpiDev_Soft_SetDelay(struct _SpiDev_Soft *pDev,
//                          unsigned short Delay);
#define SpiDev_Soft_SetDelay(pdev, delay) do{(pdev)->Dealy = delay;}while(0)

//----------------------Spi软设备置最后字节位长度函数------------------
//void SpiDev_Soft_SetLastBitLen(struct _SpiDev_Soft *pDev,
//                                  unsigned char LastBitLen);
#define SpiDev_Soft_SetLastBitLen(pdev, cmdlastbitlen) \
  do{(pdev)->LastBitLen = cmdlastbitlen;}while(0)

/******************************************************************************
                             硬件IO控制回调函数
//注:不含片选控制
******************************************************************************/
//直接实现:
#include "IOCtrl.h"

//--------------------------IO初始化--------------------------------
//void SpiDev_Soft_cbIoInit(struct _SpiDev_Soft *pDev);
#define SpiDev_Soft_cbIoInit(pdev) do{Spi2IoCfg();}while(0)

//--------------------------置位时钟--------------------------------
//void SpiDev_Soft_cbSetClk(struct _SpiDev_Soft *pDev);
#define SpiDev_Soft_cbSetClk(pdev) do{SetSPI2_SCK();}while(0)

//--------------------------清除时钟--------------------------------
//void SpiDev_Soft_cbClrClk(struct _SpiDev_Soft *pDev);
#define SpiDev_Soft_cbClrClk(pdev) do{ClrSPI2_SCK();}while(0)

//--------------------------置位MOSI--------------------------------
//void SpiDev_Soft_cbSetMosi(struct _SpiDev_Soft *pDev);
#define SpiDev_Soft_cbSetMosi(pdev) do{SetSPI2_MOSI();}while(0)

//--------------------------清除MOSI--------------------------------
//void SpiDev_Soft_cbClrMosi(struct _SpiDev_Soft *pDev);
#define SpiDev_Soft_cbClrMosi(pdev) do{ClrSPI2_MOSI();}while(0)

//--------------------------判断MISO电平-----------------------------
//signed char SpiDev_Soft_cbIsSetMiso(struct _SpiDev_Soft *pDev);
#define SpiDev_Soft_cbIsSetMiso(pdev)   (IsSPI2_MISO())


/******************************************************************************
                             其它回调函数
******************************************************************************/

//------------------------延时函数调用-------------------------------
//延时时间与设置的时间对应
//void SpiDev_Soft_cbDelay(unsigned short Delay);
#include "Delay.h"
#define SpiDev_Soft_cbDelay(delay) DelayUs(delay)

//------------------------进入临界区-------------------------------
//void SpiDev_Soft_cbEnterCritical(void);
#define SpiDev_Soft_cbEnterCritical() do{}while(0)

//------------------------退出临界区-------------------------------
//void SpiDev_Soft_cbExitCritical(void);
#define SpiDev_Soft_cbExitCritical() do{}while(0)


#endif // #define __SPI_DEV_SOFT_H

