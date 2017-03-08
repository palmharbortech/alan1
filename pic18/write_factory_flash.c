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


unsigned char read_status_reg(void) {
    spi_fact_cs = 0; // select

    spi_send(0x05);
    unsigned char status_reg = spi_recv();

    spi_fact_cs = 1; // deselect
    return status_reg;
}


void write_factory_flash(void) {
    spi_user_cs = 1; // deselect USER chip
    spi_user_cs_tri = 0; // output (0)

    spi_fact_cs = 1; // deselect FACT chip
    spi_fact_cs_tri = 0; // output (0)

    spi_out_tri = 0; // output (0)
    spi_clk_tri = 0; // output (0)

    led_prog = 1; // turn off
    led_prog_tri = 0; // output (0)

    pickit_cts_write = 0; // init low
    pickit_cts_tri = 0; // output (0)

    ANSELH = 0x00; // set inputs to digital
    ANSEL = 0x00; // set inputs to digital
    switch_prog_tri = 1; // input (1)
    spi_in_tri = 1; // input (1)
    pickit_pgc_tri = 1; // input (1)
    //pickit_pgd_tri = 1; // input (1)

    // wait for the button to be pressed
    for (;;) {
        unsigned char cnt = 0;
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

    led_prog = 0; // turn on

    spi_fact_cs = 0; // select
    spi_send(0x06); // Write Enable
    spi_fact_cs = 1; // deselect

    // Poll Status Register
    unsigned char status_reg;
    for (;;) {
        status_reg = read_status_reg();
        unsigned char rdy = (status_reg >> 0) & 1;
        if (rdy == 0)
            break;
    }
    
    spi_fact_cs = 0; // select
    spi_send(0x60); // Chip Erase
    spi_fact_cs = 1; // deselect
    
    // Poll Status Register to see when erase is complete
    for (;;) {
        status_reg = read_status_reg();
        unsigned char rdy = (status_reg >> 0) & 1;
        if (rdy == 0)
            break;
    }

    // wait for the button to be released
    for (;;) {
        unsigned char cnt = 0;
        bool button_was_released = false;
        while (switch_prog == 1) { // button released
            if (++cnt > 5) {
                button_was_released = true;
                break;
            }

            //__delay_ms(1);
        }

        if (button_was_released)
            break;
    }

    led_prog = 1; // turn off

    spi_fact_cs = 0; // select
    spi_send(0x06); // Write Enable
    spi_fact_cs = 1; // deselect

    // Poll Status Register
    for (;;) {
        status_reg = read_status_reg();
        unsigned char rdy = (status_reg >> 0) & 1;
        if (rdy == 0)
            break;
    }

    spi_fact_cs = 0; // select
    spi_send(0x02); // opcode for Byte Program
    spi_send(0x00); // address byte 1
    spi_send(0x00); // address byte 2
    spi_send(0x00); // address byte 3

    // listen on pgc/pgd for data
    unsigned char bits_received = 0;
    unsigned char bytes_received = 0;
    unsigned int page_number = 0;
    unsigned char last_pickit_pgc_read = pickit_pgc_read;
    for (;;) {
        // Read both pgc and pgd as close together as possible
        unsigned char this_pickit_pgc_read = pickit_pgc_read;
        unsigned char this_pickit_pgd_read = pickit_pgd_read;

        if (last_pickit_pgc_read == 0 && this_pickit_pgc_read == 1) {
            led_prog = 0; // turn on
            // transitioned low to high
            last_pickit_pgc_read = this_pickit_pgc_read;

            // tell sender they are clear-to-send
            pickit_cts_write = 1;
        } else if (last_pickit_pgc_read == 1 && this_pickit_pgc_read == 0) {
            led_prog = 1; // turn off
            // transitioned high to low
            last_pickit_pgc_read = this_pickit_pgc_read;

            // write into factory flash

            // switch to next page if needed
            if (bits_received++ == 8) {
                bits_received = 1;
                if (++bytes_received == 0) { // rolls over after 255
                    ++page_number;

                    spi_fact_cs = 1; // deselect

                    // Poll Status Register to see when program is complete
                    for (;;) {
                        status_reg = read_status_reg();
                        unsigned char rdy = (status_reg >> 0) & 1;
                        if (rdy == 0)
                            break;
                    }

                    spi_fact_cs = 0; // select
                    spi_send(0x06); // Write Enable
                    spi_fact_cs = 1; // deselect

                    // Poll Status Register
                    for (;;) {
                        status_reg = read_status_reg();
                        unsigned char rdy = (status_reg >> 0) & 1;
                        if (rdy == 0)
                            break;
                    }

                    spi_fact_cs = 0; // select
                    spi_send(0x02); // opcode for Byte Program
                    spi_send((page_number >> 8) & 0xff); // address byte 1
                    spi_send(page_number & 0xff); // address byte 2
                    spi_send(0x00); // address byte 3
                }
            }

            if (this_pickit_pgd_read)
                spi_out = 1;
            else
                spi_out = 0;

            // pulse clock
            spi_clk = 0;
            spi_clk = 1;

            // tell sender they are clear-to-send
            pickit_cts_write = 0;
        } else {
            // break if button press
            unsigned char cnt = 0;
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
    }

    led_prog = 0; // turn on

    spi_fact_cs = 1; // deselect

    // Poll Status Register to see when program is complete
    for (;;) {
        status_reg = read_status_reg();
        unsigned char rdy = (status_reg >> 0) & 1;
        if (rdy == 0)
            break;
    }

    /*
    spi_fact_cs = 0; // select
    spi_send(0x04); // Write Disable
    spi_fact_cs = 1; // deselect
    */

    // 100ms
    for (uint8_t i = 0; i < 10; ++i)
        __delay_ms(10);

    led_prog = 1; // turn off
}


void main(void) {
    write_factory_flash();

    // Don't ever return from main, otherwise the pic will reboot. It's crafty like that.
    for (;;) {
    }
}
