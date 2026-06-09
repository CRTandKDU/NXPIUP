/**
 * netw_internals.c -- Ancillaries and utilities for the Rule Network
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

void netw__trace( col_rec_ptr col ){
  col_rec_ptr c;
  netw_cell_rec_ptr cell;
  for( c = col; c; c=c->next ){
    cell = c->first;
    while( cell ){
      printf( "Col %d: Cell %d %d %s\n", c->x, cell->head->x, cell->y,
	      cell->client_data_t  == _NETW_JUNCTION_T ?
	      "JUNCTION" :
	      ((sign_rec_ptr) cell->client_data)->str );
      cell = cell->next;
    }
  }
}

int netw__trim( const char * str, cdCanvas *canvas ){
  int n, w;
  char buf[_NETW_TEMP_BUFSIZE];
  short i;
  cdCanvasGetTextSize( canvas, str, &w, NULL );
  if( w<CELL_W ) return 0;
  // The hard way
  for( i=0; i<strlen( str ); i++ ){
    buf[i] = str[i]; buf[i+1] = 0x00;
    cdCanvasGetTextSize( canvas, buf, &w, NULL );
    if( w>CELL_W ) break;
  }
  /* printf( "Trim: %s %d %d - %d %d\n", str, strlen(str), maxw, i, w ); */
  return i;
}

void netw__col_append_cell( col_rec_ptr col, netw_cell_rec_ptr cell ){
  netw_cell_rec_ptr c = col->first;
  if( c ){
    while( c->next ) c = c->next;
    c->next = cell;
  }
  else{
    col->first = cell;
  }
  cell->next = NULL;
}

void netw__col_unlink_cell( col_rec_ptr col, netw_cell_rec_ptr cell ){
  netw_cell_rec_ptr c = col->first;
  /* printf( "\t\tUNLINK (x) col=%d, (y) cell=%d\n", col->x, cell->y ); */
  if( c ){
    if( cell == c ){
      col->first = cell->next;
      /* printf( "\t\tUnlinked first\n" ); */
    }
    else{
      while( c->next ){
	if( cell == c->next ){
	  c->next = cell->next;
	  /* printf( "\t\tUnlinked at %d\n", c->y ); */
	  break;
	}
	c = c->next;
      }
    }
  }
  /* printf( "\t\tDone\n" ); */
}

netw_cell_rec_ptr  netw__cell_parent( col_rec_ptr head, netw_cell_rec_ptr cell ){
  int idcol		= cell->head->x;
  col_rec_ptr col	= head;
  netw_cell_rec_ptr c	= NULL;
  short i;
  // Find previous column
  while( col ){
    if( (idcol - 1) == col->x ){
      c = col->first;
      // Find cell with a left link to input cell
      /* printf( "\tParent of %d cell (%ld) in col=%d\n", idcol, (long int) cell, col->x ); */
      while( c ){
	for( i=0; i<c->nleft; i++ ){
	  /* printf( "\t\t%ld\n", (long int) c->left[i] ); */
	  if( cell == (c->left[i]) ) return c;
	}
	c = c->next;
      }
      return NULL;
    }
    col = col->next;
  }
  return NULL;
}

int netw__col_ymax_cell( col_rec_ptr head, col_rec_ptr col, netw_cell_rec_ptr *parent ){
  netw_cell_rec_ptr c = col->first;
  int ymax = 0;
  //
  while( c ){
    if( ymax < c->y ){
      *parent = netw__cell_parent( head, c );
      ymax = c->y;
    }
    c = c->next;
  }
  return ymax;
}

int netw__col_ymin_cell( col_rec_ptr head, col_rec_ptr col, netw_cell_rec_ptr *parent ){
  netw_cell_rec_ptr c = col->first;
  int ymin = _NETW_INFINITY;
  //
  while( c ){
    if( ymin > c->y ){
      *parent = netw__cell_parent( head, c );
      ymin = c->y;
    }
    c = c->next;
  }
  return ymin;
}

