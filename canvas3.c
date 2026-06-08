#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <cstring>
#include <iup.h>
#include <iupim.h>
#include <iupcontrols.h>
#include <cd.h>
#include <cdiup.h>
#include <wd.h>

#include "agenda.h"
#include "netw.h"
#include "nxpiup.h"
#include "nxp_hash.h"

static Ihandle *S_Timer  = NULL;
static Ihandle *S_splash = NULL;

/* World:
   The canvas will be a window into that space.
   If canvas is smaller than the virtual space, scrollbars are active.
   The drawing is a red X connecting the corners of the world,
   plus a box inside the borders.

   Remember that:
   XMIN<=POSX<=XMAX-DX
*/

static int scale = 1;

static int DEFAULT_W = 600 - 19;
static int DEFAULT_H = 400 - 19;

//----------------------------------------------------------------------
// IUP GUI logic
//----------------------------------------------------------------------

static void update_scrollbar(Ihandle* ih, int canvas_w, int canvas_h)
{
  /* update page size, it is always the client size of the canvas,
     but must convert it to world coordinates.
     If you change canvas size or scale must call this function. */
  double ww, wh;
  if (scale > 0)
  {
    ww = (double)canvas_w/scale;
    wh = (double)canvas_h/scale;
  }
  else
  {
    ww = canvas_w*abs(scale);
    wh = canvas_h*abs(scale);
  }
  IupSetfAttribute(ih, "DX", "%g", ww);
  IupSetfAttribute(ih, "DY", "%g", wh);
}

static void update_viewport(Ihandle* ih, cdCanvas *canvas, float posx, float posy)
{
  int view_x, view_y, view_w, view_h;

  /* The CD viewport is the same area represented by the virtual space of the scrollbar,
     but not using the same coordinates. */

  /* posy is top-bottom, CD is bottom-top.
     invert posy reference (YMAX-DY - POSY) */
  posy = IupGetFloat(ih, "YMAX")-IupGetFloat(ih, "DY") - posy;
  if (posy < 0) posy = 0;

  if (scale > 0)
  {
    view_w = WORLD_W*scale;
    view_h = WORLD_H*scale;
    view_x = (int)(posx*scale);
    view_y = (int)(posy*scale);
  }
  else
  {
    view_w = WORLD_W/abs(scale);
    view_h = WORLD_H/abs(scale);
    view_x = (int)(posx/abs(scale));
    view_y = (int)(posy/abs(scale));
  }

  wdCanvasViewport(canvas, -view_x, view_w-1 - view_x, -view_y, view_h-1 - view_y);
}

/************************************************************************************/

static int action(Ihandle *ih)
{
  cdCanvas *canvas = (cdCanvas*)IupGetAttribute(ih, "_CD_CANVAS");
  Ihandle *ihradio = IupGetHandle( "netw_radio" );
  unsigned short orientation = NETW_RL;
  /* printf("ACTION\n"); */
  cdCanvasActivate(canvas);
  cdCanvasClear(canvas);
  if( ih ){
    orientation = (0 == strcmp( IupGetAttribute( ihradio, "VALUE" ), "netw_rl" ) ) ? NETW_RL : NETW_LR;
  }
  netw_redrawkb( canvas, scale, WORLD_W, WORLD_H, orientation );

  /* cdCanvasForeground(canvas, CD_RED); */
  /* wdCanvasLine(canvas, 0, 0, WORLD_W, WORLD_H); */
  /* wdCanvasLine(canvas, 0, WORLD_H, WORLD_W, 0); */
  /* wdCanvasArc(canvas, WORLD_W/2, WORLD_H/2+WORLD_H/10, WORLD_W/10, WORLD_H/10, 0, 360); */

  /* wdCanvasLine(canvas, 0, 0, WORLD_W, 0); */
  /* wdCanvasLine(canvas, 0, WORLD_H, WORLD_W, WORLD_H); */
  /* wdCanvasLine(canvas, 0, 0, 0, WORLD_H); */
  /* wdCanvasLine(canvas, WORLD_W, 0, WORLD_W, WORLD_H); */

  return IUP_DEFAULT;
}

