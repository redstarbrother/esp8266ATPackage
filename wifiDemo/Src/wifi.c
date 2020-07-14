
#include "wifi.h"

Userdatatype Espdatatype;

UART_HandleTypeDef *wifiCom = &huart1;			// ***需要根据实际串口进行修改***

uint8_t persent_mode;
bool atFlag = FALSE;
bool rstFlag = FALSE;
bool modeFlag = FALSE;
bool sendReadyFlag = FALSE;
bool sendOkFlag = FALSE;
bool wifiConnectFlag = FALSE;
bool serverConnectFlag = FALSE;
bool serverCreateFlag = FALSE;
bool hotspotFlag =FALSE;
bool mulConFlag = FALSE;
bool dataAnalyzeFlag = TRUE; // 是否对接收的数据进行解析

//uint8_t data[512];

/* 用户数据处理函数
* 传入参数：用户数据数组，数据长度，控制数据，地址数据
*/
void userToDo(uint8_t *data, uint16_t length, uint16_t con, uint16_t addr)
{
	// write user code here.
}


// 串口中断初始化函数
void EnableUsart_IT(void)
{
    __HAL_UART_ENABLE_IT(wifiCom, UART_IT_RXNE);//开启串口接收中断
    __HAL_UART_ENABLE_IT(wifiCom, UART_IT_IDLE);//开启串口1空闲接收中断
    __HAL_UART_CLEAR_IDLEFLAG(wifiCom);         //清除串口1空闲中断标志位
    HAL_UART_Receive_DMA(wifiCom,Espdatatype.DMARecBuffer,DMA_REC_SIZE);  //使能DMA接收
		HAL_TIM_Base_Start_IT(&htim2);		// 使能定时器
}


// 用户自定义中断处理函数
void USER_UART_Handler(void)
{
	if(__HAL_UART_GET_FLAG(wifiCom,UART_FLAG_IDLE) ==SET)  //触发空闲中断
    {
        uint16_t temp = 0;                        
        __HAL_UART_CLEAR_IDLEFLAG(wifiCom);       //清除串口1空闲中断标志位
        HAL_UART_DMAStop(wifiCom);                //关闭DMA
        temp = wifiCom->Instance->SR;              //清除SR状态寄存器  F0  ISR
        temp = wifiCom->Instance->DR;              //读取DR数据寄存器 F0  RDR    用来清除中断
        temp = hdma_usart1_rx.Instance->CNDTR;   //获取DMA中未传输的数据个数
        Espdatatype.DMARecLen = DMA_REC_SIZE - temp;           //总计数减去未传输的数据个数，得到已经接收的数据个数
        HAL_UART_RxCpltCallback(wifiCom);		  //串口接收回调函数
    }
}

/* wifi初始化函数
*	传入参数：wifi工作模式
*/
void wifiInit(uint8_t MODE)
{
	EnableUsart_IT();
	HAL_Delay(10);
	if(!wifiStart())
		while(1);
	if(MODE == CLIENT)
		clientStart();
	else if(MODE == SERVER)
		serverStart();
	#if(PASSTHROUGH)
		HAL_UART_Transmit(wifiCom, (uint8_t *)"AT+CIPMODE=1\r\n", 14, 0xFFFF); // 设置为透传方式
		HAL_Delay(20);
		HAL_UART_Transmit(wifiCom, (uint8_t *)"AT+CIPSEND\r\n", 12, 0xFFFF); // 开启透传
	#endif
}


// 关闭透传
void closePassThrough(void)
{
	HAL_UART_Transmit(wifiCom, (uint8_t *)"+++", 3, 0xFFFF); // 开启透传
	HAL_Delay(20);
	HAL_UART_Transmit(wifiCom, (uint8_t *)"AT+CIPMODE=0\r\n", 14, 0xFFFF); // 设置为透传方式
}

