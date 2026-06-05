/**
 * engine_dsl.c -- Integrating a DSL for compound signs
 *
 * Written on jeudi, 29 mai 2025.
 */


/* #define ENGINE_DSL */
#include <termios.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <ctype.h>
#include <stdio.h>
#include <stddef.h>

#include "agenda.h"

extern void  repl_log( const char *s );

#ifdef ENGINE_DSL_HOWERJFORTH
#include "embed.h"
#include "util.h"
#include <csv.h>

#define _MARSHALL_BUFLEN 80

struct vm_extension_t;
typedef struct vm_extension_t vm_extension_t;

typedef float vm_float_t;
typedef int32_t sdc_t;   /**< signed double cell type */

typedef int (*embed_callback_extended_t)(vm_extension_t *v);
typedef struct {
  const char *name;             /**< Forth function */
  embed_callback_extended_t cb; /**< Callback for function */
  bool use;                     /**< Use this callback? */
} callbacks_t;

struct vm_extension_t {
  embed_t *h;              /**< embed VM instance we are operating with */
  callbacks_t *callbacks;  /**< callbacks to use with this instance */
  size_t callbacks_length; /**< length of 'callbacks' field */
  embed_opt_t o;           /**< embed virtual machine options */
  cell_t error;            /**< current error condition */

  int *suspend;
};

#define CALLBACK_XMACRO				\
  X("d+",       cb_dplus,      false)		\
  X("d*",       cb_dmul,       false)		\
  X("d.",       cb_dprint,     false)		\
  X("d-",       cb_dsub,       false)		\
  X("d/",       cb_ddiv,       false)		\
  X("d<",       cb_dless,      false)		\
  X("d>",       cb_dmore,      false)		\
  X("d=",       cb_dequal,     false)		\
  X("dnegate",  cb_dnegate,    false)		\
  X("f.",       cb_flt_print,  true)		\
  X("f+",       cb_fadd,       true)		\
  X("f-",       cb_fsub,       true)		\
  X("f*",       cb_fmul,       true)		\
  X("f/",       cb_fdiv,       true)		\
  X("f>d",      cb_f2d,        true)		\
  X("f<",       cb_fless,      true)		\
  X("f>",       cb_fmore,      true)		\
  X("fdup",     cb_fdup,       true)		\
  X("fswap",    cb_fswap,      true)		\
  X("fdrop",    cb_fdrop,      true)		\
  X("fover",    cb_fover,      true)		\
  X("fnip",     cb_fnip,       true)		\
  X("s>f",      cb_s2f,        true)		\
  X("f>s",      cb_f2s,        true)		\
  X("fsin",     cb_fsin,       true)		\
  X("fcos",     cb_fcos,       true)		\
  X("ftan",     cb_ftan,       true)		\
  X("fasin",    cb_fasin,      true)		\
  X("facos",    cb_facos,      true)		\
  X("fatan",    cb_fatan,      true)		\
  X("fatan2",   cb_fatan2,     true)		\
  X("flog",     cb_flog,       true)		\
  X("flog10",   cb_flog10,     true)		\
  X("fpow",     cb_fpow,       true)		\
  X("fexp",     cb_fexp,       true)		\
  X("fsqrt",    cb_fsqrt,      true)		\
  X("fget",     cb_fget,       true)		\
  X("floor",    cb_floor,      true)		\
  X("fceil",    cb_fceil,      true)		\
  X("fround",   cb_fround,     true)		\
  X("fabs",     cb_fabs,       true)		\
  X("ferfc",    cb_ferfc,      false)		\
  X("ferf",     cb_ferf,       false)		\
  X("flgamma",  cb_flgamma,    false)		\
  X("ftgamma",  cb_ftgamma,    false)		\
  X("fmin",     cb_fmin,       true)		\
  X("fmax",     cb_fmax,       true)		\
  \
  X("nxp@",     cb_nxpget_async,     true)      \
  X("nxp!",     cb_nxpset,     true)            \
  X("nxp_show", cb_nxpshow,    true)            \
  X("nxp_csv@", cb_nxpcsv_r,   true)            \
  X("nxp_csv!", cb_nxpcsv_w,   true)            \
  X("slog",     cb_nxpslog,    true)            \

#define X(NAME, FUNCTION, USE) static int FUNCTION ( vm_extension_t * const v );
CALLBACK_XMACRO
#undef X

static callbacks_t callbacks[] = {
#define X(NAME, FUNCTION, USE) { .name = NAME, .cb = FUNCTION, .use = USE },
  CALLBACK_XMACRO
#undef X
};

static vm_extension_t *S_v;

static inline size_t number_of_callbacks(void) { return sizeof(callbacks) / sizeof(callbacks[0]); }

static inline cell_t eset(vm_extension_t * const v, const cell_t error) { /**< set error register if not set */
  assert(v);
  if (!(v->error))
    v->error = error;
  return v->error;
}

static inline cell_t eget(vm_extension_t const * const v) { /**< get current error register */
  assert(v);
  return v->error;
}