col_rec_ptr netw__col_get_create( cdCanvas *canvas, int x ){
  col_rec_ptr  tophead = (col_rec_ptr) cdCanvasGetAttribute( canvas, "USERDATA" );
  // ASSERT: Cols are in increasing x-order from head
  printf( "COL_GET_CREATE %d %d >\n", tophead->x, x );
  /* netw__trace(tophead); */
  col_rec_ptr newc, c = tophead;
  if( c ){
    while( c ){
      if( x == c->x ) return c;
      c = c->next;
    }
    // Not found, create
    newc = _NEW_COL;
    newc->x	= x;
    newc->first	= NULL;
    // Insert in ordered sequence
    c = tophead;
    if( x < c->x ){
      newc->next = c;
      cdCanvasSetAttribute( canvas, "USERDATA", (char *) newc);
    }
    else{
      while( c->next && c->next->x < x ) c = c->next;
      printf( "Inserting new c after %d before %d\n", c->x, c->next ? c->next->x : -1 ); 
      newc->next = c->next;
      c->next = newc;
    }
  }
  else{
    newc = _NEW_COL;
    newc->x	= x;
    newc->first	= NULL;  
    newc->next	= NULL;  
  }
  //
  tophead = (col_rec_ptr) cdCanvasGetAttribute( canvas, "USERDATA" );
  /* netw__trace(tophead); */
  printf( "> COL_GET_CREATE %d %d\n", tophead->x, newc->x );
  return newc;
}

void netw__text( cdCanvas *canvas, netw_cell_rec_ptr cell, char *buf, int *bufsize ){
  sign_rec_ptr s;
  short i;
  int p;
  char *param = IupGetAttribute( IupGetHandle( "netw_compound" ), "VALUE" );
  int  compound_p = (0 == strcmp( "ON", param ));
  //
  switch( cell->client_data_t ){
  case _NETW_JUNCTION_T:
    if( compound_p ){
      s = (sign_rec_ptr) cell->client_data;
      sprintf( buf, "%s", s->str );
    }
    else{
      *bufsize = 0;
      buf[0] = 0x00;
      return;
    }
    break;
  case _NETW_RULE_T:
    s = (sign_rec_ptr) cell->client_data;
    sprintf( buf, "%s", s->str );
    break;
  case _NETW_SIGN_T:
  case _NETW_SIGN_YES_T:
  case _NETW_SIGN_NO_T:
    s = (sign_rec_ptr) cell->client_data;
    if( COMPOUND_MASK == (s->len_type & TYPE_MASK) ){
      if( compound_p ){
	sprintf( buf, "%s", s->str );
      }
      else{
	char *c = ((compound_rec_ptr) s)->dsl_expression;
	for( i=0; i<_NETW_TEMP_BUFSIZE; i++ ){
	  buf[i]=c[i];
	  if( 0 == c[i] || '\n' == c[i] ){
	    buf[i] = 0x00;
	    break;
	  }
	}
      }
    }
    else{
      if( _NETW_SIGN_T == cell->client_data_t )
	sprintf( buf, "(%d) %s (%d)", s->ngetters, s->str, s->nsetters );
      else{
	sprintf( buf, "%s %s",
		_NETW_SIGN_YES_T == cell->client_data_t ? "YES" : "NO", s->str );
      }
    }
    break;
  case _NETW_STR_T:
    char *c = (char *) cell->client_data;
    buf[0] = '='; buf[1] = '>'; buf[2] = ' ';
    for( i=0; i<_NETW_TEMP_BUFSIZE-3; i++ ){
      buf[i + 3]=c[i];
      if( 0 == c[i] || '\n' == c[i] ){
	buf[i + 3] = 0x00;
	break;
      }
    }
    break;
  }
  if( p = netw__trim( buf, canvas ) ) buf[p] = 0x00;
  cdCanvasGetTextSize( canvas, buf, bufsize, NULL );
}

char * netw__client_text( netw_cell_rec_ptr cell ){
  char *ret = NULL;
  switch( cell->client_data_t ){
  case _NETW_SIGN_T:
  case _NETW_SIGN_YES_T:
  case _NETW_SIGN_NO_T:
  case _NETW_RULE_T:
    ret = ((sign_rec_ptr) cell->client_data)->str;
    break;
  case _NETW_STR_T:
    ret = (char *) cell->client_data;
    break;
  }
  return ret;
}

void netw__adjust_col_vert( netw_cell_rec_ptr cell, int inc ){
  netw_cell_rec_ptr c = cell;
  while( c ){ c->y += inc; c = c->next; }
}

//

void netw_adjust_horz( col_rec_ptr col, int inc ){
  col_rec_ptr c = col;
  while( c ){ c->x += inc; c = c->next; }
}

void netw_get_cell_size( int *w, int *h ){
  *w = CELL_W; *h = CELL_H;
}

void netw_colors_default(){
  S_NETW_COLOR_TRUE =	CD_GREEN;
  S_NETW_COLOR_FALSE =	CD_RED;
  S_NETW_COLOR_UNKNOWN =	CD_BLACK;
  // No distinction on type of DSL variable: INT, STR, or FLOAT (NIY).
  S_NETW_COLOR_KNOWN =	CD_BLUE;
}
