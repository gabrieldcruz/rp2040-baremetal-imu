#include "hardware/regs/addressmap.h"
#include "hardware/regs/resets.h"
#include "resets.h"

int main(void)
{
    resets_release(RESETS_RESET_IO_BANK0_BITS | RESETS_RESET_PADS_BANK0_BITS);

    while (1) {
        
    }
}