// 客户端模式初始化
void clientStart(void)
{
	HAL_Delay(1000);
	//Send_AT_commend("ATE1", "", 100);
	HAL_UART_Transmit(wifiCom, (uint8_t *)"AT+CWMODE?", 10, 0xFFFF); // 获取模块当前工作模式
	HAL_UART_Transmit(wifiCom, (uint8_t *)"\r\n", 2, 0xFFFF);	
	HAL_Delay(500);
	if(persent_mode != CLIENT)
	{
		if(Send_AT_commend("AT+CWMODE=1", &modeFlag, 100))
				modeFlag = TRUE;
		if(Send_AT_commend("AT+RST", &rstFlag, 500))
				rstFlag = FALSE;
	}	
	HAL_Delay(2000);
	printf("客户端启动中\r\n");
	if(!wifiConnectFlag)
		Send_AT_commend(strConnect(5, "AT+CWJAP=\"", CLIENT_WIFI_NAME, "\",\"", CLIENT_WIFI_PWD, "\""), &wifiConnectFlag, 5000);
	printf("wifi连接成功\r\n");
	if(Send_AT_commend(strConnect(4, "AT+CIPSTART=\"TCP\",\"", CLIENT_IP, "\",", CLIENT_PORT), &serverConnectFlag, 3000))
		printf("服务器连接成功\r\n");
	return;
	
}

// 服务器模式初始化
void serverStart(void)
{
	HAL_Delay(1000);
		//Send_AT_commend("ATE1", "", 100);
	HAL_UART_Transmit(wifiCom, (uint8_t *)"AT+CWMODE?", 10, 0xFFFF); // 获取模块当前工作模式
	HAL_UART_Transmit(wifiCom, (uint8_t *)"\r\n", 2, 0xFFFF);	
	HAL_Delay(500);
	if(persent_mode != SERVER)
	{
		if(Send_AT_commend("AT+CWMODE=2", &modeFlag, 100))
				modeFlag = TRUE;
		if(Send_AT_commend("AT+RST", &rstFlag, 500))
				rstFlag = FALSE;
	}	
	printf("服务器启动中\r\n");
	if(Send_AT_commend(strConnect(8, "AT+CWSAP=\"", SERVER_WIFI_NAME, "\",\"", SERVER_WIFI_PASSWORD, "\",", SERVER_CHANNEL, ",", SERVER_ENCRYPTION), &hotspotFlag, 2000))
		printf("wifi热点创建成功\r\n");
	#if(MULTIPLE_CON)
		if(Send_AT_commend("AT+CIPMUX=1", &mulConFlag, 200))
			printf("设置为多链接模式\r\n");
	#else
		if(Send_AT_commend("AT+CIPMUX=0", &mulConFlag, 200))
			printf("设置为单链接模式\r\n");
	#endif
	if(Send_AT_commend(strConnect(2, "AT+CIPSERVER=1,", SERVER_PORT), &serverCreateFlag, 200))
		printf("服务器创建成功\r\n");
	HAL_UART_Transmit(wifiCom, (uint8_t *)"AT+CIFSR\r\n", 10, 0xFFFF); // 获取模块本地IP地址
	return;
}

/* 接收数据处理函数
*/
void recDataHandle(void)
{
	if(Espdatatype.AtRecFlag == 1)
	{
		if(findStr("AT\r\r\n\r\nOK"))
			atFlag = TRUE;
		else if(findStr("AT+RST") && findStr("ready"))
			rstFlag = FALSE;
		else if(findStr("AT+CWMODE=") && findStr("OK"))
			modeFlag = TRUE;
		else if(findStr("AT+CWMODE?"))
			persent_mode = Espdatatype.AtBuffer[21]-0x30;
		else if(findStr("WIFI CONNECTED"))
			wifiConnectFlag = TRUE;
		else if((findStr("AT+CIPSTART") && findStr("CONNECT") && findStr("OK")) || findStr("ALREADY CONNECTED"))
			serverConnectFlag = TRUE;
		else if(findStr("AT+CIPSEND") && findStr("OK") && findStr(">"))
			sendReadyFlag = TRUE;
		else if(findStr("SEND OK"))
			sendOkFlag = TRUE;
		else if(findStr("AT+CWSAP=") && findStr("OK"))
			hotspotFlag = TRUE;
		else if(findStr("AT+CIPSERVER") && findStr("OK"))
			serverCreateFlag = TRUE;
		else if(findStr("AT+CIPMUX=1") && findStr("OK"))
			mulConFlag = TRUE;
		else if(findStr("AT+CIPMUX=0") && findStr("OK"))
			mulConFlag = TRUE;
		else if(findStr("AT+CIPMUX") && findStr("link") && findStr("builded"))
			mulConFlag = TRUE;
		else if(findStr("AT+CIFSR") && findStr("OK"))
			;
		//else if(findStr(""))
		#if(DISPLAY_AT_INFO)
				printf((uint8_t *)Espdatatype.AtBuffer);
		#endif
		memset(Espdatatype.AtBuffer, 0, AT_BUFF_SIZE);
		Espdatatype.AtRecFlag = 0;
		Espdatatype.AtRecLen = 0;
	}
	if(Espdatatype.UserRecFlag == 1)
	{
		//printf("接收到数据：%s\r\n", Espdatatype.UserBuffer);
		recDataAnalyze(Espdatatype.UserBuffer);
		memset(Espdatatype.UserBuffer, 0, USER_BUFF_SIZE);
		Espdatatype.UserRecFlag = 0;
		Espdatatype.UserRecLen = 0;
	}
}


