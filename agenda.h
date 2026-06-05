#ifndef AGENDA_H
#define  AGENDA_H

#include <cstdint>

#define TRACE_ON 0

struct empty {};
typedef struct empty *empty_ptr;

struct sign_rec;
typedef struct sign_rec *sign_rec_ptr;
typedef void (*sign_op) (sign_rec_ptr);
typedef sign_rec_ptr hypo_rec_ptr;

struct compound_rec;
typedef struct compound_rec *compound_rec_ptr;

struct rule_rec;
typedef struct rule_rec *rule_rec_ptr;


#define SIGN_MASK	(unsigned short)0x00
#define SIGN_UNMASK	(unsigned short)0x0F
#define COMPOUND_MASK   (unsigned short)0x20
#define COMPOUND_UNMASK (unsigned short)0x0F
#define RULE_MASK	(unsigned short)0x40
#define RULE_UNMASK	(unsigned short)0x0F
#define HYPO_MASK	(unsigned short)0x80
#define HYPO_UNMASK	(unsigned short)0x0F

#define TYPE_MASK	 (unsigned short)0xE0

#define _TRUE		 ((unsigned short)1)
#define _FALSE		 ((unsigned short)0)

#define _UNKNOWN	 ((unsigned short)0xFF)
#define _KNOWN	         ((unsigned short)0xFE)

#define _CHOP 48

char *S_val_color( unsigned short val );

//
/* ** Links from signs to either rules or compound signs */
/* The ~fwrd_rec~ structure captures two types of forward links from signs: */
/*   - From Boolean signs (including compound signs) to conditions of the rules' LHS. */
/*     Here ~rule~ points to the rule structure and ~idx_count~ indexes the target condition */
/*     starting at 0 (>= 0). */
/*   - From DSL-shared signs to compound signs they appear in. */
/*     Here ~rule~ points to the compound structure and ~idx_count~ is -1 (< 0). */

  
struct fwrd_rec {
  // Rule rec pointer
  rule_rec_ptr rule;
  int          idx_cond;
};
typedef struct fwrd_rec *fwrd_rec_ptr;

#define _VAL_T_BOOL	1
#define _VAL_T_INT	2
#define _VAL_T_FLOAT	3
#define _VAL_T_STR	4

struct val_rec{
  unsigned short status;
  unsigned short type;
  char           *valptr;
  unsigned short val_bool;
  int            val_int;
  float          val_float;
#ifdef ENGINE_DSL_HOWERJFORTH
  uint16_t       val_forth;
#endif
};


#define _SIGN_INTERNALS    sign_rec_ptr   next;      \
  struct val_rec val;		\
  unsigned short len_type;	\
  char           str[_CHOP+1];  \
  int            ngetters;	\
  empty_ptr      *getters;	\
  int            nsetters;	\
  empty_ptr      *setters;	\
  empty_ptr      client_data	\


struct sign_rec {
  _SIGN_INTERNALS;
};

struct compound_rec {
  _SIGN_INTERNALS;
  char * dsl_expression;
};

typedef void (*sign_getter_t) (sign_rec_ptr sign, int *suspend);

sign_rec_ptr sign_pushnew	( sign_rec_ptr top,
				  const char *s,
				  const int ngetters, const size_t size_getter,
				  const int nsetters, const size_t size_setter
				  );
void sign_del			( sign_rec_ptr sign );
void sign_pushgetter		( sign_rec_ptr sign, empty_ptr getr );
void sign_pushsetter		( sign_rec_ptr sign, empty_ptr setr );
unsigned short sign_get_default	( sign_rec_ptr sign );
void sign_set_default           ( sign_rec_ptr sign, struct val_rec *val );
void getter_sign                ( sign_rec_ptr sign, int *suspend );
void sign_print			( sign_rec_ptr sign );
void sign_iter			( sign_rec_ptr s0, sign_op );
sign_rec_ptr sign_find          ( const char *str, sign_rec_ptr top );
hypo_rec_ptr sign_tohypo        ( hypo_rec_ptr hypo, sign_rec_ptr top_sign, hypo_rec_ptr top_hypo );
empty_ptr sign_get_client_data  ( sign_rec_ptr sign );
void sign_set_client_data       ( sign_rec_ptr sign, empty_ptr client_data );


compound_rec_ptr compound_pushnew( sign_rec_ptr top,
				   const char *s, const int ngetters );
void compound_DSLvar_pushnew( compound_rec_ptr compound, sign_rec_ptr sign );
void compound_DSL_set( compound_rec_ptr compound, const char * expr );
void compound_del( compound_rec_ptr compound );


//
struct cond_rec {
  unsigned short in;
  unsigned short out;
  unsigned short val;
  sign_rec_ptr   sign;
};
typedef struct cond_rec *cond_rec_ptr;
#define _AS_COND_ARRAY(ptr) ((cond_rec_ptr *)(ptr))

#define _LAST_RULE(ptr)     ((ptr)->ngetters-1)
#define _LAST_COND(ptr)     ((ptr)->ngetters-1)
#define _LAST_FWRD(ptr)     ((ptr)->nsetters-1)

  
/* struct rule_rec { */
/*   rule_rec_ptr   next; */
/*   unsigned short val; */
/*   unsigned short len_type; */
/*   char           str[9]; */
/*   int            ngetters; */
/*   cond_rec_ptr   *getters; */
/*   hypo_rec_ptr   setters; */
/* }; */

