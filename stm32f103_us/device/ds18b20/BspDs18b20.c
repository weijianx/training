//根据 <野火电子> DS18B20芯片驱动修改
#include "config.h"
#include "BspDs18b20.h"



static void                           DS18B20_GPIO_Config                       ( void );
static void                           DS18B20_Mode_IPU                          ( void );
static void                           DS18B20_Mode_Out_PP                       ( void );
static void                           DS18B20_Rst                               ( void );
static uint8_t                        DS18B20_Presence                          ( void );
static uint8_t                        DS18B20_ReadBit                           ( void );
static uint8_t                        DS18B20_ReadByte                          ( void );
static void                           DS18B20_WriteByte                         ( uint8_t dat );
static void                           DS18B20_SkipRom                           ( void );
static void                           DS18B20_MatchRom                          ( void );



 /**
  * @brief  DS18B20 初始化函数
  * @param  无
  * @retval 无
  */
uint8_t DS18B20_Init(void)
{
	DS18B20_GPIO_Config ();
	
	macDS18B20_DQ_1;
	
	DS18B20_Rst();
	
	return DS18B20_Presence ();
	
}


/*
 * 函数名：DS18B20_GPIO_Config
 * 描述  ：配置DS18B20用到的I/O口
 * 输入  ：无
 * 输出  ：无
 */
static void DS18B20_GPIO_Config(void)
{		
	/*定义一个GPIO_InitTypeDef类型的结构体*/
	GPIO_InitTypeDef GPIO_InitStructure;
	

	/*开启macDS18B20_DQ_GPIO_PORT的外设时钟*/
	__HAL_RCC_GPIOD_CLK_ENABLE();           	//开启GPIOD时钟 

	/*选择要控制的macDS18B20_DQ_GPIO_PORT引脚*/															   
  	GPIO_InitStructure.Pin = macDS18B20_DQ_GPIO_PIN;	

	/*设置引脚模式为通用推挽输出*/
  	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;   

	/*设置引脚速率为50MHz */   
  	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH; 

	/*调用库函数，初始化macDS18B20_DQ_GPIO_PORT*/
  	HAL_GPIO_Init ( macDS18B20_DQ_GPIO_PORT , &GPIO_InitStructure );

}

/*
 * 函数名：DS18B20_Mode_IPU
 * 描述  ：使DS18B20-DATA引脚变为输入模式
 * 输入  ：无
 * 输出  ：无
 */
static void DS18B20_Mode_IPU(void)
{
 	  GPIO_InitTypeDef GPIO_InitStructure;

	  	/*选择要控制的macDS18B20_DQ_GPIO_PORT引脚*/	
	  GPIO_InitStructure.Pin = macDS18B20_DQ_GPIO_PIN;

	   /*设置引脚模式为浮空输入模式*/ 
	  GPIO_InitStructure.Mode = GPIO_MODE_INPUT;	

	  /*调用库函数，初始化macDS18B20_DQ_GPIO_PORT*/
	  HAL_GPIO_Init(macDS18B20_DQ_GPIO_PORT, &GPIO_InitStructure);
}


/*
 * 函数名：DS18B20_Mode_Out_PP
 * 描述  ：使DS18B20-DATA引脚变为输出模式
 * 输入  ：无
 * 输出  ：无
 */
static void DS18B20_Mode_Out_PP(void)
{
 	GPIO_InitTypeDef GPIO_InitStructure;

	 	/*选择要控制的macDS18B20_DQ_GPIO_PORT引脚*/															   
  	GPIO_InitStructure.Pin = macDS18B20_DQ_GPIO_PIN;	

	/*设置引脚模式为通用推挽输出*/
  	GPIO_InitStructure.Mode = GPIO_MODE_OUTPUT_PP;   

	/*设置引脚速率为50MHz */   
  	GPIO_InitStructure.Speed = GPIO_SPEED_FREQ_HIGH;

	/*调用库函数，初始化macDS18B20_DQ_GPIO_PORT*/
  	HAL_GPIO_Init(macDS18B20_DQ_GPIO_PORT, &GPIO_InitStructure);
}


/*
 *主机给从机发送复位脉冲
 */
