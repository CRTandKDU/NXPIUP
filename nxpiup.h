#ifndef NXPIUP_H
#define NXPIUP_H

#define VERSION_NXP "2025, Neuron Data"
#define VERSION_GUI "IUP 3.32"
#define VERSION_DSL "Embed Forth VM, 2018, Richard J. Howe"
#define VERSION_LOG "NXP Architecture (40 years)\n© 2025-2026 -- %s\nGUI: %s\nDSL: %s\nNXP: %s\n"

#define NXPIUP_ENCY_HYPOS "ency_hypos"
#define NXPIUP_ENCY_SIGNS "ency_signs"
#define NXPIUP_ENCY_RULES "ency_rules"
#define NXPIUP_ENCY_HYPOS_TITLE "Hypotheses"
#define NXPIUP_ENCY_SIGNS_TITLE "Signs"
#define NXPIUP_ENCY_RULES_TITLE "Rules"
#define NXPIUP_ENCY_HYPOS_VIEW "ency_hypos_view"
#define NXPIUP_ENCY_SIGNS_VIEW "ency_signs_view"
#define NXPIUP_ENCY_RULES_VIEW "ency_rules_view"

#define NXPIUP_DLG_QUESTION 2
#define NXPIUP_DLG_VOLUNTEER 4

#define _NXP_CURRENT ((unsigned short)0xFD)

#define NXPIUP_TEMP_BUFSIZE 64

void			repl_log( const char* );
void			repl_msg( const char *, ... );
engine_state_rec_ptr	repl_getState();

int  nxpiup_inagendap( sign_rec_ptr sign );
int  item_volunteer_cb( void );
int  item_suggest_cb( void );

void		nxpiup_dlgmenu( void );
int		nxpiup_dlgloadkb( void );
void		nxpiup_dlgquestion( sign_rec_ptr, int );
void		nxpiup_dlgency( const char *, const char *, sign_rec_ptr );
void		nxpiup_dlgency_rules( const char *, const char *, rule_rec_ptr );
void		nxpiup_ency_update( Ihandle *, short );
sign_rec_ptr	nxpiup_ency_selection( char * );
void            nxpiup_netw_focus_hypo( hypo_rec_ptr hypo );

typedef void (*nxp_graph_cb_t) ( void );
Ihandle *	nxpiup_layout_dlg( const char *, nxp_graph_cb_t );
void		nxpiup_layout_add_edge( char *, char *, int, double );

#define NXPIUP_UPDATES   Ihandle *netw = IupGetHandle( "rule_network" ); \
  if( netw ) IupUpdate( netw );						\
  Ihandle *ency = IupGetHandle( NXPIUP_ENCY_SIGNS_VIEW );		\
  if( ency ){								\
    nxpiup_ency_update( ency, 0 );					\
    IupUpdate( ency );							\
  }									\
  ency = IupGetHandle( NXPIUP_ENCY_HYPOS_VIEW );			\
  if( ency ){								\
    nxpiup_ency_update( ency, 0 );					\
    IupUpdate( ency );							\
  }									\
  ency = IupGetHandle( NXPIUP_ENCY_RULES_VIEW );			\
  if( ency ){								\
    nxpiup_ency_update( ency, 1 );					\
    IupUpdate( ency );							\
  }									\
  ency = IupGetHandle( "layout_cv" );					\
  if( ency ){								\
    IupUpdate( ency );							\
  }									\
  IupLoopStep();



#endif
