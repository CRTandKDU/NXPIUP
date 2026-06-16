/**
 * nxpiup_question.c -- Handling sign-related input from the user.
 *
 * Written on 2026-05-21.
 */
#include <stdlib.h>
#include <stdio.h>
#include <cstring>
#include <iup.h>
#include <cd.h>
#include <cdiup.h>
#include <wd.h>

#include "agenda.h"
#include "nxpiup.h"
#include "netw.h"
#include "nxp_hash.h"

#define QUESTION_HANDLE		((q_type==NXPIUP_DLG_QUESTION)?"question":"volunteer")
#define QUESTION_LABEL          ((q_type==NXPIUP_DLG_QUESTION)?"question_label":"volunteer_label")
#define QUESTION_TEXT		((q_type==NXPIUP_DLG_QUESTION)?"question_text":"volunteer_text")
#define QUESTION_BOOL		((q_type==NXPIUP_DLG_QUESTION)?"question_bool":"volunteer_bool")
#define QUESTION_BOOL_TRUE	((q_type==NXPIUP_DLG_QUESTION)?"question_booltrue":"volunteer_booltrue")
#define QUESTION_BOOL_FALSE	((q_type==NXPIUP_DLG_QUESTION)?"question_boolfalse":"volunteer_boolfalse")
#define QUESTION_CHOICES	((q_type==NXPIUP_DLG_QUESTION)?"question_choices":"volunteer_choices")
#define QUESTION_OK_CB          ((q_type==NXPIUP_DLG_QUESTION)?(Icallback) qbut_question_cb:(Icallback) qbut_volunteer_cb)

sign_rec_ptr S_current_sign = NULL;

int qcancel_cb( Ihandle *ih ){
  return IUP_CLOSE;
}

void nxpiup_question__setvalue( int q_type ){
  char buf[NXPIUP_TEMP_BUFSIZE+NXPIUP_TEMP_BUFSIZE] = {0};
  char ans[NXPIUP_TEMP_BUFSIZE] = {0};
  Ihandle *qwgt = ( nxp_hash_exists( S_current_sign->str, (char *) "VALUE" ) ) ?
    IupGetHandle( QUESTION_CHOICES ) :
    (( _VAL_T_BOOL == S_current_sign->val.type ) ?
     IupGetHandle( QUESTION_BOOL ) :
     IupGetHandle( QUESTION_TEXT ) );
  
  char *c = IupGetAttribute( qwgt, (char *) "VALUE" );
  strcpy( ans, c );
  //
  if( NXPIUP_DLG_QUESTION == q_type )
    IupHide( IupGetHandle( QUESTION_HANDLE ) );
  printf( NXPIUP_DLG_VOLUNTEER == q_type ? "Volunteered: %s for %s\n" : "Answered %s for %s\n",
	  ans, S_current_sign->str );
  // Handle sign value's expected type
  struct val_rec val;
  sign_rec_ptr sign = S_current_sign;
  S_current_sign = NULL;
  if( sign ){
    val.status = _KNOWN;
    val.type   = sign->val.type;
    switch( sign->val.type ){
    case _VAL_T_STR:
      if( val.valptr ) free( val.valptr );
      val.valptr = (char *)malloc( strlen( ans ) );
      strcpy( val.valptr, ans );
      //
      sprintf( buf, "[SESSION] Set (%s): %s DSL: %d", sign->str, val.valptr, sign->val.val_forth );
      repl_log( buf );
      break;
    case _VAL_T_INT:
      val.val_int = atoi( ans );
      sprintf( buf, "[SESSION] Set (%s): %d DSL: %d", sign->str, val.val_int, sign->val.val_forth );
      repl_log( buf );
      break;
    case _VAL_T_BOOL:
      if( 0 == strcmp( ans, QUESTION_BOOL_TRUE ) )
	val.val_bool = 1;
      else
	val.val_bool = 0;
      sprintf( buf, "[SESSION] Set (%s): %d DSL: %d", sign->str, val.val_bool, sign->val.val_forth );
      repl_log( buf );
      break;
    }
    sign_set_default( sign, &val );
    if( NXPIUP_DLG_QUESTION == q_type )
      engine_resume_knowcess( repl_getState() );
  }
}

