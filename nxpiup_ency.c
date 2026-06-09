/**
 * nxpiup_ency.c -- Encyclopedia management
 *
 * Written on 2026-05-09.
 */
#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <cstring>
#include <iup.h>
#include <iupcontrols.h>
#include <cd.h>
#include <cdiup.h>
#include <im.h>
#include <iupim.h>
#include <wd.h>

#include "agenda.h"
#include "nxpiup.h"
#include "netw.h"
#include "netw_internals.h"

/* #define NXPIUP_RED	"255 0 0" */
/* #define NXPIUP_GREEN	"0 255 0" */
/* #define NXPIUP_BLUE	"0 0 255" */

#define NXPIUP_UNKNOWN		"Unknown"
#define NXPIUP_KNOWN		"Known"
#define NXPIUP_KNOWNTRUE	"True"
#define NXPIUP_KNOWNFALSE	"False"
#define NXPIUP_KNOWNINT		"%d"
#define NXPIUP_KNOWNSTR		"%s"

#define NXPIUP_ENCY_WIDTH  200
#define NXPIUP_ENCY_HEIGHT  20

static Ihandle *S_ImgFirst  = NULL,
  *S_ImgNext  = NULL,
  *S_ImgPrev  = NULL,
  *S_ImgLast  = NULL;

// -------------------------------------------------------------------------------
// View Widgets of Encyclopediae have USERDATA attached keeping track of selection
// and a sorted array of signs/hypos/rules.

struct ency_rec{
  int size;
  sign_rec_ptr *seq;
  int selected;
};
typedef struct ency_rec *ency_rec_ptr;

int S_LineClicked	= -1;

ency_rec_ptr nxpiup_ency__newrec( int size ){
  ency_rec_ptr userdata = (ency_rec_ptr) malloc( sizeof( struct ency_rec ) );
  userdata->size	= size;
  userdata->selected	= -1;
  userdata->seq		= (sign_rec_ptr *) malloc( size * sizeof( sign_rec_ptr ) );
  return userdata;
}

void nxpiup_ency__freerec( ency_rec_ptr userdata ){
  // Voids argument `userdata'
  if( userdata->seq ) free( (void *)userdata->seq );
  free( userdata );
}

// -------------------------------------------------------------------------------
// Formatting (for IUP) tries to enforce a convention based on value and state:
//   - Known TRUE (boolean): green
//   - Known FALSE (boolean): red
//   - Known (non-boolean): blue
//   - Unknown, in Agenda: bold style
//   - Unknown: IUP default

Ihandle *nxpiup_ency__newtags( long int color, const char *weight ){
  char fgcolor[12] = {0};
  Ihandle *ftag = IupUser();
  IupSetAttribute(ftag, "ALIGNMENT", "CENTER");
  IupSetAttribute(ftag, "SPACEAFTER", "10");
  IupSetAttribute(ftag, "FONTSIZE", "12");
  IupSetAttribute(ftag, "SELECTION", "1,1:1,48");
  if( color ){
    sprintf( fgcolor, "%d %d %d", cdRed( color ), cdGreen( color ), cdBlue( color ) );
    IupSetAttribute(ftag, "FGCOLOR", fgcolor );
  }
  if( weight  ) IupSetAttribute(ftag, "WEIGHT", weight );
  return ftag;
}


long int nxpiup_ency__textcolor( sign_rec_ptr sign ){
  long int text_color = S_NETW_COLOR_UNKNOWN;
  struct val_rec val = sign->val;
  if( _KNOWN == val.status ){
    switch( val.type ){
    case _VAL_T_BOOL:
      text_color = (_FALSE == val.val_bool) ? S_NETW_COLOR_FALSE : S_NETW_COLOR_TRUE;
      break;
      
    case _VAL_T_INT:
      text_color = S_NETW_COLOR_KNOWN;
      break;
      
    case _VAL_T_FLOAT:
      break;
      
    case _VAL_T_STR:
      if( val.valptr )
	text_color = S_NETW_COLOR_KNOWN;
      break;
    }
  }
  return text_color;
}

void nxpiup_ency__fgcolor( sign_rec_ptr sign, char *scolor ){
  long int color = nxpiup_ency__textcolor( sign );
  sprintf( scolor, "%d %d %d", cdRed( color ), cdGreen( color ), cdBlue( color ) );
  return;
}

