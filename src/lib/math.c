#include "math.h"
#include "time.h"

float64 sqrt(float64 x){
    if(x < 0) return -1;        // Can't sqrt negatives
    float64 result;
    asm volatile("fsqrt" : "=t" (result) : "0" (x));
    return result;
}

int abs(int x){
    return (x < 0) ? -x : x;
}

uint32 randSeed = 0;

// Seed the random number generator. If not called, the seed will be 0. While that's fine, it won't really be random.
void srand(int32 seed){
    if(seed == 0){
        // If we seed with 0, make the seed the current system time
        randSeed = (int32)GetTicks();
    }else{
        randSeed = seed;
    }
}

int rand(){
    randSeed = randSeed * 1103515245 + 12345;
    return (int32)(randSeed / 65536) % 32768;
}

int randrange(int min, int max){
    randSeed = randSeed * 1103515245 + 12345;
    return min + (int32)(randSeed / 65536) % (max - min + 1);
}

double pow(double base, double exp){
    if(exp == 0) return 1;
    double result = 1;
    for(int i = 0; i < exp; ++i){
        result *= base;
    }
    return result;
}

double factorial(int n){
    if(n == 0 || n == 1) return 1.0;
    double result = 1.0;
    for(int i = 2; i <= n; i++) result *= i;
    return result;
}

double sin(double x, int precision){
    double result = 0.0;
    for(int i = 0; i < precision; i++){
        double term = pow(-1, i) * pow(x, 2 * i + 1) / factorial(2 * i + 1);
        result += term;
    }
    return result;
}

double cos(double x, int precision){
    double result = 0.0;
    for(int i = 0; i < precision; i++){
        double term = pow(-1, i) * pow(x, 2 * i) / factorial(2 * i);
        result += term;
    }
    return result;
}

double tan(double x, int precision){
    double c = cos(x, precision);
    if(c == 0){
        // Return the maximum possible floating-point number (actual answer is infinity)
        return 1.8e38;
    }
    return sin(x, precision) / c;
}


// Replacements for lGCC functions
uint64 __udivdi3(uint64 dividend, uint64 divisor){
    if(divisor == 0){
        return 0;
    }

    uint64 quotient = 0;
    uint64 remainder = 0;

    for(int i = 63; i >= 0; i--){
        remainder = (remainder << 1) | ((dividend >> i) & 1);

        if(remainder >= divisor){
            remainder -= divisor;
            quotient |= (1ULL << i);
        }
    }

    return quotient;
}

uint64 __udivmoddi4(uint64 dividend, uint64 divisor, uint64* rem){
    if(divisor == 0){
        if(rem) *rem = 0;
        return 0;
    }

    uint64 quotient = 0;
    uint64 remainder = 0;

    for(int i = 63; i >= 0; i--){
        remainder = (remainder << 1) | ((dividend >> i) & 1);

        if(remainder >= divisor){
            remainder -= divisor;
            quotient |= (1ULL << i);
        }
    }

    if(rem){
        *rem = remainder;
    }

    return quotient;
}


int64 __divdi3(int64 a, int64 b) {
    // Handle division by zero
    if (b == 0) {
        return 0;
    }

    int neg = (a < 0) ^ (b < 0); // Result is negative if signs of a and b differ
    uint64 ua = (a < 0) ? -a : a; // Absolute value of a
    uint64 ub = (b < 0) ? -b : b; // Absolute value of b

    uint64 result = 0;
    while (ua >= ub) {
        uint64 temp = ub, multiple = 1;
        while ((temp << 1) > temp && (temp << 1) <= ua) {
            temp <<= 1;
            multiple <<= 1;
        }
        ua -= temp;
        result += multiple;
    }

    return neg ? -(int64)result : (int64)result;
}

int64 __moddi3(int64 a, int64 b) {
    // Handle division by zero
    if (b == 0) {
        return 0;
    }

    int neg = (a < 0); // Result is negative if a is negative
    uint64 ua = (a < 0) ? -a : a; // Absolute value of a
    uint64 ub = (b < 0) ? -b : b; // Absolute value of b

    while (ua >= ub) {
        uint64 temp = ub;
        while ((temp << 1) > temp && (temp << 1) <= ua) {
            temp <<= 1;
        }
        ua -= temp;
    }

    return neg ? -(int64)ua : (int64)ua;
}

uint64 __umoddi3(uint64 a, uint64 b) {
    // Handle division by zero
    if (b == 0) {
        return 0;
    }

    while (a >= b) {
        uint64 temp = b;
        while ((temp << 1) > temp && (temp << 1) <= a) {
            temp <<= 1;
        }
        a -= temp;
    }

    return a;
}