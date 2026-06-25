/**
 * rule.c -- Rules associate a set of conditions to a hypothesis.
 *
 * Written on mardi, 20 mai 2025.
 * Refactored version, on Thursday, June 25, 2026, by ChatGPT:
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#ifdef FLTK
#include <string>
#endif

#include "agenda.h"

extern void repl_log(const char *s);

/* ------------------------------------------------------------------------- */
/* Helpers                                                                   */
/* ------------------------------------------------------------------------- */

static void rule_append_ptr(
    empty_ptr **array,
    unsigned short *count,
    empty_ptr item)
{
  *array = 
    (*count == 0)
    ? (empty_ptr *) xmalloc(sizeof(empty_ptr))
    : (empty_ptr *) xrealloc( (void *) (*array),
			      (size_t) ( (*count + 1) * sizeof(empty_ptr)) );
    

  (*array)[(*count)++] = item;
}

static char *rule_strdup(const char *s)
{
    size_t len = strlen(s) + 1;

    char *copy = (char *) xmalloc(len);

    memcpy(copy, s, len);

    return copy;
}

/* ------------------------------------------------------------------------- */
/* Builders                                                                  */
/* ------------------------------------------------------------------------- */

rule_rec_ptr rule_pushnew(
    rule_rec_ptr top,
    const char *name,
    int ngetters,
    hypo_rec_ptr hypo)
{
  rule_rec_ptr rule = (rule_rec_ptr) xmalloc(sizeof(*rule));

    rule->next = (sign_rec_ptr)top;

    sign_init_value((sign_rec_ptr)rule);

    unsigned short len =
        (unsigned short)strlen(name);

    if (len > _CHOP)
        len = _CHOP;

    memcpy(rule->str, name, len);
    rule->str[len] = '\0';

    rule->len_type = len | RULE_MASK;

    rule->ngetters = (unsigned short)ngetters;
    rule->getters  = NULL;

    /*
     * A rule points to its target hypothesis through
     * the setters field.
     */
    rule->nsetters = 0;
    rule->setters  = (empty_ptr *)hypo;

    rule->nrhs = 0;
    rule->rhs  = NULL;

    if (hypo) {
        bwrd_rec_ptr bwrd = (bwrd_rec_ptr) xmalloc(sizeof(struct bwrd_rec));

        sign_pushgetter(
            (sign_rec_ptr)hypo,
            (empty_ptr) bwrd );

        bwrd->rule = rule;
    }

    return rule;
}

/* ------------------------------------------------------------------------- */
/* RHS                                                                        */
/* ------------------------------------------------------------------------- */

void rule_pushnewrhs(
    rule_rec_ptr rule,
    char *dsl_expr)
{
#ifdef FLTK

    std::string rhs(dsl_expr);

    if (rhs.empty() || rhs.back() != '\n')
        rhs.push_back('\n');

    rule_append_ptr(
        &rule->rhs,
        &rule->nrhs,
        rule_strdup(rhs.c_str()));

#else

    rule_append_ptr(
        &rule->rhs,
        (short unsigned int *) &rule->nrhs,
        (empty_ptr) rule_strdup(dsl_expr));

#endif
}

/* ------------------------------------------------------------------------- */
/* Conditions                                                                 */
/* ------------------------------------------------------------------------- */

void rule_pushnewcond(
    rule_rec_ptr rule,
    unsigned short op,
    sign_rec_ptr sign)
{
    cond_rec_ptr cond  = (cond_rec_ptr) xmalloc(sizeof(struct cond_rec));
    fwrd_rec_ptr fwrd  = (fwrd_rec_ptr) xmalloc(sizeof(struct fwrd_rec));

    sign_pushgetter(
        (sign_rec_ptr)rule,
        (empty_ptr) cond);
    cond->in   = op;
    cond->out  = op;
    cond->val  = 0xff;
    cond->sign = sign;

    if (TRACE_ON) {
        printf("> Nsetters: %d. "
               "Pushing fwrd_rec: %s -> %s\t",
               sign->nsetters,
               sign->str,
               rule->str);
    }

    sign_pushsetter(
        sign,
        (empty_ptr) fwrd);
    fwrd->rule     = rule;
    fwrd->idx_cond = _LAST_COND(rule);

    if (TRACE_ON) {
        printf("Nsetters: %d\n",
               sign->nsetters);
    }
}

/* ------------------------------------------------------------------------- */
/* Destruction                                                                */
/* ------------------------------------------------------------------------- */

void rule_del(rule_rec_ptr rule)
{
    if (!rule)
        return;

    for (unsigned short i = 0;
         i < rule->ngetters;
         ++i)
    {
        free(rule->getters[i]);
    }

    free(rule->getters);

    for (unsigned short i = 0;
         i < rule->nrhs;
         ++i)
    {
        free(rule->rhs[i]);
    }

    free(rule->rhs);

    free(rule);
}

/* ------------------------------------------------------------------------- */
/* Debug                                                                      */
/* ------------------------------------------------------------------------- */

void rule_print(rule_rec_ptr rule)
{
    if (!TRACE_ON || !rule)
        return;

    char *esc;

    if (rule->val.status == _KNOWN &&
        rule->val.type == _VAL_T_BOOL)
    {
        esc = S_val_color(rule->val.val_bool);
    }
    else
    {
        esc = S_val_color(2);
    }

    printf("%sRULE:\t%s (%d, %d, %d)\n",
           esc,
           rule->str,
           rule->len_type & RULE_UNMASK,
           rule->len_type,
           rule->len_type & TYPE_MASK);

    printf("\tGetters: %d, Setters: %d\n",
           rule->ngetters,
           rule->nsetters);

    if (rule->setters) {
        hypo_print((hypo_rec_ptr)rule->setters);
    }

    printf("%s\tCOND: %d\n",
           S_val_color(2),
           rule->ngetters);

    for (unsigned short i = 0;
         i < rule->ngetters;
         ++i)
    {
        cond_rec_ptr cond =
            _AS_COND_ARRAY(rule->getters)[i];

        printf("\t\tCOND %d: %d == %s\tVal: %d\n",
               i,
               cond->out,
               cond->sign->str,
               cond->val);

        sign_rec_ptr sign = cond->sign;

        for (unsigned short j = 0;
             j < sign->nsetters;
             ++j)
        {
            fwrd_rec_ptr fwrd =
                (fwrd_rec_ptr)sign->setters[j];

            printf("\t\tin %s at %d\n",
                   fwrd->rule->str,
                   fwrd->idx_cond);
        }
    }

    for (unsigned short i = 0;
         i < rule->nrhs;
         ++i)
    {
        printf("\t\tRHS: %s\n",
               (char *)rule->rhs[i]);
    }

    printf("%s\n", esc);
}