void nxpiup_ency__valuestr( sign_rec_ptr sign, char *svalue ){
  struct val_rec val = sign->val;
  if( _UNKNOWN == val.status ){
    strcpy( svalue, NXPIUP_UNKNOWN );
    return;
  }
  //
  switch( val.type ){
  case _VAL_T_BOOL:
    strcpy( svalue, (_FALSE == val.val_bool) ? NXPIUP_KNOWNFALSE : NXPIUP_KNOWNTRUE );
    break;
    
  case _VAL_T_INT:
    sprintf( svalue, NXPIUP_KNOWNINT, val.val_int );	    
    break;
    
  case _VAL_T_FLOAT:
    break;
     
  case _VAL_T_STR:
    if( val.valptr )
      sprintf( svalue, NXPIUP_KNOWNSTR, val.valptr );	    
    break;
  }
  return;
}

sign_rec_ptr nxpiup_ency_selection( char *view ){
  Ihandle *ih = IupGetHandle( view );
  if( ih ){
    ency_rec_ptr userdata = (ency_rec_ptr) IupGetAttribute( ih, "USERDATA" );
    /* int index = atoi( IupGetAttribute( ih, "VALUE" ) ); */
    int index = IupGetInt( ih, "VALUE" );
    return userdata->seq[ index-1 ];
  }
  else
    return NULL;
}

// -------------------------------------------------------------------------------
// Signs and Hypos Encyclopediae share a dialog template.
// The Rules Encyclopedia has additional features and a separate dialog.
Ihandle *nxpiup_netw__get_rn_or_create(){
  Ihandle *ih		= IupGetHandle( "rule_network" );
  if( !ih ){
    CanvasScrollbarTest();
    ih = IupGetHandle( "rule_network" );
  }
  return ih;
}

void            nxpiup_netw_focus_sign( sign_rec_ptr hypo, int hypo_p ){
  Ihandle *ih           = nxpiup_netw__get_rn_or_create();
  cdCanvas *canvas	= (cdCanvas*)IupGetAttribute( ih, "_CD_CANVAS" );
  col_rec_ptr col	= (col_rec_ptr) cdCanvasGetAttribute( canvas, "USERDATA" ); 
  netw_cell_rec_ptr cptr;
  //
  if( col ) netw_free( canvas );
  col				= _NEW_COL;
  col->x			= WORLD_W/CELL_W - 3;
  col->next			= NULL;
  cptr				= (netw_cell_rec_ptr) col; // Leverages that fields 'first' and 'next' are the same in the structs
  _NETW_NEWCELL( cptr->next );
  cptr->next->y			= 1;
  cptr->next->head		= col;
  cptr->next->client_data_t	= _NETW_SIGN_T;
  cptr->next->client_data	= (void *) hypo;
  cdCanvasSetAttribute( canvas, "USERDATA", (char *) col);
  // Second pass to adjust height of cells
  int inc = (WORLD_H/CELL_H - 1)/2;
  printf("INITFILLALL i=%d, inc=%d\n", 0, inc );
  netw__adjust_col_vert( col->first, inc );
  //
  if( hypo_p ){
    netw__expand_backward( canvas, col->first, WORLD_W, WORLD_H, NETW_RL );
  }
  else{
    IupSetAttribute( IupGetHandle( "netw_compound" ), "VALUE", "ON" );
    netw__expand_forward( canvas, col->first, WORLD_W, WORLD_H, NETW_RL );
  }
  //
  IupUpdate( ih );
}

int item_focus_hypo_cb( void ){
  hypo_rec_ptr hypo     = (hypo_rec_ptr) nxpiup_ency_selection( (char *) NXPIUP_ENCY_HYPOS_VIEW );
  if( hypo ){
    nxpiup_netw_focus_sign( hypo, 1 );
  }
  return IUP_DEFAULT;
}

int item_focus_sign_cb( void ){
  sign_rec_ptr sign     = (sign_rec_ptr) nxpiup_ency_selection( (char *) NXPIUP_ENCY_SIGNS_VIEW );
  if( sign ){
    nxpiup_netw_focus_sign( sign, 0 );
  }
  return IUP_DEFAULT;
}

