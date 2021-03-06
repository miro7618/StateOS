/******************************************************************************

    @file    StateOS: os_bar.c
    @author  Rajmund Szymanski
    @date    13.04.2018
    @brief   This file provides set of functions for StateOS.

 ******************************************************************************

   Copyright (c) 2018 Rajmund Szymanski. All rights reserved.

   Permission is hereby granted, free of charge, to any person obtaining a copy
   of this software and associated documentation files (the "Software"), to
   deal in the Software without restriction, including without limitation the
   rights to use, copy, modify, merge, publish, distribute, sublicense, and/or
   sell copies of the Software, and to permit persons to whom the Software is
   furnished to do so, subject to the following conditions:

   The above copyright notice and this permission notice shall be included
   in all copies or substantial portions of the Software.

   THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS
   OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
   FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
   THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
   LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
   FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS
   IN THE SOFTWARE.

 ******************************************************************************/

#include "inc/os_bar.h"

/* -------------------------------------------------------------------------- */
void bar_init( bar_t *bar, unsigned limit )
/* -------------------------------------------------------------------------- */
{
	assert(!port_isr_inside());
	assert(bar);
	assert(limit);

	port_sys_lock();

	memset(bar, 0, sizeof(bar_t));

	bar->count = limit;
	bar->limit = limit;

	port_sys_unlock();
}

/* -------------------------------------------------------------------------- */
bar_t *bar_create( unsigned limit )
/* -------------------------------------------------------------------------- */
{
	bar_t *bar;

	assert(!port_isr_inside());
	assert(limit);

	port_sys_lock();

	bar = core_sys_alloc(sizeof(bar_t));
	bar_init(bar, limit);
	bar->res = bar;

	port_sys_unlock();

	return bar;
}

/* -------------------------------------------------------------------------- */
void bar_kill( bar_t *bar )
/* -------------------------------------------------------------------------- */
{
	assert(!port_isr_inside());
	assert(bar);
	
	port_sys_lock();

	bar->count = bar->limit;

	core_all_wakeup(bar, E_STOPPED);

	port_sys_unlock();
}

/* -------------------------------------------------------------------------- */
void bar_delete( bar_t *bar )
/* -------------------------------------------------------------------------- */
{
	port_sys_lock();

	bar_kill(bar);
	core_sys_free(bar->res);

	port_sys_unlock();
}

/* -------------------------------------------------------------------------- */
static
unsigned priv_bar_wait( bar_t *bar, cnt_t time, unsigned(*wait)(void*,cnt_t) )
/* -------------------------------------------------------------------------- */
{
	unsigned event = E_SUCCESS;

	assert(!port_isr_inside());
	assert(bar);
	assert(bar->count);

	port_sys_lock();

	if (--bar->count > 0)
	{
		event = wait(bar, time);
	}
	else
	{
		bar->count = bar->limit;

		core_all_wakeup(bar, E_SUCCESS);
	}

	port_sys_unlock();

	return event;
}

/* -------------------------------------------------------------------------- */
unsigned bar_waitUntil( bar_t *bar, cnt_t time )
/* -------------------------------------------------------------------------- */
{
	return priv_bar_wait(bar, time, core_tsk_waitUntil);
}

/* -------------------------------------------------------------------------- */
unsigned bar_waitFor( bar_t *bar, cnt_t delay )
/* -------------------------------------------------------------------------- */
{
	return priv_bar_wait(bar, delay, core_tsk_waitFor);
}

/* -------------------------------------------------------------------------- */
