/**
 * engine.c -- Forward and Backward Chaining
 *
 * Written on jeudi, 22 mai 2025.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdarg.h>
#include "agenda.h"

#ifdef FLTK
extern void  repl_log( const char *s,... );
#else
extern void  repl_log( const char *s );
#endif

extern engine_state_rec_ptr repl_getState();

effect S_on_get		= (effect)0;		// Triggered on get in `sign_default_get`
effect S_on_set		= (effect)0;		// Triggered on set in `sign_default_set`
effect_gate S_on_gate	= (effect_gate)0;	// Triggered on setting cond in `engine_forward_sign`
effect S_on_push	= (effect)0;		// Triggered on pushing hypo/compound on agenda
effect S_on_pop	        = (effect)0;		// Triggered on popping agenda
effect S_on_endsession  = (effect)0;            // Triggered on empty agenda after knowcess

//----------------------------------------------------------------------
void engine_default_on_get( sign_rec_ptr sign,  struct val_rec * val ){
  // Do nothing
}

void engine_default_on_set( sign_rec_ptr sign,  struct val_rec * val ){
  // Forward signs and hypos
  switch( sign->len_type & TYPE_MASK ){
  case SIGN_MASK:
  case COMPOUND_MASK:
  case HYPO_MASK:
    engine_forward_sign( sign );
    break;
  }
}

void engine_default_on_gate(hypo_rec_ptr hypo, short val) {
	// Postpone hypo when one of its rule might be true or
	// compound if one of its DSL-shared var is known.
	if (_TRUE == val && _UNKNOWN == hypo->val.status) {
		cell_rec_ptr new_cell, cell = repl_getState()->agenda;
		if (cell) {
			// Is hypo already on top of stack?
			if (cell->sign_or_hypo == hypo) return;
			while (cell->next) {
				// Is hypo already stacked?
				if (cell->sign_or_hypo == hypo) return;
				cell = cell->next;
			}
			new_cell = (cell_rec_ptr)malloc(sizeof(struct cell_rec));
			new_cell->sign_or_hypo = hypo;
			new_cell->val.status = _UNKNOWN;
            new_cell->val.type = hypo->val.type;
			new_cell->next = (cell_rec_ptr)0;
			cell->next = new_cell;

#ifdef FLTK
            /* repl_log("Appending %s (%d)", hypo->str, (int)val); */
#else
	    /* char buf[64]; */
	    /* sprintf(buf, "Appending %s (%d)", hypo->str, (int)val); */
	    /* repl_log(buf); */
#endif	    
		}
		else {
			engine_pushnew_hypo(repl_getState(), hypo);
		}
	}
  // engine_print_state( repl_getState() );
}

void engine_default_on_agenda_push( sign_rec_ptr,  struct val_rec * val ){
}

void engine_default_on_agenda_pop( sign_rec_ptr,  struct val_rec * val ){
}


void engine_register_effects( effect		f_get,
			      effect		f_set,
			      effect_gate	f_gate,
			      effect		f_push,
			      effect		f_pop,
			      effect		f_endsession){
  S_on_get		= f_get;
  S_on_set		= f_set;
  S_on_gate		= f_gate;
  S_on_push		= f_push;
  S_on_pop		= f_pop;
  S_on_endsession	= f_endsession;
}

//----------------------------------------------------------------------
void free_state(  engine_state_rec_ptr state ){
  cell_rec_ptr prev, cell = state->agenda;
  while( cell ){
    prev = cell;
    cell = prev->next;
    free( (void *)prev );
  }
  state->agenda = (cell_rec_ptr)0;
}

void            engine_free_state( engine_state_rec_ptr state ){
  free_state( state );
  free( (void *)state );
}

void engine_print_state( engine_state_rec_ptr state ){
  cell_rec_ptr prev, cell = state->agenda;
  if(TRACE_ON) printf( "STATE> Current sign: %s\n", (state->current_sign ? state->current_sign->str : "NONE") );
  while( cell ){
    prev = cell;
    cell = prev->next;
    if(TRACE_ON) printf( "STATE>\t%s\t%s\n",
			 _UNKNOWN == prev->val.status ? "SUGGEST" : "VOLUNTEER", prev->sign_or_hypo->str );
	    
  }
  if(TRACE_ON) printf( "----\t----\t----\t----\n" );
}

