/*  main
 *
 *  This program is run to determine the data space and work space
 *  requirements of the current version of RTEMS.
 *
 *  COPYRIGHT (c) 1989, 1990, 1991, 1992, 1993, 1994.
 *  On-Line Applications Research Corporation (OAR).
 *  All rights assigned to U.S. Government, 1994.
 *
 *  This material may be reproduced by or for the U.S. Government pursuant
 *  to the copyright license under the clause at DFARS 252.227-7013.  This
 *  notice must appear in all copies of this file and its derivatives.
 *
 *  $Id$
 */

#include <rtems/system.h>
#include <rtems/config.h>
#include <rtems/directives.h>
#include <rtems/score/apiext.h>
#include <rtems/score/copyrt.h>
#include <rtems/rtems/clock.h>
#include <rtems/rtems/tasks.h>
#include <rtems/rtems/dpmem.h>
#include <rtems/rtems/event.h>
#include <rtems/extension.h>
#include <rtems/fatal.h>
#include <rtems/init.h>
#include <rtems/score/isr.h>
#include <rtems/rtems/intr.h>
#include <rtems/io.h>
#include <rtems/rtems/message.h>
#include <rtems/rtems/mp.h>
#include <rtems/score/mpci.h>
#include <rtems/rtems/part.h>
#include <rtems/score/priority.h>
#include <rtems/rtems/ratemon.h>
#include <rtems/rtems/region.h>
#include <rtems/rtems/sem.h>
#include <rtems/rtems/signal.h>
#include <rtems/score/sysstate.h>
#include <rtems/score/thread.h>
#include <rtems/rtems/timer.h>
#include <rtems/score/tod.h>
#include <rtems/score/userext.h>
#include <rtems/score/wkspace.h>

#include <stdlib.h>

/* These are always defined by the executive.
 *
 * #include <rtems/copyrt.h>
 * #include <rtems/tables.h>
 * #include <rtems/sptables.h>
 */
#define  HEAP_OVHD        16    /* wasted heap space per task stack */
#define  NAME_PTR_SIZE     8    /* size of name and pointer table entries */
#define  READYCHAINS_SIZE  \
    ((RTEMS_MAXIMUM_PRIORITY + 1) * sizeof(Chain_Control ))

#define PER_TASK      \
     (sizeof (Thread_Control) + \
      NAME_PTR_SIZE + HEAP_OVHD + sizeof( RTEMS_API_Control ))
#define PER_SEMAPHORE \
     (sizeof (Semaphore_Control) + NAME_PTR_SIZE)
#define PER_TIMER     \
     (sizeof (Timer_Control) + NAME_PTR_SIZE)
#define PER_MSGQ      \
     (sizeof (Message_queue_Control) + NAME_PTR_SIZE)
#define PER_REGN      \
     (sizeof (Region_Control) + NAME_PTR_SIZE)
#define PER_PART      \
     (sizeof (Partition_Control) + NAME_PTR_SIZE)
#define PER_PERIOD      \
     (sizeof (Rate_monotonic_Control) + NAME_PTR_SIZE)
#define PER_PORT      \
     (sizeof (Dual_ported_memory_Control) + NAME_PTR_SIZE)
#define PER_EXTENSION     \
     (sizeof (Extension_Control) + NAME_PTR_SIZE)

#define PER_DRV       (0)
#define PER_FPTASK    (CONTEXT_FP_SIZE)
#define PER_GOBTBL    (sizeof (Chain_Control)*4)
#define PER_NODE      PER_GOBTBL
#define PER_GOBJECT   (sizeof (Objects_MP_Control))
#define PER_PROXY     (sizeof (Thread_Proxy_control))

#if (CPU_ALL_TASKS_ARE_FP == TRUE)
#define MPCI_RECEIVE_SERVER_FP (sizeof( Context_Control_fp ))
#else
#define MPCI_RECEIVE_SERVER_FP 0
#endif

