#include <xc.h>
#include <pic18f4520.h>
#include <stdint.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "conbits.h"
#include "uart_layer.h"

#define RTC_WRITE_ADDR   (0xD0)
#define RTC_READ_ADDR    (0xD1)

uint8_t uart_data = 0;
bool uart_got_data_bool = false;
uint8_t print_buffer[256] = {0};

typedef struct{
    
    uint8_t secL:4;
    uint8_t secH:3;
    uint8_t hold_clock:1;
    
    uint8_t minL:4;
    uint8_t minH:3;
    uint8_t rfu1:1;
    
    uint8_t hourL:4;
    uint8_t hourH:2;
    uint8_t hour_mode:1;
    uint8_t rfu2:1;
    
    uint8_t day:3;
    uint8_t rfu3:5;
    
    uint8_t dateL:4;
    uint8_t dateH:2;
    uint8_t rfu4:2;
    
    uint8_t monthL:4;
    uint8_t monthH:1;
    uint8_t rfu5:3;
    
    uint8_t yearL:4;
    uint8_t yearH:4;

}rtc_time_read;

rtc_time_read rtc_data;

void __interrupt() high_isr(void);
void __interrupt(low_priority) low_isr(void);

void i2c_is_idle(void){
    while((SSPCON2 & 0x1F) || (SSPSTAT & 0x04) );
}

void i2c_start(void){
    i2c_is_idle();
    SSPCON2bits.SEN = 1;
}

void i2c_rep_start(void){
    i2c_is_idle();
    SSPCON2bits.RSEN = 1;
}

void i2c_stop(void){
    i2c_is_idle();
    SSPCON2bits.PEN = 1;
}

void i2c_write(uint8_t i2c_data){
    i2c_is_idle();
    SSPBUF = i2c_data;
    while(SSPSTATbits.BF != 0);
    while(SSPCON2bits.ACKSTAT != 0);
}

uint8_t i2c_read(uint8_t ack){
    uint8_t recieve =0;
    i2c_is_idle();
    SSPCON2bits.RCEN = 1;
    while(SSPSTATbits.BF != 1);
    recieve = SSPBUF;
    SSPCON2bits.ACKEN = ack;
    return recieve;
}

void i2c_init(void){
    TRISCbits.RC3 = 1;
    TRISCbits.RC4 = 1;
    
    SSPSTATbits.SMP = 1;
    SSPSTATbits.CKE = 0;
    SSPCON1bits.SSPM = 0x08;
    SSPADD = 19;
    SSPCON1bits.SSPEN = 1;
    
}

void main(void) {
    uint8_t rtc_buff[7] = {0};
    OSCCONbits.IRCF = 0x07;
    OSCCONbits.SCS = 0x03;
    while(OSCCONbits.IOFS!=1);
    
    memset(&rtc_data,0,sizeof(rtc_data));
    
    rtc_data.secH = 0;
    rtc_data.secH = 0;
    
    rtc_data.minL = 5;
    rtc_data.minH = 1;
    
    rtc_data.hourL = 3;
    rtc_data.hourH = 2;
    
    rtc_data.day = 7;
    
    rtc_data.dateL = 7;
    rtc_data.dateH = 1;
    
    rtc_data.monthL = 1;
    rtc_data.monthH = 1;
    
    rtc_data.yearL = 8;
    rtc_data.yearH = 1;
    
    memcpy(rtc_buff,&rtc_data,sizeof(rtc_data));
  
    __delay_ms(2000);
    TRISB=0;
    LATB=0;
    
    uart_init(51,0,1,0);//baud 9600
    sprintf(print_buffer,"\n\rprogram start\n\r");
    uart_send_string(print_buffer);
    
    

    RCONbits.IPEN = 1;
    INTCONbits.GIEH = 1; 
    INTCONbits.GIEL = 1;
    
    i2c_init();
    
    i2c_start();
    i2c_write(RTC_WRITE_ADDR);
    i2c_write(0x00);
    i2c_write(rtc_buff[0]);
    i2c_write(rtc_buff[1]);
    i2c_write(rtc_buff[2]);
    i2c_write(rtc_buff[3]);
    i2c_write(rtc_buff[4]);
    i2c_write(rtc_buff[5]);
    i2c_write(rtc_buff[6]);
    i2c_stop();
    
    
    
    
    sprintf(print_buffer,"\n\r");
    uart_send_string(print_buffer);
    
    while(1){
        
        if(uart_got_data_bool){
            uart_got_data_bool = false;
        }
        i2c_start();
        i2c_write(RTC_WRITE_ADDR);
        i2c_write(0x00);
        i2c_rep_start();
        i2c_write(RTC_READ_ADDR);
        rtc_buff[0] = i2c_read(1);
        rtc_buff[1] = i2c_read(1);
        rtc_buff[2] = i2c_read(1);
        rtc_buff[3] = i2c_read(1);
        rtc_buff[4] = i2c_read(1);
        rtc_buff[5] = i2c_read(1);
        rtc_buff[6] = i2c_read(0);
        i2c_stop();
        memcpy(&rtc_data,rtc_buff,sizeof(rtc_data));
        sprintf(print_buffer,"\r20%d%d/%d%d/%d%d %d%d:%d%d:%d%d",
                rtc_data.yearH,
                rtc_data.yearL,
                rtc_data.monthH,
                rtc_data.monthL,
                rtc_data.dateH,
                rtc_data.dateL,
                rtc_data.hourH,
                rtc_data.hourL,
                rtc_data.minH,
                rtc_data.minL,
                rtc_data.secH,
                rtc_data.secL
                );
        uart_send_string(print_buffer);
        __delay_ms(500);
        
    } 
}

void __interrupt() high_isr(void){
    INTCONbits.GIEH = 0;
    if(PIR1bits.RCIF){
        uart_receiver(&uart_data,&uart_got_data_bool);
       PIR1bits.RCIF=0;
    }
    
    INTCONbits.GIEH = 1;
}

void __interrupt(low_priority) low_isr(void){
    INTCONbits.GIEH = 0;
    
    INTCONbits.GIEH = 1;
}



