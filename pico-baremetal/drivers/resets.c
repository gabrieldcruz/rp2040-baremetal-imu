#include "resets.h"
#include "reg.h"
#include "hardware/regs/addressmap.h"
#include "hardware/regs/resets.h"

/*Big upper bound returns error if more than this clocks. Coming out reset takes few cycles*/
#define RESETS_TIMEOUT_LOOPS 100000u

bool resets_release(uint32_t mask)
{
    // Clear the blocks' bits in RESET
    reg_clr(RESETS_BASE + RESETS_RESET_OFFSET, mask);

    /* Wait until RESET_DONE shows every requested block is ready*/
    for(uint32_t i = 0; i < RESETS_TIMEOUT_LOOPS; i++) {
        if((reg_read(RESETS_BASE + RESETS_RESET_DONE_OFFSET) & mask) == mask) {
            return true;
        }
    }
    return false;
}

