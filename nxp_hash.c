/**
 * nxp_hash.c -- Extending the NXP Architecture
 *
 * Written on 2026-05-13.
 */
#include <stdio.h>
#include <stdlib.h>
#include <cstring>

#include "src/zhash.h"
#include "nxp_hash.h"

typedef struct ZHashTable       *zhash_ptr;

static zhash_ptr  S_BigHash = NULL;

void nxp_hash_print(){
  printf( "BIGHASH %d entries\n", S_BigHash->entry_count );
}

void nxp_hash__free_entry( struct ZHashEntry* entry ){
  if( '|' != entry->key[ strlen(entry->key) - 1 ] ){
    printf( "BigHash Freeing %s\n", entry->val );
    free( entry->val );
  }
}

void nxp_hash__free( zhash_ptr bighash ){
  nxp_zfree_hash_table( bighash, nxp_hash__free_entry );
}

void nxp_hash_open(){
  nxp_hash_close();
  S_BigHash = zcreate_hash_table();
}
  
void nxp_hash_close(){
  if( S_BigHash ) nxp_hash__free( S_BigHash );
}

void nxp_hash_set( char *name, char *key, char *val ){
  char zkey[128];
  unsigned long long int  n;
  sprintf( zkey, "%s%s|", name, key );
  if( zhash_exists( S_BigHash, zkey ) ){
    n = (unsigned long long int) zhash_get( S_BigHash, zkey );
    zhash_set( S_BigHash, zkey, (void *) (n+1) );
    sprintf( zkey, "%s%s%d", name, key, n+1 );
    zhash_set( S_BigHash, zkey, (void *) val );
  }
  else{
    zhash_set( S_BigHash, zkey, (void *) 1 );
    sprintf( zkey, "%s%s1", name, key );
    zhash_set( S_BigHash, zkey, (void *) val );
  }
}

int nxp_hash_exists( char *name, char *key ){
  char zkey[128];
  unsigned long long int  n = 0;
  sprintf( zkey, "%s%s|", name, key );
  if( zhash_exists( S_BigHash, zkey ) )
    n = (unsigned long long int) zhash_get( S_BigHash, zkey );
  return (int) n;
}

void nxp_hash_iterate( char *name, char *key, nxp_hash_iter_t f, int context ){
  char zkey[128];
  unsigned int i;
  unsigned long long int  n;
  sprintf( zkey, "%s%s|", name, key );
  if( zhash_exists( S_BigHash, zkey ) ){
    n = (unsigned long long int) zhash_get( S_BigHash, zkey );
    for( i=1; i<=n; i++ ){
      sprintf( zkey, "%s%s%d", name, key, i );
      f( name, key, zkey, (char *) zhash_get( S_BigHash, zkey ), i, context );
    }
  }  
}