static inline cell_t eclr(vm_extension_t * const v) { /**< clear error register and return value before clear */
  assert(v);
  const cell_t error = v->error;
  v->error = 0;
  return error;
}

static inline cell_t pop(vm_extension_t *v) {
  assert(v);
  if (eget(v))
    return 0;
  cell_t rv = 0;
  int e = 0;
  if ((e = embed_pop(v->h, &rv)) < 0)
    eset(v, e);
  return rv;
}

static inline void push(vm_extension_t * const v, const cell_t value) {
  assert(v);
  if (eget(v))
    return;
  int e = 0;
  if ((e = embed_push(v->h, value)) < 0)
    eset(v, e);
}

static inline void udpush(vm_extension_t * const v, const double_cell_t value) {
  push(v, value);
  push(v, value >> 16);
}

static inline double_cell_t udpop(vm_extension_t * const v) {
  const double_cell_t hi = pop(v);
  const double_cell_t lo = pop(v);
  const double_cell_t d  = (hi << 16) | lo;
  return d;
}

static inline sdc_t dpop(vm_extension_t * const v)                     { return udpop(v); }
static inline void  dpush(vm_extension_t * const v, const sdc_t value) { udpush(v, value); }

typedef union { vm_float_t f; double_cell_t d; } fd_u;

static inline vm_float_t fpop(vm_extension_t * const v) {
  BUILD_BUG_ON(sizeof(vm_float_t) != sizeof(double_cell_t));
  const fd_u fd = { .d = udpop(v) };
  return fd.f;
}

static inline void fpush(vm_extension_t * const v, const vm_float_t f) {
  const fd_u fd = { .f = f };
  udpush(v, fd.d);
}

static int cb_dplus(vm_extension_t * const v) {
  dpush(v, dpop(v) + dpop(v));
  return eclr(v);
}

static int cb_dmul(vm_extension_t * const v) {
  dpush(v, dpop(v) * dpop(v));
  return eclr(v);
}

static int cb_dsub(vm_extension_t * const v) {
  const sdc_t d1 = dpop(v);
  const sdc_t d2 = dpop(v);
  dpush(v, d2 - d1);
  return eclr(v);
}

static int cb_ddiv(vm_extension_t * const v) {
  const sdc_t d1 = dpop(v);
  const sdc_t d2 = dpop(v);
  if (!d1) {
    eset(v, 10); /* division by zero */
    return eclr(v);
  }
  dpush(v, d2 / d1);
  return eclr(v);
}

static int cb_dnegate(vm_extension_t * const v) {
  dpush(v, -dpop(v));
  return eclr(v);
}

static int cb_dless(vm_extension_t * const v) {
  const sdc_t d1 = dpop(v);
  const sdc_t d2 = dpop(v);
  push(v, -(d2 < d1));
  return eclr(v);
}

static int cb_dmore(vm_extension_t * const v) {
  const sdc_t d1 = dpop(v);
  const sdc_t d2 = dpop(v);
  push(v, -(d2 > d1));
  return eclr(v);
}

static int cb_dequal(vm_extension_t * const v) {
  push(v, -(dpop(v) == dpop(v)));
  return eclr(v);
}

static int cb_dprint(vm_extension_t * const v) {
  const long d = dpop(v);
  if (eget(v))
    return eclr(v);
  char buf[80] = { 0 };
  snprintf(buf, sizeof(buf)-1, "%ld", d); /**@bug does not respect eForth base */
  embed_puts(v->h, buf);
  return eclr(v);
}

static int cb_flt_print(vm_extension_t * const v) {
  const vm_float_t flt = fpop(v);
  char buf[512] = { 0 }; /* floats can be quite large */
  if (eget(v))
    return eclr(v);
  snprintf(buf, sizeof(buf)-1, "%e", flt);
  embed_puts(v->h, buf);
  return eclr(v);
}

static int cb_fadd(vm_extension_t * const v) {
  fpush(v, fpop(v) + fpop(v));
  return eclr(v);
}

static int cb_fmul(vm_extension_t * const v) {
  fpush(v, fpop(v) * fpop(v));
  return eclr(v);
}

static int cb_fsub(vm_extension_t * const v) {
  const vm_float_t f1 = fpop(v);
  const vm_float_t f2 = fpop(v);
  fpush(v, f2 - f1);
  return eclr(v);
}

static int cb_fdiv(vm_extension_t * const v) {
  const vm_float_t f1 = fpop(v);
  const vm_float_t f2 = fpop(v);
  if (f1 == 0.0f) {
    eset(v, 42); /* floating point division by zero */
    return eclr(v);
  }
  fpush(v, f2 / f1);
  return eclr(v);
}

/*// 'd>f' would need take into account the 'dpl' variable
  static int cb_d2f(vm_extension_t * const v) {
  fpush(v, dpop(v));
  return eclr(v);
  }*/

static int cb_f2d(vm_extension_t * const v) {
  dpush(v, fpop(v));
  return eclr(v);
}