int qbut_volunteer_cb( Ihandle *ih ){
  nxpiup_question__setvalue( NXPIUP_DLG_VOLUNTEER );
  return IUP_CLOSE;
}

int qbut_question_cb( Ihandle *ih ){
  nxpiup_question__setvalue( NXPIUP_DLG_QUESTION );
  return IUP_DEFAULT ;
}

void choices_cb( char *name, char *prop, char *key, char *val, unsigned int idx, void *clientdata ){
  int q_type = *( (int *) clientdata ) ;
  Ihandle *qchoices = IupGetHandle( QUESTION_CHOICES );
  char buf[6];
  sprintf( buf, "%d", idx );
  IupSetAttribute( qchoices, buf, val );
}

int toggle_default_cb( Ihandle *ih ){
  return IUP_DEFAULT;
}

int qitem_web_cb( Ihandle *ih ){
  char buf[ NXPIUP_TEMP_BUFSIZE ];
  sign_rec_ptr sign = (sign_rec_ptr) IupGetAttribute( ih, "USERDATA" );
  if( sign ){
    sprintf( buf, "cygstart %s", nxp_hash_get( sign->str, (char *) "URL" ) );
    printf( "WEB: %s\n", buf );
    system( buf );
  }
  return IUP_DEFAULT;
}

int qitem_info_cb( Ihandle *ih ){
  Ihandle *qinfo;
  sign_rec_ptr sign = (sign_rec_ptr) IupGetAttribute( ih, "USERDATA" );
  if( sign ){
    qinfo = IupGetHandle( "qinfo" );
    IupSetAttribute( qinfo, "VALUE", (char *) nxp_hash_get( sign->str, (char *) "INFO" ) );
    qinfo = IupGetHandle( "qinfovbox" );
    IupSetAttribute( qinfo, "VISIBLE", "YES" );
  }
  return IUP_DEFAULT;
}

int qitem_focus_cb( Ihandle *ih ){
  sign_rec_ptr sign = (sign_rec_ptr) IupGetAttribute( ih, "USERDATA" );
  if( sign ){
    nxpiup_netw_focus_sign( sign, 0 );
  }
  return IUP_DEFAULT;
}

