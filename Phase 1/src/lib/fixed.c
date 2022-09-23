#include "fixed.h"
#include "stdint.h"


fixed convert_int_to_fixed(int n) {
    return n * F;
}

int convert_fixed_to_int_trunc(fixed x) {
    return x / F;
}

int convert_fixed_to_int_round(fixed x) {
    if (x >= 0)
        return (x + F / 2) / F;
    else
        return (x - F / 2) / F;
}

fixed add_fixed_fixed(fixed x, fixed y) {
    return x + y;
}

fixed add_fixed_int(fixed x, int n) {
    return x + convert_int_to_fixed(n);
}

fixed subtract_fixed_fixed(fixed x, fixed y) {
    return x - y;
}

fixed subtract_fixed_int(fixed x, int n) {
    return x - convert_int_to_fixed(n);
}

fixed multiply_fixed_fixed(fixed x, fixed y) {
    return ((int64_t)x * y) / F;
}

fixed multiply_fixed_int(fixed x, int n) {
    return x * n;
}

fixed divide_fixed_fixed(fixed x, fixed y) {
    return ((int64_t)x) * F / y;
}

fixed divide_fixed_int(fixed x, int n) {
    return x / n;
}