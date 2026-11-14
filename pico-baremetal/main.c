#include "hardware/regs/addressmap.h"
#include "hardware/regs/resets.h"
#include "hardware/regs/pads_bank0.h"
#include "hardware/regs/io_bank0.h"
#include "hardware/regs/sio.h"
#include "hardware/regs/timer.h"
#include "hardware/regs/intctrl.h"
#include "hardware/regs/m0plus.h"
#include "resets.h"
#include "reg.h"

#define LED_PIN   15u
#define LED_MASK  (1u << LED_PIN)
    
#define BLINK_PERIOD_US 500000u   /* 0.5 s */

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

int main(void)
{
    /* Step 1: release IO_BANK0 and PADS_BANK0 from reset. */
    resets_release(RESETS_RESET_IO_BANK0_BITS | RESETS_RESET_PADS_BANK0_BITS| RESETS_RESET_TIMER_BITS);

    /* Step 2: make sure GP15's pad output is allowed (clear output disable). */
    reg_clr(PADS_BANK0_BASE + PADS_BANK0_GPIO15_OFFSET, PADS_BANK0_GPIO15_OD_BITS);

    /* Step 3: connect GP15 to SIO (FUNCSEL = 5, all overrides normal). */
    reg_write(IO_BANK0_BASE + IO_BANK0_GPIO15_CTRL_OFFSET,
              IO_BANK0_GPIO15_CTRL_FUNCSEL_VALUE_SIO_15);
    
    /* Step 4: make GP15 an output, then drive it high. */
    reg_write(SIO_BASE + SIO_GPIO_OE_SET_OFFSET, LED_MASK);
    reg_write(SIO_BASE + SIO_GPIO_OUT_SET_OFFSET, LED_MASK);

    /* Step 5: timer interrupt setup */
    reg_set(TIMER_BASE + TIMER_INTE_OFFSET, TIMER_INTE_ALARM_0_BITS);
    reg_write(PPB_BASE + M0PLUS_NVIC_ISER_OFFSET, 1u << TIMER_IRQ_0);
    
    /* STep 6: arm the first alarm: now + period */
    next_alarm = reg_read(TIMER_BASE + TIMER_TIMERAWL_OFFSET) + BLINK_PERIOD_US;
    reg_write(TIMER_BASE + TIMER_ALARM0_OFFSET, next_alarm);

    
    while (1) {

    }
}