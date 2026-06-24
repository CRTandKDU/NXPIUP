/*
 * loadkb.c -- Refactored parser skeleton
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#include "agenda.h"
#include "nxp_hash.h"

#define INFO_BUFSIZE 1024
#define TRACE_ON 0

static const char *BEG_RULE = "#+BEGIN_RULE";
static const char *END_RULE = "#+END_RULE";
static const char *THEN     = "THEN";
static const char *BOOLYES  = "YES";
static const char *BOOLNO   = "NO";

static const char *BEG_ATTR = "#+BEGIN_ATTRIBUTE";
static const char *END_ATTR = "#+END_ATTRIBUTE";

static const char *BEG_INFO = "#+BEGIN_INFO";
static const char *END_INFO = "#+END_INFO";

typedef enum
{
    PARSE_IDLE,
    PARSE_RULE_CONDITIONS,
    PARSE_RULE_ACTIONS,
    PARSE_ATTRIBUTES,
    PARSE_INFO,
    PARSE_ERROR
} parser_state_t;

typedef enum
{
    COND_DSL,
    COND_TRUE,
    COND_FALSE
} condition_type_t;

typedef struct
{
    parser_state_t state;

    int line_no;
    int condition_count;

    rule_rec_ptr rule;
    hypo_rec_ptr hypo;
    sign_rec_ptr sign;
    compound_rec_ptr compound;

    char name[128];
    char *info;
} parser_ctx_t;


/*--------------------------------------------------------------------*/
/* Globals                                                            */
/*--------------------------------------------------------------------*/

sign_rec_ptr KB_Signs = NULL;
hypo_rec_ptr KB_Hypos = NULL;
rule_rec_ptr KB_Rules = NULL;

int rule_count = 0;
int comp_count = 0;


/*--------------------------------------------------------------------*/
/* Utility                                                            */
/*--------------------------------------------------------------------*/

static void str_toupper(char *s)
{
    while (*s)
    {
        *s = (char)toupper((unsigned char)*s);
        ++s;
    }
}

static char *first_token(char *line, const char *delims)
{
    char *tok = strtok(line, delims);

    if (tok)
        str_toupper(tok);

    return tok;
}

static char *next_token(const char *delims)
{
    char *tok = strtok(NULL, delims);

    if (!tok)
        return NULL;

    if (*tok == '\n' || *tok == '\0')
        return NULL;

    return tok;
}

static void set_state(parser_ctx_t *ctx,
                      parser_state_t next)
{
#if TRACE_ON
    printf("STATE %d -> %d\n",
           ctx->state,
           next);
#endif

    ctx->state = next;
}

static condition_type_t
parse_condition_type(const char *token)
{
    if (!strcmp(token, BOOLYES))
        return COND_TRUE;

    if (!strcmp(token, BOOLNO))
        return COND_FALSE;

    return COND_DSL;
}


/*--------------------------------------------------------------------*/
/* KB helpers                                                         */
/*--------------------------------------------------------------------*/

static sign_rec_ptr
get_or_create_sign(const char *name)
{
    sign_rec_ptr sign;

    sign = sign_find(name, KB_Signs);

    if (!sign)
        sign = sign_find(name,
                         (sign_rec_ptr)KB_Hypos);

    if (!sign)
    {
        KB_Signs =
            sign =
            sign_pushnew(KB_Signs,
                         name,
                         0,
                         sizeof(void *),
                         0,
                         sizeof(fwrd_rec_ptr));
    }

    return sign;
}

static hypo_rec_ptr
get_or_create_hypothesis(const char *name)
{
    hypo_rec_ptr hypo;

    hypo = (hypo_rec_ptr)
        sign_find(name,
                  (sign_rec_ptr)KB_Hypos);

    if (!hypo)
    {
        hypo = (hypo_rec_ptr)
            sign_find(name, KB_Signs);

        if (hypo)
        {
            KB_Hypos =
                sign_tohypo(hypo,
                            KB_Signs,
                            KB_Hypos);
        }
    }

    if (!hypo)
    {
        KB_Hypos =
            hypo =
            hypo_pushnew(KB_Hypos,
                         name,
                         0);
    }

    return hypo;
}


/*--------------------------------------------------------------------*/
/* State handlers                                                     */
/*--------------------------------------------------------------------*/

static void
parse_idle(parser_ctx_t *ctx,
           char *line)
{
    const char delims[] = " \t\r";

    char *tok = first_token(line, delims);

    if (!tok)
        return;

    if (!strcmp(tok, BEG_RULE))
    {
        char *name = next_token(delims);

        if (!name)
        {
            char buf[32];

            snprintf(buf,
                     sizeof(buf),
                     "RULE_%d",
                     rule_count++);

            KB_Rules =
                ctx->rule =
                rule_pushnew(KB_Rules,
                             buf,
                             0,
                             NULL);
        }
        else
        {
            KB_Rules =
                ctx->rule =
                rule_pushnew(KB_Rules,
                             name,
                             0,
                             NULL);
        }

        ctx->condition_count = 0;

        set_state(ctx,
                  PARSE_RULE_CONDITIONS);

        return;
    }

    if (!strcmp(tok, BEG_ATTR))
    {
        char *name = next_token(delims);

        if (!name)
        {
            set_state(ctx,
                      PARSE_ERROR);
            return;
        }

        strncpy(ctx->name,
                name,
                sizeof(ctx->name) - 1);

        set_state(ctx,
                  PARSE_ATTRIBUTES);

        return;
    }

    if (!strcmp(tok, BEG_INFO))
    {
        char *name = next_token(delims);

        if (!name)
        {
            set_state(ctx,
                      PARSE_ERROR);
            return;
        }

        strncpy(ctx->name,
                name,
                sizeof(ctx->name) - 1);

        ctx->info = calloc(INFO_BUFSIZE, 1);

        set_state(ctx,
                  PARSE_INFO);
    }
}