//----------------------------------------------------------------------
void reset( sign_rec_ptr sign ){
  sign->val.status = _UNKNOWN;
  //
  switch( sign->len_type & TYPE_MASK ){
  case HYPO_MASK:
  case COMPOUND_MASK:
    // _VAL_T_BOOL
    sign->val.val_bool = _UNKNOWN;
    break;
    //
  case SIGN_MASK:
    switch( sign->val.type ){
    case _VAL_T_BOOL:
      sign->val.val_bool = _UNKNOWN;
      break;
    case _VAL_T_INT:
      sign->val.val_int = _UNKNOWN;
      break;
    case _VAL_T_STR:
      if( sign->val.valptr ) free( sign->val.valptr );
      sign->val.valptr = (char *)0;
      break;
    }
    break;
    //
  case RULE_MASK:
    // _VAL_T_BOOL
    sign->val.val_bool = _UNKNOWN;
    if( sign->ngetters ){
      short i, nconds = sign->ngetters;
      for( i=0; i<nconds; i++ ){ ((cond_rec_ptr) sign->getters[i])->val = _UNKNOWN; }
    }
    break;
    
  }
}

void engine_reset( engine_state_rec_ptr state ){
  sign_rec_ptr top;
  if( (top = (sign_rec_ptr) loadkb_get_allsigns()) ){
    sign_iter( top, reset );
  }
  if( (top = (sign_rec_ptr) loadkb_get_allhypos()) ){
    sign_iter( top, reset );
  }
  if( (top = (sign_rec_ptr) loadkb_get_allrules()) ){
    sign_iter( top, reset );
  }
  free_state( state );
  state->current_sign = (sign_rec_ptr)0;
}

//----------------------------------------------------------------------
void            engine_pushnew_hypo( engine_state_rec_ptr state, hypo_rec_ptr h ){
  // Suggest
  cell_rec_ptr cell	= (cell_rec_ptr)malloc( sizeof( struct cell_rec ) );
  cell->next		= state->agenda;
  state->agenda		= cell;
  cell->sign_or_hypo	= h;
  cell->val.status	= _UNKNOWN;
  if( S_on_push ) S_on_push( h, (struct val_rec *)0 );
}

void engine_pushnew_signdata( engine_state_rec_ptr state, sign_rec_ptr sign, struct val_rec *val ){
  // Volunteer
  cell_rec_ptr cell = (cell_rec_ptr)malloc( sizeof( struct cell_rec ) );
  cell->next = state->agenda;
  state->agenda = cell;
  cell->sign_or_hypo = sign;
  cell->val = *val;
  if( S_on_push ) S_on_push( sign, val );
}

void engine_pop( engine_state_rec_ptr state ){
  cell_rec_ptr cell = state->agenda;
  state->agenda = cell->next;
  if( S_on_pop ) S_on_pop( cell->sign_or_hypo, &(cell->val) );
  free( cell );
}

void engine_knowcess( engine_state_rec_ptr state ){
  int suspend = _FALSE;
  // Execute `suggest` and `volunteer` commands until quiescent state
  while( state->agenda ){
    cell_rec_ptr cell = state->agenda;
    // Test for `suggest hypo` or compound
    if( _UNKNOWN == cell->val.status ){
      switch( cell->sign_or_hypo->len_type & TYPE_MASK ){
      case COMPOUND_MASK:
	engine_backward_compound( (compound_rec_ptr) cell->sign_or_hypo, &suspend );
	break;
      case HYPO_MASK:
	engine_backward_hypo( (hypo_rec_ptr) cell->sign_or_hypo, &suspend );
	break;
      }
    }
    else{
      sign_set_default( (sign_rec_ptr) cell->sign_or_hypo, &(cell->val) );
    }
    // Now pop from agenda
    /* state->agenda = cell->next; */
    engine_pop( state );
  }
  if( S_on_endsession ) S_on_endsession( (sign_rec_ptr) NULL, NULL );  
}

/* ** A somewhat async version of knowcess. */
/* The Agenda is a stack of operations (SUGGEST <hypo>, EVAL <compound>, VOLUNTEER <sign, value>). */
/* Note that a single operation may appear at several locations on the Agenda as it may be pushed or */
/* postponed several times. */
//
/* The ~resume_knowcess~ processes the Agenda until either a question is being asked and pending--indicated */
/* by the `suspend` control flag being ~_TRUE~--or the Agenda is empty. */
				     
