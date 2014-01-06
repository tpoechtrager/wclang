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
#include <fstream>
#include <typeinfo>
#include <tuple>
#include <sstream>
#include <cstring>
#include <sys/stat.h>
#include <unistd.h>
#include <limits.h>
#include <assert.h>
#include "wclang.h"
#include "wclang_time.h"
#include "wclang_cache.h"

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
 * Remove -fno-exceptions once exceptions are supported
 */
static constexpr const char* CXXFLAGS = "-fno-exceptions";
static constexpr const char* CFLAGS = "";

static constexpr const char* ENVVARS[] = {
    "AR", "AS", "CPP", "DLLTOOL", "DLLWRAP",
    "ELFEDIT","GCOV", "GNAT", "LD", "NM",
    "OBJCOPY", "OBJDUMP", "RANLIB", "READELF",
    "SIZE", "STRINGS", "STRIP", "WINDMC", "WINDRES"
};

static constexpr char COMMANDPREFIX[] = "-wc-";

/*
 * Paths where we should look for mingw C++ headers
 */

static constexpr const char* CXXINCLUDEBASE[] = {
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
    "4.9.0", "4.9",
    "4.8.2", "4.8.1", "4.8.0", "4.8",
    "4.7.3", "4.7.2", "4.7.1", "4.7.0", "4.7",
    "4.6.4", "4.6.3", "4.6.2", "4.6.1", "4.6.0", "4.6",
    "4.5.4", "4.5.3", "4.5.2", "4.5.1", "4.5.0", "4.5",
    "4.4.7", "4.4.6", "4.4.5", "4.4.4", "4.4.3",
    "4.4.2", "4.4.1", "4.4.0", "4.4"
};

static bool findcxxheaders(const char *target, commandargs &cmdargs)
{
    std::string base;
    std::string dir;
    struct stat st;
    auto &cxxpaths = cmdargs.cxxpaths;
    const auto &stdpaths = cmdargs.stdpaths;
    std::string mingw = std::string("/") + std::string(target);

    auto trydir = [&](const std::string &dir, const char *v) -> bool
    {
        if (!stat(dir.c_str(), &st) && S_ISDIR(st.st_mode))
        {
            constexpr const char *HDRCHECKFILE = "/iostream";
            std::string hdrcheck;
            bool hdrcheckok = false;

            if (!base.empty() && v)
            {
                base += std::string(v);
                hdrcheck = base + std::string(HDRCHECKFILE);
                hdrcheckok = stat(hdrcheck.c_str(), &st) == 0;
                cxxpaths.push_back(base);
            }

            hdrcheck = dir + std::string(HDRCHECKFILE);

            if (!hdrcheckok && stat(hdrcheck.c_str(), &st))
                return false;

            cxxpaths.push_back(dir);

#ifdef _DEBUG
            for (const auto &dir : cxxpaths)
                std::cout << "found C++ include dir: " << dir << std::endl;
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
            base = dir + std::string("/");
            if (trydir(dir, target)) return true;
        }
    }

    for (const char *cxxinclude : CXXINCLUDEBASE)
    {
        base = std::string(cxxinclude) + std::string("/");

        for (const char *v : MINGWVERSIONS)
        {
            dir = base + std::string(v) + mingw;
            if (trydir(dir, v)) return true;
        }
    }

    for (const char *cxxinclude : CXXINCLUDEBASE)
    {
        base  = std::string(cxxinclude) + std::string("/");
        base += std::string(target) + std::string("/");

        for (const char *v : MINGWVERSIONS)
        {
            dir = base + std::string(v) + std::string("/include/c++");
            if (trydir(dir, v)) return true;
        }
    }

    return false;
}

