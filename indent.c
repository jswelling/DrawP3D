/****************************************************************************
 * indent.c
 * Author Joel Welling
 * Copyright 1989, Pittsburgh Supercomputing Center, Carnegie Mellon University
 *
 * Permission use, copy, and modify this software and its documentation
 * without fee for personal use or use within your organization is hereby
 * granted, provided that the above copyright notice is preserved in all
 * copies and that that copyright and this permission notice appear in
 * supporting documentation.  Permission to redistribute this software to
 * other organizations or individuals is not granted;  that must be
 * negotiated with the PSC.  Neither the PSC nor Carnegie Mellon
 * University make any representations about the suitability of this
 * software for any purpose.  It is provided "as is" without express or
 * implied warranty.
 *****************************************************************************/
/*
This package provides an 'indented writing' capability.
*/
/* If __STDC__ is defined, this package uses stdarg.h-type variable length
 * parameter lists.  If not, and if USE_VARARGS is defined (it's set below),
 * varargs.h-type parameter lists are used.  Otherwise, a system which
 * 'usually' works is used.
 */
#define USE_VARARGS 1

#include <stdio.h>

#ifdef SUN_SPARC
#include <varargs.h>
#else

#ifdef __STDC__
#include <stdarg.h>
#else

#ifdef USE_VARARGS
#include <varargs.h>
#endif

#endif

#endif

#include "ge_error.h"
#include "indent.h"

/* 
Potential bug:  no length checking is done on strings written to convbuf,
so overflows are possible. 
*/
#define convbufsize 512
#define maxline 129
#define tabstep 3

static char buffer[maxline], convbuf[convbufsize], 
        *startpoint, *inpoint, *endpoint;

void ind_setup()
/*  This routine initializes the indentation package */
{
        startpoint= buffer;
        inpoint= startpoint;
        endpoint= buffer+maxline-2;
}

void ind_push()
/* This routine advances the start point */
{
        if ( startpoint < endpoint - tabstep ) 
                startpoint= startpoint + tabstep;
        else
                ger_error("ind_push: too far right to indent further");

        ind_write("   ");
        inpoint= startpoint;
}

void ind_pop()
/* This routine retreates the start point */
{
        if ( startpoint >= buffer + tabstep - 1)
                startpoint= startpoint - tabstep;
        else
                ger_error("ind_pop: this pop doesn't match an indent push");

        inpoint= startpoint;
}

#ifdef __STDC__
void ind_write(char *string, ...)
/* This routine adds text to the line buffer.  Alas, at the moment it
 * does not support the full functionality of sprintf.  See the very similar
 * routine in K&R C Second Edition, section 7.3, for a functional description.
 */
{
  va_list ap;
  char *p, *sval;
  int ival;
  double dval;
  char cval;

  va_start( ap, string );
  for (p = string; *p; p++) {
    if (*p != '%') {
      if (inpoint<endpoint) *inpoint++= *p;
      continue;
    }
    switch (*++p) {
    case 'd':
      ival= va_arg(ap,int);
      sprintf(convbuf,"%d",ival);
      break;
    case 'f':
      dval= va_arg(ap,double);
      sprintf(convbuf,"%f",dval);
      break;
    case 's':
      sval= va_arg(ap,char*);
      strncpy( convbuf, sval, convbufsize-1 );
      convbuf[convbufsize-1]= '\0';
      break;
    case 'c':
      cval= va_arg(ap,int);
      sprintf(convbuf,"%c",cval);
      break;
    default:
      convbuf[0]= *p;
      convbuf[1]= '\0';
      break;
    }
    if (inpoint<endpoint) {
      (void)strncpy( inpoint, convbuf, (int)(endpoint-inpoint+1) );
      inpoint= inpoint + strlen(inpoint);
    }
  }

  va_end(ap);
}
#else
#ifdef USE_VARARGS
void ind_write(string, va_alist)
char *string;
va_dcl /* declare the va_list parameter */
/* This routine adds text to the line buffer.  Alas, at the moment it
 * does not support the full functionality of sprintf.
 */
{
  va_list ap;
  char *p, *sval;
  int ival;
  double dval;
  char cval;

  va_start( ap );
  for (p = string; *p; p++) {
    if (*p != '%') {
      if (inpoint<endpoint) *inpoint++= *p;
      continue;
    }
    switch (*++p) {
    case 'd':
      ival= va_arg(ap,int);
      sprintf(convbuf,"%d",ival);
      break;
    case 'f':
      dval= va_arg(ap,double);
      sprintf(convbuf,"%f",dval);
      break;
    case 's':
      sval= va_arg(ap,char*);
      strncpy( convbuf, sval, convbufsize-1 );
      convbuf[convbufsize-1]= '\0';
      break;
    case 'c':
      cval= va_arg(ap,char);
      sprintf(convbuf,"%c",cval);
      break;
    default:
      convbuf[0]= *p;
      convbuf[1]= '\0';
      break;
    }
    if (inpoint<endpoint) {
      (void)strncpy( inpoint, convbuf, (int)(endpoint-inpoint+1) );
      inpoint= inpoint + strlen(inpoint);
    }
  }

  va_end(ap);
}
#else
void ind_write( string, p1, p2, p3, p4, p5, p6 )
char *string;
int p1, p2, p3, p4, p5, p6;
/* This routine adds text to the line buffer */
{
        sprintf(convbuf, string, p1, p2, p3, p4, p5, p6 );
        if (inpoint<endpoint) {
                (void)strncpy( inpoint, convbuf, (int)(endpoint-inpoint+1) );
                inpoint= inpoint + strlen(inpoint);
        };
}
#endif
#endif

void ind_eol()
/* 
This routine causes the line to be emitted, and moves back to the
start point for the beginning of the next line.
*/
{
        register char *i;

        fprintf(stdout,"%s\n",buffer);
        for ( i=buffer; i<startpoint; i++) *i= ' ';
        for ( i=startpoint; i<=endpoint+1; i++) *i= '\0';
        inpoint= startpoint;
}
