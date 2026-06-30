/**
 * hypo_remote.c -- Managing '<URL>#<hypo>' special sign aliases for hypo
 *
 * Written on 2026-06-29.
 */

/* curl https://raw.githubusercontent.com/CRTandKDU/NXPIUP/main/satfault.org */


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>

#include "agenda.h"
#include "nxp_hash.h"

#include <iup.h>
#include "nxpiup.h"

#define HYPO_REMOTE_DELIMS ((char *) "|")

extern engine_state_rec_ptr repl_getState();

typedef struct
{
  char *skbid;
  int  found;
} wkb_ctx_t;

int hypo_remote_aliasp( char *s ){
  char *buf	= strdup( s );
  char *delims	= HYPO_REMOTE_DELIMS;
  char *token	= strtok( buf, delims ); // First: URL
  if( token ){
    token = strtok( NULL, delims ); // Next: Anchor
    if( token ){
      free( buf );
      return 1;
    }
  }
  free( buf );
  return 0;
}

void hypo_remote__loadkb_post_hooks(){
  Ihandle *ency;
  if( ency = IupGetHandle( NXPIUP_ENCY_HYPOS ) ){
    IupHide( ency );
    IupDestroy( ency );
  }
  if( ency = IupGetHandle( NXPIUP_ENCY_SIGNS ) ){
    IupHide( ency );
    IupDestroy( ency );
  }
  if( ency = IupGetHandle( NXPIUP_ENCY_RULES ) ){
    IupHide( ency );
    IupDestroy( ency );
  }
}

void hypo_remote__cb( char *name, char *prop, char *key, char *val, unsigned int idx, void *clientdata ){
  wkb_ctx_t *ctx = (wkb_ctx_t *) clientdata;
  if( 0 == strcmp( val, ctx->skbid ) )
    ctx->found += 1;
}

void hypo_remote_backward( sign_rec_ptr sign, int *suspend ){
  // Parse alias sign into URL and anchor hypo
  char *buf		= strdup( sign->str );
  char sURL[256]	= {0};
  char sHYP[_CHOP]	= {0};
  char *delims		= HYPO_REMOTE_DELIMS;
  char *token		= strtok( buf, delims );
  //
  printf( "HYPO_REMOTE_BWRD Testing %s\n", token );
  if( !nxp_hash_exists( token, (char *) "URL" ) ){
      // Missing remote KB URL; Handle as a sign
      if( 0 == sign->ngetters ){
	// Field getters is a pointer to the getter function
	((sign_getter_t) (sign->getters))( sign, suspend );
      }
      free( buf );
      return;
  }
  //
  printf( "HYPO_REMOTE_BWRD Has URL %s\n", token );
  strcpy( sURL, nxp_hash_get( token, (char *) "URL" ) );
  token = strtok( NULL, delims );
  strcpy( sHYP, token );
  printf( "WKB: hypo=%s in %s\n", sHYP, sURL ); 
  // Check if KB already loaded
  wkb_ctx_t ctx = {
    .skbid = sURL,
    .found = 0
  };
  nxp_hash_iterate( (char *) TOPIC_WKB, (char *) ATTR_WKB, hypo_remote__cb, (void *) &ctx );
  // Engine logics
  if( !ctx.found ){
    // Need to loadkb first
    int err = hypo_remote_get_asfile( sURL );
    if( err ){
      printf("HypoRemote curl get failed: %d\n", err );
      if( 0 == sign->ngetters ){
	// Field getters is a pointer to the getter function
	((sign_getter_t) (sign->getters))( sign, suspend );
      }
    }
    else{
      char *dup_filename;
      char buf[256];
      err = loadkb_file( (char *) "remotekb.org", LOADKB_CUMULATE );
      printf("HypoRemote loadkb: %d\n", err );
      sprintf( buf, "[KB] Loaded KB: %s - %s", sURL, err ? "Failed" : "OK" );
      repl_log( buf );
      dup_filename = strdup( sURL );
      nxp_hash_set( (char *) TOPIC_WKB, (char *) ATTR_WKB, dup_filename );
      hypo_remote__loadkb_post_hooks();
    }
  }
  // KB has been loaded before
  hypo_rec_ptr hypo = (hypo_rec_ptr) sign_find( sHYP, loadkb_get_allhypos() );
  if( hypo ){
    if( _UNKNOWN == hypo->val.status ){
      engine_pushnew_hypo( repl_getState(), hypo );
      engine_backward_hypo( hypo, suspend );
    }
    else{
      struct val_rec val;
      val.status = _KNOWN;
      val.type   = sign->val.type; // Should be _VAL_T_BOOL
      val.val_bool = hypo->val.val_bool;
      sign_set_default( sign, &val );
    }
  }
  else{
    // Handle as a sign
    if( 0 == sign->ngetters ){
      // Field getters is a pointer to the getter function
      ((sign_getter_t) (sign->getters))( sign, suspend );
    }
  }
  free( buf );
}