static int cb_fless(vm_extension_t * const v) {
  const vm_float_t f1 = fpop(v);
  const vm_float_t f2 = fpop(v);
  push(v, -(f2 < f1));
  return eclr(v);
}

static int cb_fmore(vm_extension_t * const v) {
  const vm_float_t f1 = fpop(v);
  const vm_float_t f2 = fpop(v);
  push(v, -(f2 > f1));
  return eclr(v);
}

static int cb_fdup(vm_extension_t * const v) {
  const vm_float_t f = fpop(v);
  fpush(v, f);
  fpush(v, f);
  return eclr(v);
}

static int cb_fswap(vm_extension_t * const v) {
  const vm_float_t f1 = fpop(v);
  const vm_float_t f2 = fpop(v);
  fpush(v, f1);
  fpush(v, f2);
  return eclr(v);
}

static int cb_fdrop(vm_extension_t * const v) {
  fpop(v);
  return eclr(v);
}

static int cb_fnip(vm_extension_t * const v) {
  const vm_float_t f1 = fpop(v);
  fpop(v);
  fpush(v, f1);
  return eclr(v);
}

static int cb_fover(vm_extension_t * const v) {
  const vm_float_t f1 = fpop(v);
  const vm_float_t f2 = fpop(v);
  fpush(v, f2);
  fpush(v, f1);
  fpush(v, f2);
  return eclr(v);
}

static int cb_s2f(vm_extension_t * const v) {
  int16_t i = pop(v);
  fpush(v, i);
  return eclr(v);
}

static int cb_f2s(vm_extension_t * const v) {
  push(v, (int16_t)fpop(v));
  return eclr(v);
}

static int cb_fsin(vm_extension_t * const v) {
  fpush(v, sinf(fpop(v)));
  return eclr(v);
}

static int cb_fcos(vm_extension_t * const v) {
  fpush(v, cosf(fpop(v)));
  return eclr(v);
}

static int cb_ftan(vm_extension_t * const v) {
  fpush(v, tanf(fpop(v)));
  return eclr(v);
}

static int cb_fasin(vm_extension_t * const v) {
  fpush(v, asinf(fpop(v)));
  return eclr(v);
}

static int cb_facos(vm_extension_t * const v) {
  fpush(v, acosf(fpop(v)));
  return eclr(v);
}

static int cb_fatan(vm_extension_t * const v) {
  fpush(v, atanf(fpop(v)));
  return eclr(v);
}

static int cb_fexp(vm_extension_t * const v) {
  fpush(v, expf(fpop(v)));
  return eclr(v);
}

static int cb_fatan2(vm_extension_t * const v) {
  const vm_float_t f1 = fpop(v);
  const vm_float_t f2 = fpop(v);
  fpush(v, atan2f(f1, f2));
  return eclr(v);
}

static int cb_fpow(vm_extension_t * const v) {
  const vm_float_t f1 = fpop(v);
  const vm_float_t f2 = fpop(v);
  fpush(v, powf(f1, f2));
  return eclr(v);
}

static int cb_fsqrt(vm_extension_t * const v) {
  const vm_float_t f = fpop(v);
  if (f < 0.0f)
    return eset(v, 43);
  fpush(v, sqrtf(f));
  return eclr(v);
}

static int cb_flog(vm_extension_t * const v) {
  const vm_float_t f = fpop(v);
  if (f <= 0.0f)
    return eset(v, 43);
  fpush(v, logf(f));
  return eclr(v);
}

static int cb_flog10(vm_extension_t * const v) {
  const vm_float_t f = fpop(v);
  if (f <= 0.0f)
    return eset(v, 43);
  fpush(v, log10f(f));
  return eclr(v);
}

static int get_a_char(vm_extension_t * const v) {
  embed_fgetc_t get = v->o.get;
  void *getp = v->o.in;
  int ch, no_data = 0;
  do { ch = get(getp, &no_data); } while (no_data);
  return ch;
}

static int cb_fget(vm_extension_t * const v) {
  char buf[512] = { 0 };
  int ch = 0;
  vm_float_t f = 0.0;

  while (isspace(ch = get_a_char(v)))
    ;

  if (ch == EOF)
    return 57;

  buf[0] = ch;

  for (size_t i = 1; i < (sizeof(buf)-1); i++) {
    if ((ch = get_a_char(v)) == EOF)
      return 57;
    if (isspace(ch))
      break;
    buf[i] = ch;
  }

  if (sscanf(buf, "%f", &f) != 1)
    return 13;

  fpush(v, f);

  return eclr(v);
}

static int cb_fround(vm_extension_t * const v) {
  fpush(v, roundf(fpop(v)));
  return eclr(v);
}

static int cb_floor(vm_extension_t * const v) {
  fpush(v, floorf(fpop(v)));
  return eclr(v);
}

static int cb_fceil(vm_extension_t * const v) {
  fpush(v, ceilf(fpop(v)));
  return eclr(v);
}

