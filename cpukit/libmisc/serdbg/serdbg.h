/*===============================================================*\
| Project: RTEMS remote gdb over serial line                      |
+-----------------------------------------------------------------+
| File: serdbg.h                                                  |
+-----------------------------------------------------------------+
|                    Copyright (c) 2002 IMD                       |
|      Ingenieurbuero fuer Microcomputertechnik Th. Doerfler      |
|               <Thomas.Doerfler@imd-systems.de>                  |
|                       all rights reserved                       |
+-----------------------------------------------------------------+
| this file declares intialization functions to add               |
| a gdb remote debug stub to an RTEMS system                      |
|                                                                 |
+-----------------------------------------------------------------+
|   date                      history                        ID   |
| ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~ |
| 04.04.02  creation                                         doe  |
\*===============================================================*/
/*
 * $Id$
 */
#ifndef _SERDBG_H
#define _SERDBG_H

#include <rtems.h>
#include <termios.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
  uint32_t   baudrate;      /* debug baud rate, e.g. 57600            */
  void (*callout)(void);    /* callout pointer during polling         */
  int  (*open_io)(const char *dev_name, uint32_t baudrate); /* I/O open fnc */
  const char *devname;      /* debug device, e.g. "/dev/tty01"        */
  bool      skip_init_bkpt; /* if TRUE, do not stop when initializing */
} serdbg_conf_t;

/*
 * must be defined in init module...
 */
extern serdbg_conf_t serdbg_conf;


/*=========================================================================*\
| Function:                                                                 |
\*-------------------------------------------------------------------------*/
void putDebugChar
(
/*-------------------------------------------------------------------------*\
| Purpose:                                                                  |
|   send character to remote debugger                                       |
+---------------------------------------------------------------------------+
| Input Parameters:                                                         |
\*-------------------------------------------------------------------------*/
 char        c                          /* char to send                    */
 );
/*-------------------------------------------------------------------------*\
| Return Value:                                                             |
|    <none>                                                                 |
\*=========================================================================*/

/*=========================================================================*\
| Function:                                                                 |
\*-------------------------------------------------------------------------*/
int getDebugChar
(
/*-------------------------------------------------------------------------*\
| Purpose:                                                                  |
|   get character from remote debugger                                      |
+---------------------------------------------------------------------------+
| Input Parameters:                                                         |
\*-------------------------------------------------------------------------*/
 void                                   /* <none>                          */
 );
/*-------------------------------------------------------------------------*\
| Return Value:                                                             |
|    <none>                                                                 |
\*=========================================================================*/

/*=========================================================================*\
| Function:                                                                 |
\*-------------------------------------------------------------------------*/
void serdbg_exceptionHandler
(
/*-------------------------------------------------------------------------*\
| Purpose:                                                                  |
|   hook directly to an exception vector                                    |
+---------------------------------------------------------------------------+
| Input Parameters:                                                         |
\*-------------------------------------------------------------------------*/
 int vecnum,                            /* vector index to hook at         */
 void *vector                           /* address of handler function     */
 );
/*-------------------------------------------------------------------------*\
| Return Value:                                                             |
|    <none>                                                                 |
\*=========================================================================*/

/*=========================================================================*\
| Function:                                                                 |
\*-------------------------------------------------------------------------*/
int serdbg_init
(
/*-------------------------------------------------------------------------*\
| Purpose:                                                                  |
|   initialize remote gdb session over serial line                          |
+---------------------------------------------------------------------------+
| Input Parameters:                                                         |
\*-------------------------------------------------------------------------*/
 void
 );
/*-------------------------------------------------------------------------*\
| Return Value:                                                             |
|    rtems_status_code                                                      |
\*=========================================================================*/

/*
 * stuff from serdbgio.c
 */
/*=========================================================================*\
| Function:                                                                 |
\*-------------------------------------------------------------------------*/
int serdbg_open

/*-------------------------------------------------------------------------*\
| Purpose:                                                                  |
|    try to open given serial debug port                                    |
+---------------------------------------------------------------------------+
| Input Parameters:                                                         |
\*-------------------------------------------------------------------------*/
(
 const char *dev_name, /* name of device to open */
 uint32_t   baudrate   /* baud rate to use       */
 );
/*-------------------------------------------------------------------------*\
| Return Value:                                                             |
|    0 on success, -1 and errno otherwise                                   |
\*=========================================================================*/

/*=========================================================================*\
| Function:                                                                 |
\*-------------------------------------------------------------------------*/
void putDebugChar
/*-------------------------------------------------------------------------*\
| Purpose:                                                                  |
|    send one character to serial port                                      |
+---------------------------------------------------------------------------+
| Input Parameters:                                                         |
\*-------------------------------------------------------------------------*/
(
 char c  /* character to print */
 );
/*-------------------------------------------------------------------------*\
| Return Value:                                                             |
|    <none>                                                                 |
\*=========================================================================*/

/*=========================================================================*\
| Function:                                                                 |
\*-------------------------------------------------------------------------*/
int getDebugChar
/*-------------------------------------------------------------------------*\
| Purpose:                                                                  |
|    wait for one character from serial port                                |
+---------------------------------------------------------------------------+
| Input Parameters:                                                         |
\*-------------------------------------------------------------------------*/
(
 void  /* none */
 );
/*-------------------------------------------------------------------------*\
| Return Value:                                                             |
|    received character                                                     |
\*=========================================================================*/

/*
 * Assumed to be provided by the BSP
 */
extern void set_debug_traps(void);
extern void breakpoint(void);
#ifdef __cplusplus
}
#endif

#endif /* _SERDBG_H */
