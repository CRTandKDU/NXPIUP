/**
 * netw.c -- Renewed experimentations with knowled base networks
 *
 * Written on 2026-04-29.
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

#include "nxpiup.h"

static netw_cell_rec_ptr S_cellclicked = NULL;

int netw__findcell( col_rec_ptr head, double xw, double yw, double WORLD_W, double WORLD_H, unsigned short orientation,
		    netw_cell_rec_ptr *result_cell ){
  col_rec_ptr  col = head;
  netw_cell_rec_ptr cell;
  int xmin, xmax;
  while( col ){
    switch( orientation ){
    case NETW_RL:
      xmax = WORLD_W - CELL_W*col->x ;
      xmin = WORLD_W - CELL_W*col->x - CELL_W;
      break;
    case NETW_LR:
      xmax = CELL_W*col->x + CELL_W;
      xmin = CELL_W*col->x ;
      break;
    }
    if( ( xw < xmax ) && ( xmin < xw ) ){
      cell = col->first;
      while( cell ){
	if( ( cell->y*CELL_H < yw ) && ( yw < cell->y*CELL_H + CELL_H ) ){
	  *result_cell = cell;
	  return 1;
	}
	cell = cell->next;
      }
    }
    col = col->next;
  }
  return 0;
}

int netw_click( cdCanvas *canvas, int but, int press, int x, int y, int shifted,
		double WORLD_W, double WORLD_H, unsigned short orientation ){
  double xw, yw;
  netw_cell_rec_ptr cell;
  int needredraw = 0;
  wdCanvasCanvas2World( canvas, x, y, &xw, &yw);
  printf( "Converts %f %f, shifted=%d\n", xw, yw, shifted );
  if( netw__findcell( (col_rec_ptr) cdCanvasGetAttribute( canvas, "USERDATA" ), xw, yw, WORLD_W, WORLD_H, orientation,
		      &cell ) ){
    /* printf( "Found at col=%d, cell=%d, %s\n", cell->head->x, cell->y, */
    /* 	    ((sign_rec_ptr)cell->client_data) ? ((sign_rec_ptr)cell->client_data)->str : "NO SIGN" ); */
    if( press && NULL == S_cellclicked ){
      S_cellclicked = cell;
    }
    else if ( !press && S_cellclicked ){
      if( S_cellclicked == cell ){
	/* printf( "Clicked in col=%d, cell=%d, %s\n", cell->head->x, cell->y, */
	/* 	((sign_rec_ptr)cell->client_data) ? ((sign_rec_ptr)cell->client_data)->str : "NO SIGN" ); */
	netw__toggle_expand( canvas, cell, shifted, WORLD_W,  WORLD_H, NETW_RL );
	needredraw = 1;
      }
      else{
	/* printf( "No click\n" ); */
      }
      S_cellclicked = NULL;
    }
  }
  else{
    if( !press ) S_cellclicked = NULL;
  }
  return needredraw;	
}

void netw_adjust_vert( cdCanvas *canvas, int inc ){
  col_rec_ptr col = (col_rec_ptr) cdCanvasGetAttribute( canvas, "USERDATA" );
  while( col ){
    netw__adjust_col_vert( col->first, inc );
    col = col->next;
  }
}

void netw_initfill_all( cdCanvas *canvas, double WORLD_W, double WORLD_H, unsigned short orientation ){
  sign_rec_ptr s, top = (sign_rec_ptr) loadkb_get_allhypos();
  col_rec_ptr col = _NEW_COL;
  col->x	= WORLD_W/CELL_W - 3;
  col->next	= NULL;
  netw_cell_rec_ptr cptr;
  short i;
  for( s=top, i=0, cptr = (netw_cell_rec_ptr)col; s; s=s->next  ){
    if( 0 == s->nsetters ){
      // Final hypothesis only
      _NETW_NEWCELL( cptr->next );
      cptr->next->y		= i+1;
      cptr->next->head		= col;
      cptr->next->client_data_t = _NETW_SIGN_T;
      cptr->next->client_data	= (void *)s;
      i += 1;
      cptr=cptr->next;
    }
  }
  // Second pass to adjust height of cells
  int inc = (WORLD_H/CELL_H - i)/2;
  printf("INITFILLALL i=%d, inc=%d\n", i, inc );
  netw__adjust_col_vert( col->first, inc );
  /* for( cptr=col->first; cptr; cptr=cptr->next ){ cptr->y += inc; } */
  //
  cdCanvasSetAttribute( canvas, "USERDATA", (char *) col);
}

