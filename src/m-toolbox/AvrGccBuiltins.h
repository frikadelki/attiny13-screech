#ifndef MTBX_AVR_GCC_BUILTINS_H
#define MTBX_AVR_GCC_BUILTINS_H

extern void __builtin_avr_sleep();

extern void __builtin_avr_delay_cycles(unsigned long count);

extern void __builtin_avr_nops(unsigned long count);

extern void __builtin_avr_no_operation();

unsigned char __builtin_avr_swap (unsigned char);

#endif // MTBX_AVR_GCC_BUILTINS_H
