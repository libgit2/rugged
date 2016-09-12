/*
 * The MIT License
 *
 * Copyright (c) 2014 GitHub, Inc
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

 #ifndef __H_RUGGED_TIME__
 #define __H_RUGGED_TIME__

#if defined(_MSC_VER)
# define GIT_INLINE(type) static __inline type
#else
# define GIT_INLINE(type) static inline type
#endif

#ifdef GIT_WIN32

GIT_INLINE(double) rugged__timer(void)
{
	/* We need the initial tick count to detect if the tick
	 * count has rolled over. */
	static DWORD initial_tick_count = 0;

	/* GetTickCount returns the number of milliseconds that have
	 * elapsed since the system was started. */
	DWORD count = GetTickCount();

	if(initial_tick_count == 0) {
		initial_tick_count = count;
	} else if (count < initial_tick_count) {
		/* The tick count has rolled over - adjust for it. */
		count = (0xFFFFFFFF - initial_tick_count) + count;
	}

	return (double) count / (double) 1000;
}

#elif __APPLE__

#include <mach/mach_time.h>

GIT_INLINE(double) rugged__timer(void)
{
   uint64_t time = mach_absolute_time();
   static double scaling_factor = 0;

   if (scaling_factor == 0) {
       mach_timebase_info_data_t info;
       (void)mach_timebase_info(&info);
       scaling_factor = (double)info.numer / (double)info.denom;
   }

   return (double)time * scaling_factor / 1.0E9;
}

#elif defined(AMIGA)

#include <proto/timer.h>

GIT_INLINE(double) rugged__timer(void)
{
	struct TimeVal tv;
	ITimer->GetUpTime(&tv);
	return (double)tv.Seconds + (double)tv.Microseconds / 1.0E6;
}

#else

#include <sys/time.h>

GIT_INLINE(double) rugged__timer(void)
{
	struct timespec tp;

	if (clock_gettime(CLOCK_MONOTONIC, &tp) == 0) {
		return (double) tp.tv_sec + (double) tp.tv_nsec / 1.0E9;
	} else {
		/* Fall back to using gettimeofday */
		struct timeval tv;
		struct timezone tz;
		gettimeofday(&tv, &tz);
		return (double)tv.tv_sec + (double)tv.tv_usec / 1.0E6;
	}
}

#endif

#endif
