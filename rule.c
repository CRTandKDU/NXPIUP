/**
 * rule.c -- Rules associate a set of conditions to a hypothesis
 *
 * Written on mardi, 20 mai 2025.
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

rule_rec_ptr rule_pushnew( rule_rec_ptr top,
			   const char *s, const int ngetters, hypo_rec_ptr h ){
  bwrd_rec_ptr bwrd;
  unsigned short len;
  // Set up rule sign-instance
  rule_rec_ptr rule			= (rule_rec_ptr) malloc( sizeof( struct rule_rec ) );
  rule->next				= (sign_rec_ptr)top;
  _INIT_VAL(rule);
  len					= (unsigned short)strlen( s );
  rule->len_type			= (len <= _CHOP) ? len : _CHOP;
  char *to				= rule->str;
  char *from				= (char *)s;
  for( unsigned short i			= 0; i < rule->len_type; *to++ = *from++, i++ ); *to = 0;
  rule->len_type 	                |= RULE_MASK;
  rule->ngetters			= ngetters;
  rule->getters				= (empty_ptr *)0;
  // Set up hypothesis in `setters` field, keeping `nsetters` to 0
  rule->nsetters			= 0;
  rule->setters				= (empty_ptr *)h;
  if( h ){
    // Point back from hypo to rules
    sign_pushgetter( h, (empty_ptr)malloc( sizeof(struct bwrd_rec) ) );
    bwrd = (bwrd_rec_ptr) (h->getters)[_LAST_RULE(h)];
    bwrd->rule = rule;
  }
  //
  rule->nrhs = 0;
  rule->rhs  = (empty_ptr *)0;
  return rule;
}

void rule_pushnewrhs( rule_rec_ptr rule, char *dsl_expr ){
  int len = strlen( dsl_expr );
  if( 0 == rule->nrhs ){
    rule->rhs = (empty_ptr *)malloc( sizeof(empty_ptr) );
  }
  else{
    rule->rhs = (empty_ptr *)realloc( rule->rhs, (1 + rule->nrhs)*sizeof(empty_ptr) );
  }
  //
#ifdef FLTK
  std::string str = std::string(dsl_expr);
  if ('\n' != str.back()) {
      str.push_back('\n');
  }
  (rule->rhs)[rule->nrhs] = (empty_ptr)malloc(str.length());
  strcpy((char *) (rule->rhs)[rule->nrhs], str.c_str());
#else
  (rule->rhs)[ rule->nrhs ] = (empty_ptr)malloc( len*sizeof(char) );
  strcpy( (char *) (rule->rhs)[ rule->nrhs ], dsl_expr );
#endif
  rule->nrhs += 1;
}

void rule_pushnewcond( rule_rec_ptr rule, unsigned short op, sign_rec_ptr sign ){
  cond_rec_ptr cond;
  fwrd_rec_ptr fwrd;
  sign_pushgetter( (sign_rec_ptr)rule, (empty_ptr)malloc( sizeof(struct cond_rec) ) );
  cond = (cond_rec_ptr)  (rule->getters)[_LAST_COND(rule)];
  cond->in = cond->out = op;
  cond->val = (unsigned short)0xff;
  cond->sign = sign;
  // Point back from sign to cond
  if(TRACE_ON) printf( "> Nsetters: %d. Pushing fwrd_rec: %s -> %s\t", sign->nsetters,
	  sign->str,
	  rule->str );
  sign_pushsetter( sign, (empty_ptr)malloc( sizeof(struct fwrd_rec) ) );
  fwrd = (fwrd_rec_ptr) (sign->setters)[_LAST_FWRD(sign)];
  fwrd->rule = rule;
  fwrd->idx_cond = _LAST_COND(rule);
  if(TRACE_ON) printf( "Nsetters: %d\n", sign->nsetters );
}

void rule_del( rule_rec_ptr rule ){
  if( rule->ngetters )
    for( unsigned short i=0; i<rule->ngetters; i++ ){ free( (void *) (rule->getters)[i] ); }
  if( rule->getters ) free( rule->getters );
  //
#ifndef FLTK
  if( rule->nrhs )
    for( unsigned short i=0; i<rule->nrhs; i++ ){ 
        free(  rule->rhs[i] ); 
    }
#endif
  if( rule->rhs ) free( rule->rhs );
  free( rule );
}

void rule_print( rule_rec_ptr rule ){
  char *esc;
  short len = rule->len_type & RULE_UNMASK;

  if( _KNOWN == rule->val.status &&
      _VAL_T_BOOL == rule->val.type ){
    esc = S_val_color( rule->val.val_bool );
  }
  else esc = S_val_color( 2 );

  if(TRACE_ON) printf( "%sRULE:\t%s (%d, %d, %d)\n", esc, rule->str,
		       len, rule->len_type, rule->len_type & TYPE_MASK );
  if(TRACE_ON) printf( "\tGetters: %d, Setters: %d\n", rule->ngetters, rule->nsetters );
  if( rule->setters ){
    hypo_print( (hypo_rec_ptr)rule->setters );
  }
  esc = S_val_color( 2 );
  if(TRACE_ON) printf( "%s\tCOND: %d (%d)\n", esc, rule->ngetters, sizeof((rule->getters)) );
  if( rule->getters ){
    for( unsigned short i = 0; i<rule->ngetters; i++ ){
      if(TRACE_ON) printf( "\t\tCOND %d: %d == %s\tVal: %d\n", i,
	      (_AS_COND_ARRAY(rule->getters)[i])->out,
	      (_AS_COND_ARRAY(rule->getters)[i])->sign->str,
	      (_AS_COND_ARRAY(rule->getters)[i])->val);
      sign_rec_ptr sign = (_AS_COND_ARRAY(rule->getters)[i])->sign;
      for( unsigned short j = 0; j < sign->nsetters; j++ ){
	if(TRACE_ON) printf( "\t\tin %s at %d\n",
		((fwrd_rec_ptr)(sign->setters)[j])->rule->str,
		((fwrd_rec_ptr)(sign->setters)[j])->idx_cond );
      }
    }
  }
  //
  if( rule->rhs ){
    for( unsigned short i =0; i < rule->nrhs; i++ ){
      if(TRACE_ON) printf( "\t\tRHS: %s\n", (char *) rule->rhs[i] );
    }
  }
  if(TRACE_ON) printf( "%s\n", esc );
}
