/***************************************************************************
 *   wclang                                                                *
 *   Copyright (C) 2013 by Thomas Poechtrager                              *
 *   t.poechtrager@gmail.com                                               *
 *                                                                         *
 *   All rights reserved. This program and the accompanying materials      *
 *   are made available under the terms of the GNU Lesser General Public   *
 *   License (LGPL) version 2.1 which accompanies this distribution,       *
 *   and is available at  http://www.gnu.org/licenses/lgpl-2.1.html        *
 *                                                                         *
 *   This library is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU      *
 *   Lesser General Public License for more details.                       *
 ***************************************************************************/

#include <iostream>
#include <typeinfo>
#include <stdexcept>
#include <tuple>
#include <sstream>
#include <string>
#include <vector>
#include <cstring>
#include <cstdlib>
#include <sys/stat.h>
#include <unistd.h>
#include "wclang.h"
#include "wclang_time.h"

typedef std::vector<std::string> string_vector;

/*
 * Supported targets
 */

enum {
    TARGET_WIN32 = 0,
    TARGET_WIN64
};

static constexpr const char* TARGET32[] = {
    "i686-w64-mingw32",
    "i586-mingw32",
    "i586-mingw32msvc",
    "i486-mingw32"
};

static constexpr const char* TARGET64[] = {
    "x86_64-w64-mingw32",
    "amd64-mingw32msvc",
};

/*
 * Remove once exceptions are supported
 */
static constexpr const char* CXXFLAGS = "-fno-exceptions";

static constexpr const char* ENVVARS[] = {
    "AR", "AS", "CPP", "DLLTOOL", "DLLWRAP",
    "ELFEDIT","GCOV", "GNAT", "LD", "NM",
    "OBJCOPY", "OBJDUMP", "RANLIB", "READELF",
    "SIZE", "STRINGS", "STRIP", "WINDMC", "WINDRES"
};

static constexpr const char* COMMANDPREFIX = "-wc-";

struct commandargs {
    bool verbose;
    commandargs() : verbose(false) {}
};

/*
 * Paths where we should look for mingw c++ headers
 */

static constexpr const char* CPPINCLUDEBASE[] = {
    "/usr",
    "/usr/lib/gcc",
    "/usr/local/include/c++",
    "/usr/include/c++",
    "/opt"
};

/*
 * Paths where we should look for mingw C headers
 */

static constexpr const char* STDINCLUDEBASE[] = {
    "/usr/local",
    "/usr",
    "/opt"
};

/*
 * Supported mingw versions
 */

static constexpr const char* MINGWVERSIONS[] = {
    "4.8.1", "4.8",
    "4.7.3", "4.7.2", "4.7.1", "4.7",
    "4.6.4", "4.6.3", "4.6.2", "4.6.1", "4.6",
    "4.5.4", "4.5.3", "4.5.2", "4.5.1", "4.5",
    "4.4.7", "4.4.6", "4.4.5", "4.4.4", "4.4.3",
    "4.4.2", "4.4.1", "4.4"
};

static bool findcppheaders(const char *target, string_vector &cpppaths,
                           const string_vector &stdpaths)
{
    std::string base;
    std::string dir;
    struct stat st;
    std::string mingw = std::string("/") + std::string(target);

    auto trydir = [&](const std::string &dir, const char *v) -> bool
    {
        if (!stat(dir.c_str(), &st) && S_ISDIR(st.st_mode))
        {
            if (!base.empty() && v)
            {
                base += std::string(v);
                cpppaths.push_back(base);
            }

            cpppaths.push_back(dir);

#ifdef _DEBUG
            for (const auto &dir : cpppaths)
                std::cout << "found c++ include dir: " << dir << std::endl;
#endif

            return true;
        }

        return false;
    };

    for (const auto &stddir : stdpaths)
    {
        dir = stddir + std::string("/c++");
        if (trydir(dir, nullptr)) return true;

        for (const char *v : MINGWVERSIONS)
        {
            dir = stddir + std::string("/c++/") + std::string(v);
            if (trydir(dir, nullptr)) return true;
        }
    }

    for (const char *cppinclude : CPPINCLUDEBASE)
    {
        base = std::string(cppinclude) + std::string("/");

        for (const char *v : MINGWVERSIONS)
        {
            dir = base + std::string(v) + mingw;
            if (trydir(dir, v)) return true;
        }
    }

    for (const char *cppinclude : CPPINCLUDEBASE)
    {
        base  = std::string(cppinclude) + std::string("/");
        base += std::string(target) + std::string("/");

        for (const char *v : MINGWVERSIONS)
        {
            dir = base + std::string(v) + std::string("/include/c++");
            if (trydir(dir, v)) return true;
        }
    }

    return false;
}

