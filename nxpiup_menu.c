/**
 * nxpiup_menu.c -- Main operations menu
 *
 * Written on 2026-05-06.
 */
/*
TODO:
  - EVOKES zhash annotation and external agendas in engine, rule network and graph
  - DONE Volunteer, reusing questions
  - right click contextual menus
  - Web comms
  - LLM comms
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

// Loading knowledge bases. (Specially formatted Org-mode files.)
int nxpiup_dlgloadkb( void ){
  int res = 0;
  Ihandle *dlg = IupFileDlg(); 

  IupSetAttribute(dlg, "DIALOGTYPE", "OPEN");
  IupSetAttribute(dlg, "TITLE", "Load Knowledge Base");
  IupSetAttributes(dlg, "FILTER = \"*.org\", FILTERINFO = \"Org-mode Files\"");
  /* IupSetCallback(dlg, "HELP_CB", (Icallback)help_cb); */

  IupPopup(dlg, IUP_CURRENT, IUP_CURRENT); 

  if (IupGetInt(dlg, "STATUS") != -1)
    {
      printf("OK\n");
      printf("  VALUE(%s)\n", IupGetAttribute(dlg, "VALUE"));
      res = loadkb_file( IupGetAttribute(dlg, "VALUE"), LOADKB_OVERWRITE );
      char buf[64];
      sprintf( buf, "[KB] Loaded KB: %s - %s", IupGetAttribute(dlg, "VALUE"), res ? "Failed" : "OK" );
      repl_log( buf );
      nxp_hash_print();
    }
  else
    printf("CANCEL\n");

  IupDestroy(dlg);
  return res;
}

// Main window w. menubar
int exit_cb(void) {
  Ihandle *ih = IupGetHandle( "graph" );
  if( ih ) IupDestroy( ih );
  ih = IupGetHandle( "rule_network" );
  if( ih ) IupDestroy( ih );
  return IUP_CLOSE;
}

static void build_graph_cb(){
  short i, j;
  int k = 0;
  rule_rec_ptr rule;
  cond_rec_ptr cond;
  fwrd_rec_ptr cfwrd, fwrd;
  float w		= IupGetFloat( IupGetHandle( "layout_weight" ), "VALUE" );
  double _BASEWEIGHT	= 1.0;
  double _OVERLAYWEIGHT = _BASEWEIGHT * (2.*w + 1.);
  // All hypos to signs in their respective rules
  sign_rec_ptr compound, s, top = (sign_rec_ptr) loadkb_get_allhypos();
  s = top;
  while( s ){
    for( i=0; i<s->ngetters; i++ ){
      rule = ((bwrd_rec_ptr) s->getters[i])->rule;
      for( j=0; j<rule->ngetters; j++ ){
	cond = (cond_rec_ptr) rule->getters[j];
	if( COMPOUND_MASK != (cond->sign->len_type & TYPE_MASK ) )
	    nxpiup_layout_add_edge( s->str, cond->sign->str, k++, _BASEWEIGHT );
      }
    }
    s = s->next;
  }
  // All sign and their forward links
  top = loadkb_get_allsigns();
  s = top;
  while( s ){
    if( COMPOUND_MASK != (s->len_type & TYPE_MASK ) ){
      for( i=0; i<s->nsetters; i++ ){
	fwrd = (fwrd_rec_ptr) s->setters[i];
	if( fwrd->idx_cond >= 0 ){
	  nxpiup_layout_add_edge( ((sign_rec_ptr) fwrd->rule->setters)->str, s->str, k++, _OVERLAYWEIGHT );
	}
	else{
	  compound = (sign_rec_ptr) fwrd->rule;
	  for( j=0; j<compound->nsetters; j++ ){
	    cfwrd = (fwrd_rec_ptr) compound->setters[j];
	    if( cfwrd->idx_cond >= 0 ){
	      nxpiup_layout_add_edge( ((sign_rec_ptr) cfwrd->rule->setters)->str, s->str, k++, _OVERLAYWEIGHT );
	    }
	  }
	}
      }
    }
    s = s->next;
  }
}

int item_graph_cb(void){
  Ihandle *dlg = IupGetHandle( "graph" );
  if( !dlg )
    dlg = nxpiup_layout_dlg( "600x400", build_graph_cb );
  IupShowXY(dlg, IUP_CENTER, IUP_CENTER);

  return IUP_DEFAULT;
}

