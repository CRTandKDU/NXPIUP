/**
 * compound.c -- DSL expressions
 *
 * Written on jeudi, 29 mai 2025.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef FLTK
#include <string>
#endif

#include "agenda.h"

#define _INIT_VAL(sign)  (sign)->val.status = _UNKNOWN; \
  (sign)->val.type = _VAL_T_BOOL; \
  (sign)->val.val_bool = 0;           \
  (sign)->val.val_int  = 0;           \
  (sign)->val.val_float = 0.0;        \
  (sign)->val.valptr = (char *)0;     \

extern void  repl_log( const char *s );

compound_rec_ptr compound_pushnew( sign_rec_ptr top,
				   const char *s, const int ngetters ){
  compound_rec_ptr compound;
  unsigned short len;

  compound			= (compound_rec_ptr) malloc( sizeof( struct compound_rec ) );
  compound->next		= top;
  _INIT_VAL(compound);
  len				= (unsigned short)strlen( s );
  compound->len_type		= (len <= _CHOP) ? len : _CHOP;
  char *to			= compound->str;
  char *from			= (char *)s;
  for( unsigned short i		= 0; i < compound->len_type; *to++ = *from++, i++ ); *to = 0;
  compound->len_type 	        |= COMPOUND_MASK;
  compound->ngetters		= ngetters;
  compound->nsetters		= 0;
  compound->getters		= (empty_ptr *) &engine_dsl_getter_compound;
  compound->setters		= (empty_ptr *)NULL;
  compound->dsl_expression      = (char *)NULL;
  return compound;
}

void compound_DSL_set( compound_rec_ptr compound, const char * expr ){
  compound_del( compound );
#ifdef FLTK
  std::string str = std::string(expr);
  if ('\n' != str.back()) {
      str.push_back('\n');
  }
  compound->dsl_expression = (char*)malloc(str.length());
  strcpy(compound->dsl_expression, str.c_str());
#else
  char *s, *t;
  compound->dsl_expression = (char *) malloc( 1 + strlen( expr ) );
  if( compound->dsl_expression ){
    /* strcpy( compound->dsl_expression, expr ); */
    s = (char *) expr; t = compound->dsl_expression;
    while( *s ){
      if( 0x0d == *s ){ *t++ = '\n'; break; }
      *t++ = *s++;
    }
    *t = 0x0;
  }
#endif
}

void compound_del( compound_rec_ptr compound ){
  if( compound->dsl_expression ) free( compound->dsl_expression );
}

void compound_DSLvar_pushnew( compound_rec_ptr compound, sign_rec_ptr sign ){
    fwrd_rec_ptr fwrd;
    // Point back from sign to cond
    if(TRACE_ON) printf( "> Nsetters: %d. Pushing fwrd_rec: %s -> %s\t", sign->nsetters,
	    sign->str, compound->str );
    sign_pushsetter( sign, (empty_ptr)malloc( sizeof(struct fwrd_rec) ) );
    fwrd = (fwrd_rec_ptr) (sign->setters)[_LAST_FWRD(sign)];
    fwrd->rule = (rule_rec_ptr) compound; // Force type
    fwrd->idx_cond = -1;
    if(TRACE_ON) printf( "Nsetters: %d\n", sign->nsetters );
}
