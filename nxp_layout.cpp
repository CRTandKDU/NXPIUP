/**
 * nxp_layout.cpp -- Another graph for NXP
 *
 * Written on Tuesday, May 19, 2026
 */

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <cstring>
#include <iup.h>
#include <iupcontrols.h>
#include <cd.h>
#include <cdiup.h>
#include <wd.h>

#include "agenda.h"
#include "nxpiup.h"
#include "layout.h"

struct layout_rec{
  void *g;
  void *names;
  void *positions;
  void *weights;
  void *selectedv;
};

typedef struct layout_rec layout_rec, *layout_rec_ptr;

#define WORLD_W 1000
#define WORLD_H 1000

static nxp_graph_cb_t S_graph_cb = NULL;

int need_redraw, redraw_count = 0;

void redrawv_cb( char *, double, double );
void redrawe_cb( char *, double, double, char *, double, double );
int redraw( Ihandle * );

// From canvas3
int nxp_graph_scale = 1;

void update_scrollbar(Ihandle* ih, int canvas_w, int canvas_h)
{
  /* update page size, it is always the client size of the canvas,
     but must convert it to world coordinates.
     If you change canvas size or nxp_graph_scale must call this function. */
  double ww, wh;
  if (nxp_graph_scale > 0)
  {
    ww = (double)canvas_w/nxp_graph_scale;
    wh = (double)canvas_h/nxp_graph_scale;
  }
  else
  {
    ww = canvas_w*abs(nxp_graph_scale);
    wh = canvas_h*abs(nxp_graph_scale);
  }
  IupSetfAttribute(ih, "DX", "%g", ww);
  IupSetfAttribute(ih, "DY", "%g", wh);
}

void update_viewport(Ihandle* ih, cdCanvas *canvas, float posx, float posy)
{
  int view_x, view_y, view_w, view_h;

  /* The CD viewport is the same area represented by the virtual space of the scrollbar,
     but not using the same coordinates. */

  /* posy is top-bottom, CD is bottom-top.
     invert posy reference (YMAX-DY - POSY) */
  posy = IupGetFloat(ih, "YMAX")-IupGetFloat(ih, "DY") - posy;
  if (posy < 0) posy = 0;

  if (nxp_graph_scale > 0)
  {
    view_w = WORLD_W*nxp_graph_scale;
    view_h = WORLD_H*nxp_graph_scale;
    view_x = (int)(posx*nxp_graph_scale);
    view_y = (int)(posy*nxp_graph_scale);
  }
  else
  {
    view_w = WORLD_W/abs(nxp_graph_scale);
    view_h = WORLD_H/abs(nxp_graph_scale);
    view_x = (int)(posx/abs(nxp_graph_scale));
    view_y = (int)(posy/abs(nxp_graph_scale));
  }

  wdCanvasViewport(canvas, -view_x, view_w-1 - view_x, -view_y, view_h-1 - view_y);
}

// IUP dialog and its callbacks

int layout_resize_cb(Ihandle *ih, int canvas_w, int canvas_h)
{
  cdCanvas *canvas = (cdCanvas*)IupGetAttribute(ih, "_CD_CANVAS");

  printf("LAYOUT_RESIZE_CB(%d, %d) RASTERSIZE=%s DRAWSIZE=%s \n", canvas_w, canvas_h, IupGetAttribute(ih, "RASTERSIZE"), IupGetAttribute(ih, "DRAWSIZE"));
  /* When *AUTOHIDE=Yes, this can hide a scrollbar and so change the canvas drawsize */
  update_scrollbar(ih, canvas_w, canvas_h);  
  /* printf("                                DRAWSIZE=%s \n", IupGetAttribute(ih, "DRAWSIZE")); */
  /* update the canvas size */
  IupGetIntInt(ih, "DRAWSIZE", &canvas_w, &canvas_h);
  update_scrollbar(ih, canvas_w, canvas_h);

  /* update the application */
  cdCanvasActivate(canvas);
  update_viewport(ih, canvas, IupGetFloat(ih, "POSX"), IupGetFloat(ih, "POSY"));

  return IUP_DEFAULT;
}

int layout_scroll_cb(Ihandle *ih, int op, float posx, float posy)
{
  cdCanvas *canvas = (cdCanvas*)IupGetAttribute(ih, "_CD_CANVAS");
  /* printf("LAYOUT_SCROLL_CB(%g, %g)\n", posx, posy); */
  cdCanvasActivate(canvas);
  update_viewport(ih, canvas, posx, posy);
  IupRedraw(ih, 0);
  (void)op;
  return IUP_DEFAULT;
}


