#ifndef __LIB_FIXED_H
#define __LIB_FIXED_H

typedef int fixed;

#define Q 14
#define P 17
#define F 16384

fixed convert_int_to_fixed(int n);

int convert_fixed_to_int_trunc(fixed x);

int convert_fixed_to_int_round(fixed x);

fixed add_fixed_fixed(fixed x, fixed y);

fixed add_fixed_int(fixed x, int n);

fixed subtract_fixed_fixed(fixed x, fixed y);

fixed subtract_fixed_int(fixed x, int n);

fixed multiply_fixed_fixed(fixed x, fixed y);

fixed multiply_fixed_int(fixed x, int n);

fixed divide_fixed_fixed(fixed x, fixed y);

fixed divide_fixed_int(fixed x, int n);


#endif /* lib/fixed.h */