static int cb_fabs(vm_extension_t * const v) {
  fpush(v, fabsf(fpop(v)));
  return eclr(v);
}

static int cb_ferf(vm_extension_t * const v) {
  fpush(v, fabsf(fpop(v)));
  return eclr(v);
}

static int cb_ferfc(vm_extension_t * const v) {
  vm_float_t f = fpop(v);
  if (eget(v))
    return eclr(v);
  errno = 0;
  f = erff(f);
  if (errno == ERANGE)
    return eset(v, 43);
  fpush(v, f);
  return eclr(v);
}

static int cb_flgamma(vm_extension_t * const v) {
  vm_float_t f = fpop(v);
  errno = 0;
  f = lgammaf(f);
  if (errno == ERANGE)
    return eset(v, 43);
  fpush(v, f);
  return eclr(v);
}

static int cb_ftgamma(vm_extension_t * const v) {
  vm_float_t f = fpop(v);
  errno = 0;
  f = tgammaf(f);
  if (errno == ERANGE || errno == EDOM)
    return eset(v, 43);
  fpush(v, f);
  return eclr(v);
}

static int cb_fmin(vm_extension_t * const v) {
  const vm_float_t f1 = fpop(v), f2 = fpop(v);
  fpush(v, f1 < f2 ? f1 : f2);
  return eclr(v);
}

static int cb_fmax(vm_extension_t * const v) {
  const vm_float_t f1 = fpop(v), f2 = fpop(v);
  fpush(v, f1 > f2 ? f1 : f2);
  return eclr(v);
}

/*! The virtual machine has only one callback, which we can then use to vector
 * into a table of callbacks provided in 'param', which is a pointer to an
 * instance of 'vm_extension_t' */
static int callback_selector(embed_t *h, void *param) {
  assert(h);
  assert(param);
  vm_extension_t *e = (vm_extension_t*)param;
  if (e->h != h)
    embed_fatal("embed extensions: instance corruption");
  eclr(e);
  const cell_t func = pop(e);
  if (eget(e))
    return eclr(e);
  if (func >= e->callbacks_length)
    return -21;
  const callbacks_t *cb = &e->callbacks[func];
  if (!(cb->use))
    return -21;
  return cb->cb(e);
}

/*! This adds the call backs to an instance of the virtual machine running
 * an eForth image by defining new words in it with 'embed_eval'.  */
static int callbacks_add(embed_t * const h, const bool optimize,  callbacks_t *cb, const size_t number) {
  assert(h && cb);
  const char *optimizer = optimize ? "-2 cells allot ' vm chars ," : "";
  static const char *preamble = "only forth definitions system +order\n";
  int r = 0;
  if ((r = embed_eval(h, preamble)) < 0) {
    embed_error("embed: eval(%s) returned %d", preamble, r);
    return r;
  }

  for (size_t i = 0; i < number; i++) {
    char line[80] = { 0 };
    if (!cb[i].use)
      continue;
    r = snprintf(line, sizeof(line), ": %s %u vm ; %s\n", cb[i].name, (unsigned)i, optimizer);
    assert(strlen(line) < sizeof(line) - 1);
    if (r < 0) {
      embed_error("format error in snprintf (returned %d)", r);
      return -1;
    }
    if ((r = embed_eval(h, line)) < 0) {
      embed_error("embed: eval(%s) returned %d", line, r);
      return r;
    }
  }
  embed_reset(h);
  return 0;
}

static vm_extension_t *vm_extension_new(void) {
  vm_extension_t *v = (vm_extension_t *) embed_alloc(sizeof(*v));
  if (!v)
    return NULL;
  v->h = embed_new();
  if (!(v->h))
    goto fail;

  v->callbacks_length = number_of_callbacks(),
    v->callbacks        = callbacks;
  v->o                = embed_opt_default_hosted();
  v->o.callback       = callback_selector;
  v->o.param          = v;
  embed_opt_set(v->h, &v->o);

  if (callbacks_add(v->h, true, v->callbacks, v->callbacks_length) < 0) {
    embed_error("adding callbacks failed");
    goto fail;
  }

  return v;
 fail:
  if (v->h)
    embed_free(v->h);
  return NULL;
}

/* static int vm_extension_run(vm_extension_t *v) { */
/*   assert(v); */
/*   return embed_vm(v->h); */
/* } */

static void vm_extension_free(vm_extension_t *v) {
  assert(v);
  embed_free(v->h);
  // TODO: Free strings
  memset(v, 0, sizeof(*v));
  free(v);
}

/* ----------------------------------------------------------------------------- */
void marshall_forth_string( char *str, vm_extension_t * const v ){
  cell_t val;
  int    res, r;
  char   *s;
  r   = embed_pop( v->h, &val );
  s   = str;
  r   = (int)val;
  for( short i=0; i<r ; i++ ){
    res = embed_pop( v->h, &val );
    *s++ = val;
  }
  res = embed_pop( v->h, &val );
  *s = 0;
}