/*数据解析函数
*传入参数: 用户数据，发送数据长度
*将接收到来自服务器的数据进行解析，得到有用的部分
*关于校验和的校验方式：~(控制域+地址域+用户数据) + (控制域+地址域+用户数据) = 0xff
*所以传过来的cs值与控制域、地址域、用户数据依次相加再加1结果应为0
*/
void recDataAnalyze(uint8_t *recData)
{
	uint16_t counter = 0, length = 0, con = 0, addr = 0;
	uint8_t cs = 0;
	if(dataAnalyzeFlag) // 默认对接收到的数据进行解析
		while(recData[counter] != NULL)	// 处理多帧数据
		{
			cs = 0;
			if(recData[counter] != 0x68)
				return;
			length = recData[++counter];
			length <<=  8;
			length += recData[++counter];
			uint8_t *userData = (uint8_t *)malloc(sizeof(uint8_t)*(length-4));
			if(recData[counter+1] != 0x68 || recData[counter + length + 3] != 0x16)	// 判断第二个帧头和帧尾是否正确
				return;
			counter += 2;
			con = recData[counter++];		// 获取控制数据高8位
			con <<= 8;									
			con += recData[counter++];// 获取控制数据低8位
			cs += con;
			addr = recData[counter++];
			addr <<= 8;
			addr += recData[counter++];
			cs += addr;
			for(int i=0;i<length-4;i++)
			{
				userData[i] = recData[counter++];
				cs += userData[i];
			}
			if(cs+recData[counter]+1 == 0x00)	// 检验和计算，cs+1=0则校验通过
				return;
			counter += 2;
			userToDo(userData, length-4, con, addr);
			free(userData);
		}
	else
	{
		while(recData[counter] != NULL)counter++;
		uint8_t *userData = (uint8_t *)malloc(sizeof(uint8_t)*(counter));
		for(int i=0;i<counter;i++)
			userData[i] = recData[i];
		userToDo(userData, length-4, con, addr);
		free(userData);
	}
	
	
}


/*数据发送函数
*传入参数: 用户数据，发送数据长度
*性能分析：非透传模式下最快200ms发送一次
*/ 
void sendData(uint8_t *userdata, uint16_t userLength)
{
	printf("发送数据\r\n");
	#if(!PASSTHROUGH)
		sendCommandCreate(userLength);
		while(sendReadyFlag == FALSE);
	#endif
	HAL_UART_Transmit(wifiCom, userdata, userLength, 0xFFFF);
	#if(!PASSTHROUGH)
		while(sendOkFlag == FALSE);
		sendReadyFlag = FALSE;
		sendOkFlag = FALSE;
	#endif
	printf("发送成功\r\n");
	
}


/*数据发送函数（101规约）
*传入参数:控制数据，地址数据，用户数据，发送数据长度
*控制、地址数据为16位数据，用户数据传入值为数组，但只发送指定长度的数据
*帧格式：帧头(68H) 长度(2字节) 帧头(68H) 控制域(2字节) 地址域(2字节) 用户数据(length个字节) 校验位(1字节) 帧尾(16H)
*校验位CS = ~(控制域 + 地址域 + 用户数据)
*数据长度 = 控制域到用户数据结尾的总字节长度
*性能分析：非透传模式下最快300ms发送一次
*/ 

