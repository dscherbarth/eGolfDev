/*
 * File:   main_bal.c
 * Author: dscherbarth
 *
 * Created on January 16, 2015, 6:09 AM
 *
 * Sequence:
 *
 *  top:
 *    Init
 *      While not in alarm
 *          if last op was fet balance wait for batt to settle before reading
 *          collect cell voltage and heatsink temp
 *          if temp too high (and has been for 5 scans) alarm
 *          if volt too high (and has been for 5 scans) alarm
 *          if volt too low (and has been for 5 scans) alarm
 *          if volt over balance setpoint
 *              calc and apply balance fet pwm (for 5 sec) with led on
 *          if voltage and temp are ok led off
 *      end while
 *
 *  alarm:
 *      turn on opto
 *      for 5 seconds
 *          flash led based on alarm state
 *              (temp too high 2 fast)
 *              (volt too high 3 fast)
 *              (volt too low 4 fast)
 *      go to top:
 *
 */

//#include <stdio.h>
//#include <stdlib.h>
#if defined(__XC)
    #include <xc.h>         /* XC8 General Include File */
#elif defined(HI_TECH_C)
    #include <htc.h>        /* HiTech General Include File */
#endif

#include <stdint.h>        /* For uint8_t definition */
#include <stdbool.h>       /* For true/false definition */

#include <delays.h>

#define _XTAL_FREQ  8000000

#define ID	11				// which cell is this (1 - 11)

#define NORMAL      0
#define HIGHVOLT    1
#define BALANCE     2
#define LOWVOLT     3
#define HIGHTEMP    4

#define BALVOLTS    50000
#define LOWVOLTS    40000
#define HIGHVOLTS   55800

#define CONDCOUNT   5           // number of polls condition needs to persist to be counted

#pragma config FOSC = INTOSCIO  // Oscillator Selection bits (INTOSCIO oscillator: I/O function on RA4/OSC2/CLKOUT pin, I/O function on RA5/OSC1/CLKIN)
#pragma config WDTE = OFF       // Watchdog Timer Enable bit (WDT disabled)
#pragma config PWRTE = OFF      // Power-up Timer Enable bit (PWRT disabled)
#pragma config MCLRE = ON       // MCLR Pin Function Select bit (MCLR pin function is MCLR)
#pragma config CP = OFF         // Code Protection bit (Program memory code protection is disabled)
#pragma config CPD = OFF        // Data Code Protection bit (Data memory code protection is disabled)
#pragma config BOREN = ON       // Brown Out Detect (BOR enabled)
#pragma config IESO = ON        // Internal External Switchover bit (Internal External Switchover mode is enabled)
#pragma config FCMEN = ON       // Fail-Safe Clock Monitor Enabled bit (Fail-Safe Clock Monitor is enabled)

void send_4bits (int val)
{
    int i;

    // clock bit on
    GPIO = 4;       //opto on
    __delay_us(200);

    // send the data
    for(i=0; i<4; i++)
    {
	if((val>>i) & 1)
	{
            GPIO = 4;
	}
	else
	{
            GPIO = 5;
	}
    __delay_us(200);
    }
}

// bit pattern of id and status sent serially
void send_status (int condition, unsigned long volts)
{
    int check;

    // indicate activity
    GPIO = 0;       //led on
    __delay_us(300);
    GPIO = 4;       //led off

    // adjust volts to fit in 8 bits
    volts -= LOWVOLTS;
    volts /= 66;    // (HIGVOLTS-LOWVOLTS) / (2 << 8) (~5mv per bit)

    // calculate checksum
    check = (ID ^ condition ^ ((volts >> 4) & 0x0f) ^ (volts & 0x0f) );

    TRISIO = 0x1A;                            //GPIO 0 output
    // bits sent at 5Khz
    // send 4 bits of id
    send_4bits(ID);

    // send 4 bits of status
    send_4bits(condition);

    // send lower 4 bits of voltage
    send_4bits((volts & 0x0f));

    // send upper 4 bits of voltage
    send_4bits(((volts >> 4) & 0x0f));

    // send 4 bits of checksum
    send_4bits(check);

    TRISIO = 0x1B;  //input MCLR and AN0,AN1,AN3

    // done
    GPIO = 4;       //led off
}

void indicate_leds (int led)
{
    static int toggle;

    if (led == NORMAL)       // normal, off
    {
        GPIO = 4;       //led off
    }
    else if (led == HIGHVOLT)  // on high volt
    {
        GPIO = 0;       //led on and gate off
    }
    else if (led == BALANCE)  // flash balancing
    {
        if (toggle)
        {
            toggle = 0;
            GPIO = 0;       //led on and gate off
        }
        else
        {
            toggle = 1;
            GPIO = 4;       //led off and gate off
        }
    }
    else if (led == HIGHTEMP)  // dbl flash for hi temp
    {
        if (toggle)
        {
            toggle = 0;
            GPIO = 0;       //led on and gate off
            __delay_ms(100);
        }
        else
        {
            toggle = 1;
            GPIO = 4;       //led off and gate off
            __delay_ms(100);
        }
    }
    else            // slow flash low volt
    {
        if (toggle)
        {
            toggle = 0;
            GPIO = 0;       //led on and gate off
            __delay_ms(300);
        }
        else
        {
            toggle = 1;
            GPIO = 4;       //led off and gate off
            __delay_ms(300);
        }
    }
}

