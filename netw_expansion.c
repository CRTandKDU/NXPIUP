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
				  ((cell)->client_data_t == _NETW_SIGN_NO_T) )	)
				 /* &&							\ */
				 /* COMPOUND_MASK != (((sign_rec_ptr) (cell)->client_data)->len_type & TYPE_MASK)) */

/*
Intensive reworking and factoring of the forward linking
representation in the Rule Network.  One overlay over the "boolean"
rule network is introduced, in relation with the DSL used in conds and
rhses.

In the base boolean rule networks backward links (full lines) appear:

  - From hypothesis to its rules,
  - From rule to all its conditions and rhses.
  - From boolean conditions (YES hypo, NO hypo) on hypotheses to
    mentioned hypothesis.

and forward links (dotted lines) appear:

  - From hypothesis to forwarded hypotheses,
  - From boolean condition to forwarded hypotheses.

This constitutes the base boolean rule network.

When the overlay is visible, according to the parameter checkbox,
additional forward linking information appears:

  - Junction points are annotated with the sign or hypo at the source
    of the forward link,
  - Additional forward links from DSL conditions and rhses to the
    forwarded hypotheses are displayed.

Backward links are unaffected by this overlay.

In both base and overlay, clicking or shify-clicking on a node
recursively removes the backward or forward links, respectively.
*/

/*-----------------------------------------------------------------
  FORWARD LINKING
  ---------------------------------------------------------------*/  

netw_cell_rec_ptr netw__forward_junction( cdCanvas *canvas, netw_cell_rec_ptr cell,
					  double WORLD_W, double WORLD_H, unsigned short orientation,
					  sign_rec_ptr sign, int *y1ptr, int *y2ptr ){
  netw_cell_rec_ptr junction;
  netw_cell_rec_ptr cp1, cp2, cp1min, cp2min;
  col_rec_ptr  head	= (col_rec_ptr) cdCanvasGetAttribute( canvas, "USERDATA" );
  col_rec_ptr  col1	= netw__col_get_create( canvas, cell->head->x - 1 );
  col_rec_ptr  col2	= netw__col_get_create( canvas, cell->head->x - 2 );
  int y1		= netw__col_ymax_cell( head, col1, &cp1 );
  int y2		= netw__col_ymax_cell( head, col2, &cp2 );
  int y1min		= netw__col_ymin_cell( head, col1, &cp1min );
  int y2min		= netw__col_ymin_cell( head, col2, &cp2min );
  int z                 = WORLD_H/(CELL_H+CELL_H);

  // Create junction cell in col+1
  /* printf( "Exp FWRD y1=%d, y2=%d\n", y1, y2 ); */
  if( 0 == y1 && 0 == y2 ){
    // COL1 and COL2 are both empty
    y1 = y2 = cell->y - 1;
  }
  else{
    if( cell->y >= z ){
      y1 += 1; y2 += 1;
    }
    else{
      y1 = y1min - 2;
     }
  }
  /* printf( "* Exp FWRD junction at y1=%d, y2=%d\n", y1, y2 ); */
  //
  _NETW_NEWCELL( junction );
  junction->y			= y1;
  junction->head		= col1;
  junction->client_data_t	= _NETW_JUNCTION_T;
  junction->client_data		= sign;
  netw__col_append_cell( col1, junction );
  // Update shift-clicked cell
  if( 0 == cell->nright ){
    cell->nright = 1;
    cell->right  = (netw_cell_rec_ptr *) malloc( sizeof(netw_cell_rec_ptr) );
  }
  else{
    cell->nright += 1;
    cell->right   = (netw_cell_rec_ptr *) realloc( (void *) cell->right, cell->nright * sizeof(netw_cell_rec_ptr) );
  }
  cell->right[ cell->nright - 1 ] = junction;
  /* printf( "* FWRD Added junction in [%d], col=%d at y=%d\n", */
  /* 	  cell->nright - 1, junction->head->x, junction->y ); */
  // Link junction to forward hypotheses (w/o repetition) and readjust heights
  /* printf( "\t* FWRD y1=%d, y2=%d, y1min=%d, y2min=%d\n", y1, y2, y1min, y2min ); */
  if( cell->y >= z && y1 > y2 ){
    // Junction height is heigher than top of COL2
    // Move it higher to accomodate the number of forward signs.
    // (The number of distinct forward signs would be better.)
    if( (y1 - sign->nsetters/2) > (y2 + 1) )
      y2 = y1 - sign->nsetters/2;
    else{
      y1 = y2 + 1 + sign->nsetters/2;
    }
  }
  else if( cell->y >= z ){
    y1 = y2 + 1 + sign->nsetters/2;
  }
  else if( cell->y < z && y1 < y2min ){
    // Junction height is lower than the min height of COL2
    if( y1 + sign->nsetters/2 < y2min -1 ){
      y2 = y1 - sign->nsetters/2;
    }
    else{
      y1 = y2min - 1 - sign->nsetters/2;
      y2 = y2min - 1 - sign->nsetters;
    }
  }
  else if( cell->y < z ){
      y1 = y2min - 1 - sign->nsetters/2;
      y2 = y2min - 2 - sign->nsetters;
  }
  /* printf( "\tFWRD y1=%d, y2=%d, y1min=%d, y2min=%d\n", y1, y2, y1min, y2min ); */
  junction->y = y1;
  //
  *y1ptr = y1; *y2ptr = y2;
  return junction;
}

