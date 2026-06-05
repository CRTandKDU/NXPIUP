#ifndef NETW_INTERNALS_H
#define NETW_INTERNALS_H

#define _NETW_LINE_STYLE_BWRD CD_CONTINUOUS
#define _NETW_LINE_STYLE_FWRD CD_DOTTED
#define _NETW_LINE_COLOR_BWRD CD_BLACK
#define _NETW_LINE_COLOR_FWRD CD_BLACK

#define  _NETW_NEWCELL(cptr)       (cptr) = _NEW_CELL; \
      (cptr)->y	= 0; \
      (cptr)->head = NULL; \
      (cptr)->next = NULL; \
      (cptr)->expanded = (unsigned short)0; \
      (cptr)->client_data_t = 0;\
      (cptr)->client_data = (void *)0;\
      (cptr)->nleft = 0; \
      (cptr)->left = NULL; \
      (cptr)->nright = 0; \
      (cptr)->right = NULL;

#define _NETW_INFINITY 100000
#define _NETW_TEMP_BUFSIZE 64

// Utilities: node labels, column/cell management
void		netw__trace( col_rec_ptr );
int		netw__trim( const char * , cdCanvas * );
void		netw__col_append_cell( col_rec_ptr, netw_cell_rec_ptr );
void		netw__col_unlink_cell( col_rec_ptr, netw_cell_rec_ptr );
int		netw__col_ymax_cell( col_rec_ptr, col_rec_ptr, netw_cell_rec_ptr * );
int		netw__col_ymin_cell( col_rec_ptr, col_rec_ptr, netw_cell_rec_ptr * );
col_rec_ptr	netw__col_get_create( cdCanvas *, int );
void		netw__text( cdCanvas *, netw_cell_rec_ptr, char *, int * );
char *          netw__client_text( netw_cell_rec_ptr );
void            netw__adjust_col_vert( netw_cell_rec_ptr, int );

// Expanding backward and forward
void netw__expand_forward(  cdCanvas *, netw_cell_rec_ptr,
			    double, double, unsigned short );
void netw__expand_backward(  cdCanvas *, netw_cell_rec_ptr,
			    double, double, unsigned short );
void netw__recursive_remove_backward( cdCanvas *, netw_cell_rec_ptr, unsigned short );
void netw__recursive_remove_forward ( cdCanvas *, netw_cell_rec_ptr, unsigned short );
void netw__toggle_expand( cdCanvas *, netw_cell_rec_ptr, int,
			  double, double, unsigned short );

// Redrawing at scale
void netw__redraw_onscale( cdCanvas *, int, double, double, unsigned short );

#endif