Ihandle *nxpiup_question__generic( char *buf, sign_rec_ptr sign, int q_type ){
  Ihandle *qlabel,
    *qtext,
    *qchoices;
  Ihandle *qbut,
    *qcancel;
  Ihandle *qradio,
    *qtrue, *qfalse;
  Ihandle *dlg;
  // Label: "what is the value of"/"volunteer value of"
  qlabel = IupLabel( buf );
  IupSetHandle( QUESTION_LABEL, qlabel );
  IupSetAttribute( qlabel, "ALIGNMENT", "ACENTER:ACENTER" );
  // Text input for str-valued signs, w./o. predefined choices
  qtext  = IupText( NULL );
  IupSetHandle( QUESTION_TEXT, qtext );
  IupSetAttribute( qtext, "VISIBLECOLUMNS", "16" );
  // Boolean input
  qtrue = IupToggle( "True", NULL );
  IupSetHandle( QUESTION_BOOL_TRUE, qtrue );
  IupSetCallback( qtrue, "ACTION", toggle_default_cb );
  qfalse = IupToggle( "False", NULL );
  IupSetHandle( QUESTION_BOOL_FALSE, qfalse );
  IupSetCallback( qfalse, "ACTION", toggle_default_cb );
  Ihandle *radiobox = IupHbox( qtrue, qfalse, NULL );
  IupSetAttribute( radiobox, "GAP", "10" );
  IupSetAttribute( radiobox, "MARGIN", "5x5" );
  /* IupSetAttribute( radiobox, "EXPANDCHILDREN", "YES" ); */
  qradio = IupRadio( IupFrame( radiobox ) );
  IupSetHandle( QUESTION_BOOL, qradio );
  // Common OK button, different callbacks
  qbut   = IupButton( "OK", "qbut" );
  IupSetHandle( "qbut_OK", qbut );
  IupSetCallback( qbut, "ACTION", QUESTION_OK_CB );
  // Cancel button present only in volunteer
  qcancel   = IupButton( "Cancel", "qcancel" );
  IupSetHandle( "qbut_CANCEL", qcancel );
  IupSetCallback( qcancel, "ACTION", (Icallback) qcancel_cb );
  // Text input for str-valued string w. predefined choices
  qchoices = IupList( NULL );
  IupSetAttribute( qchoices, "EDITBOX", "YES" );
  IupSetAttribute( qchoices, "DROPDOWN", "YES" );
  IupSetAttribute( qchoices, "EXPAND", "YES" );
  IupSetAttribute( qtext, "VISIBLECOLUMNS", "16" );
  IupSetHandle( QUESTION_CHOICES, qchoices );
  //  
  Ihandle *qtext_box;
  if( NXPIUP_DLG_QUESTION == q_type ){
    qtext_box = ( nxp_hash_exists( sign->str, (char *) "VALUE" ) ) ?
      IupHbox( qchoices, qbut, NULL ) :
      (( _VAL_T_BOOL == sign->val.type ) ?
       IupHbox( qradio, qbut, NULL ) :
       IupHbox( qtext,    qbut, NULL ) );
  }
  else if ( NXPIUP_DLG_VOLUNTEER == q_type ){
    Ihandle *vb = IupVbox( qbut, qcancel, NULL );
    IupSetAttribute( vb, "GAP", "10" );
    IupSetAttribute( vb, "EXPANDCHILDREN", "YES" );
    qtext_box = ( nxp_hash_exists( sign->str, (char *) "VALUE" ) ) ?
      IupHbox( qchoices, vb, NULL ) :
      (( _VAL_T_BOOL == sign->val.type ) ?
       IupHbox( qradio, vb, NULL ) :
       IupHbox( qtext, vb, NULL ) );
  }
  else
    return NULL;
      
  IupSetAttribute( qtext_box, "ALIGNMENT", "ATOP" );
  IupSetAttribute( qtext_box, "GAP", "20" );
  IupSetAttribute( qtext_box, "MARGIN", "20x20" );

  Ihandle *qinfo = IupText( NULL );
  IupSetAttribute( qinfo, "SIZE", "280x" );
  IupSetAttribute( qinfo, "MULTILINE", "YES" );
  IupSetAttribute( qinfo, "VISIBLELINES", "5" );
  IupSetHandle( "qinfo", qinfo );
  Ihandle *info_frame = IupFrame( qinfo );
  IupSetAttribute( info_frame, "TITLE", "Info" );
  IupSetAttribute( info_frame, "EXPAND", "YES" );
  Ihandle *info_vbox = IupVbox( info_frame, NULL );
  IupSetAttribute( info_vbox, "MARGIN", "5x5" );
  IupSetAttribute( info_vbox, "EXPANDCHILDREN", "YES" );
  IupSetAttribute( info_vbox, "ALIGNMENT", "ACENTER" );
  IupSetAttribute( info_vbox, "VISIBLE", "NO" );
  IupSetHandle( "qinfovbox", info_vbox );

  //
  Ihandle *qvbox = IupVbox( qlabel, qtext_box, info_vbox, NULL );
  IupSetAttribute( qvbox, "EXPANDCHILDREN", "YES" );
  IupSetAttribute( qvbox, "ALIGNMENT", "ACENTER" );


  Ihandle *q_item_focus = IupItem ("Focus Network", NULL);
  Ihandle *q_item_web   = IupItem ("Web", NULL);
  Ihandle *q_item_info  = IupItem ("Info", NULL);
  IupSetAttribute( q_item_focus, "USERDATA", (char *) sign );
  IupSetCallback( q_item_focus, "ACTION", (Icallback) qitem_focus_cb );
  IupSetAttribute( q_item_web, "USERDATA", (char *) sign );
  IupSetCallback( q_item_web, "ACTION", (Icallback) qitem_web_cb );
  IupSetAttribute( q_item_info, "USERDATA", (char *) sign );
  IupSetCallback( q_item_info, "ACTION", (Icallback) qitem_info_cb );
  Ihandle *local_menu = IupMenu( q_item_focus, q_item_web, q_item_info, NULL );
  IupSetHandle( "qlocal_menu", local_menu );

  dlg = IupDialog( qvbox  );
  IupSetAttributes( dlg, "EXPAND = YES, TITLE = Question, RESIZE = NO" );
  IupSetAttributes( dlg, "MAXBOX = NO, MINBOX = NO" );
  IupSetAttribute( dlg, "SIZE", "300xQUARTER" );
  IupSetAttribute( dlg, "MENU", "qlocal_menu" );
  IupSetHandle( QUESTION_HANDLE, dlg );

  IupMap( dlg );

  // Value-dependent visibility
  int n;
  if( n = nxp_hash_exists( sign->str, (char *) "URL" ) ){
    IupSetAttribute( q_item_web, "ACTIVE", "YES" );
  }
  else{
    IupSetAttribute( q_item_web, "ACTIVE", "NO" );
  }    

  if( n = nxp_hash_exists( sign->str, (char *) "INFO" ) ){
    IupSetAttribute( q_item_info, "ACTIVE", "YES" );
  }
  else{
    IupSetAttribute( q_item_info, "ACTIVE", "NO" );
  }    

  if( n = nxp_hash_exists( sign->str, (char *) "VALUE" ) ){
    int qtype = q_type;
    nxp_hash_iterate( sign->str, (char *) "VALUE", choices_cb, (void *) &qtype );
    sprintf( buf, "%d", n+1 );
    IupSetAttribute( qchoices, buf, NULL );

  }
  /* else{ */
  /*   IupSetAttribute( qchoices, "VISIBLE", "NO" ); */
  /*   IupSetAttribute( qtext, "VISIBLE", "YES" ); */
  /* } */
  
  return dlg;
}

