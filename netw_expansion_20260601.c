/**
 * netw_expansion.c -- Handle Rule Network backward (left-click) and forward (shifht-left-click) expansions
 *
 * Written on 2026-05-05.
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <iup.h>
#include <cd.h>
#include <cdiup.h>
#include <wd.h>

#include "agenda.h"

#include "netw.h"
#include "netw_internals.h"

#define NETW_BOOLEAN_SIGN(cell) ((((cell)->client_data_t == _NETW_SIGN_T) ||	\
				  ((cell)->client_data_t == _NETW_SIGN_YES_T) ||	\
				  ((cell)->client_data_t == _NETW_SIGN_NO_T) )	\
				 &&							\
				 COMPOUND_MASK != (((sign_rec_ptr) (cell)->client_data)->len_type & TYPE_MASK))

void netw__expand_forward(  cdCanvas *canvas, netw_cell_rec_ptr cell,
		    double WORLD_W, double WORLD_H, unsigned short orientation ){
  sign_rec_ptr sign = (sign_rec_ptr) cell->client_data;
  if( sign->nsetters == 0 ) return;
  if( cell->head->x <= 2  ) return;
  //
  netw_cell_rec_ptr cp1, cp2;
  col_rec_ptr  head	= (col_rec_ptr) cdCanvasGetAttribute( canvas, "USERDATA" );
  col_rec_ptr  col1	= netw__col_get_create( canvas, cell->head->x - 1 );
  col_rec_ptr  col2	= netw__col_get_create( canvas, cell->head->x - 2 );
  int y1		= netw__col_ymax_cell( head, col1, &cp1 ) + 1;
  int y2		= netw__col_ymax_cell( head, col2, &cp2 ) + 1;
  netw_cell_rec_ptr junction, chypo;
  sign_rec_ptr h;
  int ncol2 = 0;
  short i, j;
  // Create junction cell in col+1
  _NETW_NEWCELL( junction );
  junction->y			= y1 + 1;
  junction->head		= col1;
  junction->client_data_t	= _NETW_JUNCTION_T;
  junction->client_data	= NULL;
  netw__col_append_cell( col1, junction );
  // Update shift-clicked cell
  cell->nright = 1;
  cell->right  = (netw_cell_rec_ptr *) malloc( sizeof(netw_cell_rec_ptr) );
  cell->right[0] = junction;
  printf( "FWRD Added junction in %d, col=%d at y=%d\n",
	  cell->nright - 1, junction->head->x, junction->y );
  // Link junction to forward hypotheses (w/o repetition)
  if( y1 > y2 ){
    // Junction height is heigher than top of COL2
    // Move it higher to accomodate the number of forward signs.
    // (The number of distinct forward signs would be better.)
    if( (y1 - sign->nsetters/2) > (y2 + 1) )
      y2 = y1 - sign->nsetters/2;
    else{
      y1 = y2 + 1 + sign->nsetters/2;
    }
  }
  for( i=0; i<sign->nsetters; i++ ){
    h = (sign_rec_ptr) ((fwrd_rec_ptr) sign->setters[i])->rule->setters;
    printf( "FWRD %s\n", h->str );
    if( 0 == junction->nright ){
      junction->nright = 1;
      junction->right = (netw_cell_rec_ptr *) malloc( sizeof(netw_cell_rec_ptr) );
      _NETW_NEWCELL( chypo );
      chypo->y		= y2 + 1;
      chypo->head	= col2;
      chypo->client_data_t	= _NETW_SIGN_T;
      chypo->client_data	= (void *)h;
      netw__col_append_cell( col2, chypo );
      y2 += 1;
      junction->right[0] = chypo;
      printf( "FWRD Added %s in %d, col=%d at y=%d\n",
	      h->str, junction->nright - 1, chypo->head->x, chypo->y );
    }
    else{
      // Is it already present?
      short found = 0;
      for( j=0; j<junction->nright; j++ ){
	if( h == (sign_rec_ptr) junction->right[j]->client_data ){
	  found = 1;
	  break;
	}
      }
      if( !found ){
	junction->nright += 1;
	junction->right = (netw_cell_rec_ptr *) realloc( junction->right, junction->nright*sizeof(netw_cell_rec_ptr) );
	if( !junction->right ) printf( "*** ERROR reallocating\n\n" );
	_NETW_NEWCELL( chypo );
	chypo->y		= y2 + 1;
	chypo->head		= col2;
	chypo->client_data_t	= _NETW_SIGN_T;
	chypo->client_data	= (void *)h;
	netw__col_append_cell( col2, chypo );
	y2 += 1;
	junction->right[ junction->nright - 1 ] = chypo;
	printf( "FWRD Added %s in %d, col=%d at y=%d\n",
		h->str, junction->nright - 1, chypo->head->x, chypo->y );
      }
    }
  }
  // Mark as expanded
  _EXP_LR_SET(cell);
}

void netw__expand_backward(  cdCanvas *canvas, netw_cell_rec_ptr cell,
			     double WORLD_W, double WORLD_H, unsigned short orientation ){
  rule_rec_ptr r;
  sign_rec_ptr sign	= (sign_rec_ptr) cell->client_data;
  int ncol1		= sign->ngetters;
  int ncol2		= 0;
  col_rec_ptr  head	= (col_rec_ptr) cdCanvasGetAttribute( canvas, "USERDATA" );
  col_rec_ptr  col1	= netw__col_get_create( canvas, cell->head->x + 1 );
  col_rec_ptr  col2	= netw__col_get_create( canvas, cell->head->x + 2 );
  netw_cell_rec_ptr c, crule;
  netw_cell_rec_ptr cparent1 = NULL,
    cparent2 = NULL,
    cparent1min = NULL,
    cparent2min = NULL;
  short ir, i;
  int y1		= netw__col_ymax_cell( head, col1, &cparent1 );
  int y2		= netw__col_ymax_cell( head, col2, &cparent2 );
  int y1min		= netw__col_ymin_cell( head, col1, &cparent1min );
  int y2min		= netw__col_ymin_cell( head, col2, &cparent2min );
  printf( "ToggleExpand %s (%d,%d): nrules=%d, ymax1=%d, ymax2=%d\npmax1=%d, pmax2=%d, pmin1=%d, pmin2=%d\n",
	  sign->str, cell->head->x, cell->y, ncol1, y1, y2,
	  cparent1 ? cparent1->y : -1,
	  cparent2 ? cparent2->y : -1,
	  cparent1min ? cparent1min->y : -1,
	  cparent2min ? cparent2min->y : -1 );
  // Allocate left links in RL orientation
  cell->nleft = ncol1;
  cell->left = (netw_cell_rec_ptr *) malloc( ncol1 * sizeof(netw_cell_rec_ptr) );
  // Impossible Aesthetics!
  /* y1 = (y2) ? y2 + 1 : y2 ; */
  /* y2 = y1; */
  for( ir=0; ir<ncol1; ir++ ){
    r = (rule_rec_ptr) ((bwrd_rec_ptr) sign->getters[ ncol1 - 1 - ir ])->rule;
    ncol2 += r->nrhs + r->ngetters + 1;
  }
  printf( "ToggleExpand: nconds=%d, ymin1=%d, ymin2=%d\n",
	  ncol2, y1min, y2min );
  //
  if( y2 ){
    // COL2 has cells between y2min and y2
    if( ncol2 < y2min ){
      // There is a wide enough gap under the minimum height in COL2
      y1 = y2min - ncol2 - 1;
      y2 = y1;
    }
    else{
      // Use upper space in COL2
      if( (cell->y - ncol2/2) > y2 ){
	// Bottom of a horizontal new block in COL2 higher than max height
	// This might create gaps within COL2
	y2 = cell->y - ncol2/2 + 1;
	y1 = y2 > y1 ? y2 : y1 ;
      }
      else{
	// Default: higher than the max height in COL2
	y1 = y2 + 1 > y1 ? y2 + 1 : y1 + 1 ;
	y2 = y1;
      }
    }
  }
  else{
    // COL2 is empty (ymax2 == 0)
    y2 = (cell->y - ncol2/2) > 0 ? (cell->y - ncol2/2) : 1;
    y1 = y2;
  }
  //
  for( ir=0; ir<ncol1; ir++ ){
    r = (rule_rec_ptr) ((bwrd_rec_ptr) sign->getters[ ncol1 - 1 - ir ])->rule;
    _NETW_NEWCELL( c );
    c->y		= y1 + 1 + (r->ngetters + r->nrhs)/2 ;
    y1                 += 1 + (r->ngetters + r->nrhs);
    c->head		= col1;
    c->client_data_t	= _NETW_RULE_T;
    c->client_data	= (void *) r;
    netw__col_append_cell( col1, c );
    crule = cell->left[ir] = c;
    // Allocate for links from rule to LHS and RHS
    crule->nleft = (r->nrhs + r->ngetters);
    crule->left = (netw_cell_rec_ptr *) malloc( (r->nrhs + r->ngetters) * sizeof(netw_cell_rec_ptr) );
    y2 += 1;
    for( i=0; i < r->nrhs; i++ ){
      _NETW_NEWCELL( c );
      c->y = y2++;
      c->head = col2;
      c->client_data_t = _NETW_STR_T;
      c->client_data   = (void *) r->rhs[ r->nrhs - 1 - i ];
      netw__col_append_cell( col2, c );
      crule->left[i] = c;
    }
    for( i=0; i < r->ngetters; i++ ){
      _NETW_NEWCELL( c );
      c->y = y2++;
      c->head = col2;
      c->client_data_t = ((cond_rec_ptr) r->getters[ r->ngetters - 1 - i ])->out ?
	_NETW_SIGN_YES_T : _NETW_SIGN_NO_T ;
      c->client_data   = (void *) ((cond_rec_ptr) r->getters[ r->ngetters - 1 - i ])->sign;
      netw__col_append_cell( col2, c );
      crule->left[ r->nrhs + i ] = c;
    }
  }
  // Mark as expanded
  _EXP_RL_SET(cell);
}