void update_redraw_cb( char *node, double x, double y ){
  printf( "\tUPDATE %s\t%f\t%f\n", node, x, y );
}

void layout_update_cb (int iter, int delay ){
  Ihandle *ih =  IupGetHandle( "layout_cv" );
  cdCanvas *canvas = (cdCanvas *) IupGetAttribute( ih, "_CD_CANVAS" );
  layout_rec_ptr userdata = (layout_rec_ptr) IupGetAttribute( ih, "USERDATA" );
  if( userdata ){
    usleep( delay );
    cdCanvasActivate(canvas);
    cdCanvasClear(canvas);
    layout_enumerate_edges( (userdata->g), (userdata->names), (userdata->positions), redrawe_cb );
    layout_enumerate_vertices( (userdata->g), (userdata->names), (userdata->positions), redrawv_cb );
    IupUpdate( ih );
    /* printf( "%d ", iter ); */
  }
  int ret = IupLoopStep();
}

void nxpiup_layout_add_edge( char *s, char *t, int index, double weight ){
  Ihandle *ih			=  IupGetHandle( "layout_cv" );
  cdCanvas *canvas		= (cdCanvas *) IupGetAttribute( ih, "_CD_CANVAS" );
  layout_rec_ptr userdata	= (layout_rec_ptr) IupGetAttribute( ih, "USERDATA" );
  if( userdata ){
    layout_add_edge( (userdata->g), (userdata->names), (userdata->weights), s, t, index, weight );
  }
}

// Set up
int toggle_redraw(void)
{
  Ihandle *ih =  IupGetHandle( "layout_cv" );
  cdCanvas *canvas = (cdCanvas *) IupGetAttribute( ih, "_CD_CANVAS" );
  layout_rec_ptr data = (layout_rec_ptr) IupGetAttribute( ih, "USERDATA" );
  //
  Ihandle *ihalg = IupGetHandle( "layout_alg" );
  
  if( data ){
    layout_close( (data->g), (data->names), (data->positions), (data->weights) );
    free( (void *) data );
  }
  //
  // int canvas_w, canvas_h;
  // IupGetIntInt( ih, "DRAWSIZE", &canvas_w, &canvas_h );
  // printf( "DRAWSIZE %d by %d\n", canvas_w, canvas_h );
  //
  nxp_graph_cb_t f = (nxp_graph_cb_t) IupGetAttribute( IupGetHandle( "graph_bt" ), "USERDATA" );
  layout_rec_ptr userdata = (layout_rec_ptr) malloc( sizeof( layout_rec ) );
  IupSetAttribute( ih, "USERDATA", (char *) userdata );

  userdata->selectedv = NULL;
  layout_open( &(userdata->g), &(userdata->names), &(userdata->weights) );

  S_graph_cb();
  // layout_add_edge( (userdata->g), (userdata->names), (userdata->weights), (char *) "0", (char *) "1", 0, 2.0 );
  // layout_add_edge( (userdata->g), (userdata->names), (userdata->weights), (char *) "0", (char *) "2", 1, 2.0 );
  // layout_add_edge( (userdata->g), (userdata->names), (userdata->weights), (char *) "0", (char *) "3", 2, 2.0 );
  // layout_add_edge( (userdata->g), (userdata->names), (userdata->weights), (char *) "1", (char *) "4", 3, 1.0 );
  // layout_add_edge( (userdata->g), (userdata->names), (userdata->weights), (char *) "1", (char *) "5", 4, 1.0 );
  // layout_add_edge( (userdata->g), (userdata->names), (userdata->weights), (char *) "2", (char *) "6", 5, 1.0 );
  // layout_add_edge( (userdata->g), (userdata->names), (userdata->weights), (char *) "3", (char *) "7", 6, 1.0 );
  // layout_add_edge( (userdata->g), (userdata->names), (userdata->weights), (char *) "3", (char *) "8", 7, 1.0 );
  // layout_add_edge( (userdata->g), (userdata->names), (userdata->weights), (char *) "3", (char *) "9", 8, 1.0 );
  // layout_add_edge( (userdata->g), (userdata->names), (userdata->weights), (char *) "2", (char *) "9", 9, 1.0 );

  if( 0 == strcmp( "bykk", (char *) IupGetAttribute( ihalg, "VALUE" ) ) )
    layout_run_kk( (userdata->g), &(userdata->positions), (userdata->weights),
		   (double) WORLD_W, (double) WORLD_H, layout_update_cb );
  if( 0 == strcmp( "byfr", (char *) IupGetAttribute( ihalg, "VALUE" ) ) )
    layout_run_fr( (userdata->g), &(userdata->positions), WORLD_H/10., (double) WORLD_W, (double) WORLD_H, layout_update_cb );

  IupUpdate( ih );
  //
  return IUP_DEFAULT;
}