static void
parse_rule_conditions(parser_ctx_t *ctx,
                      char *line)
{
    const char delims[] = " \t\r";

    char *copy = strdup(line);
    char *tok  = first_token(line, delims);

    if (!tok)
    {
        free(copy);
        return;
    }

    if (!strcmp(tok, THEN))
    {
        char *hypo_name = next_token(delims);

        if (!hypo_name)
        {
            set_state(ctx,
                      PARSE_ERROR);
            free(copy);
            return;
        }

        ctx->hypo =
            get_or_create_hypothesis(hypo_name);

        ctx->rule->setters =
            (empty_ptr *)ctx->hypo;

        set_state(ctx,
                  PARSE_RULE_ACTIONS);

        free(copy);
        return;
    }

    switch (parse_condition_type(tok))
    {
        case COND_TRUE:
        case COND_FALSE:
        {
            char *name = next_token(delims);

            if (!name)
            {
                set_state(ctx,
                          PARSE_ERROR);
                break;
            }

            sign_rec_ptr sign =
                get_or_create_sign(name);

            rule_pushnewcond(
                ctx->rule,
                strcmp(tok, BOOLYES) == 0,
                sign);

            ctx->condition_count++;
            break;
        }

        case COND_DSL:
        {
            char cname[32];

            snprintf(cname,
                     sizeof(cname),
                     "COMPOUND_%d",
                     comp_count++);

            compound_rec_ptr compound =
                compound_pushnew(
                    KB_Signs,
                    cname,
                    0);

            KB_Signs =
                (sign_rec_ptr)compound;

            compound_DSL_set(
                compound,
                copy);

            rule_pushnewcond(
                ctx->rule,
                1,
                (sign_rec_ptr)compound);

            ctx->condition_count++;
            break;
        }
    }

    free(copy);
}


static void
parse_rule_actions(parser_ctx_t *ctx,
                   char *line)
{
    const char delims[] = " \t\r";

    char *copy = strdup(line);
    char *tok  = first_token(line, delims);

    if (tok && !strcmp(tok, END_RULE))
    {
        set_state(ctx,
                  PARSE_IDLE);

        free(copy);
        return;
    }

    rule_pushnewrhs(ctx->rule, copy);

    free(copy);
}


static void
parse_attributes(parser_ctx_t *ctx,
                 char *line)
{
    const char delims[] = " \t\r";

    char *key = first_token(line, delims);

    if (!key)
        return;

    if (!strcmp(key, END_ATTR))
    {
        set_state(ctx,
                  PARSE_IDLE);
        return;
    }

    char *value = next_token(delims);

    if (!value)
    {
        set_state(ctx,
                  PARSE_ERROR);
        return;
    }

    nxp_hash_set(ctx->name,
                 key,
                 strdup(value));
}


static void
parse_info(parser_ctx_t *ctx,
           char *line)
{
    if (!strncmp(line,
                 END_INFO,
                 strlen(END_INFO)))
    {
        nxp_hash_set(ctx->name,
                     "INFO",
                     strdup(ctx->info));

        free(ctx->info);
        ctx->info = NULL;

        set_state(ctx,
                  PARSE_IDLE);

        return;
    }

    strncat(ctx->info,
            line,
            INFO_BUFSIZE -
            strlen(ctx->info) - 1);
}


/*--------------------------------------------------------------------*/
/* Main parser                                                        */
/*--------------------------------------------------------------------*/

int loadkb_file(const char *filename)
{
    FILE *fp;
    char *line = NULL;
    size_t len = 0;

    parser_ctx_t ctx = {
        .state = PARSE_IDLE,
        .line_no = 1
    };

    fp = fopen(filename, "r");

    if (!fp)
        return 1;

    loadkb_reset();

    while (getline(&line, &len, fp) != -1)
    {
        switch (ctx.state)
        {
            case PARSE_IDLE:
                parse_idle(&ctx, line);
                break;

            case PARSE_RULE_CONDITIONS:
                parse_rule_conditions(&ctx, line);
                break;

            case PARSE_RULE_ACTIONS:
                parse_rule_actions(&ctx, line);
                break;

            case PARSE_ATTRIBUTES:
                parse_attributes(&ctx, line);
                break;

            case PARSE_INFO:
                parse_info(&ctx, line);
                break;

            case PARSE_ERROR:
                fprintf(stderr,
                        "Parse error at line %d\n",
                        ctx.line_no);

                free(line);
                fclose(fp);
                return 255;
        }

        ctx.line_no++;
    }

    free(line);
    fclose(fp);

    return 0;
}