#if (CPU_IDLE_TASK_IS_FP == TRUE)
#define SYSTEM_IDLE_FP (sizeof( Context_Control_fp ))
#else
#define SYSTEM_IDLE_FP 0
#endif

#define SYSTEM_TASKS  \
    (THREAD_IDLE_STACK_SIZE + \
     MPCI_RECEIVE_SERVER_STACK_SIZE + \
     (2*sizeof(Thread_Control))) + \
     MPCI_RECEIVE_SERVER_FP + \
     SYSTEM_IDLE_FP

#define rtems_unsigned32 unsigned32

rtems_unsigned32 sys_req;

/* to avoid warnings */
int puts();
int printf();
int getint();
#undef getchar
int getchar();
void help_size();
void print_formula();

void size_rtems(
  int mode
)
{
int uninitialized = 0;
int initialized = 0;

/*
 *  The following data is allocated for each Manager:
 *
 *    + Per Manager Object Information
 *      - local pointer table
 *      - local name table
 *      - the object's control blocks
 *      - global name chains
 *
 *  The following is the data allocate from the RTEMS Workspace Area.
 *  The order indicates the order in which RTEMS allocates it.
 *
 *    + Object MP
 *      - Global Object CB's
 *    + Thread
 *      - Ready Chain
 *    + Thread MP
 *      - Proxies Chain
 *    + Interrupt Manager
 *      - Interrupt Stack
 *    + Timer Manager
 *      - per Manager Object Data
 *    + Extension Manager
 *      - per Manager Object Data
 *    + Message Queue Manager
 *      - per Manager Object Data
 *      - Message Buffers
 *    + Semaphore Manager
 *      - per Manager Object Data
 *    + Partition Manager
 *      - per Manager Object Data
 *    + Region Manager
 *      - per Manager Object Data
 *    + Dual Ported Memory Manager
 *      - per Manager Object Data
 *    + Rate Monotonic Manager
 *      - per Manager Object Data
 *    + Internal Threads Handler
 *      - MPCI Receive Server Thread TCB
 *      - IDLE Thread TCB
 *      - MPCI Receive Server Thread stack
 *      - MPCI Receive Server Thread FP area (if CPU requires this)
 *      - IDLE Thread stack
 *      - IDLE Thread FP area (if CPU requires this)
 *
 *  This does not take into account any CPU dependent alignment requirements.
 *
 *  The following calculates the overhead needed by RTEMS from the
 *  Workspace Area.
 */
sys_req = SYSTEM_TASKS     +     /* MPCI Receive Server and IDLE */
          NAME_PTR_SIZE    +     /* Task Overhead */
          READYCHAINS_SIZE +     /* Ready Chains */
          NAME_PTR_SIZE    +     /* Timer Overhead */
          NAME_PTR_SIZE    +     /* Semaphore Overhead */
          NAME_PTR_SIZE    +     /* Message Queue Overhead */
          NAME_PTR_SIZE    +     /* Region Overhead */
          NAME_PTR_SIZE    +     /* Partition Overhead */
          NAME_PTR_SIZE    +     /* Dual-Ported Memory Overhead */
          NAME_PTR_SIZE    +     /* Rate Monotonic Overhead */
          NAME_PTR_SIZE    +     /* Extension Overhead */
          PER_NODE;              /* Extra Gobject Table */

uninitialized =
/*address.h*/   0                                         +

/*apiext.h*/    (sizeof _API_extensions_List)             +

/*asr.h*/       0                                         +

/*attr.h*/      0                                         +

/*bitfield.h*/  0                                         +

/*chain.h*/     0                                         +

/*clock.h*/     0                                         +

/*config.h*/    (sizeof _Configuration_Table)             +
                (sizeof _Configuration_MP_table)          +

/*context.h*/   (sizeof _Context_Switch_necessary)        +

/*copyrt.h*/    0                                         +

/*debug.h*/     (sizeof _Debug_Level)                     +

/*dpmem.h*/     (sizeof _Dual_ported_memory_Information)  +

/*event.h*/     (sizeof _Event_Sync_state)                +

/*eventmp.h*/   0                                         +

/*eventset.h*/  0                                         +

/*extension.h*/ (sizeof _Extension_Information)           +

/*fatal.h*/     0                                         +

/*heap.h*/      0                                         +

/*init.h*/      0                                         +

/*interr.h*/    (sizeof Internal_errors_What_happened)    +

/*intr.h*/      0                                         +

/*io.h*/        (sizeof _IO_Number_of_drivers)            +
                (sizeof _IO_Driver_address_table)         +
                (sizeof _IO_Number_of_devices)            +
                (sizeof _IO_Driver_name_table)            +

/*isr.h*/       (sizeof _ISR_Nest_level)                  +
                (sizeof _ISR_Vector_table)                +
                (sizeof _ISR_Signals_to_thread_executing) +

/*message.h*/   (sizeof _Message_queue_Information)       +

/*modes.h*/     0                                         +

/*mp.h*/        0                                         +

/*mpci.h*/      (sizeof _MPCI_Remote_blocked_threads)     +
                (sizeof _MPCI_Semaphore)                  +
                (sizeof _MPCI_table)                      +
                (sizeof _MPCI_Receive_server_tcb)         +
                (sizeof _MPCI_Packet_processors)          +

/*mppkt.h*/     0                                         +

/*mptables.h*/  0                                         +

/*msgmp.h*/     0                                         +

/*object.h*/    (sizeof _Objects_Local_node)              +
                (sizeof _Objects_Maximum_nodes)           +
                (sizeof _Objects_Information_table)       +

/*objectmp.h*/  (sizeof _Objects_MP_Maximum_global_objects) +
                (sizeof _Objects_MP_Inactive_global_objects) +

/*options.h*/   0                                         +

/*part.h*/      (sizeof _Partition_Information)           +

/*partmp.h*/    0                                         +

/*priority.h*/  (sizeof _Priority_Major_bit_map)          +
                (sizeof _Priority_Bit_map)                +

/*ratemon.h*/   (sizeof _Rate_monotonic_Information)      +

/*region.h*/    (sizeof _Region_Information)              +

/*regionmp.h*/  0                                         +

/*rtems.h*/     /* Not applicable */

/*sem.h*/       (sizeof _Semaphore_Information)           +

/*semmp.h*/     0                                         +

/*signal.h*/    0                                         +

/*signalmp.h*/  0                                         +

/*stack.h*/     0                                         +

/*states.h*/    0                                         +

/*status.h*/    0                                         +

/*sysstate.h*/  (sizeof _System_state_Is_multiprocessing) +
                (sizeof _System_state_Current)            +

/*system.h*/    (sizeof _CPU_Table)                       +

/*taskmp.h*/    0                                         +

/*tasks.h*/     (sizeof _RTEMS_tasks_Information)         +
                (sizeof _RTEMS_tasks_User_initialization_tasks) +
                (sizeof _RTEMS_tasks_Number_of_initialization_tasks) +

/*thread.h*/    (sizeof _Thread_BSP_context)              +
                (sizeof _Thread_Dispatch_disable_level)   +
                (sizeof _Thread_Maximum_extensions)       +
                (sizeof _Thread_Ticks_per_timeslice)      +
                (sizeof _Thread_Ready_chain)              +
                (sizeof _Thread_Executing)                +
                (sizeof _Thread_Heir)                     +
                (sizeof _Thread_Allocated_fp)             +
                (sizeof _Thread_Internal_information)     +
                (sizeof _Thread_Idle)                     +

/*threadmp.h*/  (sizeof _Thread_MP_Receive)               +
                (sizeof _Thread_MP_Active_proxies)        +
                (sizeof _Thread_MP_Inactive_proxies)      +

/*threadq.h*/   (sizeof _Thread_queue_Extract_table)      +

/*timer.h*/     (sizeof _Timer_Information)               +

/*tod.h*/       (sizeof _TOD_Current)                     +
                (sizeof _TOD_Seconds_since_epoch)         +
                (sizeof _TOD_Microseconds_per_tick)       +
                (sizeof _TOD_Ticks_per_second)            +
                (sizeof _TOD_Seconds_watchdog)            +

/*tqdata.h*/    0                                         +

/*types.h*/     0                                         +

/*userext.h*/   (sizeof _User_extensions_Initial)         +
                (sizeof _User_extensions_List)            +

/*watchdog.h*/  (sizeof _Watchdog_Sync_level)             +
                (sizeof _Watchdog_Sync_count)             +
                (sizeof _Watchdog_Ticks_since_boot)       +
                (sizeof _Watchdog_Ticks_chain)            +
                (sizeof _Watchdog_Seconds_chain)          +

/*wkspace.h*/   (sizeof _Workspace_Area);

uninitialized = 0;

#ifndef unix  /* make sure this is not a native compile */

#ifdef i386

/* cpu.h */
uninitialized += (sizeof _CPU_Null_fp_context) +
                 (sizeof _CPU_Interrupt_stack_low) +
                 (sizeof _CPU_Interrupt_stack_high);

#endif

#ifdef i960

/* cpu.h */
uninitialized += (sizeof _CPU_Interrupt_stack_low) +
                 (sizeof _CPU_Interrupt_stack_high);

#endif

#ifdef hppa1_1

/* cpu.h */
uninitialized += (sizeof _CPU_Null_fp_context) +
#ifndef RTEMS_UNIX
                 (sizeof _CPU_Default_gr27) +
#endif
                 (sizeof _CPU_Interrupt_stack_low) +
                 (sizeof _CPU_Interrupt_stack_high);
#endif

#ifdef m68k

/* cpu.h */
uninitialized += (sizeof _CPU_Interrupt_stack_low) +
                 (sizeof _CPU_Interrupt_stack_high);

#endif

#ifdef sparc
 
/* cpu.h */
uninitialized += (sizeof _CPU_Interrupt_stack_low) +
                 (sizeof _CPU_Interrupt_stack_high) +
                 (sizeof _CPU_Null_fp_context) +
                 (sizeof _CPU_Trap_Table_area);

#ifdef erc32
uninitialized += (sizeof _ERC32_MEC_Timer_Control_Mirror);
#endif

 
#endif


#ifdef no_cpu

/* cpu.h */
uninitialized += (sizeof _CPU_Null_fp_context) +
                 (sizeof _CPU_Interrupt_stack_low) +
                 (sizeof _CPU_Interrupt_stack_high) +
                 (sizeof _CPU_Thread_dispatch_pointer);

#endif

#ifdef ppc

/* cpu.h */
uninitialized += (sizeof _CPU_Interrupt_stack_low) +
                 (sizeof _CPU_Interrupt_stack_high) +
                 (sizeof _CPU_IRQ_info);

#endif
#endif /* !unix */

initialized +=
/*copyrt.h*/    (strlen(_Copyright_Notice)+1)             +

/*sptables.h*/  (sizeof _Initialization_Default_multiprocessing_table)  +
                (strlen(_RTEMS_version)+1)      +
                (sizeof _Entry_points)          +


/*tod.h*/       (sizeof _TOD_Days_per_month)    +
                (sizeof _TOD_Days_to_date)      +
                (sizeof _TOD_Days_since_last_leap_year);

#ifndef unix /* make sure this is not native */
#ifdef sparc

initialized +=  (sizeof _CPU_Trap_slot_template);

#endif
#endif /* !unix */

puts( "" );

  if ( mode == 0 ) help_size();
  else             print_formula();

printf( "\n" );
printf( "RTEMS uninitialized data consumes %d bytes\n", uninitialized );
printf( "RTEMS intialized data consumes %d bytes\n", initialized );

}

