#ifndef RESETS_H
#define RESETS_H

#include <stdint.h>
#include <stdbool.h>

/* Take the blocks in `mask` out of reset returns true on success, false timed out*/
bool resets_release(uint32_t mask);

#endif /* RESETS_H */