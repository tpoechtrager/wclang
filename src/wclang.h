#include <iostream>
#include <utility>
#include <cstdlib>
#include <string>
#include <vector>
#include "config.h"

static inline void ERRORMSG(const char *msg, const char *file,
                            int line, const char *func)
{
    std::cerr << "runtime error: " << msg << std::endl;
    std::cerr << file << " " << func << "():";
    std::cerr << line << std::endl;

    std::exit(EXIT_FAILURE);
}

__attribute__ ((noreturn))
static inline void ERRORMSG(const char *msg, const char *file,
                            int line, const char *func);

#define ERROR(msg) \
do { \
    ERRORMSG(msg, __FILE__, __LINE__, __func__); \
} while (0)

template<class T>
constexpr size_t STRLEN(const T &str)
{
    static_assert(sizeof(*str) == 1, "not a string");
    return sizeof(T)-1;
}

static_assert(STRLEN("test string") == 11, "");

typedef unsigned long long ullong;
typedef std::vector<std::string> string_vector;

#define KNRM "\x1B[0m"
#define KBLD "\x1B[1m"

typedef bool (* realpathcmp)(const char *file, const struct stat &st);

std::string &realpath(const char *file, std::string &result, realpathcmp cmp = nullptr);
std::string &getpathofcommand(const char *bin, std::string &result);

enum optimize {
    LEVEL_0,
    LEVEL_1,
    LEVEL_3,
    FAST,
    SIZE_1,
    SIZE_2
};

struct commandargs {
    bool cached;
    bool verbose;
    string_vector &intrinpaths;
    string_vector &stdpaths;
    string_vector &cxxpaths;
    string_vector &cflags;
    string_vector &cxxflags;
    std::string &target;
    std::string &compiler;
    string_vector &env;
    string_vector &args;
    bool &iscxx;
    bool appendexe;
    bool iscompilestep;
    bool islinkstep;
    bool nointrinsics;
    int exceptions;
    int optimizationlevel;
    int usemingwlinker;

    commandargs(string_vector &intrinpaths, string_vector &stdpaths,
                string_vector &cxxpaths, string_vector &cflags,
                string_vector &cxxflags, std::string &target,
                std::string &compiler, string_vector &env,
                string_vector &args, bool &iscxx) :
                cached(false), verbose(false), intrinpaths(intrinpaths),
                stdpaths(stdpaths), cxxpaths(cxxpaths), cflags(cflags),
                cxxflags(cxxflags), target(target), compiler(compiler),
                env(env), args(args), iscxx(iscxx), appendexe(false),
                iscompilestep(false), islinkstep(false), nointrinsics(false),
                exceptions(-1), optimizationlevel(0), usemingwlinker(0) {}
} __attribute__ ((aligned (8)));
