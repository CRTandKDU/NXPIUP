/**
 * sign.c -- Hypos and leaves in the or-and trees.
 *
 * Written on mardi, 20 mai 2025.
 * Refactored version, on Thursday, June 25, 2026, by ChatGPT:
 *   - Removed duplicated initialization macro.
 *   - Added safe allocation helpers.
 *   - Consolidated getter/setter insertion logic.
 *   - Fixed setter allocation bug in sign_pushnew().
 *   - Simplified iteration and lookup functions. Switched back iteration to previous v. (JMC)
 *   - Removed redundant variables and manual string copying.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "agenda.h"

extern effect S_on_get;
extern effect S_on_set;

/* ------------------------------------------------------------------------- */
/* Helpers                                                                   */
/* ------------------------------------------------------------------------- */

void *xmalloc(size_t size)
{
    void *ptr = malloc(size);

    if (!ptr) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    return ptr;
}


void *xrealloc(void *ptr, size_t size)
{
    void *new_ptr = realloc(ptr, size);

    if (!new_ptr) {
        fprintf(stderr, "Out of memory\n");
        exit(EXIT_FAILURE);
    }

    return new_ptr;
}

static void push_callback(
    empty_ptr **array,
    unsigned short *count,
    empty_ptr callback)
{
  *array = (empty_ptr *) ( (*count == 0)
			 ? xmalloc(sizeof(empty_ptr))
			 : xrealloc(*array, (*count + 1) * sizeof(empty_ptr)) );

    (*array)[(*count)++] = callback;
}

static void free_callback_array(
    empty_ptr *array,
    unsigned short count)
{
    if (!array)
        return;

    for (unsigned short i = 0; i < count; ++i) {
        free(array[i]);
    }

    free(array);
}

/* ------------------------------------------------------------------------- */
/* Builders                                                                   */
/* ------------------------------------------------------------------------- */

sign_rec_ptr sign_pushnew(
    sign_rec_ptr top,
    const char *name,
    const int ngetters,
    const size_t size_getter,
    const int nsetters,
    const size_t size_setter)
{
  sign_rec_ptr sign = (sign_rec_ptr) xmalloc(sizeof(*sign));

    sign->next = top;

    sign_init_value(sign);

    size_t len = strlen(name);
    unsigned short chopped_len =
        (unsigned short)((len <= _CHOP) ? len : _CHOP);

    sign->len_type = chopped_len | SIGN_MASK;

    memcpy(sign->str, name, chopped_len);
    sign->str[chopped_len] = '\0';

    sign->ngetters = (unsigned short)ngetters;
    sign->nsetters = (unsigned short)nsetters;

    if (ngetters > 0) {
      sign->getters = (empty_ptr *) xmalloc((size_t)ngetters * size_getter);
    } else {
        /* Default getter */
        sign->getters = (empty_ptr *)&getter_sign;
    }

    if (nsetters > 0) {
      sign->setters = (empty_ptr *) xmalloc((size_t)nsetters * size_setter);
    } else {
        sign->setters = NULL;
    }

    return sign;
}

void sign_del(sign_rec_ptr sign)
{
    if (!sign)
        return;

    fprintf(stderr, "Deleting %s\n", sign->str);

    if ((sign->len_type & TYPE_MASK) == COMPOUND_MASK) {
        compound_del((compound_rec_ptr)sign);
    }

    if (sign->ngetters && sign->getters) {
        free_callback_array(sign->getters, sign->ngetters);
    }

    if (sign->nsetters && sign->setters) {
        free_callback_array(sign->setters, sign->nsetters);
    }

#ifndef FLTK
    free(sign->val.valptr);
#endif

    free(sign);
}

void sign_pushgetter(sign_rec_ptr sign, empty_ptr getter)
{
  push_callback(&sign->getters, (short unsigned int*) &sign->ngetters, getter);
}

void sign_pushsetter(sign_rec_ptr sign, empty_ptr setter)
{
  push_callback(&sign->setters, (short unsigned int*) &sign->nsetters, setter);
}

