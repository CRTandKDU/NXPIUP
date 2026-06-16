/**
 * nxp_evoke.c -- Managing secondary agenda of evoked hypotheses and signs
 *
 * Written on 2026-06-16.
 */
#include <cstddef>
#include <cstdlib>
#include "agenda.h"
#include "nxp_evoke.h"

#define _NEWCELL(c) (c) = (cell_rec_ptr) malloc( sizeof( struct cell_rec ) );	\
  (c)->next = NULL;								\
  (c)->sign_or_hypo = NULL;							\
  (c)->val.status = _UNKNOWN;


cell_rec_ptr S_SecondaryAgenda = NULL;

typedef void (*evoke_iterate_cb) (cell_rec_ptr);

void evoke__iterate( cell_rec_ptr agenda, evoke_iterate_cb f ){
  cell_rec_ptr c, current = agenda;
  while( current ){
    c       = current;
    current = current->next;
    f( c );
  }
}

int evoke_notemptyp(){
  return S_SecondaryAgenda ? 1 : 0;
}

void evoke_init(){
  if( S_SecondaryAgenda ) evoke_free();
  S_SecondaryAgenda = NULL;
}

void evoke__free_cb( cell_rec_ptr c ){
  free( c );
}

void evoke_free(){
  evoke__iterate( S_SecondaryAgenda, evoke__free_cb );
  S_SecondaryAgenda = NULL;
}

void evoke_push( sign_rec_ptr sign ){
  cell_rec_ptr c;
  _NEWCELL(c);
  c->next		= S_SecondaryAgenda;
  c->sign_or_hypo	= sign;
  S_SecondaryAgenda	= c;
}

cell_rec_ptr evoke_pop(){
  cell_rec_ptr c = S_SecondaryAgenda;
  if( c )
    S_SecondaryAgenda = c->next;
  // Option c->next = NULL ?
  return c;
}

void evoke_switch_agenda( engine_state_rec_ptr state ){
  // Is the primary agenda empty?
  if( state->agenda ) return;

  state->agenda = S_SecondaryAgenda;
  S_SecondaryAgenda = NULL;
}
