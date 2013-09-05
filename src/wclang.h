#include <utility>
#include <cstdlib>
#include "config.h"

#define ERROR(msg) \
do { \
    std::runtime_error(msg); \
    std::exit(EXIT_FAILURE); \
} while(0)
