#ifndef STDBOOL_H
<<<<<<< HEAD
#define STDBOOL_H 1

#define bool _Bool
=======
#define STDBOOL_H

#define __bool_true_false_are_defined 1

typedef _Bool bool;
>>>>>>> a9a2e67 (Rewrote again, back at 32-bit and implemented a fully functional paging setup that is totally flawless and has no flaws at all)

#define true 1
#define false 0

#endif