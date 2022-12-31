/*
	Copyright (C) 2022 Brett Kuskie <fullaxx@gmail.com>

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; version 2 of the License.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
*/

#include "chronometry.h"

void chron_start(stopwatch_t *s, int tsrc)
{
	if(tsrc == -1) { tsrc = CLOCK_MONOTONIC; }
	s->source = (tsrc);
	clock_gettime(s->source, &s->start);
}

long chron_stop(stopwatch_t *s)
{
	clock_gettime(s->source, &s->stop);
	return (((s->stop.tv_sec - s->start.tv_sec)*1e9) + (s->stop.tv_nsec - s->start.tv_nsec));
}