void netw__forward_junction_expand( cdCanvas *canvas, netw_cell_rec_ptr cell,
				    double WORLD_W, double WORLD_H, unsigned short orientation,
				    netw_cell_rec_ptr junction, sign_rec_ptr h, int *y1ptr, int *y2ptr ){
  netw_cell_rec_ptr chypo;
  short j;
  col_rec_ptr  head	= (col_rec_ptr) cdCanvasGetAttribute( canvas, "USERDATA" );
  col_rec_ptr  col1	= netw__col_get_create( canvas, cell->head->x - 1 );
  col_rec_ptr  col2	= netw__col_get_create( canvas, cell->head->x - 2 );
  printf( "FWRD Junction expand %s\n", h->str );
  if( 0 == junction->nright ){
    junction->nright = 1;
    junction->right = (netw_cell_rec_ptr *) malloc( sizeof(netw_cell_rec_ptr) );
    _NETW_NEWCELL( chypo );
    chypo->y			= *y2ptr + 1;
    chypo->head			= col2;
    chypo->client_data_t	= _NETW_SIGN_T;
    chypo->client_data		= (void *)h;
    netw__col_append_cell( col2, chypo );
    *y2ptr += 1;
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
      if( !junction->right ){
	printf( "*** ERROR reallocating\n\n" );
      }
      _NETW_NEWCELL( chypo );
      chypo->y			= *y2ptr + 1;
      chypo->head		= col2;
      chypo->client_data_t	= _NETW_SIGN_T;
      chypo->client_data	= (void *)h;
      netw__col_append_cell( col2, chypo );
      *y2ptr += 1;
      junction->right[ junction->nright - 1 ] = chypo;
      printf( "FWRD Added %s in %d, col=%d at y=%d\n",
	      h->str, junction->nright - 1, chypo->head->x, chypo->y );
    }
  }
}

void netw__forward_dslvar( cdCanvas *canvas, netw_cell_rec_ptr cell,
			     double WORLD_W, double WORLD_H, unsigned short orientation,
			     sign_rec_ptr dsl_var ){
  netw_cell_rec_ptr junction;
  sign_rec_ptr h;
  int y1, y2;
  short i, j;
  junction = netw__forward_junction( canvas, cell, WORLD_W, WORLD_H, orientation,
				     dsl_var, &y1, &y2 );
  for( i=0; i<dsl_var->nsetters; i++ ){
    // Compounds have only one setter-rule
    h = (sign_rec_ptr) (((fwrd_rec_ptr) ((sign_rec_ptr) ((fwrd_rec_ptr) dsl_var->setters[i])->rule)->setters[0])->rule->setters);
    netw__forward_junction_expand( canvas, cell, WORLD_W, WORLD_H, orientation, junction, h, &y1, &y2 );
  }
}

