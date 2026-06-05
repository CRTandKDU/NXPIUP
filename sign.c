/**
 * sign.c -- Hypos and leaves in the or-and trees.
 *
 * Written on mardi, 20 mai 2025.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#include "agenda.h"

#ifdef ENGINE_DSL_HOWERJFORTH
#define _INIT_VAL(sign)  (sign)->val.status = _UNKNOWN; \
  (sign)->val.type = _VAL_T_BOOL; \
  (sign)->val.val_bool = 0;           \
  (sign)->val.val_int  = 0;           \
  (sign)->val.val_float = 0.0;        \
  (sign)->val.valptr = (char *)0;     \
  (sign)->val.val_forth = (uint16_t) 0;   \

#else
#define _INIT_VAL(sign)  (sign)->val.status = _UNKNOWN; \
  (sign)->val.type = _VAL_T_BOOL; \
  (sign)->val.val_bool = 0;           \
  (sign)->val.val_int  = 0;           \
  (sign)->val.val_float = 0.0;        \
  (sign)->val.valptr = (char *)0;     \

#endif
  

extern effect S_on_get;
extern effect S_on_set;

// Builders
sign_rec_ptr sign_pushnew( sign_rec_ptr top,
			   const char *s,
			   const int ngetters, const size_t size_getter,
			   const int nsetters, const size_t size_setter
			   ){
  sign_rec_ptr sign;
  unsigned short len;

  //
  sign			= (sign_rec_ptr) malloc( sizeof( struct sign_rec ) );;
  sign->next		= top;
  // Default to boolean type (most common).
  _INIT_VAL(sign);
  len			= (unsigned short)strlen( s );
  sign->len_type	= (len <= _CHOP) ? len : _CHOP;
  char *to		= sign->str;
  char *from		= (char *)s;
  for( unsigned short i = 0; i < sign->len_type; *to++ = *from++, i++ ); *to = 0;
  sign->len_type 	|= SIGN_MASK;
  sign->ngetters	= ngetters;
  sign->nsetters	= nsetters;

  if( ngetters ){
    sign->getters = (empty_ptr *)malloc( ngetters*size_getter );
    if( NULL == sign->getters ){
      fprintf( stderr, "Allocation of getters failed>\n" );
      exit( 1 );
    }
  }
  // Default getter for a sign
  else sign->getters = (empty_ptr *) &getter_sign;
  
  sign->setters         = nsetters ? (empty_ptr *)0 : (empty_ptr *)malloc( nsetters*size_setter );
  
  /* printf( "\tsign allocation (%s): g %d, s %d\n", s, ngetters*size_getter, nsetters*size_setter ); */
  //
  return sign;
}

void sign_del( sign_rec_ptr sign ){
  fprintf( stderr, "Deleting %s\n", sign->str );
  if( COMPOUND_MASK == (sign->len_type & TYPE_MASK) )
    compound_del( (compound_rec_ptr) sign );
  if( sign->ngetters )
    for( unsigned short i=0; i<sign->ngetters; i++ ){ free( (void *) (sign->getters)[i] ); }
  if( sign->nsetters )
    for( unsigned short i=0; i<sign->nsetters; i++ ){ free( (void *) (sign->setters)[i] ); }
  if( sign->ngetters && sign->getters ) free( sign->getters );
  if( sign->nsetters && sign->setters ) free( sign->setters );
  //
#ifndef FLTK
  if( sign->val.valptr ) free( sign->val.valptr );
#endif
  free( sign );
}

void sign_pushgetter( sign_rec_ptr sign, empty_ptr getr ){
  if( 0 == sign->ngetters ){
    sign->getters = (empty_ptr *)malloc( sizeof(empty_ptr) );
  }
  else{
    sign->getters = (empty_ptr *)realloc( sign->getters, (1 + sign->ngetters)*sizeof(empty_ptr) );
  }
  (sign->getters)[ sign->ngetters ] = getr;
  sign->ngetters += 1;
}

void sign_pushsetter( sign_rec_ptr sign, empty_ptr setr ){
  if( 0 == sign->nsetters ){
    sign->setters = (empty_ptr *)malloc( sizeof(empty_ptr) );
  }
  else{
    sign->setters = (empty_ptr *)realloc( sign->setters, (1 + sign->nsetters)*sizeof(empty_ptr) );
  }
  (sign->setters)[ sign->nsetters ] = setr;
  sign->nsetters += 1;
}

