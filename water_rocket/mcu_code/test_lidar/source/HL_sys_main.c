/** @file HL_sys_main.c 
*   @brief Application main file
*   @date 07-July-2017
*   @version 04.07.00
*
*   This file contains an empty main function,
*   which can be used for the application.
*/

/* 
* Copyright (C) 2009-2016 Texas Instruments Incorporated - www.ti.com  
* 
* 
*  Redistribution and use in source and binary forms, with or without 
*  modification, are permitted provided that the following conditions 
*  are met:
*
*    Redistributions of source code must retain the above copyright 
*    notice, this list of conditions and the following disclaimer.
*
*    Redistributions in binary form must reproduce the above copyright
*    notice, this list of conditions and the following disclaimer in the 
*    documentation and/or other materials provided with the   
*    distribution.
*
*    Neither the name of Texas Instruments Incorporated nor the names of
*    its contributors may be used to endorse or promote products derived
*    from this software without specific prior written permission.
*
*  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS 
*  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT 
*  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
*  A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT 
*  OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, 
*  SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT 
*  LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
*  DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
*  THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT 
*  (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE 
*  OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
*
*/


/* USER CODE BEGIN (0) */
/* USER CODE END */

/* Include Files */

#include "HL_sys_common.h"

/* USER CODE BEGIN (1) */
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
/* USER CODE END */

/** @fn void main(void)
*   @brief Application main function
*   @note This function is empty by default.
*
*   This function is called after startup.
*   The user can use this function to implement the application.
*/

/* USER CODE BEGIN (2) */
#include "HL_i2c.h"
#include "HL_system.h"
#include "HL_sys_core.h"
#include "HL_sci.h"

#define UART                  sciREG1
#define LIDAR_SLAVE_ADDR      0x62              // 여기부터 아래 define값들은 lidar 데이터시트보고 구성

#define ACQ_COMMAND           0x0
#define STATUS                0x1
#define SIG_COUNT_VAL         0x2
#define ACQ_CONFIG_REG        0x4

#define OUTER_LOOP_COUNT      0x11
#define MEASURE_DELAY         0x45
#define REF_COUNT_VAL         0x12

#define THRESHOLD_BYPASS      0x1c
#define READ_FROM             0x8f              //------------? datasheet에 0x8f부터 2바이트 읽으라고 되어있음.

#define FULL_DELAY_HIGH       0x0f

uint8 receives[2];
volatile int flag;

uint8 bias_cnt;
uint8 avr_cnt;
uint16 avr_sum;

void sci_display(sciBASE_t *sci, uint8 *text, uint32 len);
// void pwm_set(void);
void wait(uint32);
void lidar_without_bias(void);
void lidar_bias(void);
void get_data(void);
void lidar_enable(void);

void wait(uint32 delay)
{
    int i;

    for(i = 0; i < delay; i++)
        ;
}

void sci_display(sciBASE_t *sci, uint8 *text, uint32 len)
{
    while(len--)
    {
        while((UART->FLR & 0x4) == 4)
            ;
        sciSendByte(sci, *text++);
    }
}

void disp_set(char *str)
{
    char buf[128] = {0};
    unsigned int buf_len;
    sprintf(buf, str);
    buf_len = strlen(buf);
    sci_display(sciREG1, (uint8 *)buf, buf_len);
    wait(1000000);
}

/* USER CODE END */

int main(void)
{
/* USER CODE BEGIN (3) */
    char buf[128] = {0};
    unsigned int buf_len;
    volatile int i;

    sciInit();

    disp_set("sci config success!!!\n\r\0");

    i2cInit();
    wait(100000);

    disp_set("i2c config success!!!\n\r\0");

    lidar_enable();

    disp_set("lidar enable success!!!\n\r\0");

    wait(100000);

    for(;;)
    {
        get_data();


#if 0
        if(flag)
        {
            uint16 tmp;
            tmp = receives[0] << 8;
            tmp |= receives[1];

            sprintf(buf, "Distance = %d\n\r\0", tmp);
            buf_len = strlen(buf);
            sci_display(sciREG1, (uint8 *)buf, buf_len);

            flag = 0;
        }
#endif

        lidar_without_bias();
        bias_cnt++;


        if(bias_cnt == 10)
            {
                avr_cnt++;
                uint16 tmp;
                lidar_bias();
                bias_cnt = 0;

                tmp = receives[0] << 8;
                tmp |= receives[1];
                avr_sum += tmp;

                if(avr_cnt == 5)
                {
                    avr_sum = avr_sum / avr_cnt;
                    sprintf(buf, "Distance = %d\n\r\0", avr_sum);
                    buf_len = strlen(buf);
                    sci_display(sciREG1, (uint8 *)buf, buf_len);
                    avr_cnt = 0;
                    avr_sum = 0;
                }
                flag = 0;
            }
      }

/* USER CODE END */

    return 0;
}

/* USER CODE BEGIN (4) */