void sendData101(uint16_t con, uint16_t addr, uint8_t *userdata, uint16_t userLength)
{
	uint16_t len = 0;
	uint8_t cs = 0;
	if(userLength>1024)
		return;
	uint8_t *data = (uint8_t *)malloc(sizeof(uint8_t)*(10+userLength));		// 动态申请数组，长度为10+用户数据长度
	//data = (uint8_t *)malloc(sizeof(uint8_t)*16);		// 动态申请数组，长度为10+用户数据长度
	//memset(data, 0, 512);
	data[len++] = 0x68;
	data[len++] = (uint8_t)((userLength+4)>>8);				// 数据长度高8位
	data[len++] = (uint8_t)((userLength+4) & 0x00ff);  // 数据长度低8位
	data[len++] = 0x68;
	data[len++] = (uint8_t)(con >> 8);
	data[len++] = (uint8_t)(con & 0x00ff);
	data[len++] = (uint8_t)(addr >> 8);
	data[len++] = (uint8_t)(addr & 0x00ff);
	for(int i=0; i<userLength; i++)
		data[len++] = userdata[i];
	for(int i=4; i<userLength+8; i++)
		cs += data[i];
	cs = ~cs;
	data[len++] = cs;
	data[len++] = 0x16;
	//data[len] = 0;
	printf("发送数据");
	#if(!PASSTHROUGH)
		sendCommandCreate(userLength+10);
		//HAL_Delay(500);
		while(sendReadyFlag == FALSE);
	#endif
//	for(int i=0;i<userLength+10;i++)
//		HAL_UART_Transmit(&wifiCom, &data[i], 1, 0xFFFF);
	HAL_UART_Transmit(wifiCom, data, userLength+10, 0xFFFF);
	//HAL_UART_Transmit(&wifiCom, "1234567890123456", userLength+10, 0xFFFF);
	#if(!PASSTHROUGH)
		while(sendOkFlag == FALSE);
		sendReadyFlag = FALSE;
		sendOkFlag = FALSE;
	#endif
	printf("发送成功");
	free(data);
}


/* 动态生成并发送“AT+CIPSEND=?”字符串
* 非透传模式下计算发送的数据长度并发送给wifi模块
*/
int counter = 0;
void sendCommandCreate(uint16_t length)
{
	uint8_t j=11,atlen;
	uint8_t len[4] = {0,0,0,0};
	counter =0;
	while(length)
	{
		len[counter++] = (length % 10) + 0x30;
		length /= 10;
	}
	uint8_t *at_send = (uint8_t *)malloc(sizeof(uint8_t)*(counter + 11 + 2));  // counter为发送字节长度 11位“AT+CIPSEND=” 2为“\r\n”
	atlen = counter--;
	memset(at_send, 0, counter + 11 + 2);
	strcat(at_send, "AT+CIPSEND=");
	while(counter>=0)
		at_send[j++] = len[counter--];
	at_send[j++] = 0x0d;
	at_send[j++] = 0x0a;
	at_send[j] = 0;
	HAL_UART_Transmit(wifiCom, at_send, atlen + 11 + 2, 0xFFFF); // 发送指令
	free(at_send);
	return;
}


/*字符串拼接
*传入参数:拼接字符串的个数，变参函数num为可变参数的个数，
*后面依次传入要拼接的字符串变量或常量
*/
uint8_t* strConnect(int num, ...)
{
    uint8_t len = 0;
    uint8_t temp[100] = "", *format;
    va_list arg;
    va_start(arg, num);
    uint8_t* ret = va_arg(arg, uint8_t*);
    for(int i=0; i<num; i++)
    {
        while (*ret)
        {
            temp[len++] = *ret;
            ret++;
        }
        ret = va_arg(arg, uint8_t*);
    }
    temp[len] = '\0';	// 最后一个字节补0，否则会出现乱码
    format = (uint8_t *)malloc(sizeof(uint8_t)*len);
    memset(format, 0, len);
    strcat(format,temp);
    //printf("%s", format);
    va_end(arg);
    return format;
}

/*发送AT指令
*传入参数:at指令，接收数据标志，超时时间
*/
uint8_t Send_AT_commend(char *at_commend, bool *re_flag, uint16_t time_out)
{
	for(uint8_t i=0;i<3;i++)
	{
		HAL_UART_Transmit(wifiCom, (uint8_t *)at_commend, strlen(at_commend), 0xFFFF);
		HAL_UART_Transmit(wifiCom, (uint8_t *)"\r\n", 2, 0xFFFF);				//自动发送回车换行
		HAL_Delay(time_out);
		if(*re_flag)
			return 1;
	}
	return 0;
}

/*查找字符串
*传入参数：需要匹配的字符串
*/
uint8_t findStr(char *a)
{
	
	char *addr = strstr((char *)Espdatatype.AtBuffer,a);
	if(addr)
		return 1;
	return 0;
	
}