static bool findstdheader(const char *target, commandargs &cmdargs)
{
    std::string dir;
    struct stat st;
    auto &stdpaths = cmdargs.stdpaths;
    const char *mingwpath = getenv("MINGW_PATH");

    auto checkdir = [&](const char *stdinclude) -> bool
    {
        auto trydir = [&](const std::string &dir) -> bool
        {
            if (!stat(dir.c_str(), &st) && S_ISDIR(st.st_mode))
            {
                std::string filecheck = dir + std::string("/stdlib.h");

                if (stat(filecheck.c_str(), &st))
                    return false;

                stdpaths.push_back(dir);

#ifdef _DEBUG
                for (const auto &dir : stdpaths)
                    std::cout << "found C include dir: " << dir << std::endl;
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

        return false;
    };

    auto checkpath = [&](const char *p) -> bool
    {
        std::string path;

        do
        {
            if (*p == ':') p++;

            while (*p && *p != ':')
                path += *p++;

            if (path.find_last_of("/bin") != std::string::npos)
                path.resize(path.size()-STRLEN("/bin"));

            if (checkdir(path.c_str()))
                return true;

            path.clear();
        } while (*p);

        return false;
    };

    if (mingwpath && *mingwpath)
        return checkpath(mingwpath);

#ifdef MINGW_PATH
    if (*MINGW_PATH && checkpath(MINGW_PATH))
        return true;
#endif

    for (const char *stdinclude : STDINCLUDEBASE)
        if (checkdir(stdinclude)) return true;

    return false;
}

static const char *findtarget32(commandargs &cmdargs)
{
    for (const char *target : TARGET32)
        if (findstdheader(target, cmdargs)) return target;

    return nullptr;
}

static const char *findtarget64(commandargs &cmdargs)
{
    for (const char *target : TARGET64)
        if (findstdheader(target, cmdargs)) return target;

    return nullptr;
}

void appendexetooutputname(char **cargs)
{
    for (char **arg = cargs; *arg; ++arg)
    {
        if (!std::strncmp(*arg, "-o", STRLEN("-o")))
        {
            if (!(*arg)[2] && (!*++arg || **arg == '-'))
                break;

            const char *filename = !std::strncmp(*arg, "-o", STRLEN("-o")) ? &(*arg)[2] : *arg;
            const char *pfix = std::strrchr(filename, '.');

            if (pfix && (!std::strcmp(pfix, ".exe") || !std::strcmp(pfix, ".dll") || !std::strcmp(pfix, ".S")))
                return;

            for (char **p = arg+1; *p; ++p)
            {
                if (!std::strcmp(*p, "-c"))
                    return;
            }

            std::cerr << R"(wclang: appending ".exe" to output filename ")" << filename << R"(")" << std::endl;

            filename = nullptr;
            pfix = nullptr;

            *arg = reinterpret_cast<char*>(std::realloc(*arg, std::strlen(*arg)+STRLEN(".exe")+1));

            if (!*arg)
            {
                std::cerr << "out of memory" << std::endl;
                std::exit(EXIT_FAILURE);
            }

            std::strcat(*arg, ".exe");
            break;
        }
        else if (!std::strcmp(*arg, "-c")) {
            break;
        }
    }
}

/*
 * Tools
 */

static std::string &realpath(const char *file, std::string &result)
{
    char *PATH = getenv("PATH");
    const char *p = PATH;
    std::string sfile;
    struct stat st;

    assert(PATH && "this shouldn't happen");

    do
    {
        if (*p == ':') p++;

        while (*p && *p != ':')
            sfile += *p++;

        sfile += std::string("/") + std::string(file);

        if (!stat(sfile.c_str(), &st))
            break;

        sfile.clear();
    } while (*p);

#ifdef HAVE_READLINK
    if (!sfile.empty())
    {
        char buf[PATH_MAX+1];
        ssize_t len;

        if ((len = readlink(sfile.c_str(), buf, PATH_MAX)) != -1)
            result.assign(buf, len);
    }
#endif

    return result;

}

template<class... T>
static void envvar(string_vector &env, const char *varname,
                   const char *val, T... values)
{
    const char *vals[] = { val, values... };

    std::string var = varname + std::string("=");
    for (auto &val : vals) var += val;

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
    std::cerr << PACKAGE_NAME << ": verbose: " << msg << std::endl;
}

template<typename T = const char*, typename... Args>
static void warn(const char *str, T value, Args... args)
{
    std::ostringstream buf;
    std::string warnmsg = fmtstring(buf, str, value, std::forward<Args>(args)...);
    std::cerr << PACKAGE_NAME << ": warning: " << warnmsg << std::endl;
}

static void warn(const char *str)
{
    warn("%", str);
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
    typedef void (*dcfun)(commandargs &cmdargs, char *arg);
    typedef std::tuple<dcfun, char*> dc_tuple;
    std::vector<dc_tuple> delayedcommands;

    auto printheader = []()
    {
        std::cout << PACKAGE_NAME << ", Version: " << VERSION << std::endl;
    };

    for (int i = 0; i < argc; ++i)
    {
        char *arg = argv[i];

        if (!std::strncmp(arg, "-o", STRLEN("-o")))
        {
            cmdargs.islinkstep = true;
            continue;
        }
        else if (!std::strcmp(arg, "-c") || !std::strcmp(arg, "-S"))
        {
            cmdargs.iscompilestep = true;
            continue;
        }
        else if (!std::strcmp(arg, "-mwindows") && !cmdargs.usemingwlinker)
        {
            /*
             * Clang doesn't support -mwindows (yet)
             */

            cmdargs.usemingwlinker = 2;
            continue;
        }

        /*
         * Everything with COMMANDPREFIX belongs to us
         */

        if (!std::strncmp(arg, "--", STRLEN("--")))
            ++arg;

        if (!std::strncmp(arg, COMMANDPREFIX, STRLEN(COMMANDPREFIX)))
        {
            arg += STRLEN(COMMANDPREFIX);

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
                        val += std::strlen(var) + 1; /* skip variable name */

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
            else if (!std::strcmp(arg, "static-runtime"))
            {
                static constexpr const char* GCCRUNTIME = "-static-libgcc";
                static constexpr const char* LIBSTDCXXRUNTIME = "-static-libstdc++";

                /*
                 * Postpone execution to later
                 * We don't know yet, if it is the link step or not
                 */
                auto staticruntime = [](commandargs &cmdargs, char *arg)
                {
                    /*
                     * Avoid "argument unused during compilation: '...'"
                     */
                    if (!cmdargs.islinkstep)
                    {
                        if (cmdargs.verbose)
                            verbosemsg("ignoring %", arg);
                        return;
                    }

                    if (cmdargs.iscxx)
                    {
                        cmdargs.cxxflags.push_back(GCCRUNTIME);
                        cmdargs.cxxflags.push_back(LIBSTDCXXRUNTIME);
                    }
                    else {
                        cmdargs.cflags.push_back(GCCRUNTIME);
                    }
                };

                delayedcommands.push_back(dc_tuple(staticruntime, arg-STRLEN(COMMANDPREFIX)));
                continue;
            }
            else if (!std::strcmp(arg, "append-exe"))
            {
                cmdargs.appendexe = true;
                continue;
            }
            else if (!std::strcmp(arg, "use-mingw-linker"))
            {
                auto usemingwlinker = [](commandargs &cmdargs, char *arg)
                {
                    if (!cmdargs.islinkstep)
                    {
                        if (cmdargs.verbose)
                            verbosemsg("ignoring %", arg);
                        return;
                    }

                    cmdargs.usemingwlinker = 1;
                };

                delayedcommands.push_back(dc_tuple(usemingwlinker, arg-STRLEN(COMMANDPREFIX)));
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
                printcmdhelp("static-runtime", "link runtime statically");
                printcmdhelp("append-exe", "append .exe automatically to output filenames");
                printcmdhelp("use-mingw-linker", "link with mingw");
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

    if (cmdargs.islinkstep && cmdargs.iscompilestep)
    {
        /* w32-clang file.c -c -o file.o */
        cmdargs.islinkstep = false;
    }
    else if (!cmdargs.islinkstep && !cmdargs.iscompilestep)
    {
        /* w32-clang file.c */
        cmdargs.islinkstep = true;
    }

    for (auto dc : delayedcommands)
    {
        auto fun = std::get<0>(dc);
        fun(cmdargs, std::get<1>(dc));
    }
}

int main(int argc, char **argv)
{
    std::string target;
    int targettype = -1;
    const char *e = std::strrchr(argv[0], '/');
    const char *p = nullptr;

    bool iscxx = false;
    string_vector stdpaths;
    string_vector cxxpaths;
    std::string compiler;
    string_vector env;
    string_vector args;
    char **cargs = nullptr;
    int cargsi = 0;
    string_vector cflags;
    string_vector cxxflags;

    commandargs cmdargs(stdpaths, cxxpaths, cflags, cxxflags,
                        target, compiler, env, args, iscxx);

    const char *cachefile = nullptr;

    timepoint("start");

    if (!e) e = argv[0];
    else ++e;

    p = std::strchr(e, '-');
    if (!p++ || std::strncmp(p, "clang", STRLEN("clang")))
    {
        std::cerr << "invalid invocation name: clang should be followed "
                     "after target (e.g: w32-clang)" << std::endl;
        return 1;
    }

    /*
     * Check if we want the C or the C++ compiler
     */

    p += STRLEN("clang");
    if (!std::strcmp(p, "++")) iscxx = true;
    else if (*p) {
        std::cerr << "invalid invocation name: ++ (or nothing) should be "
                     "followed after clang (e.g: w32-clang++)" << std::endl;
        return 1;
    }

    /*
     * Load cache when requested
     */

    if ((cachefile = getenv(iscxx ? "WCLANG_LOAD_CXX_CACHE" :
                                    "WCLANG_LOAD_CC_CACHE")))
    {
        timepoint("load cache");
        loadcache(cachefile, cmdargs);
        timepoint("load cache end");
        goto cached;
    }

    /*
     * Check if we should target win32 or win64...
     */

    find_target_and_headers:;

    if (!std::strncmp(e, "w32", STRLEN("w32")))
    {
        const char *t = findtarget32(cmdargs);
        target = t ? t : "";
        targettype = TARGET_WIN32;
    }
    else if (!std::strncmp(e, "w64", STRLEN("w64")))
    {
        const char *t = findtarget64(cmdargs);
        target = t ? t : "";
        targettype = TARGET_WIN64;
    }

    if (targettype == -1)
    {
        std::cerr << "invalid target: " << e << std::endl;
        return 1;
    }
    else if (target.empty())
    {
        const char *type;
        std::string desc;

        if (getenv("MINGW_PATH"))
        {
            warn("MINGW_PATH env variable does not point to any "
                 "valid mingw installation for the current target!");

            unsetenv("MINGW_PATH");
            goto find_target_and_headers;
        }

        switch (targettype)
        {
            case TARGET_WIN32: type = "32 bit"; break;
            case TARGET_WIN64: type = "64 bit"; break;
            default: type = nullptr;
        }

        desc = std::string("mingw-w64 (") + std::string(type) + std::string(")");

        std::cerr << "cannot find " << desc << " installation" << std::endl;
        std::cerr << "make sure " << desc << " is installed on your system" << std::endl;
        std::cerr << "if you have moved your mingw installation, "
                     "then re-run the installation process!" << std::endl;
        return 1;
    }

    /*
     * Lookup C and C++ include paths
     */

    if (stdpaths.empty())
    {
        std::cerr << "cannot find " << target << " C headers" << std::endl;
        std::cerr << "make sure " << target << " C headers are installed on your system " << std::endl;
        return 1;
    }

    if (iscxx)
    {
        if (!findcxxheaders(target.c_str(), cmdargs))
        {
            std::cerr << "can not find " << target << " C++ headers" << std::endl;
            std::cerr << "make sure " << target << " C++ headers are installed on your system " << std::endl;
            return 1;
        }
    }

    /*
     * Setup compiler command
     */

    if (iscxx)
    {
        compiler = "clang++";

        if (CXXFLAGS && *CXXFLAGS)
            cxxflags.push_back(CXXFLAGS);
    }
    else
    {
        compiler = "clang";

        if (CFLAGS && *CFLAGS)
            cflags.push_back(CFLAGS);
    }

    /*
     * Setup environment variables
     */

    for (const char *var : ENVVARS)
    {
        size_t len = std::strlen(var);
        char *buf = new char[1+len+1];

        buf[len+1] = '\0';
        *buf++ = '-';

        for (size_t i = 0; i < len; ++i)
            buf[i] = tolower(var[i]);

        envvar(env, var, target.c_str(), --buf);
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

    cached:;

    /*
     * Parse command arguments late,
     * when we know our environment already
     */

    parseargs(argc, argv, target.c_str(), cmdargs, env); /* may not return */

    /*
     * Setup compiler Arguments
     */

    if (cmdargs.islinkstep && cmdargs.usemingwlinker)
    {
        compiler = target + (iscxx ? "-g++" : "-gcc");

        if (cmdargs.usemingwlinker == 2)
            warn("linking with % because of unimplemented -mwindows", compiler);

        realpath(compiler.c_str(), compiler);
    }

    if (!cmdargs.cached)
    {
        args.push_back(compiler);

        auto pushcompilerflags = [&](const string_vector &flags)
        {
            for (const auto &flag : flags)
                args.push_back(flag);
        };

        pushcompilerflags(iscxx ? cxxflags : cflags);

        if (!cmdargs.islinkstep || !cmdargs.usemingwlinker)
        {
            args.push_back(CLANG_TARGET_OPT);
            args.push_back(target);

            for (const auto &dir : stdpaths)
            {
                args.push_back("-isystem");
                args.push_back(dir);
            }

            for (const auto &dir : cxxpaths)
            {
                args.push_back("-isystem");
                args.push_back(dir);
            }
        }
    }

    for (int i = 1; i < argc; ++i)
    {
        const char *arg = argv[i];
        static constexpr size_t BUFSIZE = STRLEN(COMMANDPREFIX)+2;
        char buf[BUFSIZE];

        if (!std::strncmp(arg, COMMANDPREFIX, STRLEN(COMMANDPREFIX)))
            continue;

        *buf = '-';
        std::memcpy(buf+1, COMMANDPREFIX, BUFSIZE-1);

        if (!std::strncmp(arg, buf, BUFSIZE))
            continue;

        if (cmdargs.islinkstep && cmdargs.usemingwlinker)
        {
            if (!std::strncmp(arg, "-Qunused", STRLEN("-Qunused")))
                continue;
        }

        args.push_back(argv[i]);
    }

    cargs = new char* [args.size()+2];
    cargs[args.size()] = nullptr;

    for (const auto &opt : args)
        cargs[cargsi++] = strdup(opt.c_str());

    if (cmdargs.appendexe)
        appendexetooutputname(cargs);

    /*
     * Execute command
     */

    if (cmdargs.verbose)
    {
        std::string commandin, commandout;

        for (int i = 0; i < argc; ++i)
        {
            if (i) commandin += " ";
            commandin += argv[i];
        }

        for (char **arg = cargs; *arg; ++arg)
        {
            if (arg != cargs) commandout += " ";
            commandout += *arg;
        }

        timepoint("end");
        verbosemsg("command in: %", commandin);
        verbosemsg("command out: %", commandout);
        printtimes();
    }

    /*
     * Write cache when requested
     */

    if (!cmdargs.cached && getenv(iscxx ? "WCLANG_WRITE_CXX_CACHE" :
                                          "WCLANG_WRITE_CC_CACHE"))
    {
        writecache(cmdargs); /* no return */
    }

    execvp(compiler.c_str(), cargs);

    std::cerr << "invoking compiler failed" << std::endl;
    std::cerr << "clang not installed?" << std::endl;
    return 1;
}