static int resize_cb(Ihandle *ih, int canvas_w, int canvas_h)
{
  cdCanvas *canvas = (cdCanvas*)IupGetAttribute(ih, "_CD_CANVAS");

  printf("RESIZE_CB(%d, %d) RASTERSIZE=%s DRAWSIZE=%s \n", canvas_w, canvas_h, IupGetAttribute(ih, "RASTERSIZE"), IupGetAttribute(ih, "DRAWSIZE"));
  /* When *AUTOHIDE=Yes, this can hide a scrollbar and so change the canvas drawsize */
  update_scrollbar(ih, canvas_w, canvas_h);  
  /* printf("                                DRAWSIZE=%s \n", IupGetAttribute(ih, "DRAWSIZE")); */
  /* update the canvas size */
  IupGetIntInt(ih, "DRAWSIZE", &canvas_w, &canvas_h);
  update_scrollbar(ih, canvas_w, canvas_h);

  /* Reposition network display vertically */
  if( canvas_h - DEFAULT_H >= 2*20 ){
    DEFAULT_H += 2*20;
    netw_adjust_vert( canvas, 1 );
  }
  else if( canvas_h - DEFAULT_H <= -20*2 ){
    DEFAULT_H -= 2*20;
    netw_adjust_vert( canvas, -1 );
  }


  /* update the application */
  cdCanvasActivate(canvas);
  update_viewport(ih, canvas, IupGetFloat(ih, "POSX"), IupGetFloat(ih, "POSY"));

  return IUP_DEFAULT;
}

static int scroll_cb(Ihandle *ih, int op, float posx, float posy)
{
  cdCanvas *canvas = (cdCanvas*)IupGetAttribute(ih, "_CD_CANVAS");
  /* printf("SCROLL_CB(%g, %g)\n", posx, posy); */
  cdCanvasActivate(canvas);
  update_viewport(ih, canvas, posx, posy);
  IupRedraw(ih, 0);
  (void)op;
  return IUP_DEFAULT;
}

static int wheel_cb(Ihandle *ih,float delta,int x,int y,char* status)
{
  int canvas_w, canvas_h;
  cdCanvas *canvas = (cdCanvas*)IupGetAttribute(ih, "_CD_CANVAS");
  (void)x;
  (void)y;
  (void)status;

  if (scale+delta==0) /* skip 0 */
  {
    if (scale > 0) 
      scale = -1;
    else 
      scale = 1;
  }
  else
    scale += (int)delta;

  cdCanvasActivate(canvas);
  cdCanvasGetSize(canvas, &canvas_w, &canvas_h, NULL, NULL);
  update_scrollbar(ih, canvas_w, canvas_h);
  update_viewport(ih, canvas, IupGetFloat(ih, "POSX"), IupGetFloat(ih, "POSY"));
  IupRedraw(ih, 0);
  return IUP_DEFAULT;
}

static int map_cb(Ihandle *ih)
{
  /* canvas will be automatically saved in "_CD_CANVAS" attribute */
  cdCanvas *canvas = cdCreateCanvas(CD_IUP, ih);

  /* World size is fixed */
  wdCanvasWindow(canvas, 0, WORLD_W, 0, WORLD_H);

  /* handle scrollbar in world coordinates, so we only have to update DX/DY */
  IupSetAttribute(ih, "XMIN", "0");
  IupSetAttribute(ih, "YMIN", "0");
  IupSetfAttribute(ih, "XMAX", "%d", WORLD_W);
  IupSetfAttribute(ih, "YMAX", "%d", WORLD_H);

  netw_initfill_all( canvas, WORLD_W, WORLD_H, NETW_RL );

  return IUP_DEFAULT;
}

static int unmap_cb(Ihandle *ih)
{
  cdCanvas *canvas = (cdCanvas*)IupGetAttribute(ih, "_CD_CANVAS");

  netw_free( canvas );
  
  cdKillCanvas(canvas);
  return IUP_DEFAULT;
}