// Redrawing vertices
void redrawv_cb( char * node, double x, double y ){
  int xx, yy;
  int canvas_w, canvas_h;
  Ihandle *ih			= IupGetHandle( "layout_cv" );
  cdCanvas *canvas		= (cdCanvas *) IupGetAttribute( ih, "_CD_CANVAS" );
  layout_rec_ptr userdata	= (layout_rec_ptr) IupGetAttribute( ih, "USERDATA" );
  long int color, ori_color;
  int line_style	= 1,
    style		= 0,
    selected_p		= 0;
  sign_rec_ptr sign		= sign_find( node, loadkb_get_allhypos() );
  if( !sign ){
    // The vertex might be a (non-hypo) sign
    sign = sign_find( node, loadkb_get_allsigns() );
  }
  // printf( "Redraw v %s, x=%f, y=%f\n", node, x, y );
  //
  IupGetIntInt( ih, "DRAWSIZE", &canvas_w, &canvas_h );
  wdCanvasWorld2Canvas( canvas, x + WORLD_W/2, y + WORLD_H/2, &xx, &yy );
  // Gray interior if terminal hypo
  if( sign && (0 == sign->nsetters) ){
    style	= cdCanvasInteriorStyle( canvas, CD_SOLID );
    color	= cdCanvasForeground(canvas, CD_GRAY);
    cdCanvasBox( canvas, 
		  (int) xx - _MARK_SIZE, (int) xx + _MARK_SIZE,
		  (int) yy - _MARK_SIZE, (int) yy + _MARK_SIZE );
    color = cdCanvasForeground(canvas, color);
    style = cdCanvasInteriorStyle( canvas, style );
  }

  selected_p = (userdata->selectedv &&
		(( node == (char *) userdata->selectedv ) ||
		 layout_adjacent_p( (userdata->g), (userdata->names), (userdata->positions),
				    node, (char *) userdata->selectedv )));

  // Value-dependent exterior color
  if( sign && _KNOWN == sign->val.status ){
    switch( sign->val.type ){
    case _VAL_T_BOOL:
      color = cdCanvasForeground( canvas, _FALSE == sign->val.val_bool ?
				  (selected_p ? CD_RED : (userdata->selectedv ? 0xFF8080L : CD_RED)) :
				  (selected_p ? CD_GREEN : (userdata->selectedv ? 0x80FF80L : CD_GREEN) ) );
      break;
    default:
      color = cdCanvasForeground( canvas,
				  (selected_p ? CD_BLUE : (userdata->selectedv ? 0x8080FFL : CD_BLUE) ) ); 
      break;
    }
  }
  else{
    color = cdCanvasForeground( canvas,
				(selected_p ? CD_BLACK : (userdata->selectedv ? CD_GRAY : CD_BLACK) ) ); 
  }
  
  if( selected_p  ){
    line_style = cdCanvasLineWidth( canvas,  LAYOUT_SELECTED_LINESTYLE );
    cdCanvasFont( canvas, NULL, CD_BOLD, 0);
  }

  cdCanvasRect( canvas,
		(int) xx - _MARK_SIZE, (int) xx + _MARK_SIZE,
		(int) yy - _MARK_SIZE, (int) yy + _MARK_SIZE );
  if( 1<=nxp_graph_scale ){
    cdCanvasText( canvas, (int) xx + _MARK_SIZE + 2, (int) yy, node );
  }

  if( selected_p ){
    line_style = cdCanvasLineWidth( canvas,  line_style );
    cdCanvasFont( canvas, NULL, CD_PLAIN, 0);
  }

  color = cdCanvasForeground( canvas, color );
}