int ency_destroy_cb( Ihandle *ih ){
  ency_rec_ptr userdata = (ency_rec_ptr) IupGetAttribute( ih, "USERDATA" );
  if( userdata ) nxpiup_ency__freerec( userdata );
  return IUP_DEFAULT;
}

// Used for the `qsort' calls when sorting the USERDATA array of strings
int nxpiup_ency__compare( const void *arg1, const void *arg2 ){
  return strcmp( (* ((sign_rec_ptr *) arg1))->str, (* ((sign_rec_ptr *) arg2))->str );
}

int nxpiup_ency__compare_hypos( const void *arg1, const void *arg2 ){
  rule_rec_ptr *r1 = (rule_rec_ptr *) arg1;
  rule_rec_ptr *r2 = (rule_rec_ptr *) arg2;
  return strcmp( ((sign_rec_ptr) (*r1)->setters)->str, ((sign_rec_ptr) (*r2)->setters)->str );
}

// Signs/Hypos Encyclopedia dialog template
void nxpiup_dlgency( const char *ency_title, const char *ency_handle, sign_rec_ptr top ){
  // IupListbox implementation
  Ihandle *dlg = IupGetHandle( ency_handle );
  if( !dlg ){
    if( !top ) return;
    //
    sign_rec_ptr sign;
    short item = 0;
    sign = top;
    while( sign ){
      item += 1;
      sign = sign->next;
    }
    ency_rec_ptr userdata = nxpiup_ency__newrec( item );
    item = 0;
    sign = top;
    while( sign ){
      userdata->seq[ item++ ] = sign;
      sign = sign->next;
    }
    /* printf( "QSORT pre\n" ); */
    /* for( short i=0; i<userdata->size; i++ ){ printf( "\t%s\n", userdata->seq[i]->str ); } */
    qsort( (void *) userdata->seq, (size_t) userdata->size, sizeof(sign_rec_ptr), nxpiup_ency__compare );
    /* printf( "QSORT post\n" ); */
    /* for( short i=0; i<userdata->size; i++ ){ printf( "\t%s\n", userdata->seq[i]->str ); } */
    //
    char buf[NXPIUP_TEMP_BUFSIZE] = {0};
    char val[NXPIUP_TEMP_BUFSIZE] = {0};

    Ihandle *encyh = IupFlatList();
    IupSetAttribute( encyh, "USERDATA", (char *)userdata );
    IupSetCallback( encyh, "DESTROY_CB", (Icallback) ency_destroy_cb );
    /* IupSetAttribute( encyh, "SIZE", "420*400" ); */
    IupSetAttribute( encyh, "FLATSCROLLBAR", "VERTICAL" );
    IupSetAttribute( encyh, "ALIGNMENT", "ALEFT:ACENTER" );
    IupSetAttribute( encyh, "EXPAND", "YES" );
    sprintf( buf, "%s_view", ency_handle );
    IupSetHandle( buf, encyh );
    //
    for( short i=0; i<userdata->size; i++ ){
      sign = (sign_rec_ptr) userdata->seq[i];
      nxpiup_ency__valuestr( sign, val );
      sprintf( buf, "%-32.32s  %16s", sign->str, val );
      IupSetAttribute( encyh, "APPENDITEM", buf );
      *buf = 0x00;
      sprintf( buf, "ITEMFGCOLOR%d", i+1 );
      nxpiup_ency__fgcolor( sign, val );
      IupSetAttribute( encyh, buf, val );
      sprintf( buf, "ITEMFONT%d", i+1 );
      IupSetAttribute( encyh, buf, "Courier, 12" );
    }
    //
    Ihandle *ency_vbox = IupVbox( IupFrame( encyh ), NULL );
    IupSetAttribute( ency_vbox, "MARGIN","10x10" );
    //
    Ihandle *ency_item_op, *ency_item_focus;
    Ihandle *local_menu;
    ency_item_focus = IupItem ("Focus Network", NULL);
    if( 0 == strcmp( ency_handle, NXPIUP_ENCY_HYPOS) ){
      ency_item_op = IupItem ("Suggest", NULL);
      IupSetCallback( ency_item_op, "ACTION", (Icallback)item_suggest_cb );
      IupSetCallback( ency_item_focus, "ACTION", (Icallback)item_focus_hypo_cb );    }
    else{
      ency_item_op = IupItem ("Volunteer...", NULL);
      IupSetCallback( ency_item_op, "ACTION", (Icallback)item_volunteer_cb );
      IupSetCallback( ency_item_focus, "ACTION", (Icallback)item_focus_sign_cb );
    }

    local_menu = IupMenu( ency_item_op, IupSeparator(), ency_item_focus, NULL );
    IupSetHandle( "local_menu", local_menu );
    
    dlg = IupDialog( ency_vbox );
    sprintf( buf, "Encyclopedia %s", ency_title );
    IupSetAttribute( dlg, "TITLE", buf );
    IupSetAttribute( dlg, "SIZE", "HALFxHALF" );
    IupSetAttribute( dlg, "EXPANDCHILDREN", "YES" );
    IupSetAttribute( dlg, "MENU", "local_menu" );
    IupSetHandle( ency_handle, dlg );
  }
  IupShow( dlg );
}