static void DS18B20_Rst(void)
{
	/* 主机设置为推挽输出 */
	DS18B20_Mode_Out_PP();
	
	macDS18B20_DQ_0;
	/* 主机至少产生480us的低电平复位信号 */
	rt_hw_us_delay(750);
	
	/* 主机在产生复位信号后，需将总线拉高 */
	macDS18B20_DQ_1;
	
	/*从机接收到主机的复位信号后，会在15~60us后给主机发一个存在脉冲*/
	rt_hw_us_delay(15);
}


/*
 * 检测从机给主机返回的存在脉冲
 * 0：成功
 * 1：失败
 */
static uint8_t DS18B20_Presence(void)
{
	uint8_t pulse_time = 0;
	
	/* 主机设置为上拉输入 */
	DS18B20_Mode_IPU();
	
	/* 等待存在脉冲的到来，存在脉冲为一个60~240us的低电平信号 
	 * 如果存在脉冲没有来则做超时处理，从机接收到主机的复位信号后，会在15~60us后给主机发一个存在脉冲
	 */
	while( macDS18B20_DQ_IN() && pulse_time<100 )
	{
		pulse_time++;
		rt_hw_us_delay(1);
	}	
	/* 经过100us后，存在脉冲都还没有到来*/
	if( pulse_time >=100 )
		return 1;
	else
		pulse_time = 0;
	
	/* 存在脉冲到来，且存在的时间不能超过240us */
	while( !macDS18B20_DQ_IN() && pulse_time<240 )
	{
		pulse_time++;
		rt_hw_us_delay(1);
	}	
	if( pulse_time >=240 )
		return 1;
	else
		return 0;
}


/*
 * 从DS18B20读取一个bit
 */
static uint8_t DS18B20_ReadBit(void)
{
	uint8_t dat;
	
	/* 读0和读1的时间至少要大于60us */	
	DS18B20_Mode_Out_PP();
	/* 读时间的起始：必须由主机产生 >1us <15us 的低电平信号 */
	macDS18B20_DQ_0;
	rt_hw_us_delay(10);
	
	/* 设置成输入，释放总线，由外部上拉电阻将总线拉高 */
	DS18B20_Mode_IPU();
	//rt_hw_us_delay(2);
	
	if( macDS18B20_DQ_IN() == SET )
		dat = 1;
	else
		dat = 0;
	
	/* 这个延时参数请参考时序图 */
	rt_hw_us_delay(45);
	
	return dat;
}


/*
 * 从DS18B20读一个字节，低位先行
 */
static uint8_t DS18B20_ReadByte(void)
{
	uint8_t i, j, dat = 0;	
	
	for(i=0; i<8; i++) 
	{
		j = DS18B20_ReadBit();		
		dat = (dat) | (j<<i);
	}
	
	return dat;
}


/*
 * 写一个字节到DS18B20，低位先行
 */
static void DS18B20_WriteByte(uint8_t dat)
{
	uint8_t i, testb;
	DS18B20_Mode_Out_PP();
	
	for( i=0; i<8; i++ )
	{
		testb = dat&0x01;
		dat = dat>>1;		
		/* 写0和写1的时间至少要大于60us */
		if (testb)
		{			
			macDS18B20_DQ_0;
			/* 1us < 这个延时 < 15us */
			rt_hw_us_delay(8);
			
			macDS18B20_DQ_1;
			rt_hw_us_delay(58);
		}		
		else
		{			
			macDS18B20_DQ_0;
			/* 60us < Tx 0 < 120us */
			rt_hw_us_delay(70);
			
			macDS18B20_DQ_1;			
			/* 1us < Trec(恢复时间) < 无穷大*/
			rt_hw_us_delay(2);
		}
	}
}


 /**
  * @brief  跳过匹配 DS18B20 ROM
  * @param  无
  * @retval 无
  */
static void DS18B20_SkipRom ( void )
{
	DS18B20_Rst();	   
	
	DS18B20_Presence();	 
	
	DS18B20_WriteByte(0XCC);		/* 跳过 ROM */
	
}


 /**
  * @brief  执行匹配 DS18B20 ROM
  * @param  无
  * @retval 无
  */