// Redrawing edges
void redrawe_cb( char *source, double xs, double ys,
		 char *target, double xt, double yt ){
  int xxs, yys, xxt, yyt;
  int canvas_w, canvas_h;
  int line_style;
  long int color;
  int selected_p		= 0;
  Ihandle *ih			=  IupGetHandle( "layout_cv" );
  cdCanvas *canvas		= (cdCanvas *) IupGetAttribute( ih, "_CD_CANVAS" );
  layout_rec_ptr userdata	= (layout_rec_ptr) IupGetAttribute( ih, "USERDATA" );
  /* printf( "Point %s, x=%f, y=%f\n", node, x, y ); */
  //
  IupGetIntInt( ih, "DRAWSIZE", &canvas_w, &canvas_h );
  wdCanvasWorld2Canvas( canvas, xs + WORLD_W/2, ys + WORLD_H/2, &xxs, &yys );
  wdCanvasWorld2Canvas( canvas, xt + WORLD_W/2, yt + WORLD_H/2, &xxt, &yyt );
  selected_p = (userdata->selectedv &&
		(( source == (char *) userdata->selectedv ) ||
		 ( target == (char *) userdata->selectedv )));
  if( selected_p  ){
    line_style = cdCanvasLineWidth( canvas,  LAYOUT_SELECTED_LINESTYLE );
  }
  if( userdata->selectedv && !selected_p ){
    color = cdCanvasForeground( canvas, CD_GRAY ); 
  }

  cdCanvasLine( canvas, xxs, yys, xxt, yyt );

  if( selected_p  ){
    line_style = cdCanvasLineWidth( canvas,  line_style );
  }
  if( userdata->selectedv && !selected_p ){
    color = cdCanvasForeground( canvas, color ); 
  }
}


int layout_destroy_cb( Ihandle *ih ){
  cdCanvas *cv = (cdCanvas *) IupGetAttribute( ih, "_CD_CANVAS" );
  layout_rec_ptr userdata = (layout_rec_ptr) IupGetAttribute( (Ihandle *) cv, "USERDATA" );
  if( userdata ){
    printf( "Clearing graph\n" );
    layout_close( (userdata->g), (userdata->names), (userdata->positions), (userdata->weights) );
    free( (void *) userdata );
  }
  return IUP_DEFAULT;
}

int redraw( Ihandle *ih )
{
  layout_rec_ptr userdata = (layout_rec_ptr) IupGetAttribute( ih, "USERDATA" );
  if( userdata ){
    cdCanvas *canvas = (cdCanvas *) IupGetAttribute( ih, "_CD_CANVAS" );
    cdCanvasActivate( canvas );
    cdCanvasClear( canvas );
    layout_enumerate_edges( (userdata->g), (userdata->names), (userdata->positions), redrawe_cb );
    layout_enumerate_vertices( (userdata->g), (userdata->names), (userdata->positions), redrawv_cb );
  }
  return IUP_DEFAULT;
}

void print_cb( char * node, double x, double y ){
  printf( "Point %s, x=%f, y=%f\n", node, x, y );
}

int layout_wheel_cb(Ihandle *ih,float delta,int x,int y,char* status)
{
  int canvas_w, canvas_h;
  cdCanvas *canvas = (cdCanvas*)IupGetAttribute(ih, "_CD_CANVAS");
  (void)x;
  (void)y;
  (void)status;

  if (nxp_graph_scale+delta==0) /* skip 0 */
  {
    if (nxp_graph_scale > 0) 
      nxp_graph_scale = -1;
    else 
      nxp_graph_scale = 1;
  }
  else
    nxp_graph_scale += (int)delta;

  cdCanvasActivate(canvas);
  cdCanvasGetSize(canvas, &canvas_w, &canvas_h, NULL, NULL);
  update_scrollbar(ih, canvas_w, canvas_h);
  update_viewport(ih, canvas, IupGetFloat(ih, "POSX"), IupGetFloat(ih, "POSY"));
  IupRedraw(ih, 0);
  return IUP_DEFAULT;
}

int layout_button_cb(Ihandle* ih, int button, int pressed, int x, int y, char* status){
  cdCanvas *canvas = (cdCanvas*)IupGetAttribute(ih, "_CD_CANVAS");
  layout_rec_ptr userdata = (layout_rec_ptr) IupGetAttribute( (Ihandle *) ih, "USERDATA" );
  void *need_redraw = NULL;
  double xx, yy;
  if( userdata ){
    switch( button ){
    case IUP_BUTTON1:
      if( pressed ){
	cdCanvasUpdateYAxis( canvas, &y );
	// xx = x; yy = y;
	wdCanvasCanvas2World( canvas, x, y, &xx, &yy );
	xx -= WORLD_W/2.; yy -= WORLD_H/2. ;
	printf( "MAP Click at x=%f, y=%f\n", xx, yy );
	if( need_redraw = layout_invertex_p( (userdata->g), (userdata->names), (userdata->positions),
					     xx, yy ) ){
	  userdata->selectedv = ( need_redraw == userdata->selectedv ) ? NULL : need_redraw;
	  // printf( "Found Vertex=%s\n", (char *) need_redraw );
	  IupUpdate( ih );
	}
      }
      break;
    default:
      break;
    }
  }
  return IUP_DEFAULT;
}

