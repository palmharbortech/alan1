/*
 * File:   main.c
 * Author: bshiflet
 *
 * Created on October 16, 2016, 11:46 AM
 */

#pragma config FCMEN = OFF, IESO = OFF                      // CONFIG1H
#pragma config FOSC = ECH // External Clock, high power
#pragma config BOREN = OFF, BORV = 30                       // CONFIG2L
#pragma config WDTEN = OFF, WDTPS = 32768                   // CONFIG2H
#pragma config MCLRE = OFF                                  // CONFIG3H
#pragma config STVREN = ON, LVP = OFF, XINST = OFF          // CONFIG4L
#pragma config CP0 = OFF, CP1 = OFF                         // CONFIG5L
#pragma config CPB = OFF, CPD = OFF                         // CONFIG5H
#pragma config WRT0 = OFF, WRT1 = OFF                       // CONFIG6L
#pragma config WRTB = OFF, WRTC = OFF, WRTD = OFF           // CONFIG6H
#pragma config EBTR0 = OFF, EBTR1 = OFF                     // CONFIG7L
#pragma config EBTRB = OFF                                  // CONFIG7H

#define _XTAL_FREQ 60000000 // for __delay_ms

#include <xc.h>
#include <stdbool.h>
#include <stdint.h>
//#include <stdio.h>

#define spi_clk          LATBbits.LATB6
#define spi_clk_tri      TRISBbits.TRISB6
#define spi_out          LATCbits.LATC7
#define spi_out_tri      TRISCbits.TRISC7
#define spi_in           PORTAbits.RA4
#define spi_in_tri       TRISAbits.TRISA4
#define spi_fact_cs      LATBbits.LATB7
#define spi_fact_cs_tri  TRISBbits.TRISB7
#define spi_user_cs      LATBbits.LATB5
#define spi_user_cs_tri  TRISBbits.TRISB5
#define switch_prog      PORTCbits.RC2
#define switch_prog_tri  TRISCbits.TRISC2
#define led_prog         LATCbits.LATC1
#define led_prog_tri     TRISCbits.TRISC1
#define fact_done        PORTCbits.RC0
#define fact_done_tri    TRISCbits.TRISC0
#define fpga_dclk_read   PORTCbits.RC4
#define fpga_dclk_write  LATCbits.LATC4
#define fpga_dclk_tri    TRISCbits.TRISC4
#define fpga_data0_read  PORTCbits.RC3
#define fpga_data0_write LATCbits.LATC3
#define fpga_data0_tri   TRISCbits.TRISC3
#define fpga_nconfig     LATCbits.LATC6
#define fpga_nconfig_tri TRISCbits.TRISC6
#define fpga_nstatus     PORTCbits.RC5
#define fpga_nstatus_tri TRISCbits.TRISC5
#define fpga_conf_done     PORTBbits.RB4
#define fpga_conf_done_tri TRISBbits.TRISB4
#define pickit_pgc_read  PORTAbits.RA1
#define pickit_pgc_tri   TRISAbits.TRISA1
#define pickit_pgd_read  PORTAbits.RA3
//#define pickit_pgd_tri   TRISAbits.TRISA3
#define pickit_cts_write LATAbits.LATA0
#define pickit_cts_tri   TRISAbits.TRISA0


void spi_send(unsigned char data) {
    for (int i = 0; i < 8; i++) {
        // consider leftmost bit
        // set line high if bit is 1, low if bit is 0
        if (data & 0x80)
            spi_out = 1;
        else
            spi_out = 0;

        // pulse clock to indicate that bit value should be read
        spi_clk = 0;
        spi_clk = 1;

        // shift byte left so next bit will be leftmost
        data <<= 1;
    }
}


unsigned char spi_recv(void) {
    unsigned char data = 0;
    for (int i = 7; i >= 0; --i) {
        spi_clk = 0;
        data |= spi_in << i;
        spi_clk = 1;
        //data |= spi_in << i; // this also works
    }

    return data;
}