int button_cb(Ihandle* self, int but, int press, int x, int y, char *status)
{
  /* if (but == IUP_BUTTON1 && press) */
  /* { */
  /*   cdCanvasUpdateYAxis(cdcanvas, &y); */
  /*   cdCanvasPixel(cdcanvas, x, y, CD_BLUE); */
  /*   draw = 1; */
  /* } */
  /* else */
  /* { */
  /*   cdCanvasClear(cdcanvas); */
  /*   draw = 0; */
  /* } */
  /* return IUP_DEFAULT; */
  cdCanvas *canvas = (cdCanvas*)IupGetAttribute( self, "_CD_CANVAS" );
  int needredraw = 0;
  if( IUP_BUTTON1 == but ){
    unsigned short orientation = NETW_RL;
    Ihandle *ihradio = IupGetHandle( "netw_radio" );
    if( ihradio )
      orientation = (0 == strcmp( IupGetAttribute( ihradio, "VALUE" ), "netw_rl" ) ) ? NETW_RL : NETW_LR;
    cdCanvasUpdateYAxis( canvas, &y );
    printf( "Click at x=%d, y=%d\n", x, y );
    // Left button DOWN and UP in the same cell trigger event
    needredraw = netw_click( canvas, but, press, x, y, iup_isshift(status),
			     WORLD_W, WORLD_H, orientation );
    if( needredraw ) IupUpdate( self );
  }
	
  return IUP_DEFAULT;	
}

int motion_cb(Ihandle* self, int x, int y)
{

  return IUP_DEFAULT;
}

int close_cb( Ihandle *ih ){
  printf( "Closing network\n" );
  Ihandle *ih_item = IupGetHandle( "item_browse" );
  IupSetAttribute( ih_item, "ACTIVE", "YES" );
  IupDestroy( ih );
  
  return IUP_IGNORE;
}

int netw_toggle_rl_cb( Ihandle *ih ){
  Ihandle *cnv = IupGetHandle( "rule_network" );
  cdCanvas *canvas = (cdCanvas*)IupGetAttribute(cnv, "_CD_CANVAS");

  float posx = 0.;
  printf("INIT SCROLL (%g, %g)\n", posx, 0.);
  cdCanvasActivate(canvas);
  IupSetFloat ( cnv, "POSX", posx );
  update_viewport(cnv, canvas, posx, IupGetFloat( cnv, "POSY" ) );
  IupUpdate( cnv );

  return IUP_DEFAULT;
}

int netw_toggle_lr_cb( Ihandle *ih ){
  Ihandle *cnv = IupGetHandle( "rule_network" );
  cdCanvas *canvas = (cdCanvas*)IupGetAttribute(cnv, "_CD_CANVAS");

  float posx = (float) (WORLD_W - IupGetInt( cnv, "DRAWSIZE" ));
  printf("INIT SCROLL (%g, %g)\n", posx, 0.);
  cdCanvasActivate(canvas);
  IupSetFloat ( cnv, "POSX", posx );
  update_viewport(cnv, canvas, posx, IupGetFloat( cnv, "POSY" ) );
  IupUpdate( cnv );

  return IUP_DEFAULT;
}

int netw_toggle_compound_cb( Ihandle *ih ){
  IupUpdate( IupGetHandle( "rule_network" ) );
  return IUP_DEFAULT;
}

