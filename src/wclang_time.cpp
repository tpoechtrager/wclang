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

#include <tuple>
#include <stdexcept>
#include <vector>
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