// Rules Encyclopedia dialog with additional features

// Displaying rules as text with formats and value marks in the left margin
void nxpiup_ency__logcond( rule_rec_ptr r, int i, char *val ){
  cond_rec_ptr cond = (cond_rec_ptr) r->getters[i];
  if( COMPOUND_MASK == (cond->sign->len_type & TYPE_MASK) ){
    char *c = ((compound_rec_ptr) cond->sign)->dsl_expression;
    for( i=0; i<NXPIUP_TEMP_BUFSIZE+NXPIUP_TEMP_BUFSIZE - 4; i++ ){
      val[i]=c[i];
      if( 0 == c[i] || '\r' == c[i] || '\n' == c[i] ){
	val[i] = 0x00;
	break;
      }
    }
  }
  else{
    sprintf( val, "%s %s", cond->out ? "Yes" : "No", cond->sign->str );
  }
}

void nxpiup_ency__logrule( Ihandle *ih, int id ){
  // ih is an IupMultiline w. userdata
  char buf[NXPIUP_TEMP_BUFSIZE+NXPIUP_TEMP_BUFSIZE] = {0};
  char val[NXPIUP_TEMP_BUFSIZE+NXPIUP_TEMP_BUFSIZE] = {0};
  ency_rec_ptr userdata = (ency_rec_ptr) IupGetAttribute( ih, "USERDATA" );
  short i, j;
  char *c;
  if( id < 1 || id > userdata->size ) return;
  
  IupSetAttribute( ih, "VALUE", "" );
  // Title line
  sprintf( buf, "Rule #%d: %s", id, userdata->seq[ id-1 ]->str );
  IupSetAttribute( ih, "VALUE", buf );
  // LHS
  IupSetAttribute( ih, "APPEND", "IF" );
  for( i=0; i<userdata->seq[ id-1 ]->ngetters; i++ ){
    nxpiup_ency__logcond( (rule_rec_ptr) userdata->seq[ id-1 ], i, val );
    if( i>0 ){
      sprintf( buf, "[*] AND %s", val );
    }
    else{
      sprintf( buf, "[*] %s", val );
    }
    IupSetAttribute( ih, "APPEND", buf );
  }
  // Hypo
  sprintf( buf, "THEN %s", (char *) ((sign_rec_ptr) ((rule_rec_ptr) userdata->seq[ id-1 ])->setters)->str );
  IupSetAttribute( ih, "APPEND", buf );
  // RHS
  for( i=0; i<((rule_rec_ptr) userdata->seq[ id-1 ])->nrhs; i++ ){
    c = (char *) ((rule_rec_ptr) userdata->seq[ id-1 ])->rhs[i];
    for( j=0; j<NXPIUP_TEMP_BUFSIZE+NXPIUP_TEMP_BUFSIZE - 4; j++ ){
      val[j]=c[j];
      if( 0 == c[j] || '\r' == c[j] || '\n' == c[j] ){
	val[j] = 0x00;
	break;
      }
    }
    sprintf( buf, "[*] %s", val );
    IupSetAttribute( ih, "APPEND", buf );
  }
  // Formatting Title
  if( _KNOWN == userdata->seq[ id-1 ]->val.status  &&
      _VAL_T_BOOL == userdata->seq[ id-1 ]->val.type ){
    Ihandle *ftag = nxpiup_ency__newtags((_FALSE == userdata->seq[ id-1 ]->val.val_bool) ?
					 S_NETW_COLOR_FALSE : S_NETW_COLOR_TRUE,
					 NULL );
    IupSetAttribute( ih, "ADDFORMATTAG_HANDLE", (char *) ftag );
  }
  else if ( nxpiup_inagendap( (sign_rec_ptr) userdata->seq[ id-1 ]->setters ) ){
    Ihandle *ftag = nxpiup_ency__newtags( 0L, "BOLD" );
    IupSetAttribute( ih, "ADDFORMATTAG_HANDLE", (char *) ftag );
  }
  else{
    Ihandle *ftag = nxpiup_ency__newtags( 0L, NULL );
    IupSetAttribute( ih, "ADDFORMATTAG_HANDLE", (char *) ftag );
  }
  // Formatting conditions
  cond_rec_ptr cond;
  Ihandle *ftag;
  long int color;
  for( i=0; i<userdata->seq[ id-1 ]->ngetters; i++ ){
    cond = (cond_rec_ptr) userdata->seq[ id-1 ]->getters[i];
    if( _TRUE == cond->val || _FALSE == cond->val ){
      ftag = IupUser();
      sprintf( buf, "%d,1:%d,4", 3+i, 3+i );
      IupSetAttribute(ftag, "SELECTION", buf);
      color = (_FALSE == cond->val) ? S_NETW_COLOR_FALSE : S_NETW_COLOR_TRUE;
      sprintf( val, "%d %d %d", cdRed( color ), cdGreen( color ), cdBlue( color ) );
      IupSetAttribute(ftag, "FGCOLOR", val );
      IupSetAttribute( ih, "ADDFORMATTAG_HANDLE", (char *) ftag );
    }
  }  
}