void CanvasScrollbarTest(void)
{
  Ihandle *dlg, *cnv;

  cnv = IupCanvas(NULL);
  IupSetHandle( "rule_network", cnv );
  IupSetAttribute(cnv, "RASTERSIZE", "600x400"); /* initial size */
  IupSetAttribute(cnv, "SCROLLBAR", "YES");
//  IupSetAttribute(cnv, "EXPAND", "NO");

  IupSetCallback(cnv, "RESIZE_CB",	(Icallback)resize_cb);
  IupSetCallback(cnv, "ACTION",		(Icallback)action);
  IupSetCallback(cnv, "MAP_CB",		(Icallback)map_cb);
  IupSetCallback(cnv, "UNMAP_CB",	(Icallback)unmap_cb);
  IupSetCallback(cnv, "WHEEL_CB",	(Icallback)wheel_cb);
  IupSetCallback(cnv, "SCROLL_CB",	(Icallback)scroll_cb);
  IupSetCallback(cnv, "BUTTON_CB",	(Icallback)button_cb);
  IupSetCallback(cnv, "MOTION_CB",	(Icallback)motion_cb);
  //
  Ihandle *netw_lr, *netw_rl;
  Ihandle *hbox_orientation;
  netw_lr = IupToggle( "Left-to-Right", NULL );
  IupSetHandle( "netw_lr", netw_lr );
  IupSetCallback( netw_lr, "ACTION",		(Icallback)netw_toggle_lr_cb );
  netw_rl = IupToggle( "Right-to-Left", NULL );
  IupSetHandle( "netw_rl", netw_rl );
  IupSetCallback( netw_rl, "ACTION",		(Icallback)netw_toggle_rl_cb );
  hbox_orientation = IupHbox( netw_rl, netw_lr, NULL );
  IupSetAttribute( hbox_orientation, "EXPAND", "HORIZONTAL" );
  IupSetAttribute( hbox_orientation, "MARGIN", "5x5" );
  IupSetAttribute( hbox_orientation, "GAP", "10" );
  Ihandle *netw_frame = IupFrame( hbox_orientation );
  IupSetAttribute( netw_frame, "TITLE", "Orientation" );
  Ihandle *netw_radio = IupRadio( netw_frame );
  IupSetHandle( "netw_radio", netw_radio );

  Ihandle *netw_compound = IupToggle( "Forward-expand DSL conditions and RHSes", NULL );
  IupSetHandle( "netw_compound", netw_compound );
  IupSetCallback( netw_compound, "ACTION", (Icallback)netw_toggle_compound_cb );
  hbox_orientation = IupHbox( netw_compound, NULL );
  /* IupSetAttribute( hbox_orientation, "ALIGNMENT", "ACENTER" ); */
  IupSetAttribute( hbox_orientation, "EXPAND", "HORIZONTAL" );
  IupSetAttribute( hbox_orientation, "MARGIN", "5x5" );
  IupSetAttribute( hbox_orientation, "GAP", "10" );
  
  netw_frame = IupFrame( hbox_orientation );
  IupSetAttribute( netw_frame, "TITLE", "Forward Links Details" );
  Ihandle *hbox_params = IupHbox( netw_radio, netw_frame, NULL );
  IupSetAttribute( hbox_params, "ALIGNMENT", "ACENTER" );
  IupSetAttribute( hbox_params, "NORMALIZESIZE", "VERTICAL" );
  IupSetAttribute( hbox_params, "EXPAND", "HORIZONTAL" );
  IupSetAttribute( hbox_params, "MARGIN", "5x5" );
  IupSetAttribute( hbox_params, "GAP", "10" );
  
  dlg = IupDialog(IupVbox( hbox_params, cnv, NULL));
  IupSetAttribute(dlg, "TITLE", "Rule Network");
  IupSetAttribute(dlg, "MARGIN", "10x10");
  IupSetCallback( dlg, "CLOSE_CB",	(Icallback)close_cb);

  IupMap(dlg);
  IupSetAttribute(cnv, "RASTERSIZE", NULL);  /* release the minimum limitation */
  IupShowXY(dlg,IUP_CENTER,IUP_CENTER);

  /* int canvas_h, canvas_w; */
  /* cdCanvas *canvas = (cdCanvas*)IupGetAttribute( cnv, "_CD_CANVAS" ); */
  /* cdCanvasGetSize(canvas, &canvas_w, &canvas_h, NULL, NULL); */
  /* (void) scroll_cb( cnv, IUP_SBPOSV, 0., WORLD_H/2. - 200. ); */
  /* update_scrollbar(cnv, canvas_w, canvas_h); */
  cdCanvas *canvas = (cdCanvas*)IupGetAttribute(cnv, "_CD_CANVAS");
  float posy = WORLD_H/2. - 200.;
  printf("INIT SCROLL (%g, %g)\n", 0., posy);
  cdCanvasActivate(canvas);
  IupSetFloat ( cnv, "POSY", posy );
  update_viewport(cnv, canvas, 0, posy );
  IupUpdate( cnv );

}

#ifndef BIG_TEST
//----------------------------------------------------------------------
// Minimal setup and ancillaries for engine
//----------------------------------------------------------------------
engine_state_rec_ptr S_State;
engine_state_rec_ptr repl_getState(){ return S_State; }

void print_local_val_repr( struct val_rec *val ){
  if( _UNKNOWN == val->status ){
    printf( "UNKNOWN" );
    return;
  }
  
  switch( val->type ){
  case _VAL_T_BOOL:
    printf( _FALSE == val->val_bool ? "FALSE"  : "TRUE" );
    return;
    break;
    
  case _VAL_T_INT:
    printf( "%d", val->val_int );
    return;
    break;
    
  case _VAL_T_FLOAT:
     break;
     
  case _VAL_T_STR:
    if( val->valptr )
      printf( val->valptr );
    else
      printf( "VAL_T_STR error" );
    return;
    break;
  }
  printf( "Error in printing value" );
  return;
}