void netw__recursive_remove_forward( cdCanvas *canvas, netw_cell_rec_ptr cell, unsigned short orientation ){
  short i;
  netw_cell_rec_ptr junction, c;

  /* printf( "REMOVE cell=%s, nleft=%d, expanded=%d\n", */
  /* 	  netw__client_text(cell), */
  /* 	  cell->nleft, */
  /* 	  cell->expanded ); */
  switch( orientation ){
  case NETW_RL:
    // Remove right subtree
    junction = cell->right[0];
    for( i=0; i<junction->nright; i++ ){
      c = junction->right[i];
      if( NETW_BOOLEAN_SIGN(c) && _EXP_RL_P( c ) ) netw__recursive_remove_backward( canvas, c, orientation );
      if( NETW_BOOLEAN_SIGN(c) && _EXP_LR_P( c ) ) netw__recursive_remove_forward( canvas, c, orientation );
      netw__col_unlink_cell( c->head, c );
      free( (void *) c );
    }
    netw__col_unlink_cell( junction->head, junction );
    if( junction->right ) free( (void *) junction->right );
    free( (void *) junction );
    if( cell->nright ) free( (void *) cell->right );
    cell->nright = 0;
    cell->right = NULL;
    _EXP_LR_RESET(cell);
    break;

  case NETW_LR:
    break;
  }
}