/* ------------------------------------------------------------------------- */
/* Default I/O                                                               */
/* ------------------------------------------------------------------------- */

unsigned short sign_get_default(sign_rec_ptr sign)
{
    char buf[32] = {0};

    if (S_on_get) {
        S_on_get(sign, NULL);
    }

    printf("Q: What is the value of %s?\n", sign->str);
    printf("(Type 0 or 1)\n");
    printf("A: ");

    if (!fgets(buf, sizeof(buf), stdin)) {
        return 0;
    }

    return (unsigned short)strtoul(buf, NULL, 0);
}

void sign_set_default(sign_rec_ptr sign, struct val_rec *val)
{
    if (!sign || !val)
        return;

    if (val->status == _UNKNOWN)
        return;

    if (sign->val.type != val->type)
        return;

    if (S_on_set) {
        S_on_set(sign, val);
    }

    sign->val.status = val->status;

    switch (val->type) {

    case _VAL_T_BOOL:
        sign->val.val_bool = val->val_bool;
        break;

    case _VAL_T_INT:
        sign->val.val_int = val->val_int;
        break;

    case _VAL_T_FLOAT:
        sign->val.val_float = val->val_float;
        break;

    case _VAL_T_STR:
        sign->val.valptr = val->valptr;
        break;

    default:
        break;
    }

    /*
     * IMPORTANT:
     * This is where sign values are forwarded.
     */
    engine_default_on_set(sign, val);
}

/* ------------------------------------------------------------------------- */
/* Managers                                                                  */
/* ------------------------------------------------------------------------- */

void sign_print(sign_rec_ptr sign)
{
    char *esc;
    short len = sign->len_type & SIGN_UNMASK;

    if (sign->val.status == _KNOWN &&
        sign->val.type == _VAL_T_BOOL) {
        esc = S_val_color(sign->val.val_bool);
    } else {
        esc = S_val_color(2);
    }

    if (TRACE_ON) {
        printf("%sSIGN:\t%s (%d, %d, %d)",
               esc,
               sign->str,
               len,
               sign->len_type,
               sign->len_type & TYPE_MASK);

        printf("\tGetters %d (%zu), Setters %d (%zu)\n",
               sign->ngetters,
               sizeof(sign->getters),
               sign->nsetters,
               sizeof(sign->setters));
    }
}

void sign_iter(sign_rec_ptr sign, sign_op func)
{
    if (!func)
        return;

    /* for (; sign; sign = sign->next) { */
    /*     func(sign); */
    /* } */
    sign_rec_ptr s_prev, s = sign;
    while( s ){
      s_prev	= s;
      s		= s->next;
      func( s_prev );
    }
}

sign_rec_ptr sign_find(const char *name, sign_rec_ptr top)
{
    for (; top; top = top->next) {
        if (strcmp(top->str, name) == 0) {
            return top;
        }
    }

    return NULL;
}

hypo_rec_ptr sign_tohypo(
    hypo_rec_ptr hypo,
    sign_rec_ptr top_sign,
    hypo_rec_ptr top_hypo)
{
    sign_rec_ptr current = top_sign;

    if (TRACE_ON) {
        printf("Changing sign %s to hypo\n", hypo->str);
    }

    hypo->len_type =
        (unsigned short)strlen(hypo->str) | HYPO_MASK;

    while (current && hypo != current->next) {
        current = current->next;
    }

    if (!current) {
        return hypo;
    }

    current->next = hypo->next;
    hypo->next = top_hypo;

    return hypo;
}

/* ------------------------------------------------------------------------- */
/* Client data                                                               */
/* ------------------------------------------------------------------------- */

empty_ptr sign_get_client_data(sign_rec_ptr sign)
{
    return sign->client_data;
}

void sign_set_client_data(
    sign_rec_ptr sign,
    empty_ptr client_data)
{
    sign->client_data = client_data;
}
