/**
 * netw_redraw.c -- Redrawing the Rule Network at scale 1
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

#include "nxpiup.h"

void netw__redraw_onscale( cdCanvas *canvas, int scale, double WORLD_W, double WORLD_H, unsigned short orientation ){
  col_rec_ptr  col;
  netw_cell_rec_ptr cell;
  int xv, yv, p = 0;
  int x0, y0, p0 = 0;
  char buf[_NETW_TEMP_BUFSIZE]={0};
  int font_height, line_style;
  long int line_color, text_color;
  short current = 0, i;

  cdCanvasGetFontDim( canvas, NULL, &font_height, NULL, NULL );
  col = (col_rec_ptr) cdCanvasGetAttribute( canvas, "USERDATA" );
  // Draw nodes. Iterate on cols then on cells.
  while( col ){
    cell = col->first;
    while( cell ){
      // What to write
      netw__text( canvas, cell, buf, &p );
      // Where to write it
      switch( cell->client_data_t ){
      case _NETW_SIGN_T:
      case _NETW_SIGN_YES_T:
      case _NETW_SIGN_NO_T:
      case _NETW_STR_T:
	// Right-aligned in LR
	wdCanvasWorld2Canvas( canvas, (double)
			      (NETW_RL == orientation ?
			       (WORLD_W - CELL_W*col->x) : CELL_W*col->x ),
			      (double) (CELL_H*cell->y), &xv, &yv);
	break;
      case _NETW_RULE_T:
      case _NETW_JUNCTION_T:
	// Centered
	wdCanvasWorld2Canvas( canvas, (double)
			      (NETW_RL == orientation ?
			       (WORLD_W - CELL_W*col->x - (CELL_W - p)/2) : CELL_W*col->x + (CELL_W - p)/2 ),
			       (double) (CELL_H*cell->y), &xv, &yv);
	break;
      }
      // Write it
      if( _NETW_JUNCTION_T != cell->client_data_t ){
	current =  ( _NETW_RULE_T == cell->client_data_t ) ?
	  nxpiup_inagendap( (sign_rec_ptr) (((rule_rec_ptr) cell->client_data)->setters) ) :
	  nxpiup_inagendap( (sign_rec_ptr) cell->client_data );
	cdCanvasFont( canvas, NULL,  current ? CD_BOLD : CD_PLAIN, 0 );
	if( !current &&
	    _KNOWN      == ((sign_rec_ptr) cell->client_data)->val.status ) {
	  switch( ((sign_rec_ptr) cell->client_data)->val.type ){
	  case _VAL_T_BOOL:
	    text_color = cdCanvasForeground( canvas,
					     _FALSE == ((sign_rec_ptr) cell->client_data)->val.val_bool ?
					     S_NETW_COLOR_FALSE :
					     S_NETW_COLOR_TRUE );
	    break;
	  case _VAL_T_FLOAT:
	  case _VAL_T_INT:
	  case _VAL_T_STR:
	    text_color = cdCanvasForeground( canvas, S_NETW_COLOR_KNOWN );
	    break;
	  }
	}

	cdCanvasText( canvas, xv, yv, buf );

	if( !current &&
	   _KNOWN      == ((sign_rec_ptr) cell->client_data)->val.status ){
	  text_color = cdCanvasForeground( canvas, text_color );
	}
	cdCanvasFont( canvas, NULL, CD_PLAIN, 0 );
	current = 0;
      }
      else{
	int font_size;
	char *param = IupGetAttribute( IupGetHandle( "netw_compound" ), "VALUE" );
	if( 0 == strcmp( "ON", param ) ){
	  cdCanvasGetFont( canvas, NULL, NULL, &font_size );
	  cdCanvasFont( canvas, NULL, -1, 8 );
	  cdCanvasText( canvas, xv, yv, buf );
	  cdCanvasFont( canvas, NULL, -1, font_size );
	}
      }

      // Draw right links
      if( cell->nright ){
	switch( cell->client_data_t ){
	case _NETW_JUNCTION_T:
	  wdCanvasWorld2Canvas( canvas, (double)
				(NETW_RL == orientation ?
				 (WORLD_W - CELL_W*col->x - CELL_W/2) : CELL_W*col->x + CELL_W/2 ),
				 (double) (CELL_H*cell->y + CELL_H/2), &xv, &yv );
	  break;
	default:
	  wdCanvasWorld2Canvas( canvas, (double)
				(NETW_RL == orientation ?
				 (WORLD_W - CELL_W*col->x) : CELL_W*col->x ),
				 (double) (CELL_H*cell->y + font_height/2), &xv, &yv );
	  break;
	}
	// Right-link Destinations
	for( i=0; i<cell->nright; i++ ){
	  switch( cell->right[i]->client_data_t ){
	  case _NETW_JUNCTION_T:
	    wdCanvasWorld2Canvas( canvas, (double)
				  (NETW_RL == orientation ?
				   (WORLD_W - CELL_W*cell->right[i]->head->x - CELL_W/2) :
				   CELL_W*cell->right[i]->head->x + CELL_W/2 ),
				   (double) (CELL_H*cell->right[i]->y + CELL_H/2), &x0, &y0 );
	    break;
	  default:
	    netw__text( canvas, cell->right[i], buf, &p0 );
	    wdCanvasWorld2Canvas( canvas, (double)
				  (NETW_RL == orientation ?
				   (WORLD_W - CELL_W*cell->right[i]->head->x - p0) :
				   CELL_W*cell->right[i]->head->x + p0 ),
				   (double) (CELL_H*cell->right[i]->y + font_height/2), &x0, &y0 );
	    break;
	  }
	  line_color = cdCanvasForeground( canvas,
					   cell->rmask[i] == _NETW_FWRD_LINKTYPE_RULE ?
					   _NETW_LINE_COLOR_FWRD : _NETW_LINE_COLOR_FWRDEVOKE );
	  line_style = cdCanvasLineStyle( canvas,
					  cell->rmask[i] == _NETW_FWRD_LINKTYPE_RULE ?
					  _NETW_LINE_STYLE_FWRD : _NETW_LINE_STYLE_FWRDEVOKE );
	  cdCanvasLine( canvas, xv, yv, x0, y0 );
	  line_style = cdCanvasLineStyle( canvas, line_style );
	  line_color = cdCanvasForeground( canvas, line_color );
	}
      }
  
      // Draw left links
      if( cell->nleft ){
	// Find origin point
	switch( cell->client_data_t ){
	case _NETW_SIGN_T:
	case _NETW_SIGN_YES_T:
	case _NETW_SIGN_NO_T:
	case _NETW_STR_T:
	  wdCanvasWorld2Canvas( canvas, (double)
				(NETW_RL == orientation ?
				 (WORLD_W - CELL_W*col->x - p) : CELL_W*col->x + p ),
				(double) (CELL_H*cell->y + font_height/2), &xv, &yv );
	  break;
	case _NETW_RULE_T:
	  wdCanvasWorld2Canvas( canvas, (double)
				(NETW_RL == orientation ?
				 (WORLD_W - CELL_W*col->x - (CELL_W + p)/2) :
				 CELL_W*col->x + (CELL_W + p)/2 ),
				(double) (CELL_H*cell->y + font_height/2), &xv, &yv );
	  break;
	}
	// Find all destination points
	for( i=0; i<cell->nleft; i++ ){
	  // Recomputes the text dims?
	  netw__text( canvas, cell->left[i], buf, &p0 );
	  switch( cell->left[i]->client_data_t ){
	  case _NETW_SIGN_T:
	  case _NETW_SIGN_YES_T:
	  case _NETW_SIGN_NO_T:
	  case _NETW_STR_T:
	    wdCanvasWorld2Canvas( canvas, (double)
				  (NETW_RL == orientation ?
				   (WORLD_W - CELL_W*cell->left[i]->head->x) :
				   CELL_W*cell->left[i]->head->x ),
				  (double) (CELL_H*cell->left[i]->y + font_height/2), &x0, &y0);
	    break;
	  case _NETW_RULE_T:
	  case _NETW_JUNCTION_T:
	    wdCanvasWorld2Canvas( canvas, (double)
				  (NETW_RL == orientation ?
				   (WORLD_W - CELL_W*cell->left[i]->head->x - (CELL_W - p0)/2 ) :
				   CELL_W*cell->left[i]->head->x + (CELL_W - p0)/2 ),
				  (double) (CELL_H*cell->left[i]->y + font_height/2), &x0, &y0);
	    break;
	  }
	  // Draw line
	  line_color = cdCanvasForeground( canvas, _NETW_LINE_COLOR_FWRD );
	  line_style = cdCanvasLineStyle( canvas, _NETW_LINE_STYLE_BWRD );
	  cdCanvasLine( canvas, xv, yv, x0, y0 );
	  line_style = cdCanvasLineStyle( canvas, line_style );
	  line_color = cdCanvasForeground( canvas, line_color );
	}
      }
      cell = cell->next;
    }
    col = col->next;
  }
}