void netw__recursive_remove_backward( cdCanvas *canvas, netw_cell_rec_ptr cell, unsigned short orientation ){
  short i, ir;
  netw_cell_rec_ptr crule, c;

  /* printf( "REMOVE cell=%s, nleft=%d, expanded=%d\n", */
  /* 	  netw__client_text(cell), */
  /* 	  cell->nleft, */
  /* 	  cell->expanded ); */
  
  switch( orientation ){
  case NETW_RL:
    // Remove left subtree
    for( ir=0; ir<cell->nleft; ir++ ){
      crule = cell->left[ir];
      /* printf( "\tRule %s nleft=%d\n", netw__client_text(crule), crule->nleft ); */
      for( i=0; i<(crule->nleft); i++ ){
	c = crule->left[i];
	if( NETW_BOOLEAN_SIGN(c) && _EXP_RL_P( c ) ) netw__recursive_remove_backward( canvas, c, orientation );
	if( NETW_BOOLEAN_SIGN(c) && _EXP_LR_P( c ) ) netw__recursive_remove_forward( canvas, c, orientation );
	netw__col_unlink_cell( c->head, c );
	free( (void *) c );
      }
      netw__col_unlink_cell( crule->head, crule );
      if( crule->nleft ) free( (void *) crule->left );
      free( (void *) crule );
      /* printf( "\tRemoved\n" ); */
    }
    if( cell->nleft ) free( (void *) cell->left );
    cell->nleft = 0;
    cell->left  = NULL;
    _EXP_RL_RESET(cell);
    break;
  case NETW_LR:
    break;
  }
}

void netw__toggle_expand( cdCanvas *canvas, netw_cell_rec_ptr cell, int shifted,
			  double WORLD_W, double WORLD_H, unsigned short orientation ){
  switch( orientation ){
  case NETW_RL:
    if( 0 == shifted ){
      // Backward chaining
      if( _EXP_RL_P(cell) ){
	if( NETW_BOOLEAN_SIGN( cell ) ){
	  netw__recursive_remove_backward( canvas, cell, orientation );
	}
      }
      else{
	if( NETW_BOOLEAN_SIGN( cell ) ){
	  netw__expand_backward( canvas, cell, WORLD_W, WORLD_H, orientation );
	  // Expand DRAWSIZE left as needed, by shifting colums right
	  /* printf( "EXPANDED cell at x=%d, y=%d, max=%d\n", cell->head->x, cell->y, WORLD_W/CELL_W ); */
	  if( (cell->head->x) > (WORLD_W/CELL_W - 2) )
	    netw_adjust_horz( (col_rec_ptr) cdCanvasGetAttribute( canvas, "USERDATA" ), -2 );
	}
      }
    }
    else{
      // Forward chaining
      if( _EXP_LR_P(cell) ){
	// TODO: (recursive?) remove
	if( NETW_BOOLEAN_SIGN( cell ) )
	  netw__recursive_remove_forward( canvas, cell, orientation );
      }
      else{
	if( NETW_BOOLEAN_SIGN( cell ) )
	  netw__expand_forward( canvas, cell, WORLD_W, WORLD_H, orientation );
      }
    }
    break;
    
  case NETW_LR:
    break;
  }
}