static bool findstdheader(const char *target, string_vector &stdpaths)
{
    std::string dir;
    struct stat st;

    for (const char *stdinclude : STDINCLUDEBASE)
    {
        auto trydir = [&](const std::string &dir) -> bool
        {
            if (!stat(dir.c_str(), &st) && S_ISDIR(st.st_mode))
            {
                stdpaths.push_back(dir);

#ifdef _DEBUG
                for (const auto &dir : stdpaths)
                    std::cout << "found c include dir: " << dir << std::endl;
#endif

                return true;
            }

            return false;
        };

        dir  = std::string(stdinclude) + std::string("/");
        dir += std::string(target) + std::string("/include");
        if (trydir(dir)) return true;

        dir  = std::string(stdinclude) + std::string("/");
        dir += std::string(target) + std::string("/sys-root/mingw/include");
        if (trydir(dir)) return true;
    }

    return false;
}

static const char *findtarget32(string_vector &stdpaths)
{
    for (const char *target : TARGET32)
        if (findstdheader(target, stdpaths)) return target;

    return nullptr;
}

static const char *findtarget64(string_vector &stdpaths)
{
    for (const char *target : TARGET64)
        if (findstdheader(target, stdpaths)) return target;

    return nullptr;
}

/*
 * Tools
 */

template<class... T>
static void envvar(string_vector &env, const char *varname,
                   const char *val, T... values)
{
    constexpr size_t vc = sizeof...(T) + 1;
    const char *vals[vc] = { val, values... };

    std::string var = varname + std::string("=");
    for (size_t i = 0; i < vc; ++i) var += vals[i];

    env.push_back(var);
}

static void fmtstring(std::ostringstream &sbuf, const char *s)
{
    while (*s)
    {
        if (*s == '%')
        {
            if (s[1] == '%') ++s;
            else ERROR("fmtstring() error");
        }

        sbuf << *s++;
    }
}

template<typename T, typename... Args>
static std::string fmtstring(std::ostringstream &buf, const char *str,
                             T value, Args... args)
{
    while (*str)
    {
        if (*str == '%')
        {
            if (str[1] != '%')
            {
                buf << value;
                fmtstring(buf, str + 1, args...);
                return buf.str();
            }
            else {
                ++str;
            }
        }

        buf << *str++;
    }

    ERROR("fmtstring() error");
}

template<typename T = const char*, typename... Args>
static void verbosemsg(const char *str, T value, Args... args)
{
    std::ostringstream buf;
    std::string msg = fmtstring(buf, str, value, std::forward<Args>(args)...);
    std::cerr << PACKAGE_NAME << " verbose: " << msg << std::endl;
}

static time_vector times;
static time_point start = getticks();

static void timepoint(const char *description)
{
    time_point now = getticks();
    times.push_back(time_tuple(description, now));
}

static void printtimes()
{
    for (const auto &tp : times)
    {
        float ms = getmicrodiff(start, std::get<1>(tp)) / 1000.0f;
        verbosemsg("% +% ms", std::get<0>(tp), ms);
    }
}

