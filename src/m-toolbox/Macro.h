#ifndef MTBX_MACRO_H
#define MTBX_MACRO_H

#pragma clang diagnostic push
#pragma ide diagnostic ignored "OCUnusedMacroInspection"

// we need this to use raw addresses when using definitions from <avr/*>
#define _SFR_ASM_COMPAT 1

#pragma clang diagnostic pop

#include <avr/io.h>
#include <avr/interrupt.h>

#define ACCESS_BYTE(ADDRESS)            (_SFR_BYTE(ADDRESS))
#define ACCESS_WORD(ADDRESS)            (_SFR_WORD(ADDRESS))

#define BIT_MASK(BIT)                   (1u << (static_cast<uint8_t>(BIT)))

#define IS_BYTE_BIT_SET(ADDRESS, BIT)   (0 != (ACCESS_BYTE(ADDRESS) & BIT_MASK(BIT)))

#define SET_BYTE_BIT(ADDRESS, BIT)      (ACCESS_BYTE(ADDRESS) |= BIT_MASK(BIT))
#define CLEAR_BYTE_BIT(ADDRESS, BIT)    (ACCESS_BYTE(ADDRESS) &= ~BIT_MASK(BIT))


#endif // MTBX_MACRO_H