int bykk_cb( Ihandle *ih, int state ){
  // if( 1 == state ){
  // }
  return IUP_DEFAULT;
}

int byfr_cb( Ihandle *ih, int state ){
  // if( 1 == state ){
  // }
  return IUP_DEFAULT;
}

Ihandle * nxpiup_layout_dlg( const char *size_str, nxp_graph_cb_t f ){
  Ihandle *cv    = IupCanvas(NULL);
  Ihandle *bt    = IupButton("Start/Stop", NULL);

  Ihandle *bykk = IupToggle( "K.-K.", NULL );
  Ihandle *byfr = IupToggle( "F.-R.", NULL );
  IupSetHandle( "bykk", bykk );
  IupSetAttribute( bykk, "EXPAND", "HORIZONTAL" );
  IupSetCallback( bykk,  "ACTION", (Icallback) bykk_cb  );
  
  IupSetHandle( "byfr", byfr );
  IupSetAttribute( byfr, "EXPAND", "HORIZONTAL" );
  IupSetCallback( byfr,  "ACTION", (Icallback) byfr_cb  );

  Ihandle *ihhbox = IupHbox( bt, bykk, byfr, NULL );
  IupSetAttribute( ihhbox, "GAP", "15" );
  IupSetAttribute( ihhbox, "EXPAND", "HORIZONTAL" );
  Ihandle *ihframe = IupFrame( ihhbox );
  IupSetAttribute( ihframe, "EXPAND", "HORIZONTAL" );
  IupSetAttribute( ihframe, "MARGIN", "5x5" );
  IupSetAttribute( ihframe, "TITLE", "Layout" );
  
  Ihandle *ihalg = IupRadio( ihframe );
  IupSetAttribute( ihalg, "EXPAND", "HORIZONTAL" );
  IupSetAttribute( ihalg, "VALUE", "bykk" );
  IupSetHandle( "layout_alg", ihalg );

  /* IupSetAttribute(gauge, "SIZE", "200x15"); */
  
  IupSetAttribute( cv, "SIZE", size_str );
  IupSetAttribute( cv, "SCROLLBAR", "YES" );
  IupSetCallback( cv, "RESIZE_CB",	(Icallback) layout_resize_cb);
  IupSetCallback( cv, "SCROLL_CB",	(Icallback) layout_scroll_cb);
  IupSetCallback( cv, "WHEEL_CB",	(Icallback) layout_wheel_cb);
  IupSetCallback( cv, "BUTTON_CB",	(Icallback) layout_button_cb);
  IupSetCallback( cv, "DESTROY_CB",	(Icallback) layout_destroy_cb);
  IupSetCallback( cv, "ACTION",		(Icallback) redraw );
  IupSetHandle( "layout_cv", cv );
  
  Ihandle *dlg   = IupDialog( IupVbox( cv, ihalg, NULL ) );
  IupSetAttribute(dlg, "TITLE", "Redraw test");
  IupSetHandle( "graph", dlg );
  IupMap(dlg);
  
  cdCanvas *cdcanvas = cdCreateCanvas(CD_IUP, cv);
  wdCanvasWindow(cdcanvas, 0, WORLD_W, 0, WORLD_H);
  cdCanvasFont( cdcanvas, "Times", CD_PLAIN, 10);
  cdCanvasClear(cdcanvas);
  /* World size is fixed */

  /* handle scrollbar in world coordinates, so we only have to update DX/DY */
  IupSetAttribute(cv, "XMIN", "0");
  IupSetAttribute(cv, "YMIN", "0");
  IupSetfAttribute(cv, "XMAX", "%d", WORLD_W);
  IupSetfAttribute(cv, "YMAX", "%d", WORLD_H);
  
  IupSetHandle( "action_bt", bt );
  S_graph_cb = f;
  IupSetCallback(bt, "ACTION", (Icallback)toggle_redraw);
  //
  /* layout_enumerate_vertices( (userdata->g), (userdata->names), (userdata->positions), print_cb ); */
  //
  return dlg;
}

// int main(int argc, char **argv) 
// {
//   IupOpen(&argc, &argv);

//   IupMainLoop();
//   //
//   //
//   IupClose();

//   return EXIT_SUCCESS;
// }