void help_size()
{
int c = '\0';
int break_loop;
int total_size;
int task_stacks;
int interrupt_stack;
int maximum_tasks, size_tasks;
int maximum_sems, size_sems;
int maximum_timers, size_timers;
int maximum_msgqs, size_msgqs;
int maximum_msgs, size_msgs_overhead;
int maximum_regns, size_regns;
int maximum_parts, size_parts;
int maximum_ports, size_ports;
int maximum_periods, size_periods;
int maximum_extensions, size_extensions;
int maximum_drvs, size_drvs;
int maximum_fps, size_fps;
int maximum_nodes, size_nodes;
int maximum_gobjs, size_gobjs;
int maximum_proxies, size_proxies;

total_size = sys_req;    /* Fixed Overhead */
printf( "What is maximum_tasks? " );
maximum_tasks = getint();
size_tasks = PER_TASK * maximum_tasks;
total_size += size_tasks;

printf( "What is maximum_semaphores? " );
maximum_sems = getint();
size_sems = PER_SEMAPHORE * maximum_sems;
total_size += size_sems;

printf( "What is maximum_timers? " );
maximum_timers = getint();
size_timers = PER_TIMER * maximum_timers;
total_size += size_timers;

printf( "What is maximum_message_queues? " );
maximum_msgqs = getint();
size_msgqs = PER_MSGQ * maximum_msgqs;
total_size += size_msgqs;

printf( "What is maximum_messages?  XXXX " );
maximum_msgs = getint();
size_msgs_overhead = 0;
total_size += size_msgs_overhead;

printf( "What is maximum_regions? " );
maximum_regns = getint();
size_regns = PER_REGN * maximum_regns;
total_size += size_regns;

printf( "What is maximum_partitions? " );
maximum_parts = getint();
size_parts = PER_PART * maximum_parts;
total_size += size_parts;

printf( "What is maximum_ports? " );
maximum_ports = getint();
size_ports = PER_PORT * maximum_ports;
total_size += size_ports;

printf( "What is maximum_periods? " );
maximum_periods = getint();
size_periods = PER_PORT * maximum_periods;
total_size += size_periods;

printf( "What is maximum_extensions? " );
maximum_extensions = getint();
size_extensions = PER_EXTENSION * maximum_extensions;
total_size += size_extensions;

printf( "What is number_of_device_drivers? " );
maximum_drvs = getint();
size_drvs = PER_DRV  * maximum_drvs;
total_size += size_drvs;

printf( "What will be total stack requirement for all tasks? " );
task_stacks = getint();
total_size += task_stacks;

printf( "What is the size of the interrupt stack? " );
interrupt_stack = getint();
total_size += interrupt_stack;

printf( "How many tasks will be created with the FP flag? " );
maximum_fps = getint();
size_fps = PER_FPTASK  * maximum_fps;
total_size += size_fps;

printf( "Is this a single processor system? " );
for ( break_loop=0 ; !break_loop; c = getchar() ) {
  switch ( c ) {
    case 'Y':  case 'y':
    case 'N':  case 'n':
      break_loop = 1;
      break;
  }
}
printf( "%c\n", c );
if ( c == 'n' || c == 'N' ) {
  printf( "What is maximum_nodes? " );
  maximum_nodes = getint();
  size_nodes = PER_NODE * maximum_nodes;
  total_size += size_nodes;
  printf( "What is maximum_global_objects? " );
  maximum_gobjs = getint();
  size_gobjs = PER_GOBJECT * maximum_gobjs;
  total_size += size_gobjs;
  printf( "What is maximum_proxies? " );
  maximum_proxies = getint();
  size_proxies = PER_PROXY * maximum_proxies;
  total_size += size_proxies;
} else {
  maximum_nodes = 0;
  size_nodes = PER_NODE * 0;
  maximum_gobjs = 0;
  size_gobjs = PER_GOBJECT * 0;
  maximum_proxies = 0;
  size_proxies = PER_PROXY * 0;
}

printf( "\n\n" );
printf( " ************** EXECUTIVE WORK SPACE REQUIRED **************\n" );
printf( " Tasks                - %03d * %03d            =  %d\n",
          maximum_tasks, PER_TASK, size_tasks );
printf( " Semaphores           - %03d * %03d            =  %d\n",
          maximum_sems, PER_SEMAPHORE, size_sems );
printf( " Timers               - %03d * %03d            =  %d\n",
          maximum_timers, PER_TIMER, size_timers );
printf( " Msg Queues           - %03d * %03d            =  %d\n",
          maximum_msgqs, PER_MSGQ, size_msgqs );
printf( " Messages Overhead    - %03d * %03d            =  %d\n",
          maximum_msgs, 0 /* PER_MSG_OVERHEAD */, size_msgs_overhead );
printf( " Regions              - %03d * %03d            =  %d\n",
          maximum_regns, PER_REGN, size_regns);
printf( " Partitions           - %03d * %03d            =  %d\n",
          maximum_parts, PER_PART, size_parts );
printf( " Periods              - %03d * %03d            =  %d\n",
          maximum_periods, PER_PERIOD, size_periods );
printf( " Extensions           - %03d * %03d            =  %d\n",
          maximum_extensions, PER_EXTENSION, size_extensions );
printf( " Device Drivers       - %03d * %03d            =  %d\n",
          maximum_drvs, PER_DRV, size_drvs );

printf( " System Requirements  - %04d                 =  %d\n",
          sys_req, sys_req );

printf( " Floating Point Tasks - %03d * %03d            =  %d\n",
          maximum_fps, PER_FPTASK, size_fps );
printf( " Application Task Stacks -                     =  %d\n",
          task_stacks );
printf( " Interrupt Stacks -                            =  %d\n",
          task_stacks );
printf( " \n" );
printf( " Global object tables - %03d * %03d            =  %d\n",
          maximum_nodes, PER_NODE, size_nodes );
printf( " Global objects       - %03d * %03d            =  %d\n",
          maximum_gobjs, PER_GOBJECT, size_gobjs );
printf( " Proxies              - %03d * %03d            =  %d\n",
          maximum_proxies, PER_PROXY, size_proxies );
printf( "\n\n" );
printf( " TOTAL                                       = %d bytes\n",
      total_size );
}