int item_browse_cb(void){
  Ihandle *ih_item = IupGetHandle( "item_browse" );
  IupSetAttribute( ih_item, "ACTIVE", "NO" );
  CanvasScrollbarTest();
  return IUP_DEFAULT;
}

int item_open_cb( void ){
  int res = nxpiup_dlgloadkb();
  return IUP_DEFAULT;
}

int item_reset_cb( void ){
  engine_reset( repl_getState() );
  repl_log( "[SESSION] Reset\n" );
  NXPIUP_UPDATES;
  return IUP_DEFAULT;
}

int item_volunteer_cb( void ){
  sign_rec_ptr sign = nxpiup_ency_selection( (char *) NXPIUP_ENCY_SIGNS_VIEW );
  if( sign ){
    nxpiup_dlgquestion( sign, NXPIUP_DLG_VOLUNTEER );
  }
  return IUP_DEFAULT;
}

int item_suggest_cb( void ){
  hypo_rec_ptr hypo = (hypo_rec_ptr) nxpiup_ency_selection( (char *) NXPIUP_ENCY_HYPOS_VIEW );
  if( hypo ){
    int ret;
    Ihandle* dlg = IupMessageDlg();
    IupSetAttribute(dlg, "DIALOGTYPE", "QUESTION");
    IupSetAttribute(dlg, "TITLE", "Confirm SUGGEST");
    IupSetAttribute(dlg, "BUTTONS", "OKCANCEL");
    IupSetAttribute(dlg, "VALUE", hypo->str );
    /* IupSetCallback(dlg, "HELP_CB", (Icallback)help_cb); */
    IupPopup(dlg, IUP_CURRENT, IUP_CURRENT);
    ret = atoi( IupGetAttribute(dlg, "BUTTONRESPONSE") );
    printf("BUTTONRESPONSE(%d)\n", ret );
    IupDestroy(dlg);
    if( 1 == ret ){
      engine_pushnew_hypo( repl_getState(), hypo );
      repl_msg( "[SESSION] Suggest %s", hypo->str );
    }
  }
  return IUP_DEFAULT;
}

int item_knowcess_cb( void ){
  //
  Ihandle *ih_item = IupGetHandle( "item_knowcess" );
  IupSetAttribute( ih_item, "ACTIVE", "NO" );
  // For tests
  /* hypo_rec_ptr h = (hypo_rec_ptr)sign_find( "POSSIBLE_LEAK", loadkb_get_allhypos() ); */
  /* engine_pushnew_hypo( repl_getState(), h ); */
  /* repl_msg( "[SESSION] Suggest %s", h->str ); */
  //
  engine_resume_knowcess( repl_getState() );
  
  return IUP_DEFAULT;
}

int item_hypos_cb( void ){
  nxpiup_dlgency( NXPIUP_ENCY_HYPOS_TITLE, NXPIUP_ENCY_HYPOS, (sign_rec_ptr) loadkb_get_allhypos() );
  return IUP_DEFAULT;
}

int item_signs_cb( void ){
  nxpiup_dlgency( NXPIUP_ENCY_SIGNS_TITLE, NXPIUP_ENCY_SIGNS, (sign_rec_ptr) loadkb_get_allsigns() );
  return IUP_DEFAULT;
}

int item_rules_cb( void ){
  nxpiup_dlgency_rules( NXPIUP_ENCY_RULES_TITLE, NXPIUP_ENCY_RULES, (rule_rec_ptr) loadkb_get_allrules() );
  return IUP_DEFAULT;
}