static void parseargs(int argc, char **argv, const char *target,
                      commandargs &cmdargs, const string_vector &env)
{
    auto printheader = []()
    {
        std::cout << PACKAGE_NAME << ", Version: " << VERSION << std::endl;
    };

    for (int i = 0; i < argc; ++i)
    {
        char *arg = argv[i];

        /*
         * Everything with COMMANDPREFIX belongs to us
         */

        if (!std::strncmp(arg, "--", STRLEN("--")))
            ++arg;

        if (!std::strncmp(arg, COMMANDPREFIX, std::strlen(COMMANDPREFIX)))
        {
            arg += std::strlen(COMMANDPREFIX);

            if (!std::strcmp(arg, "version") || !std::strcmp(arg, "v"))
            {
                printheader();
                std::cout << "Copyright (C) 2013 Thomas Poechtrager." << std::endl;
                std::cout << "License: LGPL v2.1" << std::endl;
                std::cout << "Bugs / Wishes: " << PACKAGE_BUGREPORT << std::endl;
            }
            else if (!std::strcmp(arg, "target") || !std::strcmp(arg, "t"))
            {
                std::cout << target << std::endl;
            }
            else if (!std::strncmp(arg, "env-", STRLEN("env-")) ||
                     !std::strncmp(arg, "e-", STRLEN("e-")))
            {
                bool found = false;

                while (*++arg != '-');
                ++arg;

                for (char *p = arg; *p; ++p) *p = toupper(*p);

                size_t i = 0;
                for (const char *var : ENVVARS)
                {
                    if (!std::strcmp(arg, var))
                    {
                        const char *val = env[i].c_str();
                        val += strlen(var) + 1; /* skip variable name */

                        std::cout << val << std::endl;
                        found = true;

                        break;
                    }

                    if (found) break;

                    ++i;
                }

                if (!found)
                {
                    std::cerr << "environment variable " << arg << " not found" << std::endl;
                    std::cerr << "available environment variables: " << std::endl;

                    for (const char *var : ENVVARS)
                        std::cerr << " " << var << std::endl;

                    std::exit(EXIT_FAILURE);
                }
            }
            else if (!std::strcmp(arg, "env") || !std::strcmp(arg, "e"))
            {
                for (const auto &v : env) std::cout << v << " ";
                std::cout << std::endl;
            }
            else if (!std::strcmp(arg, "arch") || !std::strcmp(arg, "a"))
            {
                const char *end = std::strchr(target, '-');

                if (!end)
                {
                    std::cerr << "internal error (could not determine arch)" << std::endl;
                    std::exit(EXIT_FAILURE);
                }

                std::string arch(target, end-target);
                std::cout << arch << std::endl;
            }
            else if (!std::strcmp(arg, "verbose"))
            {
                cmdargs.verbose = true;
                continue;
            }
            else if (!std::strcmp(arg, "help") || !std::strcmp(arg, "h"))
            {
                printheader();

                auto printcmdhelp = [&](const char *cmd, const std::string &text)
                {
                    std::cout << " " << COMMANDPREFIX << cmd << ": " << text << std::endl;
                };

                printcmdhelp("version", "show version");
                printcmdhelp("target", "show target");

                printcmdhelp("env-<var>", std::string("show environment variable  [e.g: ") +
                             std::string(COMMANDPREFIX) + std::string("env-ld]"));

                printcmdhelp("env", "show all environment variables at once");
                printcmdhelp("arch", "show target architecture");
                printcmdhelp("verbose", "enable verbose messages");
            }
            else {
                printheader();
                std::cerr << "invalid argument: " << COMMANDPREFIX << arg << std::endl;
                std::exit(EXIT_FAILURE);
            }

            std::exit(EXIT_SUCCESS);
        }
    }
}

