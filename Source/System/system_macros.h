#ifndef SYSTEM_MACROS_H
#define SYSTEM_MACROS_H

// System configuration macros
#define SYSTEM_VERSION_MAJOR    1
#define SYSTEM_VERSION_MINOR    0
#define SYSTEM_VERSION_PATCH    0

// Utility macros
#define ARRAY_SIZE(arr)     (sizeof(arr) / sizeof((arr)[0]))
#define BIT(n)             (1U << (n))
#define BIT_MASK(n)        ((1U << (n)) - 1)
#define IS_POWER_OF_TWO(x) ((x) && !((x) & ((x) - 1)))
#define CLAMP(x, min, max) ((x) < (min) ? (min) : ((x) > (max) ? (max) : (x)))
#define SWAP(a, b)         do { auto temp = (a); (a) = (b); (b) = temp; } while(0)


#define UNUSED(x)          (void)(x) // To avoid unused variable warnings

#endif // SYSTEM_MACROS_H