void netw_free( cdCanvas *canvas ){
  netw_cell_rec_ptr cell, old_cell;
  col_rec_ptr  old_col, col = (col_rec_ptr) cdCanvasGetAttribute( canvas, "USERDATA" );
  while( col ){
    cell = col->first;
    while( cell ){
      old_cell = cell;
      cell = cell->next;
      if( old_cell->nleft )  free( (void *) old_cell->left );
      if( old_cell->nright ) free( (void *) old_cell->right );
      free( (void *) old_cell );
    }
    old_col = col;
    col     = col->next;
    free( (void *) old_col );
  }
}

void netw_redrawkb( cdCanvas *canvas, int scale, double WORLD_W, double WORLD_H, unsigned short orientation ){
  col_rec_ptr  col;
  netw_cell_rec_ptr cell;
  short i, current = 0;
  int xv, yv;
  int x0, y0;
  int xmin, xmax, ymin, ymax, xw, yh;
  long int text_color;

  cdCanvasFont( canvas, "Times", CD_PLAIN, 10 );
  cdCanvasTextAlignment( canvas, NETW_RL == orientation ? CD_SOUTH_EAST : CD_SOUTH_WEST );
  if( 1 == scale )
    // On-scale redrawing
    netw__redraw_onscale( canvas, scale, WORLD_W, WORLD_H, orientation );
  else{
    // Off scale (default) redrawing
    col = (col_rec_ptr) cdCanvasGetAttribute( canvas, "USERDATA" ); 
    while( col ){
      cell = col->first;
      while( cell ){
	// Draw default 50%-reduced inside rectangle
	if( _NETW_JUNCTION_T  != cell->client_data_t ){
	  wdCanvasWorld2Canvas( canvas, (double)
				(NETW_RL == orientation ?
				 (WORLD_W - CELL_W*col->x - CELL_W) : CELL_W*col->x + CELL_W ),
				(double) (CELL_H*cell->y), &xmin, &ymin );
	  wdCanvasWorld2Canvas( canvas, (double)
				(NETW_RL == orientation ?
				 (WORLD_W - CELL_W*col->x) : CELL_W*col->x ),
				(double) (CELL_H*cell->y + CELL_H), &xmax, &ymax );
	  xw = (xmax - xmin)/4; yh = (ymax - ymin)/4;
	  current =  ( _NETW_RULE_T == cell->client_data_t ) ?
	    nxpiup_inagendap( (sign_rec_ptr) (((rule_rec_ptr) cell->client_data)->setters) ) :
	    nxpiup_inagendap( (sign_rec_ptr) cell->client_data );
	  if( current ){
	    cdCanvasBox(canvas, xmin + xw, xmax - xw, ymin + yh , ymax - yh );
	  }
	  else{
	    if( _KNOWN      == ((sign_rec_ptr) cell->client_data)->val.status &&
		_VAL_T_BOOL == ((sign_rec_ptr) cell->client_data)->val.type ) {
	      text_color = cdCanvasForeground( canvas,
					       _FALSE == ((sign_rec_ptr) cell->client_data)->val.val_bool ?
					       S_NETW_COLOR_FALSE :
					       S_NETW_COLOR_TRUE );
	      cdCanvasBox(canvas, xmin + xw, xmax - xw, ymin + yh , ymax - yh );
	      text_color = cdCanvasForeground( canvas, text_color );
	    }
	    else{
	      cdCanvasRect(canvas, xmin + xw, xmax - xw, ymin + yh , ymax - yh );
	    }
	  }
	  current = 0;
	}
	// Draw right links
	if( cell->nright ){
	  if( _NETW_JUNCTION_T  == cell->client_data_t ){
	    wdCanvasWorld2Canvas( canvas, (double) 
				  (NETW_RL == orientation ?
				   (WORLD_W - CELL_W*col->x - CELL_W) : CELL_W*col->x + CELL_W ),
				  (double) (CELL_H*cell->y), &xmin, &ymin );
	    wdCanvasWorld2Canvas( canvas, (double)
				  (NETW_RL == orientation ?
				   (WORLD_W - CELL_W*col->x) : CELL_W*col->x ),
				  (double) (CELL_H*cell->y + CELL_H), &xmax, &ymax );
	    xv = (xmax + xmin)/2; yv = (ymax + ymin)/2;
	  }
	  else{
	    wdCanvasWorld2Canvas( canvas, (double)
				  (NETW_RL == orientation ?
				   (WORLD_W - CELL_W*col->x - CELL_W) : CELL_W*col->x + CELL_W ),
				  (double) (CELL_H*cell->y), &xmin, &ymin );
	    wdCanvasWorld2Canvas( canvas, (double)
				  (NETW_RL == orientation ?
				   (WORLD_W - CELL_W*col->x) : CELL_W*col->x ),
				  (double) (CELL_H*cell->y + CELL_H), &xmax, &ymax );
	    xw = (xmax - xmin)/4; yh = (ymax - ymin)/4;
	    xv = xmax - xw; yv = ymin + yh + yh;
	  }
	  //
	  for( i=0; i<cell->nright; i++ ){
	    if( _NETW_JUNCTION_T  == cell->right[i]->client_data_t ){
	      wdCanvasWorld2Canvas( canvas, (double)
				    (NETW_RL == orientation ?
				     (WORLD_W - CELL_W*cell->right[i]->head->x - CELL_W) :
				     CELL_W*cell->right[i]->head->x + CELL_W ),
				    (double) (CELL_H*cell->right[i]->y), &xmin, &ymin );
	      wdCanvasWorld2Canvas( canvas, (double)
				    (NETW_RL == orientation ?
				     (WORLD_W - CELL_W*cell->right[i]->head->x) :
				     CELL_W*cell->right[i]->head->x ),
				    (double) (CELL_H*cell->right[i]->y + CELL_H), &xmax, &ymax );
	      x0 = (xmax + xmin)/2; y0 = (ymax + ymin)/2;
	    }
	    else{
	      wdCanvasWorld2Canvas( canvas, (double)
				    (NETW_RL == orientation ?
				     (WORLD_W - CELL_W*cell->right[i]->head->x - CELL_W) :
				     CELL_W*cell->right[i]->head->x + CELL_W ),
				    (double) (CELL_H*cell->right[i]->y), &xmin, &ymin );
	      wdCanvasWorld2Canvas( canvas, (double)
				    (NETW_RL == orientation ?
				     (WORLD_W - CELL_W*cell->right[i]->head->x) :
				     CELL_W*cell->right[i]->head->x ),
				    (double) (CELL_H*cell->right[i]->y + CELL_H), &xmax, &ymax );
	      xw = (xmax - xmin)/4; yh = (ymax - ymin)/4;
	      x0 = xmin + xw; y0 = ymin + yh + yh;
	    }
	    cdCanvasLine( canvas, xv, yv, x0, y0 );
	  }
	}
	
	// Draw left links
	if( cell->nleft ){
	  wdCanvasWorld2Canvas( canvas, (double)
				  (NETW_RL == orientation ?
				   (WORLD_W - CELL_W*col->x - CELL_W) : CELL_W*col->x + CELL_W ),
				(double) (CELL_H*cell->y), &xmin, &ymin );
	  wdCanvasWorld2Canvas( canvas, (double)
				  (NETW_RL == orientation ?
				   (WORLD_W - CELL_W*col->x ) : CELL_W*col->x  ),
				(double) (CELL_H*cell->y + CELL_H), &xmax, &ymax );
	  xw = (xmax - xmin)/4; yh = (ymax - ymin)/4;
	  xv = xmin + xw; yv = ymin + yh + yh;
	  //
	  for( i=0; i<cell->nleft; i++ ){
	    wdCanvasWorld2Canvas( canvas, (double)
				  (NETW_RL == orientation ?
				   (WORLD_W - CELL_W*cell->left[i]->head->x - CELL_W) :
				   CELL_W*cell->left[i]->head->x + CELL_W ),
				  (double) (CELL_H*cell->left[i]->y), &xmin, &ymin );
	    wdCanvasWorld2Canvas( canvas, (double)
				  (NETW_RL == orientation ?
				   (WORLD_W - CELL_W*cell->left[i]->head->x) :
				   CELL_W*cell->left[i]->head->x ),
				  (double) (CELL_H*cell->left[i]->y + CELL_H), &xmax, &ymax );
	    xw = (xmax - xmin)/4; yh = (ymax - ymin)/4;
	    x0 = xmax - xw; y0 = ymin + yh + yh;
	    cdCanvasLine( canvas, xv, yv, x0, y0 );
	  }
	}
	cell = cell->next;
      }
      col = col->next;
    }
  }
}