// I/O
unsigned short sign_get_default( sign_rec_ptr sign ){
  char buf[32] = { 0 };
  if( S_on_get ) S_on_get( sign, (struct val_rec *)0 );
  printf( "Q: What is the value of %s?\n(Type 0 or 1)\nA: ", sign->str );
  fgets( buf, 30, stdin );
  return (unsigned short)strtoul( buf, NULL, 0 );
}

void sign_set_default( sign_rec_ptr sign, struct val_rec *val ){
  /* char buf[128]; */
  /* sprintf( buf, "SetDef in %s Status %d Type %d Bool %d\n", sign->str, */
  /* 	   sign->val.status, */
  /* 	   sign->val.type, */
  /* 	   sign->val.val_bool */
  /* 	   ); */
  /* printf( buf ); */
  /* sprintf( buf, "SetDef val %s Status %d Type %d Bool %d\n", sign->str, */
  /* 	   val->status, */
  /* 	   val->type, */
  /* 	   val->val_bool */
  /* 	   ); */
  /* printf( buf ); */
  
  if( _UNKNOWN == val->status ) return;
  if( sign->val.type != val->type ) return;
  // A chance to handle previous and new value
  if( S_on_set ) S_on_set( sign, val );
  //
  sign->val.status = val->status;
  switch( sign->val.type ){
  case _VAL_T_BOOL:
    sign->val.val_bool = val->val_bool;
    break;
  case _VAL_T_INT:
    sign->val.val_int = val->val_int;
    break;
  case _VAL_T_FLOAT:
    sign->val.val_float = val->val_float;
    break;
  case _VAL_T_STR:
    sign->val.valptr = val->valptr;
    break;
  }
  
  /* sprintf( buf, "SetDef out %s Status %d Type %d Bool %d\n", sign->str, */
  /* 	   sign->val.status, */
  /* 	   sign->val.type, */
  /* 	   sign->val.val_bool */
  /* 	   ); */
  /* printf( "%s", buf ); */

  // IMPORTANT! This is where sign's values are forwarded.
  engine_default_on_set( sign, val );

}

// Default sync sign-getter
/* void getter_sign( sign_rec_ptr sign ){ */
/*   if(TRACE_ON) printf ("__FUNCTION__ = %s\n", __FUNCTION__); */
/*   sign_set_default( sign, sign_get_default( sign ) ); */
/* } */


// Managers
void sign_print( sign_rec_ptr sign ){
  char *esc;
  short len = sign->len_type & SIGN_UNMASK;
  if( _KNOWN == sign->val.status &&
      _VAL_T_BOOL == sign->val.type ){
    esc = S_val_color( sign->val.val_bool );
  }
  else esc = S_val_color( 2 );
  
  if(TRACE_ON) printf( "%sSIGN:\t%s (%d, %d, %d)", esc, sign->str,
		       len, sign->len_type, sign->len_type & TYPE_MASK
		       );
  if(TRACE_ON) printf( "\tGetters %d (%d), Setters %d (%d)\n",
	  sign->ngetters, sizeof( sign->getters ),
	  sign->nsetters, sizeof( sign->setters ) );
}

void sign_iter( sign_rec_ptr s0, sign_op func ){
  sign_rec_ptr prev, s=s0;
  while( s ){
    prev = s;
    s = s->next;
    if( prev ) func( prev );
  }
}

sign_rec_ptr sign_find ( const char *str, sign_rec_ptr top ){
  sign_rec_ptr s = top, res = (sign_rec_ptr)NULL;
  while( s ){
    if( 0 == strcmp( s->str, str ) ){
      res = s;
      break;
    }
    s = s->next;
  }
  return res;
}

hypo_rec_ptr sign_tohypo( hypo_rec_ptr hypo, sign_rec_ptr top_sign, hypo_rec_ptr top_hypo ){
  sign_rec_ptr s = top_sign;
  if(TRACE_ON) printf( "Changing sign %s to hypo\n", hypo->str );
  hypo->len_type = (unsigned short)strlen(hypo->str) | HYPO_MASK;
  while( hypo != s->next ) s = s->next;
  s->next = hypo->next;
  hypo->next = top_hypo;
  return hypo;
}

empty_ptr sign_get_client_data  ( sign_rec_ptr sign ){
  return sign->client_data;
}
void sign_set_client_data       ( sign_rec_ptr sign, empty_ptr client_data ){
  sign->client_data = client_data;
}