int main(int argc, char **argv)
{
    const char *target = nullptr;
    int targettype = -1;
    const char *e = std::strrchr(argv[0], '/');
    const char *p = nullptr;

    bool iscpp = false;
    string_vector cpppaths;
    string_vector stdpaths;

    const char *compiler = nullptr;

    string_vector env;
#ifdef HAVE_EXECVPE
    char **cenv = nullptr;
    int cenvi = 0;
#endif

    string_vector args;
    char **cargs = nullptr;
    int cargsi = 0;

    std::string compilerflags;
    commandargs cmdargs;

    timepoint("start");

    if (!e) e = argv[0];
    else ++e;

    p = std::strchr(e, '-');
    if (!p++ || std::strncmp(p, "clang", STRLEN("clang")))
    {
        std::cerr << "invalid invokation name: clang should be followed "
                     "after target (e.g: w32-clang)" << std::endl;
        return 1;
    }

    /*
     * Check if we want the C or the C++ compiler
     */

    p += STRLEN("clang");
    if (!std::strcmp(p, "++")) iscpp = true;
    else if (*p) {
        std::cerr << "invalid invokation name: ++ (or nothing) should be "
                     "followed after clang (e.g: w32-clang++)" << std::endl;
        return 1;
    }

    /*
     * Check if we should target win32 or win64...
     */

    if (!std::strncmp(e, "w32", STRLEN("w32")))
    {
        target = findtarget32(stdpaths);
        targettype = TARGET_WIN32;
    }
    else if (!std::strncmp(e, "w64", STRLEN("w64")))
    {
        target = findtarget64(stdpaths);
        targettype = TARGET_WIN64;
    }

    if (targettype == -1)
    {
        std::cerr << "invalid target: " << e << std::endl;
        return 1;
    }
    else if (!target)
    {
        const char *type;
        std::string desc;

        switch (targettype)
        {
            case TARGET_WIN32: type = "32 bit"; break;
            case TARGET_WIN64: type = "64 bit"; break;
            default: type = nullptr;
        }

        desc = std::string("mingw (") + std::string(type) + std::string(")");

        std::cerr << "can not find " << desc << " installation" << std::endl;
        std::cerr << "make sure " << desc << " is installed on your system" << std::endl;
        return 1;
    }

    /*
     * Lookup C++ and C include paths
     */

    if (stdpaths.empty())
    {
        std::cerr << "can not find " << target << " c headers" << std::endl;
        std::cerr << "make sure " << target << " c headers are installed on your system " << std::endl;
        return 1;
    }

    if (iscpp)
    {
        if (!findcppheaders(target, cpppaths, stdpaths))
        {
            std::cerr << "can not find " << target << " c++ headers" << std::endl;
            std::cerr << "make sure " << target << " c++ headers are installed on your system " << std::endl;
            return 1;
        }
    }

    /*
     * Setup compiler command
     */

    if (iscpp)
    {
        compiler = "clang++";
        compilerflags = CXXFLAGS;
    }
    else compiler = "clang";

    /*
     * Setup compiler Arguments
     */

    args.push_back(compiler);

    if (!compilerflags.empty())
        args.push_back(compilerflags);

    args.push_back(CLANG_TARGET_OPT);
    args.push_back(target);

    for (const auto &dir : stdpaths)
    {
        args.push_back("-isystem");
        args.push_back(dir);
    }

    for (const auto &dir : cpppaths)
    {
        args.push_back("-isystem");
        args.push_back(dir);
    }

    for (int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];
        char buf[1+sizeof(COMMANDPREFIX)];

        if (!std::strncmp(arg, COMMANDPREFIX, strlen(COMMANDPREFIX)))
            continue;

        *buf = '-';
        std::strcpy(buf+1, COMMANDPREFIX);

        if (!std::strncmp(arg, buf, strlen(buf)))
            continue;

        args.push_back(argv[i]);
    }

    cargs = new char* [args.size()+2];
    cargs[args.size()] = nullptr;

    for (const auto &opt : args)
        cargs[cargsi++] = strdup(opt.c_str());

    /*
     * Setup environment variables
     */

    for (const char *var : ENVVARS)
    {
        size_t len = strlen(var);
        char *buf = new char[1+len+1];

        buf[len+1] = '\0';
        *buf++ = '-';

        for (size_t i = 0; i < len; ++i)
            buf[i] = tolower(var[i]);

        envvar(env, var, target, --buf);
        delete[] buf;
    }

    switch (targettype)
    {
    case TARGET_WIN32:
        env.push_back("CC=w32-clang");
        env.push_back("CXX=w32-clang++");
        break;
    case TARGET_WIN64:
        env.push_back("CC=w64-clang");
        env.push_back("CXX=w64-clang++");
        break;
    }

#ifdef HAVE_EXECVPE
    cenv = new char* [env.size()+2];
    cenv[env.size()] = nullptr;

    for (const auto &var : env)
        cenv[cenvi++] = strdup(var.c_str());
#endif

    /*
     * Parse command arguments late,
     * when we know our environment already
     */

    parseargs(argc, argv, target, cmdargs, env); /* may not return */

    /*
     * Execute command
     */

    if (cmdargs.verbose)
    {
        std::string command;

        for (char **arg = cargs; *arg; ++arg)
        {
            if (arg != cargs) command += " ";
            command += *arg;
        }

        verbosemsg("%", command.c_str());

        timepoint("end");
        printtimes();
    }

#ifdef HAVE_EXECVPE
    execvpe(compiler, cargs, cenv);
#else
    execvp(compiler, cargs);
#endif

    std::cerr << "invoking compiler failed" << std::endl;
    std::cerr << "clang not installed?" << std::endl;
    return 1;
}