void lidar_enable(void)
{
    // 내부 레지스트 설정.
    uint8 tmp[7] = {0xf0, 0x0c, 0x60, 0x04, 0x00, 0x14, 0x20};

    volatile unsigned int cnt = 13; //컴파일러가 멋대로 최적화를 해서 변수값이 변하는걸 방지하기 위해 volatile 사용
    i2cSetSlaveAdd(i2cREG2, LIDAR_SLAVE_ADDR);
    i2cSetDirection(i2cREG2, I2C_TRANSMITTER);

    i2cSetCount(i2cREG2, cnt + 1);
    i2cSetMode(i2cREG2, I2C_MASTER);
    i2cSetStop(i2cREG2);
    i2cSetStart(i2cREG2);

    i2cSendByte(i2cREG2, SIG_COUNT_VAL); //상관계수!
    i2cSend(i2cREG2, 1, &tmp[0]);

    i2cSendByte(i2cREG2, ACQ_CONFIG_REG);
    i2cSend(i2cREG2, 1, &tmp[1]);

    i2cSendByte(i2cREG2, THRESHOLD_BYPASS);
    i2cSend(i2cREG2, 1, &tmp[2]);

    i2cSendByte(i2cREG2, ACQ_COMMAND);
    i2cSend(i2cREG2, 1, &tmp[3]);

    i2cSendByte(i2cREG2, OUTER_LOOP_COUNT);
    i2cSend(i2cREG2, 1, &tmp[4]);

    i2cSendByte(i2cREG2, MEASURE_DELAY);
    i2cSend(i2cREG2, 1, &tmp[5]);

    i2cSendByte(i2cREG2, REF_COUNT_VAL);
    i2cSend(i2cREG2, 1, &tmp[6]);

    disp_set("Lidar Default Initial Success!!!\n\r\0");

    while(i2cIsBusBusy(i2cREG2) == true)
        ;
    while(i2cIsStopDetected(i2cREG2) == 0)
        ;

    i2cClearSCD(i2cREG2);

    wait(100000);

}


void get_data(void)
{
    i2cSetSlaveAdd(i2cREG2, LIDAR_SLAVE_ADDR);
    i2cSetDirection(i2cREG2, I2C_TRANSMITTER);
    i2cSetCount(i2cREG2, 1);
    i2cSetMode(i2cREG2, I2C_MASTER);
    i2cSetStop(i2cREG2);
    i2cSetStart(i2cREG2);
    i2cSendByte(i2cREG2, READ_FROM);

    while(i2cIsBusBusy(i2cREG2) == true)
        ;
    while(i2cIsStopDetected(i2cREG2) == 0)
        ;

    i2cClearSCD(i2cREG2);

    i2cSetDirection(i2cREG2, I2C_RECEIVER);
    i2cSetCount(i2cREG2, 2);
    i2cSetMode(i2cREG2, I2C_MASTER);
    i2cSetStart(i2cREG2);
    i2cReceive(i2cREG2, 2, (unsigned char *)receives);
    i2cSetStop(i2cREG2);

    while(i2cIsBusBusy(i2cREG2) == true)
        ;
    while(i2cIsStopDetected(i2cREG2) == 0)
        ;
    i2cClearSCD(i2cREG2);

    flag = 1;
}

void lidar_bias(void)
{
    volatile unsigned int cnt = 1;
    unsigned char data[1] = {0x04U};

    i2cSetSlaveAdd(i2cREG2, LIDAR_SLAVE_ADDR);
    i2cSetDirection(i2cREG2, I2C_TRANSMITTER);
    i2cSetCount(i2cREG2, cnt + 1);
    i2cSetMode(i2cREG2, I2C_MASTER);
    i2cSetStop(i2cREG2);                    // 필요할꺼임. I2C는 표준이 없다. = 맞추어줘야할 포맷이 있따.
    i2cSetStart(i2cREG2);
    i2cSendByte(i2cREG2, ACQ_COMMAND);
    i2cSend(i2cREG2, cnt, data);
    i2cSetStop(i2cREG2);

    while(i2cIsBusBusy(i2cREG2) == true)
            ;
    while(i2cIsStopDetected(i2cREG2) == 0)
            ;

    i2cClearSCD(i2cREG2);

    wait(100000);

}

void lidar_without_bias(void)
{
    volatile unsigned int cnt = 1;
    unsigned char data[1] = {0x03U};

    i2cSetSlaveAdd(i2cREG2, LIDAR_SLAVE_ADDR);
    i2cSetDirection(i2cREG2, I2C_TRANSMITTER);
    i2cSetCount(i2cREG2, cnt + 1);
    i2cSetMode(i2cREG2, I2C_MASTER);
    i2cSetStop(i2cREG2);
    i2cSetStart(i2cREG2);
    i2cSendByte(i2cREG2, ACQ_COMMAND);
    i2cSend(i2cREG2, cnt, data);
    i2cSetStop(i2cREG2);

    while(i2cIsBusBusy(i2cREG2) == true)
        ;
    while(i2cIsStopDetected(i2cREG2) == 0)
        ;

    i2cClearSCD(i2cREG2);

    wait(100000);

}