// Bottom-side control panel to navigate the sorted list of rules

// Simplistic MVC: `ency_rules__update' is called whenever the selection changes
void ency_rules__update( int selection ){
  Ihandle *view = IupGetHandle( NXPIUP_ENCY_RULES_VIEW );
  Ihandle *rule = IupGetHandle( "ency_current_rule" );
  nxpiup_ency__logrule( view, selection );
  IupSetAttribute( rule, "VALUE",
		   ((ency_rec_ptr) IupGetAttribute( view, "USERDATA" ))->seq[selection-1]->str );
}

int ency_rules_first_cb( Ihandle *ih ){
  Ihandle *view = IupGetHandle( NXPIUP_ENCY_RULES_VIEW );
  ency_rec_ptr userdata = (ency_rec_ptr) IupGetAttribute( view, "USERDATA" );
  printf( "ENCY RULES %d\n", userdata->selected );
  if( userdata->selected ){
    userdata->selected = 0;
    ency_rules__update( 1 );
  }
  return IUP_DEFAULT;
}

int ency_rules_last_cb( Ihandle *ih ){
  Ihandle *view = IupGetHandle( NXPIUP_ENCY_RULES_VIEW );
  ency_rec_ptr userdata = (ency_rec_ptr) IupGetAttribute( view, "USERDATA" );
  printf( "ENCY RULES %d\n", userdata->selected );
  if( userdata->selected < (userdata->size - 1) ){
    userdata->selected = (userdata->size - 1);
    ency_rules__update( userdata->size );
  }
  return IUP_DEFAULT;
}

int ency_rules_next_cb( Ihandle *ih ){
  Ihandle *view = IupGetHandle( NXPIUP_ENCY_RULES_VIEW );
  ency_rec_ptr userdata = (ency_rec_ptr) IupGetAttribute( view, "USERDATA" );
  printf( "ENCY RULES %d\n", userdata->selected );
  if( userdata->selected < (userdata->size - 1) ){
    userdata->selected += 1;
    ency_rules__update( userdata->selected + 1 );
  }
  return IUP_DEFAULT;
}

int ency_rules_prev_cb( Ihandle *ih ){
  Ihandle *view = IupGetHandle( NXPIUP_ENCY_RULES_VIEW );
  ency_rec_ptr userdata = (ency_rec_ptr) IupGetAttribute( view, "USERDATA" );
  printf( "ENCY RULES %d\n", userdata->selected );
  if( userdata->selected > 0 ){
    userdata->selected -= 1;
    ency_rules__update( userdata->selected + 1 );
  }
  return IUP_DEFAULT;
}