void netw__forward_compound( cdCanvas *canvas, netw_cell_rec_ptr cell,
			     double WORLD_W, double WORLD_H, unsigned short orientation ){
  short i;
  fwrd_rec_ptr fwrd;
  sign_rec_ptr compound = (sign_rec_ptr) cell->client_data;
  sign_rec_ptr s, top;
  s = top = loadkb_get_allsigns();
  while( s ){
    for( i=0; i<s->nsetters; i++ ){
      fwrd = (fwrd_rec_ptr) s->setters[i];
      if( fwrd->idx_cond < 0 && compound == (sign_rec_ptr) fwrd->rule ){
	/* printf( "DSL VAR: %s, in %s\n", s->str, compound->str ); */
	netw__forward_dslvar( canvas, cell, WORLD_W, WORLD_H, orientation, s );
      }
    }
    s = s->next;
  }
}


typedef struct netw_exp_context_rec{
  cdCanvas *canvas;
  double world_w;
  double world_h;
  unsigned short orientation;
  netw_cell_rec_ptr cell;
} *netw_exp_context_rec_ptr;

sign_rec_ptr forward_rhs_cb (char *pw, compound_rec_ptr compound, sign_rec_ptr top ){
  netw_exp_context_rec_ptr context = (netw_exp_context_rec_ptr) compound;
  sign_rec_ptr s = sign_find( pw, top );
  if( s ){
    netw__forward_dslvar( context->canvas, context->cell, context->world_w, context->world_h, context->orientation, s );
  }
  return NULL;
}

void netw__forward_rhs( cdCanvas *canvas, netw_cell_rec_ptr cell,
			double WORLD_W, double WORLD_H, unsigned short orientation ){
  struct netw_exp_context_rec context;
  context.canvas        = canvas;
  context.world_w	= WORLD_W;
  context.world_h	= WORLD_H;
  context.orientation	= orientation;
  context.cell		= cell;
  sign_rec_ptr ignore;
  char buf[ _NETW_TEMP_BUFSIZE + _NETW_TEMP_BUFSIZE ] = {0};
  sprintf( buf, (char *) cell->client_data );
  ignore = loadkb_parse( buf, (compound_rec_ptr) &context, loadkb_get_allsigns(), forward_rhs_cb );
  // Mark as expanded
  _EXP_LR_SET(cell);
}

void netw__forward_single( cdCanvas *canvas, netw_cell_rec_ptr cell,
			   double WORLD_W, double WORLD_H, unsigned short orientation ){
  short i, j;
  netw_cell_rec_ptr junction, chypo;
  sign_rec_ptr h, sign = (sign_rec_ptr) cell->client_data;
  col_rec_ptr  head	= (col_rec_ptr) cdCanvasGetAttribute( canvas, "USERDATA" );
  col_rec_ptr  col1	= netw__col_get_create( canvas, cell->head->x - 1 );
  col_rec_ptr  col2	= netw__col_get_create( canvas, cell->head->x - 2 );
  int y1, y2;

  junction = netw__forward_junction( canvas, cell, WORLD_W, WORLD_H, orientation,
				     sign, &y1, &y2 );
  //
  for( i=0; i<sign->nsetters; i++ ){
    h = (sign_rec_ptr) ((fwrd_rec_ptr) sign->setters[i])->rule->setters;
    netw__forward_junction_expand( canvas, cell, WORLD_W, WORLD_H, orientation, junction, h, &y1, &y2 );
  }
}