int marshall_forth_compactstring( char *str, vm_extension_t * const v ){
  int len;
  short i;
  embed_mmu_read_t  mr = v->h->o.read;
  cell_t val, addr;
  // Read length
  int r = embed_pop( v->h, &val );
  len	= (int)val;
  char buf[32];
  /* sprintf( buf, "Marshall len %d\n", len ); */
  /* repl_log( buf ); */
  // Decode string at address `addr`
  r	= embed_pop( v->h, &addr );
  int loc = (addr>>1)%32768;
  for( i=0; i <= len; i+=2 ){
    val		= mr( v->h, loc + (i>>1) );
    /* sprintf( buf, "%d|%c|%d\n", loc + (i>>1), ((int)val) & 0xFF, ((int)val) & 0xFF ); */
    /* repl_log( buf ); */
    if( i>0 ) str[i-1] = ((int)val) & 0xFF;
    /* sprintf( buf, "%d|%c|%d\n", loc + (i>>1), (((int)val) >> 8) & 0xFF, (((int)val) >> 8) & 0xFF ); */
    /* repl_log( buf ); */
    str[i] = (i<len) ? (((int)val) >> 8) & 0xFF : 0;
  }
  str[len] = 0;
  /* sprintf( buf, "Marshall str |%s|\n", str ); */
  /* repl_log( buf ); */
  return r;
}

/* ----------------------------------------------------------------------------- */
sign_rec_ptr nxpget_sign( vm_extension_t * const v ){
  // Marshall string from FORTH
  cell_t val;
  char   str[_MARSHALL_BUFLEN], *s;
  int    res, r = embed_pop( v->h, &val );
  s   = str;
  r   = (int)val;
  for( short i=0; i<r ; i++ ){
    res = embed_pop( v->h, &val );
    *s++ = val;
  }
  *s = 0;
  //
  return sign_find( str, loadkb_get_allsigns() );
}

int nxpget_unknown( vm_extension_t * const v, sign_rec_ptr sign ){
  int res;
  ((sign_getter_t) sign->getters) ( sign, v->suspend );
  switch( sign->val.type ){
  case _VAL_T_INT:
    res = embed_push( v->h, (cell_t)_UNKNOWN );
    break;
  case _VAL_T_STR:
    res = embed_push( v->h, (cell_t) sign->val.val_forth );
    res = embed_push( v->h, (cell_t) 0 );
    break;
  }
  return eclr(v);
}

int nxpget_known( vm_extension_t * const v, sign_rec_ptr sign ){
  int res;
  cell_t val;
  switch( sign->val.type ){
  case _VAL_T_INT:
    val = (cell_t) sign->val.val_int;
    res = embed_push( v->h, val );
    break;
    //
  case _VAL_T_STR:
    cell_t cell = sign->val.val_forth;
    /* embed_mmu_read_t  mr = v->h->o.read; */
    embed_mmu_write_t mw = v->h->o.write;
    int len;
    // Fill val_forth predefined FORTH variable for this sign
    if( sign->val.valptr ){
      len = strlen( sign->val.valptr );
      for( short i=0; i<len; i += 2 ){
	cell_t addr = (cell + i)>>1%32768;
	cell_t val  = sign->val.valptr[i] + ( (((i+1) > len) ? 0 : sign->val.valptr[i+1])<<8 );
	mw( v->h, addr, val );
      }
    }
    else
      len = 0;
    //
    res = embed_push( v->h, cell );
    if( 0 != res )
      embed_fatal( "can't push sign$ in nxp@" );
    res = embed_push( v->h, len );
    if( 0 != res )
      embed_fatal( "can't push len in nxp@" );
    break;
  }
  return eclr(v);
}

static int cb_nxpget_async(vm_extension_t * const v) {
  sign_rec_ptr sign = nxpget_sign( v );
  if( sign ){
    if( _UNKNOWN == sign->val.status ){
      return nxpget_unknown( v, sign );
    }
    //
    if( _KNOWN == sign->val.status ){
      return nxpget_known( v, sign );
    }
  }
  else{
    // Report undefined DSL-shared variable
    embed_error( "No sign\r" );
  }
  return eclr(v);
}

/* ----------------------------------------------------------------------------- */
static int S_csv_col		= 0;
static sign_rec_ptr S_csv_sign	= NULL;
static FILE *S_temp		= NULL;
static short put_comma		= 0;

typedef void (*csv_cb1_t) (void *s, size_t i, void *p);
typedef void (*csv_cb2_t) (int c, void *p);

void cb1 (void *s, size_t i, void *p) {
  struct val_rec vrec = { _KNOWN, _VAL_T_BOOL, (char *)0, _FALSE, 0, 0.0 };
  switch( S_csv_col ){
  case 0:
    S_csv_sign = sign_find( (char *)s, loadkb_get_allsigns() );
    break;
  case 1:
    if( S_csv_sign ){
      switch( S_csv_sign->val.type ){
      case _VAL_T_INT:
	vrec.type    = _VAL_T_INT;
	vrec.val_int = (int) strtol((char *)s, (char **)NULL, 10);
	sign_set_default( S_csv_sign, &vrec );
	break;
      case _VAL_T_STR:
	vrec.type   = _VAL_T_STR;
	vrec.valptr = (char *)malloc( strlen((char *)s)*sizeof(char) );
	strcpy( vrec.valptr, (char *)s );
	sign_set_default( S_csv_sign, &vrec );
	break;
      }
      
    }
  }
  S_csv_col += 1;
}

