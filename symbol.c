/****************************************************************************
 * symbol.c
 * Author Joel Welling
 * Copyright 1990, Pittsburgh Supercomputing Center, Carnegie Mellon University
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
This module provides methods for symbols.
It maintains the table of unique symbol names.
*/

#include <stdio.h>
#include "p3dgen.h"
#include "pgen_objects.h"
#include "indent.h"
#include "ge_error.h"

/* Hash table to use to unify all the symbols, and table size.
 * Note that there is one symbol environment, not separate environments 
 * for each renderer.
 */
#define SYMBOL_TABLE_SIZE 107 /* must be prime */
static P_String_Hash *symbol_table= NULL;

static char *dup_string(char *string)
/* This function creates a new copy of a string */
{
  char *result;

  ger_debug("chash_mthd: dup_string");
  if ( !(result= (char *)malloc( sizeof(char)*(strlen(string)+1) ) ) )
    ger_fatal("symbol: dup_string: unable to allocate %d bytes!",
	      sizeof(char)*(strlen(string)+1) );
  strcpy(result,string);
  return(result);
}

P_Symbol create_symbol( char *name )
{
  P_Symbol result;

  if (symbol_table==NULL) symbol_table= po_create_chash(SYMBOL_TABLE_SIZE);

  METHOD_RDY(symbol_table);
  if ( !(result= (P_Symbol)(*(symbol_table->lookup))(name)) )
    result= (P_Symbol)(*(symbol_table->add))(name, dup_string(name));

  return(result);
}

void dump_symbol( P_Symbol sym )
{
  ind_write("Symbol <%s>", symbol_string(sym));
  ind_eol();
}