const char *S_Color[] = { "\x1b[38;5;46m", "\x1b[38;5;160m", "\x1b[38;5;15m" };

char *S_val_color( unsigned short val ){
  char *esc;
  switch( val ){
  case _TRUE:
    esc = (char *) S_Color[0];
    break;
  case _FALSE:
    esc = (char *) S_Color[1];
    break;
  default:
    esc = (char *) S_Color[2];
  }
  return esc;
}

void getter_sign( sign_rec_ptr sign, int *suspend ){
  printf( "GETTER Question %s\n", sign->str );
  /* cell_rec_ptr cell = repl_getState()->agenda; */
  /* while( cell ){ */
  /*   printf( "\t%s\n", cell->sign_or_hypo->str ); */
  /*   cell = cell->next; */
  /* } */
  //
  nxpiup_dlgquestion( sign, NXPIUP_DLG_QUESTION );
  *suspend = _TRUE;
}

static  struct val_rec v_true  = { _KNOWN, _VAL_T_BOOL, (char *)0, _TRUE, 0, 0.0, 0 };
static  struct val_rec v_false = { _KNOWN, _VAL_T_BOOL, (char *)0, _FALSE, 0, 0.0, 0 };

void engine_dsl_getter_compound( compound_rec_ptr compound, int *suspend ){
#ifdef ENGINE_DSL_HOWERJFORTH
  if( _KNOWN == compound->val.status ) return;
  
  int  err;
  printf( "Getter compound %s (%d)\n", compound->str,
	   // (char *) (compound->dsl_expression)
	   *suspend
	   );
  /* repl_log( buf ); */
  // printf( buf );
  // WHY?
  // fixCR( compound->dsl_expression );
  int r = engine_dsl_eval_async( (const char *) compound->dsl_expression, &err, suspend );

  printf( "FORTH Res %d Err %d Susp %d\n", r, err, *suspend );
  /* repl_log( buf ); */
  // printf( buf );
  printf( "Post-eval compound %s (%d)\n", compound->str,
	   // (char *) (compound->dsl_expression)
	   *suspend
	   );
  /* repl_log( buf ); */
  switch( err ){
  case 0:
    // Ignore DSL evaluation if a question is pending! Re-evaluation will happen later.
    if( _FALSE == *suspend ){
      // sprintf( buf, "Getter compound %s (%d)\n", compound->str,
      // 	       // (char *) (compound->dsl_expression)
      // 	       *suspend
      // 	       );
      // printf( buf );
      sign_set_default( (sign_rec_ptr)compound, r ? &v_true : &v_false );
      // sprintf( buf, "Compound Status %d Type %d\n", compound->val.status, compound->val.type );
      // printf( "%s", buf );
    }
    break;
  }
  
#endif  
}

/* int repl_filter( const char *s, const char *prefix ){ */
/*   char *c = (char *) s, *p = (char *) prefix; */
/*   while( *p ){ */
/*     if( *c != *p ) return 1; */
/*     c++; p++; */
/*   } */
/*   return 0; */
/* } */


void  repl_log( const char *s ){
  Ihandle *log = IupGetHandle( "ih_log" );
  if( log ){
    if( *s != '<' )
      IupSetAttribute( log, "APPEND", s );
  }
  printf( "Log: %s\n", s );
}

void repl_msg( const char *fmt, ... ){
  char buffer[NXPIUP_TEMP_BUFSIZE] = {0};
  va_list args;
  va_start (args, fmt);
  vsprintf (buffer,fmt, args);
  repl_log( buffer );
  va_end (args);  
}


void cb_on_gate( sign_rec_ptr sign, short val ){
  printf( "Gating %s (%d) - %d\n", sign->str, sign->val.val_bool, val );
  //
  engine_default_on_gate( sign, val );
}

void cb_on_agenda_push( sign_rec_ptr sign, struct val_rec *val ){
  repl_msg( "[AGENDA] push %s", sign->str );
  printf( "Push %s\n", sign->str );
  IupLoopStep();
  //
  engine_default_on_agenda_push( sign, val );
}

