#include <utility>
#include <cstdlib>
#include "config.h"

#define ERROR(msg) \
do { \
    std::runtime_error(msg); \
    std::exit(EXIT_FAILURE); \
} while(0)

template<class T>
constexpr size_t STRLEN(const T &str)
{
    return sizeof(str)-1;
}

static_assert(STRLEN("test string") == 11, "");


