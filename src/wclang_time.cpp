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

#include <tuple>
#include <stdexcept>
#include "wclang.h"
#include "wclang_time.h"

#ifdef HAVE_STD_CHRONO
time_point getticks()
{
    return std::chrono::steady_clock::now();
}
ullong getmicrodiff(time_point &start, time_point time)
{
    using namespace std;
    using namespace std::chrono;
    return duration_cast<duration<ullong, micro>>(time-start).count();
}
#else
/*
 * steady_clock and monotonic_clock are broken in
 * debian/ubuntu g++-4.6 and g++-4.7
 * http://bugs.debian.org/cgi-bin/bugreport.cgi?bug=710220
 */
time_point getticks()
{
    struct timeval tv;
    if (gettimeofday(&tv, NULL) == 0)
        return (time_point)((tv.tv_sec*1000000ULL)+tv.tv_usec);
    ERROR("gettimeofday() failed");
}
ullong getmicrodiff(time_point &start, time_point time)
{
    return time-start;
}
#endif