void cb_on_agenda_pop( sign_rec_ptr sign, struct val_rec *val ){
  repl_msg( "[AGENDA]  pop %s", sign->str );
  printf( "Pop %s\n", sign->str );
  IupLoopStep();
  //
  engine_default_on_agenda_pop( sign, val );
}

void cb_on_set( sign_rec_ptr sign, struct val_rec *val ){
  printf( "Set %s:\t", sign->str );
  print_local_val_repr( &sign->val ); printf("\t");
  print_local_val_repr( val ); printf("\n");
  //
  NXPIUP_UPDATES
}

void cb_on_endsession( sign_rec_ptr sign, struct val_rec *val ){
  printf( "End of session.\n" );
  repl_log( "[SESSION] End of session\n" );
  //
  Ihandle *ih_item = IupGetHandle( "item_knowcess" );
  IupSetAttribute( ih_item, "ACTIVE", "YES" );
  //
  NXPIUP_UPDATES
}

//----------------------------------------------------------------------
// NXP utilities
//----------------------------------------------------------------------
int  nxpiup_inagendap( sign_rec_ptr sign ){
  cell_rec_ptr cell = repl_getState()->agenda;
  while( cell ){
    if( cell->sign_or_hypo == sign ) return 1;
    cell = cell->next;
  }
  return 0;
}

#include "ndug.c"

Ihandle *nxpiup_splash(){
  /* Ihandle *splash = IupLoadImage( "ndug.png" ); */
  Ihandle *splash = create_image_splash();
  IupSetHandle( "splash", splash );
  Ihandle *label  = IupLabel( "" );
  IupSetAttribute( label, "IMAGE", "splash" );
  //
  /* int res = IupSaveImageAsText( splash, "ndug.c", "C", NULL ); */
  //
  Ihandle *dlg = IupDialog( IupVbox( label, NULL ) );
  IupSetAttributes( dlg, "RESIZE=NO, MAXBOX=NO, MINBOX=NO, MENUBOX=NO" );
  IupSetAttribute(dlg, "BORDER", "NO");
  IupSetAttribute(dlg, "MARGIN", "10x10");
  IupMap( dlg );
  IupShowXY(dlg,IUP_CENTER,IUP_CENTER);
  return dlg;
}

int timer_cb( Ihandle *ih ){
  nxpiup_dlgmenu();
  IupSetAttribute( S_Timer, "RUN", "NO" );
  IupHide( S_splash );
  IupDestroy( S_splash );
  IupDestroy( IupGetHandle( "splash" ) );
  return IUP_DEFAULT;
}

int main(int argc, char* argv[])
{
  //----------------------------------------------------------------------
  // NXP prologue
  //----------------------------------------------------------------------

  S_State		= (engine_state_rec_ptr)malloc( sizeof( struct engine_state_rec ) );
  S_State->current_sign = (sign_rec_ptr)0;
  S_State->agenda	= (cell_rec_ptr)0;
  engine_register_effects( &engine_default_on_get,
			   &cb_on_set,
			   &cb_on_gate,
			   &cb_on_agenda_push,
			   &cb_on_agenda_pop,
			   &cb_on_endsession
			   );
  // Set up DSL
#ifdef ENGINE_DSL
  engine_dsl_init();
#endif

  nxp_hash_open();

  //----------------------------------------------------------------------
  // IUP application
  //----------------------------------------------------------------------

  IupOpen(&argc, &argv);
  IupControlsOpen();
  // Faking a splash screen
  S_splash = nxpiup_splash();
  S_Timer  = IupTimer();
  IupSetCallback( S_Timer, "ACTION_CB",	(Icallback)timer_cb );
  IupSetAttribute( S_Timer, "TIME", "1000" );
  IupSetAttribute( S_Timer, "RUN", "YES" );
  
  /* CanvasScrollbarTest();  */
  
  IupMainLoop();

  IupClose();

  //----------------------------------------------------------------------
  // NXP epilogue
  //----------------------------------------------------------------------
  nxp_hash_close();
#ifdef ENGINE_DSL
  printf( "Shutdown -- Freeing DSL engine\n" );
  engine_dsl_free();
#endif
  printf( "Shutdown -- Freeing Knowledge Base\n" );
  loadkb_reset();
  printf( "Shutdown -- Freeing NXP engine\n" );
  engine_free_state( S_State );
  printf( "Shutdown -- Complete\n" );

  return EXIT_SUCCESS;
}
#endif
