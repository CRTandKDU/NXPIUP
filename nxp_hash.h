#ifndef NXP_HASH_H
#define NXP_HASH_H

typedef void (*nxp_hash_iter_t) (char *, char *, char *, char *, unsigned int, void * );

void nxp_hash_open();
void nxp_hash_close();
void nxp_hash_print();
void nxp_hash_set( char *, char *, char * );
char *nxp_hash_get( char *, char * );
int nxp_hash_exists( char *, char * );
void nxp_hash_iterate( char *, char *, nxp_hash_iter_t, void * );

#endif