struct rule_rec {
  _SIGN_INTERNALS;
  int            nrhs;
  empty_ptr      *rhs;
};

rule_rec_ptr rule_pushnew( rule_rec_ptr top,
			   const char *s, const int ngetters, hypo_rec_ptr h );
void rule_pushnewcond( rule_rec_ptr rule, unsigned short op, sign_rec_ptr sign );
void rule_pushnewrhs( rule_rec_ptr rule, char *dsl_expr );
void rule_del( rule_rec_ptr rule );
void rule_print( rule_rec_ptr rule );

//
struct bwrd_rec {
  rule_rec_ptr rule;
};
typedef struct bwrd_rec *bwrd_rec_ptr;

/* struct hypo_rec { */
/*   hypo_rec_ptr   next; */
/*   unsigned short val; */
/*   unsigned short len_type; */
/*   char           str[9]; */
/*   bwrd_rec_ptr    *getters; */
/*   struct sign_rec *setters; */
/* }; */

hypo_rec_ptr hypo_pushnew( hypo_rec_ptr top,
			   const char *s, const int ngetters );
void hypo_del( hypo_rec_ptr hypo );
void hypo_print( hypo_rec_ptr hypo );

//
/* ** The engine state */
/* The engine state is stored in a structure keeping track of the 'current sign', when a question is pending, */
/* and of the 'agenda', implemented as a stack of cells. The agenda is similar to a call stack holding two */
/* different types of tasks:   */
/*   - Backward calls: evaluate an hypothesis ('SUGGEST') or a compound sign; ~cell->val~ set to ~_UNKNOWN~. */
/*   - Forward  calls: set a value for sign ('VOLUNTEER'); ~cell->val~ set to said value. */

/* On 'backward chaining' cells are pushed to the top of the agenda. (TODO) */
/* On 'gating', cells are added to the bottom of the agenda. (See also: [[https://arxiv.org/abs/cs/0211035]].) */

struct cell_rec;
typedef struct cell_rec *cell_rec_ptr;
struct cell_rec {
  cell_rec_ptr next;
  sign_rec_ptr sign_or_hypo;
  struct val_rec val;
};

struct engine_state_rec {
  sign_rec_ptr current_sign;
  cell_rec_ptr agenda;
};
typedef struct engine_state_rec *engine_state_rec_ptr;

unsigned short  engine_sc_or( hypo_rec_ptr hypo );
unsigned short	engine_sc_and( rule_rec_ptr rule );
void		engine_forward_rule( rule_rec_ptr rule );
void		engine_forward_cond( rule_rec_ptr rule, cond_rec_ptr cond );
void		engine_forward_sign( sign_rec_ptr sign );
void		engine_backward_hypo( hypo_rec_ptr hypo, int *suspend );
void		engine_backward_compound( compound_rec_ptr compound, int *suspend );
void		engine_backward_rule( rule_rec_ptr rule, int *suspend );
void		engine_backward_cond( cond_rec_ptr cond, int *suspend );
void            engine_free_state( engine_state_rec_ptr state );
void            engine_print_state( engine_state_rec_ptr state );
void            engine_pushnew_hypo( engine_state_rec_ptr state, hypo_rec_ptr h );
void            engine_pushnew_signdata( engine_state_rec_ptr state, sign_rec_ptr sign, struct val_rec *val );
void            engine_pop( engine_state_rec_ptr state );
void            engine_knowcess( engine_state_rec_ptr state );
void            engine_resume_knowcess( engine_state_rec_ptr state );
void            engine_reset( engine_state_rec_ptr state );

// Global
typedef void (*effect)      (sign_rec_ptr, struct val_rec *);
typedef void (*effect_gate) (sign_rec_ptr, short);

void engine_register_effects( effect		f_get,
			      effect		f_set,
			      effect_gate	f_gate,
			      effect		f_push,
			      effect		f_pop,
			      effect		f_endsession);
void engine_default_on_get( sign_rec_ptr sign,  struct val_rec *val);
void engine_default_on_set( sign_rec_ptr sign,  struct val_rec *val);
void engine_default_on_gate( sign_rec_ptr sign,  short val);
void engine_default_on_agenda_push( sign_rec_ptr, struct val_rec *val);
void engine_default_on_agenda_pop( sign_rec_ptr,  struct val_rec *val);

#ifdef ENGINE_DSL
int  engine_dsl_init();
void engine_dsl_free();
int  engine_dsl_eval( const char * expr );
int  engine_dsl_rhs_eval( const char * expr );
int  engine_dsl_eval_async( const char * expr, int *err, int *suspend );
int  engine_dsl_DSLvar_declare( const char *dsl_var, sign_rec_ptr sign );
void engine_dsl_getter_compound( compound_rec_ptr compound, int *suspend );
#endif

sign_rec_ptr agenda_get_allsigns();

int		loadkb_file( const char *fn );
void		loadkb_reset();
sign_rec_ptr	loadkb_get_allsigns();
hypo_rec_ptr	loadkb_get_allhypos();
rule_rec_ptr	loadkb_get_allrules();
int		loadkb_howmany( sign_rec_ptr top );

#define ENCY_SIGN 0
#define ENCY_HYPO 1
#define ENCY_AGND 2

#endif

