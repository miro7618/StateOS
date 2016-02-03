/******************************************************************************

    @file    StateOS: oskernel.h
    @author  Rajmund Szymanski
    @date    03.02.2016
    @brief   This file defines set of kernel functions for StateOS.

 ******************************************************************************

    StateOS - Copyright (C) 2013 Rajmund Szymanski.

    This file is part of StateOS distribution.

    StateOS is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published
    by the Free Software Foundation; either version 3 of the License,
    or (at your option) any later version.

    StateOS is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.

 ******************************************************************************/

#pragma once

#include <osbase.h>
#include <bitband.h>
#include <stdlib.h>

#ifdef __cplusplus
extern "C" {
#endif

/* -------------------------------------------------------------------------- */

extern sys_t System; // system data

/* -------------------------------------------------------------------------- */

// initiating and running the system timer
// the port_sys_init procedure is normally called as a constructor
void port_sys_init( void );

// procedure performed in the 'idle' task; waiting for an interrupt
void port_idle_hook( void );

/* -------------------------------------------------------------------------- */

static inline
void core_ctx_reset( void )
{
#if OS_ROBIN && OS_TIMER == 0
	System.dly = 0;
#endif
	port_ctx_reset();
}

// save status of current process and force yield system control to the next
void core_ctx_switch( void );

// enable context switch
void port_ctx_switch( void );

/* -------------------------------------------------------------------------- */

// abort and reset current process and force yield system control to the next
void core_tsk_break( void ) __noreturn;

/* -------------------------------------------------------------------------- */

// force timer interrupt
void port_tmr_force( void );

// clear time breakpoint
void port_tmr_stop( void );

// set time breakpoint
void port_tmr_start( unsigned timeout );

/* -------------------------------------------------------------------------- */

// system malloc procedure
os_id core_sys_alloc( size_t size );

/* -------------------------------------------------------------------------- */

// insert object 'obj' into tasks/timers READY queue before the 'nxt' object
// set object id to 'id'
void core_rdy_insert( os_id obj, unsigned id, os_id nxt );

// remove object 'obj' from tasks/timers READY queue
// set object id to ID_STOPPED
void core_rdy_remove( os_id obj );

/* -------------------------------------------------------------------------- */

// add timer 'tmr' to timers READY queue with id 'id'
// start countdown
void core_tmr_insert( tmr_id tmr, unsigned id );

// remove timer 'tmr' from timers READY queue
static inline
void core_tmr_remove( tmr_id tmr ) { core_rdy_remove(tmr); }

// timers queue handler procedure
void core_tmr_handler( void );

/* -------------------------------------------------------------------------- */

// add task 'tsk' to tasks READY queue with id ID_READY
// force context switch if priority of task 'tsk' is greater then priority of current task and kernel works in preemptive mode
void     core_tsk_insert( tsk_id tsk );

// remove task 'tsk' from tasks READY queue
static inline
void     core_tsk_remove( tsk_id tsk ) { core_rdy_remove(tsk); }

// append task 'tsk' to object 'obj' delayed queue
void     core_tsk_append( tsk_id tsk, os_id obj );

// remove task 'tsk' from object 'obj' delayed queue
// with 'event' event value
void     core_tsk_unlink( tsk_id tsk, unsigned event );

// delay execution of current task until given timepoint 'time'
// append current task to object 'obj' delayed queue
// remove current task from tasks READY queue
// add current task to timers READY queue
// force context switch
// return event value
unsigned core_tsk_waitUntil( os_id obj, unsigned time );

// delay execution of current task for given duration of time 'delay'
// append current task to object 'obj' delayed queue
// remove current task from tasks READY queue
// add current task to timers READY queue
// force context switch
// return event value
unsigned core_tsk_waitFor( os_id obj, unsigned delay );

// resume execution of delayed task 'tsk' with 'event' event value
// remove task 'tsk' from guard object delayed queue
// remove task 'tsk' from timers READY queue
// add task 'tsk' to tasks READY queue
// force context switch if priority of task 'tsk' is greater then priority of current task and kernel works in preemptive mode
// return 'tsk'
tsk_id   core_tsk_wakeup( tsk_id tsk, unsigned event );

// resume execution of first task from object 'obj' delayed queue with 'event' event value
// remove first task from object 'obj' delayed queue
// remove resumed task from timers READY queue
// add resumed task to tasks READY queue
// force context switch if priority of resumed task is greater then priority of current task and kernel works in preemptive mode
// return pointer to resumed task
tsk_id   core_one_wakeup( os_id obj, unsigned event );

// resume execution of all tasks from object 'obj' delayed queue with 'event' event value
// remove all tasks from object 'obj' delayed queue
// remove all resumed tasks from timers READY queue
// add all resumed tasks to tasks READY queue
// force context switch if priority of any resumed task is greater then priority of current task and kernel works in preemptive mode
void     core_all_wakeup( os_id obj, unsigned event );

// set task 'tsk' priority
// force context switch if new priority of task 'tsk' is greater then priority of current task and kernel works in preemptive mode
void     core_tsk_prio( tsk_id tsk, unsigned prio );

// tasks queue handler procedure
// reset context switch timer counter
// return a pointer to next READY task the highest priority
tsk_id   core_tsk_handler( void );

/* -------------------------------------------------------------------------- */

// procedure inside ISR?
static inline
unsigned port_isr_inside( void )
{
	return __get_IPSR();
}

/* -------------------------------------------------------------------------- */

// reset task stack pointer
static inline
void port_set_stack( void *top )
{
	__set_PSP((unsigned)top);
}

/* -------------------------------------------------------------------------- */

#if OS_LOCK_LEVEL && (__CORTEX_M >= 3)

static inline unsigned port_get_lock( void )           { return __get_BASEPRI();                                      }
static inline void     port_put_lock( unsigned state ) {        __set_BASEPRI(state);                                 }
static inline void     port_set_lock( void )           {        __set_BASEPRI((OS_LOCK_LEVEL)<<(8-__NVIC_PRIO_BITS)); }
static inline void     port_clr_lock( void )           {        __set_BASEPRI(0);                                     }

#else

static inline unsigned port_get_lock( void )           { return __get_PRIMASK();      }
static inline void     port_put_lock( unsigned state ) {        __set_PRIMASK(state); }
static inline void     port_set_lock( void )           {        __disable_irq();      }
static inline void     port_clr_lock( void )           {         __enable_irq();      }

#endif

#define port_sys_lock()                             do { unsigned __LOCK = port_get_lock(); port_set_lock()
#define port_sys_unlock()                                port_put_lock(__LOCK); } while(0)

#define port_sys_enable()                           do { unsigned __LOCK = port_get_lock(); port_clr_lock()
#define port_sys_disable()                               port_put_lock(__LOCK); } while(0)

#define port_isr_lock()                             do { port_set_lock()
#define port_isr_unlock()                                port_clr_lock(); } while(0)

#define port_isr_enable()                           do { port_clr_lock()
#define port_isr_disable()                               port_set_lock(); } while(0)

/* -------------------------------------------------------------------------- */

static inline
void port_sys_flash( void )
{
	port_sys_enable();
	port_sys_disable();
}

/* -------------------------------------------------------------------------- */

#ifdef __cplusplus
}
#endif
