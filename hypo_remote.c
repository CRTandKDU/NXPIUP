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

extern engine_state_rec_ptr repl_getState();

typedef struct
{
  char *skbid;
  int  found;
} wkb_ctx_t;

int hypo_remote_aliasp( char *s ){
  char *buf = strdup( s );
  char *delims = (char *) "#";
  char *token = strtok( buf, delims ); // First: URL
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
  char *delims		= (char *) "#";
  char *token		= strtok( buf, delims );
  strcpy( sURL, token );
  token = strtok( NULL, delims );
  strcpy( sHYP, token );
  free( buf );
  printf( "WKB: hypo=%s in %s\n", sHYP, sURL ); 
  // Check if KB already loaded
  wkb_ctx_t ctx = {
    .skbid = sURL,
    .found = 0
  };
  nxp_hash_iterate( (char *) TOPIC_WKB, (char *) ATTR_WKB, hypo_remote__cb, (void *) &ctx );
  // Engine logics
  if( ctx.found ){
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
  }
  else{
    // Need to loadkb first
    // TODO
  }
}
