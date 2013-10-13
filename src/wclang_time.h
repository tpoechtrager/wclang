#ifdef HAVE_STD_CHRONO
#include <chrono>
typedef std::chrono::steady_clock::time_point time_point;
ullong getmicrodiff(time_point &start, time_point time);
#else
#include <sys/time.h>
typedef ullong time_point;
ullong getmicrodiff(time_point &start, time_point time);
#endif

time_point getticks();

typedef std::tuple<const char*, time_point> time_tuple;
typedef std::vector<time_tuple> time_vector;