/* wifi初始化logo+
*	显示logo，并对ESP8266进行AT指令测试
*/
uint8_t wifiStart(void)
{
	printf("\r\n");
	printf("*****************************************\r\n");
	printf("*                                       *\r\n");
	printf("*    JHX_ESP8266_DEMO    version 1.0    *\r\n");
	printf("*                                       *\r\n");
	printf("*****************************************\r\n");
	for(int i=0;i<=3;i++)
	{
		if(i>=3)
		{
			printf("AT指令测试失败，请检查接线、供电或更换模块\r\n");
			return 0;
		}
		if(Send_AT_commend("AT", &atFlag, 100))
			break;
		if(Send_AT_commend("AT+RST", &rstFlag, 3000))
			rstFlag = FALSE; // 重启模块
		
		HAL_Delay(500);
	}
	//printf("AT指令测试OK");
	return 1;
}
	

/*串口回调函数
*	串口空闲中断时进入回调函数，DMARecBuffer中含有字符串“+IPD”则为用户接收到的数据
*	否则为AT指令回显数据
*/
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart)
{
	char *addr = strstr((char *)Espdatatype.DMARecBuffer,"+IPD");
	uint16_t lenaddr = 0, data_len=0, copyaddr;
	if(huart->Instance == USART1)
	{
		if(addr)
		{
			while(Espdatatype.DMARecBuffer[++lenaddr] != ':');
			while(Espdatatype.DMARecBuffer[--lenaddr] != ',');
			lenaddr++;
			while(Espdatatype.DMARecBuffer[lenaddr] != ':')data_len = data_len*10 + (Espdatatype.DMARecBuffer[lenaddr++]-0x30);
			copyaddr=lenaddr+1;
			if(Espdatatype.UserRecLen>0)      		// 判断用户数据数组中是否有未处理数据      
			{
					memcpy(&Espdatatype.UserBuffer[Espdatatype.UserRecLen], &Espdatatype.DMARecBuffer[copyaddr],data_len); // 转存到用户数据待处理区域
					Espdatatype.UserRecLen +=  data_len;
			}
			else
			{
					memcpy(Espdatatype.UserBuffer, &Espdatatype.DMARecBuffer[copyaddr],data_len);                          // 转存到用户数据待处理区域
					Espdatatype.UserRecLen =  data_len;
			}
			Espdatatype.UserRecFlag = 1;
		}
		else
		{
			if(Espdatatype.AtRecLen>0)           	// 判断AT指令数据数组中是否有未处理数据  
			{
					memcpy(&Espdatatype.AtBuffer[Espdatatype.AtRecLen],Espdatatype.DMARecBuffer,Espdatatype.DMARecLen); 	// 转存到AT指令数据待处理区域
					Espdatatype.AtRecLen +=  Espdatatype.DMARecLen;
			}
			else
			{
					memcpy(Espdatatype.AtBuffer,Espdatatype.DMARecBuffer,Espdatatype.DMARecLen);                          // 转存到AT指令数据待处理区域
					Espdatatype.AtRecLen =  Espdatatype.DMARecLen;
			}
			Espdatatype.AtRecFlag = 1;
		}
	}
	memset(Espdatatype.DMARecBuffer, 0x00, DMA_REC_SIZE);
	
//    if(huart->Instance == USART1)
//    {
//        if(Usart2type.UsartRecLen>0)            
//        {
//            memcpy(&Usart2type.Usart2RecBuffer[Usart2type.UsartRecLen],Usart2type.Usart2DMARecBuffer,Usart2type.UsartDMARecLen); //转存到待处理区域
//            Usart2type.UsartRecLen +=  Usart2type.UsartDMARecLen;
//        }
//        else
//        {
//            memcpy(Usart2type.Usart2RecBuffer,Usart2type.Usart2DMARecBuffer,Usart2type.UsartDMARecLen);                          //转存到待处理区域
//            Usart2type.UsartRecLen =  Usart2type.UsartDMARecLen;
//        }
//        memset(Usart2type.Usart2DMARecBuffer, 0x00, sizeof(Usart2type.Usart2DMARecBuffer));                                     //先清空DMA缓冲区
//        Usart2type.UsartRecFlag = 1;
//    }
}



// 重定向printf
int fputc(int ch, FILE *stream)
{
    /* 堵塞判断串口是否发送完成 */
    while((USART2->SR & 0X40) == 0);

    /* 串口发送完成，将该字符发送 */
    USART2->DR = (uint8_t) ch;

    return ch;
}
