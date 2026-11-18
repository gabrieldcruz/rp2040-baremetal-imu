#include "hardware/regs/addressmap.h"
#include "hardware/regs/resets.h"
#include "hardware/regs/pads_bank0.h"
#include "hardware/regs/io_bank0.h"
#include "hardware/regs/sio.h"
#include "hardware/regs/timer.h"
#include "hardware/regs/intctrl.h"
#include "hardware/regs/m0plus.h"
#include "hardware/regs/uart.h"
#include "resets.h"
#include "reg.h"

#define LED_PIN   15u
#define LED_MASK  (1u << LED_PIN)
    
#define BLINK_PERIOD_US 500000u   /* 0.5 s */

/* Baud 115200 from clk_peri = 125 MHz (SDK default, verified in SDK source).
   BAUDDIV = 125e6 / (16 * 115200) = 67.8168 -> IBRD 67, FBRD round(0.8168*64) = 52.
   Actual ~115207 baud, error ~0.006%. */
#define UART_BAUD_IBRD 67u
#define UART_BAUD_FBRD 52u
#define UART_TIMEOUT_LOOPS 100000u

static uint32_t next_alarm; /*target time*/

/* Runs every time ALARM0 fires. Name must the SDK vector table*/
void isr_irq0(void){
    /* a. Clear ALARM0's interrupt (INTR is write-1-to-clear). */
    reg_write(TIMER_BASE + TIMER_INTR_OFFSET, TIMER_INTR_ALARM_0_BITS);

    /* b. Toggle the LED. */
    reg_write(SIO_BASE + SIO_GPIO_OUT_XOR_OFFSET, LED_MASK);

    /* c. next target = previous target + period (no drift). */
    next_alarm += BLINK_PERIOD_US;
    reg_write(TIMER_BASE + TIMER_ALARM0_OFFSET, next_alarm);
}

/* Send one byte. Waits (with a timeout) while the TX FIFO is full.
   Returns false if it never got room. */
static bool uart_putc(char c)
{
    for (uint32_t i = 0; i < UART_TIMEOUT_LOOPS; i++) {
        if ((reg_read(UART0_BASE + UART_UARTFR_OFFSET) & UART_UARTFR_TXFF_BITS) == 0) {
            reg_write(UART0_BASE + UART_UARTDR_OFFSET, (uint8_t)c);
            return true;
        }
    }
    return false;
}

/* Send a whole string, one byte at a time, until the '\0' end marker. */
static void uart_write(const char *s)
{
    while (*s != '\0') {      /* until the end marker */
        uart_putc(*s);        /* send this letter      */
        s++;                  /* move to the next one  */
    }
}

/* Receive one byte. Returns the byte (0–255), or -1 on timeout. */
static int uart_getc(void)
{
    for (uint32_t i = 0; i < UART_TIMEOUT_LOOPS; i++) {
        if ((reg_read(UART0_BASE + UART_UARTFR_OFFSET) & UART_UARTFR_RXFE_BITS) == 0) {
            return reg_read(UART0_BASE + UART_UARTDR_OFFSET) & 0xFF;
        }
    }
    return -1;
}

/* Loopback self-test (GP0 jumpered to GP1): send 'A', expect 'A' back. */
static bool uart_loopback_test(void)
{
    while (uart_getc() != -1) {   /* drain old bytes (e.g. the looped-back hello) */
    }
    uart_putc('A');
    return uart_getc() == 'A';
}

int main(void)
{
    /* Step 1: release IO_BANK0 and PADS_BANK0 from reset. */
    resets_release(RESETS_RESET_IO_BANK0_BITS | RESETS_RESET_PADS_BANK0_BITS| RESETS_RESET_TIMER_BITS| RESETS_RESET_UART0_BITS);

    /* Step 2: make sure GP15's pad output is allowed (clear output disable). */
    reg_clr(PADS_BANK0_BASE + PADS_BANK0_GPIO15_OFFSET, PADS_BANK0_GPIO15_OD_BITS);

    /* Step 3: connect GP15 to SIO (FUNCSEL = 5, all overrides normal). */
    reg_write(IO_BANK0_BASE + IO_BANK0_GPIO15_CTRL_OFFSET,
              IO_BANK0_GPIO15_CTRL_FUNCSEL_VALUE_SIO_15);

    /* Give GP0 and GP1 to UART0 (FUNCSEL = 2 on both). */
    reg_write(IO_BANK0_BASE + IO_BANK0_GPIO0_CTRL_OFFSET,
              IO_BANK0_GPIO0_CTRL_FUNCSEL_VALUE_UART0_TX);
    reg_write(IO_BANK0_BASE + IO_BANK0_GPIO1_CTRL_OFFSET,
              IO_BANK0_GPIO1_CTRL_FUNCSEL_VALUE_UART0_RX);
    
    
    /* UART0 setup: baud rate 115200 (see math at top). */
    reg_write(UART0_BASE + UART_UARTIBRD_OFFSET, UART_BAUD_IBRD);
    reg_write(UART0_BASE + UART_UARTFBRD_OFFSET, UART_BAUD_FBRD);

    /* UART0: frame format 8N1, FIFO on. Must come after the baud writes
       (writing LCR_H is what makes the new baud values take effect). */
    reg_write(UART0_BASE + UART_UARTLCR_H_OFFSET,
              UART_UARTLCR_H_WLEN_BITS | UART_UARTLCR_H_FEN_BITS);
    

    /* Step 4: make GP15 an output, then drive it high. */
    reg_write(SIO_BASE + SIO_GPIO_OE_SET_OFFSET, LED_MASK);
    reg_write(SIO_BASE + SIO_GPIO_OUT_SET_OFFSET, LED_MASK);
    /* UART0: turn on the UART, transmitter, and receiver (last, after config). */
    reg_write(UART0_BASE + UART_UARTCR_OFFSET,
              UART_UARTCR_UARTEN_BITS | UART_UARTCR_TXE_BITS | UART_UARTCR_RXE_BITS);
    
    uart_write("Hello from my own UART driver\r\n");

    /* Loopback test: pass -> timer starts, LED blinks. Fail -> LED stays solid. */
    if (!uart_loopback_test()) {
        while (1) {          /* stop here: timer never starts */
        }
    }

    /* Step 5: timer interrupt setup */
    reg_set(TIMER_BASE + TIMER_INTE_OFFSET, TIMER_INTE_ALARM_0_BITS);
    reg_write(PPB_BASE + M0PLUS_NVIC_ISER_OFFSET, 1u << TIMER_IRQ_0);
    
    /* STep 6: arm the first alarm: now + period */
    next_alarm = reg_read(TIMER_BASE + TIMER_TIMERAWL_OFFSET) + BLINK_PERIOD_US;
    reg_write(TIMER_BASE + TIMER_ALARM0_OFFSET, next_alarm);

    
    while (1) {

    }
}