int ency_rules_current_valuechanged_cb( Ihandle *ih ){
  /* printf( "ENCY rules, current=%s\n", IupGetAttribute( ih, "VALUE" ) ); */
  short i;
  Ihandle *view = IupGetHandle( NXPIUP_ENCY_RULES_VIEW );
  ency_rec_ptr userdata = (ency_rec_ptr) IupGetAttribute( view, "USERDATA" );
  char *val = IupGetAttribute( ih, "VALUE" );
  for( i=0; i<userdata->size; i++ ){
    if( 0 == strcmp( val, userdata->seq[i]->str ) ){
      userdata->selected = i;
      ency_rules__update( i + 1 );
      break;
    }
  }
  return IUP_DEFAULT;
}

// Top control panel to sort the list of rules by name or by their hypothesis
int ency_rules_byname_cb( Ihandle *ih, int state ){
  /* printf( "ENCY rules: sort by name\n" ); */
  if( 1 == state ){
    Ihandle *view = IupGetHandle( NXPIUP_ENCY_RULES_VIEW );
    ency_rec_ptr userdata = (ency_rec_ptr) IupGetAttribute( view, "USERDATA" );
    qsort( (void *) userdata->seq, (size_t) userdata->size, sizeof(rule_rec_ptr), nxpiup_ency__compare );
    userdata->selected = 0;
    ency_rules__update( 1 );
  }
  return IUP_DEFAULT;
}

int ency_rules_byhypo_cb( Ihandle *ih, int state ){
  /* printf( "ENCY rules: sort by hypo\n" ); */
  if( 1 == state ){
    Ihandle *view = IupGetHandle( NXPIUP_ENCY_RULES_VIEW );
    ency_rec_ptr userdata = (ency_rec_ptr) IupGetAttribute( view, "USERDATA" );
    qsort( (void *) userdata->seq, (size_t) userdata->size, sizeof(rule_rec_ptr), nxpiup_ency__compare_hypos );
    userdata->selected = 0;
    ency_rules__update( 1 );
  }
  return IUP_DEFAULT;
}