void netw__expand_forward(  cdCanvas *canvas, netw_cell_rec_ptr cell,
			    double WORLD_W, double WORLD_H, unsigned short orientation ){
  sign_rec_ptr sign = (sign_rec_ptr) cell->client_data;

  // Eligible to frwrd expansion
  char *param = IupGetAttribute( IupGetHandle( "netw_compound" ), "VALUE" );
  /* printf( "FWRD %s (%d) with compound=%s\n", sign->str, sign->nsetters, param ); */
  if( COMPOUND_MASK == (sign->len_type & TYPE_MASK) && 0 == strcmp( "OFF", param ) )
    return;
  if( cell->head->x <= 2  )
    return;
  //
  if( COMPOUND_MASK != (sign->len_type & TYPE_MASK) ){
    // Not a compound condition: either a YES/NO condition, a focused-on hypo, or a focused-on sign.
    switch( (sign->len_type & TYPE_MASK) ){
    case HYPO_MASK:
      netw__forward_single( canvas, cell, WORLD_W, WORLD_H, orientation );
      break;
    case SIGN_MASK:
      netw__forward_dslvar( canvas, cell, WORLD_W, WORLD_H, orientation, sign );
      break;
    }
  }
  else{
    netw__forward_compound( canvas, cell, WORLD_W, WORLD_H, orientation );
  }
  // Mark as expanded
  _EXP_LR_SET(cell);
}

/*-----------------------------------------------------------------
  BACKWARD LINKING
  ---------------------------------------------------------------*/  

void netw__useupper( int z, int *y1, int *y2 ){
  /* printf("\tUPPER z=%d, y1=%d, y2=%d\n", z, *y1, *y2 ); */
  // Use upper space in COL2
  if( z > *y2 ){
    // Bottom of a horizontal new block in COL2 higher than max height
    // This might create gaps within COL2
    *y2 = z + 1;
    *y1 = *y2 > *y1 ? *y2 : *y1 ;
  }
  else{
    // Default: higher than the max height in COL2
    *y1 = *y2 + 1 > *y1 ? *y2 + 1 : *y1 + 1 ;
    *y2 = *y1;
  }
  /* printf("\tUPPER z=%d, y1=%d, y2=%d\n", z, *y1, *y2 ); */
}

