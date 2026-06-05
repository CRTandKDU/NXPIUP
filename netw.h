#ifndef NETW_H
#define NETW_H

struct col_rec;
typedef struct col_rec col_rec, *col_rec_ptr;

struct netw_cell_rec;
typedef struct netw_cell_rec netw_cell_rec, *netw_cell_rec_ptr;

struct col_rec {
  short x;
  col_rec_ptr  next;
  netw_cell_rec_ptr first;
};

#define _NEW_COL ((col_rec_ptr) malloc(sizeof( struct col_rec )))

#define _NETW_STR_T  1
#define _NETW_SIGN_T 2
#define _NETW_RULE_T 4
#define _NETW_JUNCTION_T 8
#define _NETW_SIGN_YES_T 16
#define _NETW_SIGN_NO_T 32

struct netw_cell_rec {
  short y;
  col_rec_ptr  head;
  netw_cell_rec_ptr next;
  //
  unsigned short expanded;
  unsigned short client_data_t;
  void *client_data;
  short nleft;
  netw_cell_rec_ptr *left;
  short nright;
  netw_cell_rec_ptr *right;
};

#define _NEW_CELL ((netw_cell_rec_ptr) malloc(sizeof( struct netw_cell_rec )))

#define CELL_W 200
#define CELL_H 20

void netw_adjust_horz( col_rec_ptr, int );
void netw_adjust_vert( cdCanvas *, int );
void netw_get_cell_size( int *, int * );

int  netw_click( cdCanvas *, int, int, int, int, int, double, double, unsigned short );
void netw_initfill_all( cdCanvas *, double, double, unsigned short );
void netw_free( cdCanvas * );
void netw_redrawkb( cdCanvas *, int, double, double, unsigned short );

void CanvasScrollbarTest( void );

#define NETW_LR ((unsigned short) 1)
#define NETW_RL ((unsigned short) 2)
#define _EXP_RL_P(cell) (((cell)->expanded) & NETW_RL)
#define _EXP_LR_P(cell) (((cell)->expanded) & NETW_LR)
#define _EXP_RL_SET(cell) ((cell)->expanded) |= NETW_RL
#define _EXP_LR_SET(cell) ((cell)->expanded) |= NETW_LR
#define _EXP_RL_RESET(cell) ((cell)->expanded) &= ~ NETW_RL
#define _EXP_LR_RESET(cell) ((cell)->expanded) &= ~ NETW_LR

static int WORLD_W = 6000;
static int WORLD_H = 2000;

#endif