void print_formula()
{
printf( " ************** EXECUTIVE WORK SPACE FORMULA **************\n" );
printf( " Tasks                - maximum_tasks * %d\n",      PER_TASK );
printf( " Timers               - maximum_timers * %d\n",     PER_TIMER );
printf( " Semaphores           - maximum_semaphores * %d\n", PER_SEMAPHORE);
printf( " Message Queues       - maximum_message_queues * %d\n", PER_MSGQ );
printf( " Messages             -\n");
printf( " Regions              - maximum_regions * %d\n",    PER_REGN );
printf( " Partitions           - maximum_partitions * %d\n", PER_PART );
printf( " Ports                - maximum_ports * %d\n",      PER_PORT );
printf( " Periods              - maximum_periods * %d\n",    PER_PORT );
printf( " Extensions           - maximum_extensions * %d\n", PER_EXTENSION );
printf( " Device Drivers       - number_of_device_drivers * %d\n", PER_DRV);
printf( " System Requirements  - %d\n",                      sys_req );
printf( " Floating Point Tasks - FPMASK Tasks * %d\n",       CONTEXT_FP_SIZE );
printf( " User's Tasks' Stacks -\n" );
printf( " Interrupt Stack      -\n" );
printf( " \n" );
printf( " Global object tables - maximum_nodes * %d\n",          PER_NODE );
printf( " Global objects       - maximum_global_objects * %d\n", PER_GOBJECT );
printf( " Proxies              - maximum_proxies * %d\n",        PER_PROXY );
}
