#ifndef MTBX_MISSING_CPP_H
#define MTBX_MISSING_CPP_H

__extension__ typedef int __guard __attribute__((mode (__DI__)));

extern "C" int __cxa_guard_acquire(const __guard *);
extern "C" void __cxa_guard_release (__guard *);
extern "C" void __cxa_guard_abort (__guard *);

extern "C" void __cxa_pure_virtual(void);

#endif // MTBX_MISSING_CPP_H