// Rules Encyclopedia dialog with sorting, navigation and formatted text display
void nxpiup_dlgency_rules( const char *ency_title, const char *ency_handle, rule_rec_ptr top ){
  Ihandle *dlg = IupGetHandle( ency_handle );
  if( !dlg ){
    if( !top ) return;
    //
    float f;
    rule_rec_ptr rule;
    short item = 0;
    rule = (rule_rec_ptr) top;
    while( rule ){
      item += 1;
      rule = (rule_rec_ptr) rule->next;
    }
    ency_rec_ptr userdata = nxpiup_ency__newrec( item );
    userdata->selected = 0;
    item = 0;
    rule = top;
    while( rule ){
      userdata->seq[ item++ ] = (sign_rec_ptr) rule;
      rule = (rule_rec_ptr) rule->next;
    }
    /* printf( "QSORT pre\n" ); */
    /* for( short i=0; i<userdata->size; i++ ){ printf( "\t%s\n", userdata->seq[i]->str ); } */
    qsort( (void *) userdata->seq, (size_t) userdata->size, sizeof(rule_rec_ptr), nxpiup_ency__compare );
    /* printf( "QSORT post\n" ); */
    /* for( short i=0; i<userdata->size; i++ ){ printf( "\t%s\n", userdata->seq[i]->str ); } */
    // Top control panel: Sorting
    Ihandle *byhypo = IupToggle( "By Hypothesis", NULL );
    Ihandle *byname = IupToggle( "By Name", NULL );
    IupSetHandle( "sort_byhypo", byhypo );
    IupSetAttribute( byhypo, "EXPAND", "HORIZONTAL" );
    IupSetCallback( byhypo,  "ACTION", (Icallback) ency_rules_byhypo_cb  );

    IupSetHandle( "sort_byname", byname );
    IupSetAttribute( byname, "EXPAND", "HORIZONTAL" );
    IupSetCallback( byname,  "ACTION", (Icallback) ency_rules_byname_cb  );

    Ihandle *ihframe = IupFrame( IupHbox( byname, byhypo, NULL ) );
    IupSetAttribute( ihframe, "MARGIN", "5x5" );
    IupSetAttribute( ihframe, "TITLE", "Sort" );
    
    Ihandle *ihsort = IupRadio( ihframe );
    IupSetAttribute( ihsort, "EXPAND", "HORIZONTAL" );
    IupSetAttribute( ihsort, "VALUE", "sort_byname" );

    // Main text display
    char buf[NXPIUP_TEMP_BUFSIZE] = {0};

    Ihandle *rule_text = IupMultiLine( NULL ); // read-only
    IupSetAttribute( rule_text, "READONLY", "YES" );
    IupSetAttribute( rule_text, "SCROLLBAR", "YES" );
    IupSetAttribute( rule_text, "VISIBLELINES", "16" );
    IupSetAttribute( rule_text, "VISIBLECOLUMNS", "48" );
    IupSetAttribute( rule_text, "EXPAND", "YES" );
    IupSetAttribute( rule_text, "USERDATA", (char *) userdata );
    IupSetCallback( rule_text, "DESTROY_CB", (Icallback) ency_destroy_cb );
    sprintf( buf, "%s_view", ency_handle );
    IupSetHandle( buf, rule_text );
    // Formatting
    IupSetAttribute( rule_text, "FORMATTING", "YES" );
    
    /* Ihandle *rule_page = IupVal( "HORIZONTAL" ); */
    /* IupSetAttribute( rule_page, "MIN", "0" ); */
    /* IupSetAttribute( rule_page, "MAX", "1" ); */
    /* f = (double) 1./userdata->size ; */
    /* sprintf( buf, "%f", f ); */
    /* printf( "ENCY Rules %s\n", buf ); */
    /* IupSetAttribute( rule_page, "STEP", buf ); */
    /* IupSetAttribute( rule_page, "PAGESTEP", buf ); */
    /* IupSetAttribute( rule_page, "VALUE", "0" ); */
    /* IupSetCallback( rule_page, "VALUECHANGED_CB", (Icallback) ency_rules_valuechanged_cb ); */

    // Bottom control panel: Navigating
    Ihandle *first = IupButton( "First", "" ),
      *prev = IupButton( "Prev", "" ),
      *next = IupButton( "Next", "" ),
      *last = IupButton( "Last", "" );
    IupSetAttribute( first, "EXPAND", "HORIZONTAL" );
    S_ImgFirst = IupLoadImage( "first.png" );
    IupSetHandle( "img_first", S_ImgFirst );
    IupSetAttribute( first, "IMAGE", "img_first" );
    
    IupSetAttribute( prev, "EXPAND", "HORIZONTAL" );
    S_ImgPrev = IupLoadImage( "prev.png" );
    IupSetHandle( "img_prev", S_ImgPrev );
    IupSetAttribute( prev, "IMAGE", "img_prev" );

    IupSetAttribute( next, "EXPAND", "HORIZONTAL" );
    S_ImgNext = IupLoadImage( "next.png" );
    IupSetHandle( "img_next", S_ImgNext );
    IupSetAttribute( next, "IMAGE", "img_next" );

    IupSetAttribute( last, "EXPAND", "HORIZONTAL" );
    S_ImgLast = IupLoadImage( "last.png" );
    IupSetHandle( "img_last", S_ImgLast );
    IupSetAttribute( last, "IMAGE", "img_last" );
    
    /* Registers callbacks */  
    IupSetCallback( first, "ACTION", (Icallback) ency_rules_first_cb );
    IupSetCallback( prev,  "ACTION", (Icallback) ency_rules_prev_cb  );     
    IupSetCallback( next,  "ACTION", (Icallback) ency_rules_next_cb  );
    IupSetCallback( last,  "ACTION", (Icallback) ency_rules_last_cb  );
    //
    Ihandle *current_rule = IupList( NULL );
    IupSetAttribute( current_rule, "EDITBOX", "YES" );
    IupSetAttribute( current_rule, "DROPDOWN", "YES" );
    IupSetAttribute( current_rule, "EXPAND", "HORIZONTAL" );
    IupSetHandle( "ency_current_rule", current_rule );
    IupSetCallback( current_rule,  "VALUECHANGED_CB", (Icallback) ency_rules_current_valuechanged_cb );
    
    Ihandle *rule_page = IupHbox( first, prev, current_rule, next, last, NULL );
    /* IupSetAttribute( rule_page, "HOMOGENEOUS", "YES" ); */
    /* IupSetAttribute( rule_page, "NORMALIZESIZE", "HORIZONTAL" ); */
    IupSetAttribute( rule_page, "EXPAND", "HORIZONTAL" );
    IupSetAttribute( rule_page, "GAP", "20" );
    
      
    Ihandle *ency_vbox = IupVbox( ihsort, rule_text, rule_page, NULL );
    IupSetAttribute( ency_vbox, "MARGIN","10x10" );
    
    dlg = IupDialog( ency_vbox );
    sprintf( buf, "Encyclopedia %s", ency_title );
    IupSetAttribute( dlg, "TITLE", buf );
    IupSetAttribute( dlg, "EXPANDCHILDREN", "YES" );
    IupSetHandle( ency_handle, dlg );
    IupMap( dlg );
    //
    for( item=0; item<userdata->size; item++ ){
      IupSetAttribute( current_rule, "APPENDITEM", userdata->seq[item]->str );
    }
    IupSetAttribute( current_rule, "VALUE", userdata->seq[0]->str);
    nxpiup_ency__logrule( rule_text, 1 );
  }
  IupShow( dlg );
}

