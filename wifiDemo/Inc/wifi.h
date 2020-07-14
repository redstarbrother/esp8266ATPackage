#ifndef __wifi_H
#define __wifi_H
#ifdef __cplusplus
 extern "C"
#endif

#include "main.h"
#include <stdio.h>
#include "string.h"
#include "stdarg.h"
#include "stdlib.h"	
#include "tim.h"
//#include "usart.h"


#define DISPLAY_AT_INFO 1			// 是否显示回显数据
#define PASSTHROUGH 0		// 是否为透传模式、只有在*客户端单链接*模式下可打开透传模式！
#define MULTIPLE_CON 1			// 设置单/多链接，默认为多链接(实测只有在多链接模式下才能正常开启服务器)


#define CLIENT_WIFI_NAME "hello_world"			//要连接的wifi热点名
#define CLIENT_WIFI_PWD "876543210"					//热点密码
#define CLIENT_IP "192.168.43.9"		//服务器ip地址
#define CLIENT_PORT "5656"				//端口号


#define SERVER_PORT "5656"					//服务器建立端口
#define SERVER_TIMEOUT 300			//服务器连接超时时间
#define SERVER_WIFI_NAME 	"wifidemo"		//服务器WLAN热点名
#define SERVER_WIFI_PASSWORD 	"12345678"	//服务器WLAN热点密码
#define SERVER_CHANNEL "12"						//服务器WLAN热点信道
#define SERVER_ENCRYPTION "4"					//服务器WLAN热点加密方式 0 OPEN 1 WEP 2 WPA_PSK 3 WPA2_PSK 4 WPA_WPA2_PSK


#define CLIENT 1
#define SERVER 2
#define CS 3


#define DMA_REC_SIZE 1024
#define USER_BUFF_SIZE 512
#define AT_BUFF_SIZE	512


#define WIFISTRCON(s1,s2,s3,s4) s1##XSTR(s2)##s3##XSTR(s4)
#define XSTR(S) STR(S)
#define STR(S) #S


extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern DMA_HandleTypeDef hdma_usart1_rx;
//extern UART_HandleTypeDef huart1;
//extern UART_HandleTypeDef huart2;


typedef struct
{   
	uint8_t  UserRecFlag;  		 //用户数据接收到标志位
	uint8_t  AtRecFlag;  			 //AT数据接收到标志位
	uint16_t DMARecLen; 			 //DMA接受长度
	uint16_t UserRecLen;   		 //用户未处理数据长度
	uint16_t AtRecLen;    		 //AT未处理数据长度
	uint8_t  DMARecBuffer[DMA_REC_SIZE];  //DMAbuffer
	uint8_t  UserBuffer[USER_BUFF_SIZE];         // 用户数据处理buffer
	uint8_t  AtBuffer[AT_BUFF_SIZE];				// AT指令数据处理buffer
}Userdatatype;


typedef enum {FALSE = 0,TRUE = 1} bool;
extern Userdatatype Espdatatype;  //结构体全局定义
extern bool dataAnalyzeFlag; 


void USER_UART_Handler(void);
void EnableUsart_IT(void);
void clientStart(void);
void serverStart(void);
void wifiInit(uint8_t MODE);
uint8_t* strConnect(int num, ...);
uint8_t Send_AT_commend(char *at_commend, bool *re_flag, uint16_t time_out);
uint8_t findStr(char *a);
uint8_t wifiStart(void);
void clientStart(void);
void serverStart(void);
void recDataHandle(void);
void sendData(uint8_t *userdata, uint16_t userLength);
void sendData101(uint16_t con, uint16_t addr, uint8_t *data, uint16_t userLength);
void sendCommandCreate(uint16_t length);
void recDataAnalyze(uint8_t *recData);
#endif


/*
****************wifi_demo v1.0*****************
* 更新日期：2020-7-12
* 版本功能：
* 1.客户端模式下自动连接WiFi热点  
* 2.客户端模式下自动连接服务器  
* 3.服务器模式下自动创建wifi热点  
* 4.服务器模式下自动开启TCP服务器
* 5.服务器模式下输出TCP服务器的IP地址及端口号，DISPLAY_AT_INFO为1的情况下  
* 6.可实时打开和关闭透传模式  
* 7.可实时更改接收数据的解析模式（需要手动更改dataAnalyzeFlag的值）
* 8.提供两种数据封装模式，101协议格式和无封装格式，分别通过调用sendData101、sendData函数实现
* 9.想起来了再写
* 发送模式及其性能（101协议封装为模式1，无封装为模式2）：
* 在非透传模式下，模式1最快300ms/次、模式2最快200ms/次
* 在透传模式下，模式1与模式2都能达到30ms/次，实测50ms/次时串口能够正常回显数据，小于50ms时可能会出错
* *********************************************
*/