void cb2 (int c, void *p) {
  S_csv_col     = 0;
  S_csv_sign	= NULL;
}

void cb3 (void *s, size_t i, void *p) {
  if (put_comma)
    putc(',', S_temp);
  switch( S_csv_col ){
  case 0:
    S_csv_sign = sign_find( (char *)s, loadkb_get_allsigns() );
    csv_fwrite(S_temp, s, i);
    break;
  case 1:
    if( S_csv_sign && (_KNOWN == S_csv_sign->val.status) ){
      switch( S_csv_sign->val.type ){
      case _VAL_T_INT:
	char buf[16];
	sprintf( buf, "%d", S_csv_sign->val.val_int );
	csv_fwrite(S_temp, (void *) buf, strlen(buf) );
	break;
      case _VAL_T_STR:
	csv_fwrite(S_temp, (void *) S_csv_sign->val.valptr, strlen(S_csv_sign->val.valptr) );
	break;
      }
    }
    else{
        csv_fwrite(S_temp, s, i);
    }
    break;
  default:
    csv_fwrite(S_temp, s, i);
    break;
  }
  //
  S_csv_col += 1;
  put_comma = 1;
}

void cb4 (int c, void *p) {
  put_comma = 0;
  putc('\n', S_temp);
  S_csv_col     = 0;
  S_csv_sign	= NULL;
}

int nxpcsv_io( char *fn, csv_cb1_t f_cb1, csv_cb2_t f_cb2 ){
  FILE *fp;
  /* int i; */
  struct csv_parser p;
  char buf[1024];
  size_t bytes_read;
  size_t pos;
  size_t retval;

  if (csv_init(&p, CSV_STRICT | CSV_APPEND_NULL) != 0) {
    /* fprintf(stderr, "Failed to initialize csv parser\n"); */
    return(EXIT_FAILURE);
  }
  pos		= 0;
  S_csv_col	= 0;
  fp = fopen(fn, "rb");
  if (!fp) {
    /* fprintf(stderr, "Failed to open %s: %s, skipping\n", fn, strerror(errno)); */
    return(EXIT_FAILURE);
  }
  while ((bytes_read=fread(buf, 1, 1024, fp)) > 0) {
    if ((retval = csv_parse(&p, buf, bytes_read, f_cb1, f_cb2, NULL)) != bytes_read) {
      if (csv_error(&p) == CSV_EPARSE) {
	printf("%s: malformed at byte %lu\n", fn, (unsigned long)pos + retval + 1);
	goto end;
      } else {
	printf("Error while processing %s: %s\n", fn, csv_strerror(csv_error(&p)));
	goto end;
      }
    }
    pos += bytes_read;
  }
  /* printf("%s well-formed\n", fn); */

 end:
  fclose(fp);
  csv_fini(&p, NULL, NULL, NULL);
  pos = 0;
  
  csv_free(&p);
  return EXIT_SUCCESS;
}

static int cb_nxpcsv_r(vm_extension_t * const v) {
  int    res, r;
  char   str[_MARSHALL_BUFLEN];
  // Marshall file name string from FORTH
  marshall_forth_compactstring( str, v );
  //
  r = nxpcsv_io( str, &cb1, &cb2 );
  res = embed_push( v->h, (EXIT_SUCCESS == r) ? (cell_t)65535 : (cell_t)0 );
  return res;
}

static int cb_nxpcsv_w(vm_extension_t * const v) {
  int    res = 0, r;
  char   str[_MARSHALL_BUFLEN];
  // Marshall file name string from FORTH
  marshall_forth_compactstring( str, v );
  S_temp = fopen("temp.csv", "wb");
  if (!S_temp) {
    /* fprintf(stderr, "Failed to open %s: %s, skipping\n", fn, strerror(errno)); */
    return(EXIT_FAILURE);
  }
  
  r = nxpcsv_io( str, &cb3, &cb4 );
  res = embed_push( v->h, (EXIT_SUCCESS == r) ? (cell_t)65535 : (cell_t)0 );
  fclose( S_temp );
  // Overwrite argument file
  char cmdstr[_MARSHALL_BUFLEN];
  sprintf( cmdstr, "mv temp.csv %s", str );
  system( cmdstr );
  //
  return res;
}