void nxpiup_question__update(){
    NXPIUP_UPDATES;
}

void nxpiup_dlgquestion( sign_rec_ptr sign, int q_type ){
  Ihandle *qlabel,
    *qtext,
    *qchoices,
    *qradio;
  char buf[NXPIUP_TEMP_BUFSIZE] = {0};
  char tmp[NXPIUP_TEMP_BUFSIZE] = {0};
  Ihandle *dlg = IupGetHandle( QUESTION_HANDLE );
  Ihandle *vdlg, *qbut;
  
  S_current_sign = sign;
  switch( q_type ){
  case NXPIUP_DLG_QUESTION:
    sprintf( buf, "What is the value of %s?", sign->str );
    if( dlg )
      IupDestroy( dlg );
    dlg = nxpiup_question__generic( buf, sign, NXPIUP_DLG_QUESTION );
    //
    nxpiup_question__update();
    IupShowXY( dlg, IUP_CENTER, IUP_CENTER );
    break;

  case NXPIUP_DLG_VOLUNTEER:
    sprintf( buf, "Volunteer the value of %s?", sign->str );
    vdlg = nxpiup_question__generic( buf, sign, NXPIUP_DLG_VOLUNTEER );
    IupPopup( vdlg, IUP_CENTER, IUP_CENTER );
    IupDestroy( vdlg );
    nxpiup_question__update();
    break;
  }
}