void engine_resume_knowcess( engine_state_rec_ptr state ){
  int suspend;
 next:
  suspend = _FALSE;
  if( state->agenda ){
    cell_rec_ptr cell = state->agenda;
    if( _UNKNOWN == cell->val.status ){
      // Either a SUGGEST or COMPOUND eval operation
      if( _UNKNOWN == cell->sign_or_hypo->val.status ){
	switch( cell->sign_or_hypo->len_type & TYPE_MASK ){
	case COMPOUND_MASK:
	  engine_backward_compound( (compound_rec_ptr) cell->sign_or_hypo, &suspend );
	  if( _FALSE == suspend )
	    goto next;
	  break;

	case HYPO_MASK:
	  engine_backward_hypo( (hypo_rec_ptr) cell->sign_or_hypo, &suspend );
	  if( _FALSE == suspend )
	    goto next;
	  break;
	}
      }
      else{
	engine_pop( state );
	goto next;
      }	
    }
    else{
      // A VOLUNTEER operation
      /* sign_set_default( (sign_rec_ptr) cell->sign_or_hypo, &(cell->val) ); */
      engine_forward_sign( (sign_rec_ptr) cell->sign_or_hypo );
      engine_pop( state );
      goto next;
    }
  }
  
  if( ( (cell_rec_ptr)0 == state->agenda ) && S_on_endsession )
    S_on_endsession( (sign_rec_ptr) NULL, NULL );
}

//----------------------------------------------------------------------
unsigned short engine_sc_or( hypo_rec_ptr hypo ){
  struct val_rec v = { _KNOWN, _VAL_T_BOOL, (char *)0, _FALSE, 0, 0.0 };
  if( _KNOWN == hypo->val.status ) return _TRUE;

  bwrd_rec_ptr bwrd;
  for( unsigned short i=0; i<hypo->ngetters; i++ ){
    bwrd = (bwrd_rec_ptr) (hypo->getters)[i];
    if( _UNKNOWN == bwrd->rule->val.status ||
	( _KNOWN == bwrd->rule->val.status && _FALSE != bwrd->rule->val.val_bool ) ){
      return _FALSE;
    }
  }
  /* hypo->val = _FALSE; */
  sign_set_default( hypo, &v );
  return _TRUE;
}


unsigned short engine_sc_and( rule_rec_ptr rule ){
  struct val_rec v = { _KNOWN, _VAL_T_BOOL, (char *)0, _TRUE, 0, 0.0 };
  if( _KNOWN == rule->val.status ) return _TRUE;

  cond_rec_ptr cond;
  for( unsigned short i=0; i<rule->ngetters; i++ ){
    cond = (cond_rec_ptr) (rule->getters)[i];
    if( _TRUE != cond->val ){
      return _FALSE;
    }
  }
  /* rule->val = _TRUE; */
  sign_set_default( (sign_rec_ptr)rule, &v );
  return _TRUE;
}


void engine_forward_rule( rule_rec_ptr rule ){
  if( _UNKNOWN == rule->val.status ) return;
  // Short-circuit OR.
  // Recursive forward (on hypos) built-in the `sign-set-default` function
  struct val_rec v = { _KNOWN, _VAL_T_BOOL, (char *)0, _TRUE, 0, 0.0 };
  hypo_rec_ptr hypo  = (hypo_rec_ptr) rule->setters;
  unsigned short val = rule->val.val_bool;
  switch( val ){
  case _TRUE:
    /* hypo->val = _TRUE; */
    sign_set_default( hypo, &v );
    break;
  case _FALSE:
    unsigned short hval = engine_sc_or( hypo );
    break;
  }
}

void engine_forward_cond( rule_rec_ptr rule, cond_rec_ptr cond ){
  // Short-circuit AND
  struct val_rec v = { _KNOWN, _VAL_T_BOOL, (char *)0, _FALSE, 0, 0.0 };
  unsigned short val = cond->val;
  if( _FALSE == val ){
    /* rule->val = _FALSE; */
    sign_set_default( (sign_rec_ptr)rule, &v );
  }
  if( _TRUE == val ){
    unsigned short rval = engine_sc_and( rule );
    // Rule could be evaluated to TRUE, execute RHS
    if( _TRUE == rval && 0 < rule->nrhs ){
#ifdef ENGINE_DSL
		int res;
		for (unsigned short i = 0; i < rule->nrhs; i++) {
			res = engine_dsl_rhs_eval((char*)(rule->rhs[i]));
		}
#endif
    }
  }
  engine_forward_rule( rule );
}