/* ----------------------------------------------------------------------------- */
/* static int cb_nxpshow(vm_extension_t * const v) { */
/*   unsigned short i; */
/*   int            res, len; */
/*   cell_t         val; */
/*   char           *cmdstr; */
/*   res = embed_pop( v->h, &val ); */
/*   len = (int)val; */
/*   cmdstr = (char *)malloc( (len + 10)*sizeof(char) ); */
/*   strcpy( cmdstr, "cygstart " ); */
/*   for( i = 0; i < len; i++ ){ */
/*     res = embed_pop( v->h, &val ); */
/*     *(cmdstr + i + 9) = (char)val; */
/*   } */
/*   res = embed_pop( v->h, &val ); // Ignore ')' */
/*   *(cmdstr + i + 9) = 0; */

/*   system( cmdstr ); */
/*   free( cmdstr ); */
/*   return res; */
/* } */

static int cb_nxpshow(vm_extension_t * const v) {
  unsigned short i;
  int            res;
  cell_t         val;
  char           cmdstr[_MARSHALL_BUFLEN] = "cygstart ";
  res = marshall_forth_compactstring( cmdstr+9, v );
  system( cmdstr );
  return res;
}

static int cb_nxpslog(vm_extension_t * const v) {
  char str[_MARSHALL_BUFLEN];
  int res = marshall_forth_compactstring( str, v );
  return 0;
}

/* ----------------------------------------------------------------------------- */
static int cb_nxpset(vm_extension_t * const v) {
  unsigned short i;
  int            res, len;
  cell_t         val;
  char           str[_MARSHALL_BUFLEN] = "";
  struct val_rec vrec = { _KNOWN, _VAL_T_BOOL, (char *)0, _FALSE, 0, 0.0 };
  sign_rec_ptr   sign = nxpget_sign( v );
  if( sign ){
    switch( sign->val.type ){
    case _VAL_T_INT:
      res = embed_pop( v->h, &val );
      vrec.type    = _VAL_T_INT;
      vrec.val_int = (int)val;
      sign_set_default( sign, &vrec );
      break;
    case _VAL_T_STR:
      /* res = embed_pop( v->h, &val ); */
      /* len = (int)val; */
      /* vrec.type    = _VAL_T_STR; */
      /* vrec.valptr = (char *)malloc( len*sizeof(char) ); */
      /* // Here expect a spelt-string as returned by s( */
      /* // nxp@, however, returns a forth-string as returned by s" */
      /* for( i = 0; i < len; i++ ){ */
      /* 	res = embed_pop( v->h, &val ); */
      /* 	vrec.valptr[i] = (char)val; */
      /* } */
      /* res = embed_pop( v->h, &val ); // Ignore ')' */
      /* vrec.valptr[i] = 0; */
      res = marshall_forth_compactstring( str, v );
      vrec.type    = _VAL_T_STR;
      vrec.valptr  = (char *)malloc( strlen(str)*sizeof(char) );
      strcpy( vrec.valptr, str );
      sign_set_default( sign, &vrec );
      break;
    }
  }
  return 0;
}

/* void engine_dsl_getter_compound( compound_rec_ptr compound ){ */
/* #ifdef ENGINE_DSL_HOWERJFORTH */
/*   int r; */
/*   if(TRACE_ON) printf("<FORTH> Compound %s\n%s\n", compound->str, (char *)compound->dsl_expression ); */
/*   r = engine_dsl_eval( (char *) (compound->dsl_expression) ); */
/*   /\* if( 65535 == r ) r = _TRUE; // -1 is true in FORTH *\/ */
/*   if(TRACE_ON) printf("<FORTH> Evaluated to %d\n", r ); */
/*   sign_set_default( (sign_rec_ptr)compound, r ); */
/* #endif   */
/* } */

/* Each Sign is represented by a "shadow" word in the FORTH environment. The shadow */
/* word for a sign spells out the name of the sign on the stack: each character */
/* is pushed, and then the length, in PASCAL style. */
/*   TANK_p1 ( -- 7 T A N K _ p 1 ) */

/* The generic operators nxp@ and nxp! pop the stack to reconstitute the Sign name */
/* and operate on the Sign in the NXP environment. */
/*     TANK_p1 nxp@ ( -- 370 )   */
/*     100 TANK_p1 nxp! ( -- ) */
    
/* In addition string-valued Signs are allocated memory (currently up to 32 chars) in */
/* the FORTH environment. The address of this memory block is passed back to NXP to be */
/* stored with the Sign record (in the 'val_forth' field). Word nxp@ fills this memory */
/* block with the NXP value and pushes its address on the stack. Word nxp! does not */
/* use it. */
/*     $CRT_andKDU nxp@ =s( AGREE)  ( -- 0 or -1 ) */
/*     s( FLUID-TRANSFER) $TASK nxp!  (  -- ) */