void nxpiup_dlgmenu( void ){
  Ihandle *item_open, *item_exit,
    *item_suggest, *item_volunteer, *item_reset, *item_agenda, *item_knowcess,
    *item_rules, *item_signs, *item_hypos,
    *item_browse, *item_graph;
  Ihandle *file_menu, *edit_menu, *expert_menu, *ency_menu, *netw_menu;
  Ihandle *menu, *sub1, *sub3, *sub4, *sub5;

  item_open = IupItem ("&Load KB...", NULL);

  IupSetCallback(item_open, "ACTION", (Icallback)item_open_cb);
  item_exit = IupItem ("Quit", NULL);
  IupSetAttribute(item_exit, "KEY", "Q");
  IupSetCallback(item_exit, "ACTION", (Icallback)exit_cb);
  //
  file_menu = IupMenu(item_open, IupSeparator(), item_exit, NULL);
  //
  item_suggest		= IupItem ("Suggest", NULL);
  IupSetAttribute(item_suggest, "KEY", "S");
  IupSetCallback(item_suggest, "ACTION", (Icallback)item_suggest_cb);
  item_volunteer	= IupItem ("Volunteer", NULL);
  IupSetAttribute(item_volunteer, "KEY", "V");
  IupSetCallback(item_volunteer, "ACTION", (Icallback)item_volunteer_cb);
  item_reset		= IupItem ("Reset", NULL);
  IupSetAttribute(item_reset, "KEY", "Z");
  IupSetCallback(item_reset, "ACTION", (Icallback)item_reset_cb);
  item_agenda		= IupItem ("Agenda", NULL);
  IupSetAttribute(item_agenda, "KEY", "A");
  item_knowcess		= IupItem ("Knowcess", NULL);
  IupSetAttribute(item_knowcess, "KEY", "K");
  IupSetCallback(item_knowcess, "ACTION", (Icallback)item_knowcess_cb);
  IupSetHandle( "item_knowcess", item_knowcess );
  //
  expert_menu = IupMenu( item_suggest, item_volunteer, item_reset,
			 IupSeparator(), item_agenda, item_knowcess, NULL );
  //
  item_rules = IupItem ("Rules", NULL);
  IupSetAttribute(item_rules, "KEY", "R");
  IupSetCallback(item_rules, "ACTION", (Icallback)item_rules_cb);
  item_signs = IupItem ("Signs", NULL);
  IupSetAttribute(item_signs, "KEY", "G");
  IupSetCallback(item_signs, "ACTION", (Icallback)item_signs_cb);
  item_hypos = IupItem ("Hypotheses", NULL);
  IupSetAttribute(item_hypos, "KEY", "H");
  IupSetCallback(item_hypos, "ACTION", (Icallback)item_hypos_cb);
  //
  ency_menu = IupMenu( item_rules, item_signs, item_hypos, NULL );
  //
  item_graph  = IupItem( "Signs Map", NULL );
  /* IupSetAttribute(item_browse, "KEY", "B"); */
  IupSetCallback(item_graph, "ACTION", (Icallback)item_graph_cb);
  IupSetHandle( "item_graph", item_graph );
  item_browse = IupItem ("Rules Network", NULL);
  IupSetAttribute(item_browse, "KEY", "B");
  IupSetCallback(item_browse, "ACTION", (Icallback)item_browse_cb);
  IupSetHandle( "item_browse", item_browse );
  //
  netw_menu = IupMenu( item_graph, item_browse, NULL );
  //
  sub1 = IupSubmenu( "File", file_menu );
  sub3 = IupSubmenu( "Expert", expert_menu );
  sub4 = IupSubmenu( "Encyclopedia", ency_menu );
  sub5 = IupSubmenu( "Networks", netw_menu );
  menu = IupMenu( sub1, sub3, sub4, sub5, NULL );
  IupSetHandle("mymenu", menu);
  //
  char log_version[192];
  sprintf( log_version, VERSION_LOG, __DATE__, VERSION_GUI, VERSION_DSL, VERSION_LIB, VERSION_NXP );
  Ihandle *log = IupMultiLine( NULL );
  IupSetAttribute( log, "READONLY", "YES" );
  IupSetAttribute( log, "VISIBLELINES", "20" );
  IupSetAttribute( log, "VISIBLECOLUMNS", "32" );
  IupSetAttribute( log, "EXPAND", "YES" );
  IupSetAttribute( log, "VALUE", log_version );
  IupSetHandle( "ih_log", log );
  /* Ihandle *vbox_log = IupVbox( log, NULL ); */
  /* IupSetAttribute( vbox_log, "EXPANDCHILDREN", "YES" );  */

  Ihandle *dlg = IupDialog( log );
  IupSetAttribute( dlg, "MENU", "mymenu" );
  IupSetAttribute( dlg, "TITLE", "NXPIUP");
  IupSetAttribute( dlg, "EXPAND", "YES");
  IupSetAttribute( dlg, "SIZE", "HALFxHALF" );

  IupShow(dlg);
}