// pic programs fpga from flash using Passive Serial (PS) mode
void program_fpga_from_flash(bool factory) {
    fpga_dclk_tri = 0; // output (0)
    fpga_data0_tri = 0; // output (0)

    // To begin the configuration, the external host device must generate a low-to-high transition on the nCONFIG pin
    fpga_nconfig = 0;
    __delay_ms(1); // The nCONFIG pin must be low for at least 500 ns
    fpga_nconfig = 1;

    while (fpga_nstatus == 0); // wait for nSTATUS to be pulled high

    for (int page = 0; page < 2048; ++page) {
        if (factory)
            spi_fact_cs = 0; // select
        else
            spi_user_cs = 0; // select

        spi_send(0x0b); // opcode for Read Array
        spi_send((page >> 8) & 0xff); // address byte 1
        spi_send(page & 0xff); // address byte 2
        spi_send(0x00); // address byte 3
        spi_send(0x00); // dummy byte

        for (int i = 0; i < 256; ++i) {
            unsigned char data = spi_recv();

            // Write data to fpga (LSB of each data byte)
            for (int j = 0; j < 8; ++j) {
                // consider rightmost bit
                if (data & 0x01)
                    fpga_data0_write = 1;
                else
                    fpga_data0_write = 0;

                // pulse clock to indicate that bit value should be read
                fpga_dclk_write = 0;
                fpga_dclk_write = 1;

                // shift byte right so next bit will be rightmost
                data >>= 1;
            }

            // After the device receives all the configuration data, the device releases the
            // open-drain CONF_DONE pin, which is pulled high by an external 10k pull-up resistor. 
            if (fpga_conf_done == 1)
                break;
        }

        if (factory)
            spi_fact_cs = 1; // deselect
        else
            spi_user_cs = 1; // deselect

        if (fpga_conf_done == 1)
            break;
    }

    while (fpga_conf_done == 0); // wait for CONF_DONE to be pulled high

    // Two DCLK falling edges are required after CONF_DONE goes high to begin the initialization of the device.
    fpga_dclk_write = 0;
    fpga_dclk_write = 1;
    fpga_dclk_write = 0;
    fpga_dclk_write = 1;

    // INIT_DONE is released and pulled high when initialization is complete
    //while (init_done == 0);

    fpga_dclk_tri = 1; // input (1)
    fpga_data0_tri = 1; // input (1)
}


void program_fpga_from_factory_flash() {
    program_fpga_from_flash(true);
}


void program_fpga_from_user_flash() {
    program_fpga_from_flash(false);
}


void bootloader(void) {
    //INTCON2bits.RCPU = 0; // enable PORTC internal pullups
    //WPUCbits.WPUC2 = 1;   // enable pull up on RC2

    ANSELH = 0x00; // set inputs to digital
    ANSEL = 0x00; // set inputs to digital
    switch_prog_tri = 1; // input (1)
    fact_done_tri = 1; // input (1)
    fpga_dclk_tri = 1; // input (1)
    fpga_data0_tri = 1; // input (1)
    led_prog = 1; // turn off
    led_prog_tri = 0; // output (0)
    spi_user_cs = 1; // deselect USER chip
    spi_user_cs_tri = 0; // output (0)
    spi_fact_cs = 1; // deselect FACT chip
    spi_fact_cs_tri = 0; // output (0)
    spi_out = 0; // start off low
    spi_out_tri = 0; // output (0)
    spi_clk = 0; // start off low
    spi_clk_tri = 0; // output (0)
    fpga_nconfig = 0; // start off low
    fpga_nconfig_tri = 0; // output (0)
    fpga_nstatus_tri = 1; // input (1)
    fpga_conf_done_tri = 1; // input (1)

    if (switch_prog == 1) { // button not pushed
        //program_fpga_from_user_flash(); // TODO: put this back
    }

    // Game loop
    for (;;) {
        unsigned char cnt = 0;
        while (switch_prog == 0) { // button pushed
            if (++cnt > 5) {
                led_prog = 0; // turn on

                program_fpga_from_factory_flash();

                // tmp
                led_prog = 1; // turn off
                break;
                // end tmp

                spi_user_cs = 0; // select
                spi_send(0x06); // Write Enable
                spi_user_cs = 1; // deselect

                spi_user_cs = 0; // select
                spi_send(0x02); // opcode for Byte Program
                spi_send(0x00); // address byte 1
                spi_send(0x00); // address byte 2
                spi_send(0x00); // address byte 3

                unsigned char last_fpga_dclk = fpga_dclk_read;
                for (;;) {
                    // fpga will push data to pic using DCLK and DATA0, pic will push data to user flash using spi
                    unsigned char this_fpga_dclk = fpga_dclk_read;
                    if (last_fpga_dclk == 0 && this_fpga_dclk == 1) {
                        // transitioned low to high
                        last_fpga_dclk = this_fpga_dclk;
                    } else if (last_fpga_dclk == 1 && this_fpga_dclk == 0) {
                        // transitioned high to low
                        last_fpga_dclk = this_fpga_dclk;

                        // write fpga_data0 to spi
                        if (fpga_data0_read)
                            spi_out = 1;
                        else
                            spi_out = 0;

                        // pulse clock
                        spi_clk = 0;
                        spi_clk = 1;
                    }

                    // fpga will hold fact_done high until pic re-programs fpga
                    if (fact_done == 0)
                        break;

                    cnt = 0;
                    bool button_was_pushed = false;
                    while (switch_prog == 0) { // button pushed
                        if (++cnt > 5) {
                            button_was_pushed = true;
                            break;
                        }

                        //__delay_ms(1);
                    }

                    if (button_was_pushed)
                        break;
                }

                spi_user_cs = 1; // deselect

                spi_user_cs = 0; // select
                spi_send(0x04); // Write Disable
                spi_user_cs = 1; // deselect

                program_fpga_from_user_flash();
                led_prog = 1; // turn off
                break;
            }

            // It would be interesting to speed test the reading of switch_prog, if it's slow, then these delays are not necessary
            //__delay_ms(1);
        }

        //__delay_ms(1);
    }
}


void main(void) {
    bootloader();

    // Don't ever return from main, otherwise the pic will reboot. It's crafty like that.
    for (;;) {
    }
}