// Another simplistic MVC: Engine callbacks should call `nxpiup_ency_update'
void nxpiup_ency_update( Ihandle * ih, short rulep ){
  ency_rec_ptr userdata = (ency_rec_ptr) IupGetAttribute( ih, "USERDATA" );
  if( rulep ){
    nxpiup_ency__logrule( ih, userdata->selected + 1 );
  }
  else{
    char buf[NXPIUP_TEMP_BUFSIZE] = {0};
    char val[NXPIUP_TEMP_BUFSIZE] = {0};
    sign_rec_ptr sign;
    for( short i=0; i<userdata->size; i++ ){
      sign = (sign_rec_ptr) userdata->seq[i];
      nxpiup_ency__valuestr( sign, val );
      sprintf( buf, "%-32.32s  %16s", sign->str, val );
      sprintf( val, "%d", i+1 );
      IupSetAttribute( ih, val, buf );
      *buf = 0x00;
      sprintf( buf, "ITEMFGCOLOR%d", i+1 );
      nxpiup_ency__fgcolor( sign, val );
      IupSetAttribute( ih, buf, val );
    }
  }
}

/* void nxpiup_dlgency_hypos(){ */
/*   // IupGridbox implementation */
/*   Ihandle *dlg = IupGetHandle( "ency_hypos" ); */
/*   if( !dlg ){ */
/*     sign_rec_ptr sign = (sign_rec_ptr) loadkb_get_allhypos(); */
/*     if( !sign ) return; */
/*     //  */
/*     Ihandle *encyh = IupGridBox( NULL ); */
/*     IupSetAttribute( encyh, "GAPCOL", "10" ); */
/*     IupSetAttribute( encyh, "MARGIN", "5x5" ); */
/*     IupSetAttribute( encyh, "ORIENTATION", "HORIZONTAL" ); */
/*     IupSetAttribute( encyh, "NUMDIV", "2" ); */
/*     IupSetAttribute( encyh, "NORMALIZESIZE", "HORIZONTAL" ); */
/*     IupSetHandle( "ency_hypos_view", encyh ); */
/*     // Fill */
/*     Ihandle *lbl_name, *lbl_value; */
/*     char scolor[NXPIUP_TEMP_BUFSIZE] = {0}; */
/*     while( sign ){ */
/*       lbl_name = IupLabel( sign->str ); */
/*       nxpiup_ency__valuestr( sign, scolor ); */
/*       lbl_value = IupLabel( scolor ); */
/*       *scolor = 0x00; */
/*       // */
/*       nxpiup_ency__fgcolor( sign, scolor ); */
/*       IupSetAttribute( lbl_name, "FGCOLOR", scolor ); */
/*       IupSetAttribute( lbl_value, "FGCOLOR", scolor ); */
/*       // */
/*       IupAppend( encyh, lbl_name ); */
/*       IupAppend( encyh, lbl_value ); */
/*       // */
/*       sign = sign->next; */
/*     } */
/*     // */
/*     dlg = IupDialog( IupHbox( IupFrame( encyh ), NULL ) ); */
/*     IupSetAttribute( dlg, "TITLE", "Encyclopedia Hypos"); */
/*     IupSetAttribute( dlg, "SIZE", "QUARTERxHALF" ); */
/*     IupSetHandle( "ency_hypos", dlg ); */
/*   } */
/*   IupShow( dlg ); */
  
/* } */
