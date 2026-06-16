#ifndef NXP_EVOKE_H
#define NXP_EVOKE_H

void		evoke_init();
void		evoke_free();
int             evoke_notemptyp();
void		evoke_push( sign_rec_ptr );
cell_rec_ptr	evoke_pop();
void		evoke_switch_agenda( engine_state_rec_ptr );


#endif