void netw__uselower( int z, int *y1, int *y2 ){
  /* printf("\tLOWER z=%d, y1=%d, y2=%d\n", z, *y1, *y2 ); */
  // There is a wide enough gap under the minimum height in COL2 (z>0)
  *y1 = z - 1;
  *y2 = *y1;
  /* printf("\tLOWER z=%d, y1=%d, y2=%d\n", z, *y1, *y2 ); */
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
  int z;
  int y1		= netw__col_ymax_cell( head, col1, &cparent1 );
  int y2		= netw__col_ymax_cell( head, col2, &cparent2 );
  int y1min		= netw__col_ymin_cell( head, col1, &cparent1min );
  int y2min		= netw__col_ymin_cell( head, col2, &cparent2min );
  // Nothing to expand, or a valid sign but compound.
  if( _NETW_SIGN_NO_T  != cell->client_data_t &&
      _NETW_SIGN_YES_T != cell->client_data_t &&
      _NETW_SIGN_T     != cell->client_data_t )
    return;
  if( COMPOUND_MASK == (sign->len_type & TYPE_MASK) )
    return;

  /* printf( "ToggleExpand %s (%d,%d): nrules=%d, ymax1=%d, ymax2=%d\npmax1=%d, pmax2=%d, pmin1=%d, pmin2=%d\n", */
  /* 	  sign->str, cell->head->x, cell->y, ncol1, y1, y2, */
  /* 	  cparent1 ? cparent1->y : -1, */
  /* 	  cparent2 ? cparent2->y : -1, */
  /* 	  cparent1min ? cparent1min->y : -1, */
  /* 	  cparent2min ? cparent2min->y : -1 ); */
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
  /* printf( "ToggleExpand: nconds=%d, ymin1=%d, ymin2=%d\n", */
  /* 	  ncol2, y1min, y2min ); */
  //
  /* printf( "ToggleExpand: y1=%d, y2=%d - p1=%d p2=%d\n", y1, y2, (long int) cparent1, (long int) cparent2 ); */
  if( y2 ){
    if( cparent1min && cparent1 ){
      if( cell->y < cparent1min->y ){
	// Cell clicked is lower than the same-column parent of the lowest cells in col1 and col2
	// COL2 has cells between y2min and y2
	z = y2min - ncol2;
	if( 0 < z ){
	  // There is a wide enough gap under the minimum height in COL2
	  /* printf( "\tCase 0\n" ); */
	  netw__uselower( z, &y1, &y2 );
	}
	else{
	  // Experiment with moving up all network ncol2 - y2min + 1
	  // For now
	  /* printf( "\tCase 1\n" ); */
	  netw__useupper( (cell->y - ncol2/2), &y1, &y2);
	}
      }
      else if( cell->y > cparent1->y ){
	// Cell clicked is higher than the same-column parent of the highest cells in col1 and col2
	/* printf( "\tCase 2\n" ); */
	netw__useupper( (cell->y - ncol2/2), &y1, &y2);
      }
      else{
	if( (cparent1->y - cell->y) >= (cell->y - cparent1min->y) ){
	  // TODO: Not enough lower space
	  /* printf( "\tCase 3\n" ); */
	  netw__uselower( y2min - ncol2, &y1, &y2);
	}
	else{
	  /* printf( "\tCase 4\n" ); */
	  netw__useupper( (cell->y - ncol2/2), &y1, &y2);
	}
      }
    }
    else{
      z = WORLD_H/(CELL_H+CELL_H);
      if( cell->y > z ){
	/* printf( "\tCase 5\n" ); */
	r = (rule_rec_ptr) ((bwrd_rec_ptr) sign->getters[ ncol1 - 1 ])->rule;
	netw__useupper( (cell->y - (r->nrhs + r->ngetters + 1)/2), &y1, &y2);
      }
      else{
	if( y2min < y1min ){
	  /* printf( "\tCase 6\n" ); */
	  netw__uselower( y2min - ncol2, &y1, &y2);
	}
	else{
	  /* printf( "\tCase 7\n" ); */
	  netw__uselower( y1min - ncol2, &y1, &y2);
	}
      }
    }
  }
  else{
    // COL2 is empty (ymax2 == 0)
    y2 = (cell->y - ncol2/2) > 0 ? (cell->y - ncol2/2) : 1;
    y1 = y2;
  }
  /* printf( "ToggleExpand: y1=%d, y2=%d\n", y1, y2 ); */
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

/*-----------------------------------------------------------------
  REMOVING LINKING
  ---------------------------------------------------------------*/  

void netw__recursive_remove_forward( cdCanvas *canvas, netw_cell_rec_ptr cell, unsigned short orientation ){
  short i, j;
  netw_cell_rec_ptr junction, c;

  switch( orientation ){
  case NETW_RL:
    // Remove right subtree
    for( j=0; j<cell->nright; j++ ){
      junction = cell->right[j];
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
    }
    //
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

  switch( orientation ){
  case NETW_RL:
    // Remove left subtree
    for( ir=0; ir<cell->nleft; ir++ ){
      crule = cell->left[ir];

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

	  if( (cell->head->x) > (WORLD_W/CELL_W - 2) )
	    netw_adjust_horz( (col_rec_ptr) cdCanvasGetAttribute( canvas, "USERDATA" ), -2 );
	}
      }
    }
    else{
      // Forward chaining
      if( _EXP_LR_P(cell) ){
	// TODO: (recursive?) remove
	if( NETW_BOOLEAN_SIGN( cell ) || ( cell->client_data_t == _NETW_STR_T ) )
	  netw__recursive_remove_forward( canvas, cell, orientation );
      }
      else{
	if( NETW_BOOLEAN_SIGN( cell ) )
	  netw__expand_forward( canvas, cell, WORLD_W, WORLD_H, orientation );
	else if( cell->client_data_t == _NETW_STR_T ){
	  char *param = IupGetAttribute( IupGetHandle( "netw_compound" ), "VALUE" );
	  if( 0 == strcmp( param, "ON" ) )
	    netw__forward_rhs( canvas, cell, WORLD_W, WORLD_H, orientation );
	}
      }
    }
    break;
    
  case NETW_LR:
    break;
  }
}
