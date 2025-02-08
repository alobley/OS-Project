#ifndef STDARG_H
<<<<<<< HEAD
#define STDARG_H 1

typedef __builtin_va_list va_list;

#define va_start(ap, param) __builtin_va_start(ap, param)

#define va_arg(ap, type) __builtin_va_arg(ap, type)

#define va_end(ap) __builtin_va_end(ap)

#define va_copy(dest, src) __builtin_va_copy(dest, src)
=======
#define STDARG_H

typedef __builtin_va_list va_list;

#define va_start(v, l) __builtin_va_start(v, l)
#define va_arg(v, l) __builtin_va_arg(v, l)
#define va_end(v) __builtin_va_end(v)

#define va_copy(d, s) __builtin_va_copy(d, s)
>>>>>>> a9a2e67 (Rewrote again, back at 32-bit and implemented a fully functional paging setup that is totally flawless and has no flaws at all)

#endif