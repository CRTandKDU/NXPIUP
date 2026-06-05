/**
 * hypo.c -- Hypotheses may also be signs
 *
 * Written on mardi, 20 mai 2025.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "agenda.h"

hypo_rec_ptr hypo_pushnew( hypo_rec_ptr top,
			   const char *s, const int ngetters ){
  hypo_rec_ptr hypo = (hypo_rec_ptr)sign_pushnew( top, s,
						  ngetters, sizeof( bwrd_rec_ptr ),
						  0, sizeof(void *) );
  hypo->len_type = (unsigned short)strlen(s) | HYPO_MASK;
  hypo->getters  = (empty_ptr *)0;
  return hypo;
};

void hypo_del( hypo_rec_ptr hypo ){
  sign_del( (sign_rec_ptr)hypo );
}

void hypo_print( hypo_rec_ptr hypo ){
  char *esc; 
  short len = hypo->len_type & HYPO_UNMASK;
  if( _KNOWN == hypo->val.status &&
      _VAL_T_BOOL == hypo->val.type ){
    esc = S_val_color( hypo->val.val_bool );
  }
  else esc = S_val_color( 2 );

  //
  if(TRACE_ON) printf( "%sHYPO:\t%s (%d, %d, %d)\n", esc, hypo->str,
		       len, hypo->len_type, hypo->len_type & TYPE_MASK );
  if(TRACE_ON) printf( "\tGetters: %d, Setters: %d\n", hypo->ngetters, hypo->nsetters );
  for( unsigned short i=0; i < hypo->ngetters; i++ ){
    bwrd_rec_ptr bwrd = (bwrd_rec_ptr) (hypo->getters)[i];
    if(TRACE_ON) printf( "\t\tfrom %s\n", bwrd->rule->str );
  }
};