/*
 *
 */
int main(int argc, char** argv)
{
unsigned long        vbatt, vba;
unsigned long        vtemp;
unsigned int         i, j, vt=65535, vb, tempco = 0, state = 0;
unsigned int        balcount = 0, highcount = 0, hightempcount = 0, lowvoltcount = 0;
int                 on, send = 35, send_rand = 5;
unsigned char        lastop=0;

OSCCON = 0x70;  //internal 8MHz oscillator
OSCTUNE = 0x0;  //frequency tuning to nominal value
CMCON0 = 0x07;
TRISIO = 0x1B;  //input MCLR and AN0,AN1,AN3
GPIO = 0;       //led on and gate off
ANSEL = 0x5B;   //adc clock set and AN0,AN1,AN3
ADCON0 = 0xC1;  //adc ON, right just, vref AN1
vbatt = 0;
vba = 0;
vtemp = 0;

    // indicate power up
    GPIO = 0;       //led on

    __delay_ms(1000);    // power up indicator
    GPIO = 4;       //led off

    // forever processing loop
    while (1)
    {
        // send current state to our marshalling point
        if (--send <= 0)
        {
            send_status (state, vba);
            __delay_us(900);    // let the voltage reference settle after using for comm

            // reset the random sender
            send = (send_rand & 0x0f) + 60; // random from 1.5 to 1.8 seconds
        }

        // show our state on the led
        indicate_leds (state);

        // wait if nec for voltage to settle
        if (lastop)
        {
            lastop = 0;
            __delay_ms(50);    // voltage to settle
        }
        // fetch temperature and battery voltage
        ADCON0 = 0xC1;  //adc ON, right just, vref AN1
        __delay_us(5);
        vbatt = 0;
        for (i=0; i<512; i++)
        {
          ADCON0 |= 2;
          while ((ADCON0 & 2) != 0);
          send_rand = ((ADRESH << 8)+ADRESL);
          vbatt += send_rand;
        }
        vbatt /= 8;
        vb = vbatt;
        vba = ((vba * 19) + vb) / 20;   // long average for status send

        ADCON0 = 0xCD;            //sample the temperature (Vbe for the 2n2222) for 32 times
        __delay_us(5);
        for (i=0; i<32; i++)
        {
          ADCON0 |= 2;
          while ((ADCON0 & 2) != 0);
          vtemp += ((ADRESH << 8)+ADRESL);
        }
        tempco = tempco+1;
        if (tempco == 512)
        {     //sample 512*32 times the temperature (24 bit)
          tempco = 0;
          vtemp /= 256;  //vtemp is on 16 bit, 0..65535
          vt = vtemp;           //vt is the 16-bit variable for the temperature
          vtemp = 0;
        }

        // determine state,  condition has to persist for CONDCOUNT TIMES order of importance low to high:
        //      balance needed, low voltage alarm, high voltage alarm, high temperature
        state = NORMAL;    // off normal

        // needs balance
        if (vb > 55000)
        {
            if (balcount++ > CONDCOUNT)
            {
                state = BALANCE;    // flash balancing
            }
        }
        else
        {
            balcount = 0;
        }

        // low voltage
        if (vb < 40000)
        {
            if (lowvoltcount++ > CONDCOUNT)
            {
                state = LOWVOLT;    // slow flash low volt
                send -= 11;         // account for the slow flash delay
            }
        }
        else
        {
            lowvoltcount = 0;
        }

        // past high limit
        if (vb > 55800)
        {
            if (highcount++ > CONDCOUNT)
            {
                state = HIGHVOLT;    // on high volt
            }
        }
        else
        {
            highcount = 0;
        }

        // over temperature
        if (vb > 50000 && vt < 14000)
        {
            if (hightempcount++ > CONDCOUNT)

            {
                state = HIGHTEMP;    // dbl flash hi temp
                send -= 5;         // account for the slow flash delay
            }
        }
        else
        {
            hightempcount = 0;
        }

        // test for balance action
        if ((state == BALANCE || state == HIGHVOLT))
        {
            // spend 1 second balancing based on how far along we are on the
            // above balance voltage
            on = vb - 55000;
            on /= 10;
            if (on < 0) on = 0; if (on > 80) on = 80;
            for(i=0; i<10; i++)
            {
                GPIO |= 32;
                for(j=0; j<on+10; j++) // up to 90 uS on
                {
                    __delay_us(100);
                }
                GPIO &= 0x1f;
                for(j=0; j<90-on; j++) // should total to 100 uS
                {
                    __delay_us(100);
                }
                if ((i % 3) == 0 && (--send <= 0))
                {
                    send_status (state, vb);

                    // reset the random sender
                    send = (send_rand & 0x0f) + 60; // random from 1.5 to 1.8 seconds
                }
            }
            lastop = 1;
        }
    }
}