/***********************************************************************
 *  wclang                                                             *
 *  Copyright (C) 2013, 2014 by Thomas Poechtrager                     *
 *  t.poechtrager@gmail.com                                            *
 *                                                                     *
 *  This program is free software; you can redistribute it and/or      *
 *  modify it under the terms of the GNU General Public License        *
 *  as published by the Free Software Foundation; either version 2     *
 *  of the License, or (at your option) any later version.             *
 *                                                                     *
 *  This program is distributed in the hope that it will be useful,    *
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of     *
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the      *
 *  GNU General Public License for more details.                       *
 *                                                                     *
 *  You should have received a copy of the GNU General Public License  *
 *  along with this program; if not, write to the Free Software        *
 *  Foundation, Inc.,                                                  *
 *  51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.      *
 ***********************************************************************/

#include <fstream>
#include <sstream>
#include <random>
#include <unistd.h>
#include <assert.h>
#include "wclang.h"
#include "wclang_cache.h"

static std::string gencachefilename()
{
    std::stringstream filename;
    std::random_device rd;
    std::mt19937_64 gen(rd());
    std::uniform_int_distribution<ullong> dis;

    filename << "/tmp/WCLANG_CACHE_";
#ifdef HAVE_GETPID
    filename << getpid() << "_";
#endif
    filename << dis(gen);

    return filename.str();
}

void writecache(commandargs &cmdargs)
{
    std::string filename = gencachefilename();
    std::ofstream out(filename.c_str(), std::ios::binary);

    auto error = [&]()
    {
        std::cerr << "failed to write cache file ";
        std::cerr << "(" << filename << ")" << std::endl;
        std::exit(EXIT_FAILURE);
    };

    if (!out.is_open())
        error();

    auto write = [&](const void *data, uint32_t length)
    {
        out.write(reinterpret_cast<const char*>(&length), sizeof(length));
        out.write(reinterpret_cast<const char*>(data), length);
    };

    auto writestring = [&](const std::string &str)
    {
        uint32_t length = str.length();
        write(&length, sizeof(length));
        out.write(str.c_str(), str.length());
    };

    auto writestringvector = [&](const string_vector &strvec)
    {
        uint32_t count = strvec.size();
        write(&count, sizeof(count));

        for (const std::string &str : strvec)
            writestring(str);
    };

    write(&cmdargs.verbose, sizeof(cmdargs.verbose));
    writestring(cmdargs.target);
    writestring(cmdargs.compiler);
    writestringvector(cmdargs.env);
    writestringvector(cmdargs.args);
    write(&cmdargs.iscxx, sizeof(cmdargs.iscxx));
    write(&cmdargs.appendexe, sizeof(cmdargs.appendexe));

    if (!out.good())
    {
        out.close();
        std::remove(filename.c_str());
        error();
    }

    out.close();
    std::cout << filename << std::endl;
    std::exit(EXIT_SUCCESS);
}

void loadcache(const char *filename, commandargs &cmdargs)
{
    std::ifstream in(filename, std::ios::binary);
    bool iscxx = cmdargs.iscxx;

    auto error = [&]()
    {
        std::cerr << "failed to read cache file ";
        std::cerr << "(" << filename << ")" << std::endl;
        std::exit(EXIT_FAILURE);
    };

    if (!in.is_open())
        error();

    auto readlength = [&]() -> uint32_t
    {
        uint32_t size;
        uint32_t length;

        in.read(reinterpret_cast<char*>(&size), sizeof(size));
        in.read(reinterpret_cast<char*>(&length), sizeof(length));

        if (!in.good() || size != sizeof(size)) error();
        return length;
    };

    auto read = [&](void *dst, size_t size)
    {
        uint32_t length;
        in.read(reinterpret_cast<char*>(&length), sizeof(length));
        in.read(reinterpret_cast<char*>(dst), size);

        if (!in.good() || length != size)
            error();
    };

    auto loadstring = [&](std::string &str)
    {
        static constexpr uint32_t MAXLENGTH = 0xffff;
        uint32_t length = readlength();
        char *buf = nullptr;

        if (length > MAXLENGTH)
            error();

        buf = new char[length];
        in.read(buf, length);
        str = std::string(buf, length);
        delete[] buf;
    };

    auto loadstringvector = [&](string_vector &strvec)
    {
        uint32_t count = readlength();
        std::string buf;

        for (uint32_t i = 0; i < count; ++i)
        {
            loadstring(buf);
            strvec.push_back(buf);
            buf.clear();
        }
    };

    read(&cmdargs.verbose, sizeof(cmdargs.verbose));
    loadstring(cmdargs.target);
    loadstring(cmdargs.compiler);
    loadstringvector(cmdargs.env);
    loadstringvector(cmdargs.args);
    read(&cmdargs.iscxx, sizeof(cmdargs.iscxx));
    read(&cmdargs.appendexe, sizeof(cmdargs.appendexe));

    if (cmdargs.args.empty())
        error();

    assert(iscxx == cmdargs.iscxx && "invalid cache file (CC and CXX cache mixed?)");
    cmdargs.cached = true;
}