void engine_forward_sign( sign_rec_ptr sign ){
  if( _UNKNOWN == sign->val.status ) return;

  if( sign->nsetters ){
    // Has to be a boolean sign (i.e. hypo or compound)
    for( unsigned short i = 0; i<sign->nsetters; i++ ){
      fwrd_rec_ptr fwrd = (fwrd_rec_ptr) (sign->setters)[i];
      if( 0 <= fwrd->idx_cond ){
	rule_rec_ptr rule = fwrd->rule;
	cond_rec_ptr cond = (cond_rec_ptr) (rule->getters)[ fwrd->idx_cond ];
	cond->val = (cond->out == sign->val.val_bool) ? _TRUE : _FALSE;
	// Gating on known condition
	if( S_on_gate ) S_on_gate( (hypo_rec_ptr) rule->setters, cond->val );
	engine_forward_cond( rule, cond );
      }
      else{
	// This is a DSL-shared sign
	if(TRACE_ON) printf( "> Postpone eval compound sign %s (from %s)\n",
		((compound_rec_ptr) (fwrd->rule))->str, sign->str );
	if( S_on_gate ) S_on_gate( (hypo_rec_ptr) (fwrd->rule), _TRUE );
      }
    }
  }
}

//
void engine_backward_hypo( hypo_rec_ptr hypo, int *suspend ){
  if(TRACE_ON) printf ("__FUNCTION__ = %s %s\n", __FUNCTION__, hypo->str);
  if( _UNKNOWN != hypo->val.status ) return;
  // Sequential OR
  bwrd_rec_ptr bwrd;
  for( unsigned short i=0; i<hypo->ngetters; i++ ){
    bwrd = (bwrd_rec_ptr) (hypo->getters)[i];
    engine_backward_rule( bwrd->rule, suspend );
    // In async mode a pending question at a leaf suspends bwrd-chaining
    if( *suspend )
      break;
    // Check for side-effects of short-circuiting forward chaining
    if( _UNKNOWN != hypo->val.status ) break;
  }
}

void engine_backward_compound( compound_rec_ptr compound, int *suspend ){
  if(TRACE_ON) printf ("__FUNCTION__ = %s %s %d %d %d\n",
		       __FUNCTION__,
		       compound->str,
		       (compound->len_type & TYPE_MASK),
		       compound->val.status,
		       compound->ngetters
		       );
  if( COMPOUND_MASK != (compound->len_type & TYPE_MASK) ) return;
  if( _UNKNOWN != compound->val.status ) return;
  if( 0 == compound->ngetters ){
    // Field getters is a pointer to the getter function
    ((sign_getter_t)(compound->getters))( (sign_rec_ptr)compound, suspend );
  }
}

void engine_backward_rule( rule_rec_ptr rule, int *suspend ){
  if(TRACE_ON) printf ("__FUNCTION__ = %s %s\n", __FUNCTION__, rule->str);
  if( _UNKNOWN != rule->val.status ) return;
  // Sequential AND
  cond_rec_ptr cond;
  for( unsigned short i=0; i<rule->ngetters; i++ ){
    cond = (cond_rec_ptr) (rule->getters)[i];
    engine_backward_cond( cond, suspend );
    if( *suspend )
      break;
    // Check for side-effects of short-circuiting forward chaining
    if( _UNKNOWN != rule->val.status ) break;
  }
}

void engine_backward_cond( cond_rec_ptr cond, int* suspend ){
  if(TRACE_ON) printf ("__FUNCTION__ = %s %s\n", __FUNCTION__, cond->sign->str);
  if( _UNKNOWN != cond->val ) return;
  if( _UNKNOWN == cond->sign->val.status ){
    if( HYPO_MASK     == (cond->sign->len_type & TYPE_MASK) ||
	COMPOUND_MASK == (cond->sign->len_type & TYPE_MASK) ){
      engine_pushnew_hypo( repl_getState(), (hypo_rec_ptr) cond->sign );
    }
    char remote_hypo[_CHOP] = {0};
    if( hypo_remote_aliasp( cond->sign->str ) ){
      hypo_remote_backward( cond->sign, suspend );
    }
    // Hypothesis: backward on rules
    else if( HYPO_MASK == (cond->sign->len_type & TYPE_MASK) ){
      engine_backward_hypo( (hypo_rec_ptr) cond->sign, suspend );
    }
    // Sign or Compound: ask or execute DSL program
    else{
      if( 0 == cond->sign->ngetters ){
	// Field getters is a pointer to the getter function
	((sign_getter_t)(cond->sign->getters))( cond->sign, suspend );
      }
    }
  }
}

/* Defeasible Inference */
/* TODO: Handling 'RESET' and 'CHANGE' of sign values triggered by RHS */
/* This could be done either in `sign.c` 'sign_set_default' function, or */
/* pre-emptively in the 'on_set' callback where both previous and new */
/* values are available. */
  
/* Backward-chaining to RHS */
/* TODO: When nxp@ applies to a sign which appears before a nxp! in another rule, */
/* first backward on rule before prompting the user. */
  
