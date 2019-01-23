#include "HL_sys_common.h"
#include "HL_system.h"
#include "HL_gio.h"
#include "HL_rti.h"
#include "HL_sci.h"

#define UART sciREG1
#define NSIZE 7
#define TSIZE 6

uint32 cnt,cnt2,start;
uint8 num[NSIZE]={0,0,0,0,0,'\n','\r'};
uint8 text[TSIZE]={'T','I','M','E',':','\t'};

void sci_display(sciBASE_t *sci, uint8 *text, uint32 len) // uint8 (8byte )
{
    while(len--)
    {
        while((UART->FLR & 0x4) == 4) // if not busy 4면   2번비트
        ;  //바쁘면 계속 루프 도는 것.
        sciSendByte(UART, *text++);
    }
}

void wait(uint32 time)
{
    int i;
    for(i=0;i<time;i++)
    ;
}

int main(void)
{
    gioInit();
    gioSetDirection(gioPORTA,0xFFFFFFFF);
    gioSetDirection(gioPORTB,0x00000040);
    sciInit();
    wait(1000);
    rtiInit();
    wait(1000);
    rtiEnableNotification(rtiREG1, rtiNOTIFICATION_COMPARE0);
    _enable_IRQ_interrupt_();
    rtiStartCounter(rtiREG1,rtiCOUNTER_BLOCK0);
    wait(1000);
    for(;;)
    {
        num[0]=(cnt/10000)%10+48;
        num[1]=(cnt/1000)%10+48;
        num[2]=(cnt/100)%10+48;
        num[3]=(cnt/10)%10+48;
        num[4]=cnt%10+48;
        sci_display(UART, &text[0],TSIZE);
        wait(10000);
        sci_display(UART,&num[0],NSIZE);
        wait(10000);
    }
    return 0;
}

void rtiNotification(rtiBASE_t *rtiREG, uint32 notification)
{
    if(start==1)
    {

        if(cnt2==5){
            gioSetBit(gioPORTB,6,gioGetBit(gioPORTB,6)^0x01);
            gioSetBit(gioPORTA,0,0);
            start=0;
            cnt2=0;
        }

        cnt2++;

    }

    if(cnt%200==0){
    gioSetBit(gioPORTB,6,gioGetBit(gioPORTB,6)^0x01);
    gioSetBit(gioPORTA,0,1);
    start=1;
    }

    cnt++;

    if(gioGetBit(gioPORTB,4)==0)
        rtiStopCounter(rtiREG1,rtiCOUNTER_BLOCK0);

}


