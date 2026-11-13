#include "hardware/regs/addressmap.h"
#include "hardware/regs/resets.h"
#include "hardware/regs/pads_bank0.h"
#include "hardware/regs/io_bank0.h"
#include "hardware/regs/sio.h"
#include "resets.h"
#include "reg.h"

#define LED_PIN   15u
#define LED_MASK  (1u << LED_PIN)
#define BLINK_DELAY_LOOPS 2000000u  // add delay to the LED


static void delay_loops(uint32_t n) {
    for (volatile uint32_t i = 0; i < n; i++) {
        }
}

int main(void)
{
    /* Step 1: release IO_BANK0 and PADS_BANK0 from reset. */
    resets_release(RESETS_RESET_IO_BANK0_BITS | RESETS_RESET_PADS_BANK0_BITS);

    /* Step 2: make sure GP15's pad output is allowed (clear output disable). */
    reg_clr(PADS_BANK0_BASE + PADS_BANK0_GPIO15_OFFSET, PADS_BANK0_GPIO15_OD_BITS);

    /* Step 3: connect GP15 to SIO (FUNCSEL = 5, all overrides normal). */
    reg_write(IO_BANK0_BASE + IO_BANK0_GPIO15_CTRL_OFFSET,
              IO_BANK0_GPIO15_CTRL_FUNCSEL_VALUE_SIO_15);
    
    /* Step 4: make GP15 an output, then drive it high. */
    reg_write(SIO_BASE + SIO_GPIO_OE_SET_OFFSET, LED_MASK);
    reg_write(SIO_BASE + SIO_GPIO_OUT_SET_OFFSET, LED_MASK);

    while (1) {
        reg_write(SIO_BASE + SIO_GPIO_OUT_XOR_OFFSET, LED_MASK); // flipping GP15
        delay_loops(BLINK_DELAY_LOOPS);
    }
}