int engine_dsl_DSLvar_declare( const char *dsl_var, sign_rec_ptr sign ){
  // Define a FORTH word to get-memoize the value of a sign to be passed to C primitive `nxp@`
  static const char templ_sign_decl[]  = ": %s $\" %s\" dup c@ for dup r@ + c@ swap next drop ;\n";
  static const char templ_sign$_decl[] = "create %s_$ 32 allot\n%s_$\n";
  int  r = 0;
  cell_t val;
  if( NULL == sign_find( dsl_var, loadkb_get_allsigns() ) ){
    char prgm[_MARSHALL_BUFLEN];
    sprintf( prgm, templ_sign_decl, dsl_var, dsl_var );
    r = embed_eval( S_v->h, prgm );
    if( 0 != r )
      embed_fatal( "can't compile sign decl" );
    if( '$' == dsl_var[0] ){
      sprintf( prgm, templ_sign$_decl, dsl_var, dsl_var );
      r = embed_eval( S_v->h, prgm );
      if( 0 != r )
	embed_fatal( "can't compile sign$ decl" );
      r = embed_pop( S_v->h, &val );
      if( 0 != r )
	embed_fatal( "can't pop sign $ address" );
      sign->val.val_forth = val;
    }
    if(TRACE_ON) printf ("__FUNCTION__ = %s res = %d\n", __FUNCTION__, r);
  }
  return r;
}

/* The HOWERJFORTH (embed FORTH by R. J. Howe) requires several words to be predefined. */
/* +----------+------------------------------+------------------------------+ */
/* |Sign Type |LHS                           |RHS                           | */
/* +----------+------------------------------+------------------------------+ */
/* |int       |none                          |none                          | */
/* +----------+------------------------------+------------------------------+ */
/* |float     |TODO                          |TODO                          | */
/* +----------+------------------------------+------------------------------+ */
/* |string    |nxp_csv@ Read col 0-1 of csv file, returning TRUE or FALSE   | */
/* |int       | s( tanks.csv) nxp_csv@                                      | */
/* +----------+------------------------------+------------------------------+ */
/* +----------+------------------------------+------------------------------+ */
/* |          |nxp_show Show an image file                                  | */
/* |          | s( DIAGNOS.PNG) nxp_show                                    | */
/* +----------+------------------------------+------------------------------+ */

/* The nxp@ and nxp! words are the generic operators to get and set values of signs */
/* from a condition or an action in a rule. */

/* Signs (integer- and string-valued) in the knowledge base are represented in FORTH */
/* by "shadow" words used to marshall values between NXP and FORTH environments. */

int  engine_dsl_init(){
  BUILD_BUG_ON(sizeof(double_cell_t) != sizeof(sdc_t));
  vm_extension_t *v = vm_extension_new();
  if (!v)
    embed_fatal("embed extensions: load failed");

  /* const int r = vm_extension_run(v); */
  cell_t val;
  char   str[_MARSHALL_BUFLEN], *s;
  int    res;
  /* int    r = nxp_init( v ); */
  if(TRACE_ON) printf( "Engine DSL: howerjforth\n\n" );
  /* res = embed_eval( v->h, ": =s( [char] ) parse compare 0 = ;\n" ); */
  /* if( 0 != res ) */
  /*   embed_fatal( "can't compile word =s(" ); */
  /* res = embed_eval( v->h, ": s( [char] ) parse dup >r for dup r@ + c@ swap next drop r> ;\n" ); */
  res = embed_eval( v->h, ": s( [char] ) parse ;\n" );
  if( 0 != res )
    embed_fatal( "can't compile word s(" );
  S_v = v;
  return 0;
}

void engine_dsl_free(){
  vm_extension_free(S_v);
}

extern void  repl_log( const char *s );

int  engine_dsl_eval( const char * expr ){
  cell_t val;
  if(TRACE_ON) printf( "<FORTH> Evaluating %s\n", expr );
  int r = embed_eval( S_v->h, expr );
  r = embed_pop( S_v->h, &val );
  // TRUE is -1 in FORTH
  r = (65535 == (int) val) ? _TRUE : _FALSE;
  return r;
}

int  engine_dsl_rhs_eval( const char * expr ){
  cell_t val;
  int r = embed_eval( S_v->h, expr );
  return r;
}

int  engine_dsl_eval_async( const char * exp, int *err, int *suspend ){
  cell_t val;
  int    ret = _UNKNOWN;
  char   expr[80];
  //
  strcpy( expr, exp );
  
  char   buf[128];
  sprintf( buf, "<FORTH> Evaluating %s (Suspend %d)", expr, *suspend );
  repl_log( buf );

  S_v->suspend = suspend;

  int r = embed_eval( S_v->h, expr );
  sprintf( buf, "<FORTH> Eval =  %d (Suspend %d)", r, *suspend );
  repl_log( buf );
  
  r = embed_pop( S_v->h, &val );

  *err = r;
  sprintf( buf, "<FORTH> Err = %d (Suspend %d)", r, *suspend );
  repl_log( buf );

    // TRUE is -1 in FORTH
  switch( (int)val ){
  case 65535:
    ret = _TRUE;
    break;
  case 0:
    ret = _FALSE;
    break;
  default:
    ret = (int)val;
    break;
  }
  return ret;
}

/* TODO: Better integration of string in embed/nxp */

#endif // ENGINE_DSL_HOWERJFORTH