static void DS18B20_MatchRom ( void )
{
	DS18B20_Rst();	   
	
	DS18B20_Presence();	 
	
	DS18B20_WriteByte(0X55);		/* 匹配 ROM */
	
}


/*
 * 存储的温度是16 位的带符号扩展的二进制补码形式
 * 当工作在12位分辨率时，其中5个符号位，7个整数位，4个小数位
 *
 *         |---------整数----------|-----小数 分辨率 1/(2^4)=0.0625----|
 * 低字节  | 2^3 | 2^2 | 2^1 | 2^0 | 2^(-1) | 2^(-2) | 2^(-3) | 2^(-4) |
 *
 *
 *         |-----符号位：0->正  1->负-------|-----------整数-----------|
 * 高字节  |  s  |  s  |  s  |  s  |    s   |   2^6  |   2^5  |   2^4  |
 *
 * 
 * 温度 = 符号位 + 整数 + 小数*0.0625
 */
 /**
  * @brief  在跳过匹配 ROM 情况下获取 DS18B20 温度值 
  * @param  无
  * @retval 温度值
  */
float DS18B20_GetTemp_SkipRom ( void )
{
	uint8_t tpmsb, tplsb;
	short s_tem;
	float f_tem;
	
	
	DS18B20_SkipRom ();
	DS18B20_WriteByte(0X44);				/* 开始转换 */
	
	
	DS18B20_SkipRom ();
  DS18B20_WriteByte(0XBE);				/* 读温度值 */
	
	tplsb = DS18B20_ReadByte();		 
	tpmsb = DS18B20_ReadByte(); 
	
	
	s_tem = tpmsb<<8;
	s_tem = s_tem | tplsb;
	
	if( s_tem < 0 )		/* 负温度 */
		f_tem = (~s_tem+1) * 0.0625;	
	else
		f_tem = s_tem * 0.0625;
	
	return f_tem; 	
	
	
}


 /**
  * @brief  在匹配 ROM 情况下获取 DS18B20 温度值 
  * @param  ds18b20_id：用于存放 DS18B20 序列号的数组的首地址
  * @retval 无
  */
void DS18B20_ReadId ( uint8_t * ds18b20_id )
{
	uint8_t uc;
	
	
	DS18B20_WriteByte(0x33);       //读取序列号
	
	for ( uc = 0; uc < 8; uc ++ )
	  ds18b20_id [ uc ] = DS18B20_ReadByte();
	
}


 /**
  * @brief  在匹配 ROM 情况下获取 DS18B20 温度值 
  * @param  ds18b20_id：存放 DS18B20 序列号的数组的首地址
  * @retval 温度值
  */
float DS18B20_GetTemp_MatchRom ( uint8_t * ds18b20_id )
{
	uint8_t tpmsb, tplsb, i;
	short s_tem;
	float f_tem;
	
	
	DS18B20_MatchRom ();            //匹配ROM
	
  for(i=0;i<8;i++)
		DS18B20_WriteByte ( ds18b20_id [ i ] );	
	
	DS18B20_WriteByte(0X44);				/* 开始转换 */

	
	DS18B20_MatchRom ();            //匹配ROM
	
	for(i=0;i<8;i++)
		DS18B20_WriteByte ( ds18b20_id [ i ] );	
	
	DS18B20_WriteByte(0XBE);				/* 读温度值 */
	
	tplsb = DS18B20_ReadByte();		 
	tpmsb = DS18B20_ReadByte(); 
	
	
	s_tem = tpmsb<<8;
	s_tem = s_tem | tplsb;
	
	if( s_tem < 0 )		/* 负温度 */
		f_tem = (~s_tem+1) * 0.0625;	
	else
		f_tem = s_tem * 0.0625;
	
	return f_tem; 	
	
	
}

static int DS18B20Init(void)
{
	DS18B20_Init();
	return RT_EOK;
}
INIT_APP_EXPORT(DS18B20Init);


void GetTemperature(void)
{
	printf ( "\r\nTemperature:%.1f\r\n", DS18B20_GetTemp_SkipRom() );		// 打印DS18B20 获取的温度值	
	
}
MSH_CMD_EXPORT(GetTemperature, Get Ds18b20 Temperature);

/*************************************END OF FILE******************************/
