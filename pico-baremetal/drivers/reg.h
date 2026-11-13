#ifndef REG_H
#define REG_H
#include <stdint.h>
/* Atomic register access. Each peripheral register also appears at these offsets*/
#define REG_ALIAS_XOR_OFFSET 0x1000u
#define REG_ALIAS_SET_OFFSET 0x2000u
#define REG_ALIAS_CLEAR_OFFSET 0x3000u

static inline void reg_write(uintptr_t addr, uint32_t value)
{
    *(volatile uint32_t *)addr = value;
}

static inline uint32_t reg_read(uintptr_t addr)
{
    return *(volatile uint32_t *)addr;
}
static inline void reg_set(uintptr_t addr, uint32_t mask)
{
    reg_write(addr + REG_ALIAS_SET_OFFSET, mask);
}
static inline void reg_clr(uintptr_t addr, uint32_t mask)
{
    reg_write(addr + REG_ALIAS_CLEAR_OFFSET, mask);
}
static inline void reg_xor(uintptr_t addr, uint32_t mask)
{
    reg_write(addr + REG_ALIAS_XOR_OFFSET, mask);
}


#endif /* REG_H */