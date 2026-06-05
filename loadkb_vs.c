/**
 * loadkb.c -- Loading knowledge base files
 *
 * Written on jeudi,  5 juin 2025.
 * Revisions:
 *     Wednesday, July 30, 2025
 */
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

#include <iostream>
#include <fstream>
#include <string>
#include <stdarg.h>

#include "agenda.h"

using namespace std;

extern void  repl_log( const char *s,... );

const char * _EMPTY = "";
const char * _BEG_RULE = "#+BEGIN_RULE";
const char * _END_RULE = "#+END_RULE";
const char * _THEN = "THEN";
const char * _BOOLYES = "YES";
const char * _BOOLNO  = "NO";

sign_rec_ptr KB_Signs = (sign_rec_ptr) NULL;;
hypo_rec_ptr KB_Hypos = (hypo_rec_ptr) NULL;;
rule_rec_ptr KB_Rules = (rule_rec_ptr) NULL;;
int rule_count = 0;
int comp_count = 0;

#define _DSL_LINE 80
#define _FW(line,pch) pch = strtok( (char *) (line).c_str(), delims );			                \
  if( (pch) ){ str_toupper((pch));}							                \
  else{ pch = (char *) &_EMPTY;} \
  if(TRACE_ON) printf( "[S:%d] FW %s (%d)\n", sno, (pch), (int)strlen((pch)) );		\

#define _TRANSITION(x) 	  if(TRACE_ON) printf( "Changing sno from %d to ", sno );    \
  sno		= (x);                                                               \
  if(TRACE_ON) printf( "%d\n", sno );                                                \


void str_toupper( char *word ){
   char *s = word;
   while( *s ){
     *s = toupper((unsigned char) *s);
     s++;
   }
}

void repl_txt( char *s ){
  char buf[32];
  sprintf( buf, "<%s> %d", s, (int)strlen(s) );
  repl_log( buf );
}

#ifdef ENGINE_DSL_HOWERJFORTH
sign_rec_ptr loadkb_parse( char *dsl_expr, compound_rec_ptr compound, sign_rec_ptr top ){
  // Destroys dsl_expr and top
  const char *ws = " \t\x0d"; // Whitespace characters that separate tokens
  char *pw, *pnw;
  unsigned short cont = _TRUE;
  pw = dsl_expr;
  dsl_expr += strcspn( dsl_expr, ws );
  if( *dsl_expr ){
    *dsl_expr++ = 0; // Terminate current first word in pw
  }
  
  while( cont ){
    dsl_expr += strspn( dsl_expr, ws );
    if( !*dsl_expr ){
      // That was actually trailing whitespace at the end of the string
      break;
    }
    pnw = dsl_expr;
    dsl_expr += strcspn( dsl_expr, ws );
    if( *dsl_expr ){
      *dsl_expr++ = 0; // Terminate current next word in pnw
    }
    /* repl_txt(pnw); */
    if( (0 == strcmp( pnw, "nxp@" )) || (0 == strcmp( pnw, "nxp!" )) ){
      sign_rec_ptr lsign;
      int r;
      if(TRACE_ON) printf( "Found DSL-shared variable: %s\n", pw );
      /* repl_log( pw ); */
      lsign = sign_find( pw, top );
      if( NULL == lsign ){
	top = lsign = sign_pushnew( top, pw, 0, sizeof(void *), 0, sizeof(fwrd_rec_ptr) );
	/* repl_log( "pushnew" ); */
	lsign->val.type = _VAL_T_INT;
	if( '$' == pw[0] ) lsign->val.type = _VAL_T_STR;
      }
      if( compound ) compound_DSLvar_pushnew( compound, lsign );
      r = engine_dsl_DSLvar_declare( pw, lsign );
    }
    pw = pnw;

  }
  return top;
}
#endif

sign_rec_ptr loadkb_get_allsigns(){ return KB_Signs; }
hypo_rec_ptr loadkb_get_allhypos(){ return KB_Hypos; }
rule_rec_ptr loadkb_get_allrules(){ return KB_Rules; }
int          loadkb_howmany( sign_rec_ptr top ){
  int count = 0;
  sign_rec_ptr s = top;
  while( s ){ count++; s = s->next; }
  return count;
}


void loadkb_reset(){
  if( KB_Hypos ) sign_iter(KB_Hypos,&hypo_del) ;
  if( KB_Signs ) sign_iter(KB_Signs,&sign_del) ;
  if( KB_Rules ) sign_iter( (sign_rec_ptr)KB_Rules, (sign_op) &rule_del) ;

  KB_Signs = (sign_rec_ptr) NULL;
  KB_Hypos = (hypo_rec_ptr) NULL;
  KB_Rules = (rule_rec_ptr) NULL;
  rule_count = 0;
  comp_count = 0;
}


int loadkb_file(const char* fn) {
	FILE* fp;
	char* pch;
	size_t len = 0;
	fstream fs;
	string line;
	//
	char delims[] = " \t\x0D";
	int lno, sno, retcode, condno, cond_t;
	sign_rec_ptr lsign;
	compound_rec_ptr lcompound;
	hypo_rec_ptr lhypo;
	rule_rec_ptr lrule;
	unsigned short schange;
	char dsl_expr[_DSL_LINE];

	//fp = fopen( fn, "r" );
	//if (fp == NULL)
	//  return 1;
	fs.open(fn, ios::in);
	if (!fs.is_open())
		return 1;

	loadkb_reset();


	retcode = 0;
	lno = 1;
	sno = 0;
	while (getline(fs, line)) {
		if (!line.empty()) {
			//
			schange = _TRUE;
			while (schange) {
				schange = _FALSE;
				switch (sno) {
				case 0: // Waiting for beginning of rule, skipping lines
					_FW(line, pch);
					// Note that rules are pushed initially without hypotheses
					lrule = (rule_rec_ptr)NULL;
					if (0 == strcmp(_BEG_RULE, pch)) {
						pch = strtok(NULL, delims);
						if (TRACE_ON) printf("\t[S:%d] FW %s (%d)\n", sno, pch, (int)strlen(pch));
						if (NULL != pch && *pch != 0x0A) {
							if (TRACE_ON) printf("NW %s\n", pch);
							/* repl_log( "Named rule:" ); */
							/* repl_log( pch ); */
							KB_Rules = lrule = rule_pushnew(KB_Rules, pch, 0, (hypo_rec_ptr)NULL);
						}
						else {
							char tmp[32];
							int  tmpn = sprintf(tmp, "RULE_%d", rule_count++);
							/* repl_log( "Unamed rule:" ); */
							KB_Rules = lrule = rule_pushnew(KB_Rules, tmp, 0, (hypo_rec_ptr)NULL);
							if (TRACE_ON) printf("NW anonymous rule: %s\n", tmp);
						}
						condno = 0;
						_TRANSITION(1);
					}
					break;

				case 1: // Parsing lines as conditions until hypothesis declaration
					/* for( short j=0; j<_DSL_LINE; dsl_expr[j++] = 0 ); */
					/* strncpy( dsl_expr, line, (size_t)read ); */
					strcpy(dsl_expr, line.c_str());

					_FW(line, pch);
					//
					if (0 == strcmp(_THEN, pch)) {
						if (0 == condno) {
							if (TRACE_ON) printf("NW missing conditions in rule %s\n", lrule->str);
							sno = 255;
							schange = _TRUE;
							break;
						}
						pch = strtok(NULL, delims);
						if (TRACE_ON) printf("\t[S:%d] FW %s (%d)\n", sno, pch, (int)strlen(pch));
						if (NULL != pch && *pch != 0x0A) {
							bwrd_rec_ptr bwrd;

							if (TRACE_ON) printf("NW HYPO %s\n", pch);
							lhypo = (hypo_rec_ptr)sign_find(pch, (sign_rec_ptr)KB_Hypos);
							if (NULL == lhypo) {
								lhypo = (hypo_rec_ptr)sign_find(pch, KB_Signs);
								if (lhypo) KB_Hypos = sign_tohypo(lhypo, KB_Signs, KB_Hypos);
							}
							if (NULL == lhypo)
								KB_Hypos = lhypo = hypo_pushnew(KB_Hypos, pch, 0);

							lrule->setters = (empty_ptr*)lhypo;
							sign_pushgetter(lhypo, (empty_ptr)malloc(sizeof(struct bwrd_rec)));
							bwrd = (bwrd_rec_ptr)(lhypo->getters)[_LAST_RULE(lhypo)];
							bwrd->rule = lrule;
						}
						else {
							if (TRACE_ON) printf("NW missing hypo\n");
							sno = 255;
							schange = _TRUE;
							break;
						}
						if (TRACE_ON) printf("Rule has %d conds\n", condno);
						_TRANSITION(2);
						break;
					}
					// This is a condition line
					condno += 1;
					cond_t = 0;
					if (0 == strcmp(_BOOLYES, pch)) cond_t = 1;
					if (0 == strcmp(_BOOLNO, pch))  cond_t = -1;
					if (0 == cond_t) {
						// This is a DSL-expression
						char tmp[32];
						int  tmpn = sprintf(tmp, "COMPOUND_%d", comp_count++);

						// DSL expression
						if (TRACE_ON) printf("Line %d DSL: %s\n", lno, dsl_expr);
						lcompound = compound_pushnew(KB_Signs, tmp, 0);
						KB_Signs = (sign_rec_ptr)lcompound;
						compound_DSL_set(lcompound, dsl_expr);
						// Parse DSL shared vars in source text
#ifdef ENGINE_DSL
						KB_Signs = loadkb_parse(dsl_expr, lcompound, KB_Signs);
#endif
						rule_pushnewcond(lrule, (unsigned short)1, (sign_rec_ptr)lcompound);
					}
					else {
						// Boolean condition NW is sign or hypo
						pch = strtok(NULL, delims);
						if (TRACE_ON) printf("\t[State: %d] FW %s (%d)\n", sno, pch, (int)strlen(pch));
						if (NULL != pch && *pch != 0x0A) {
							if (TRACE_ON) printf("NW HYPO or SIGN %s\n", pch);
							lsign = sign_find(pch, KB_Signs);
							if (NULL == lsign) lsign = sign_find(pch, (sign_rec_ptr)KB_Hypos);
							if (NULL == lsign) {
								KB_Signs = lsign = sign_pushnew(KB_Signs, pch,
									0, sizeof(void*),
									0, sizeof(fwrd_rec_ptr));
							}
							rule_pushnewcond(lrule, (1 == cond_t) ? (unsigned short)1 : (unsigned short)0, lsign);
							if (TRACE_ON) printf("Get/Create %s in rule %s\n", lsign->str, lrule->str);
						}
						else {
							if (TRACE_ON) printf("NW missing hypo or sign\n");
							sno = 255;
							schange = _TRUE;
							break;
						}
					}
					if (TRACE_ON) printf("State 1: new state %d, schange %d\n", sno, schange);
					break;

				case 2: // Waiting for end of rule parsing RHS
					strcpy(dsl_expr, line.c_str());
					_FW(line, pch);
					if (0 == strcmp(_END_RULE, pch)) {
						_TRANSITION(0);
					}
					else {
						// RHS command
						/* repl_log( "RHS:" ); */
						/* repl_log( dsl_expr ); */
						rule_pushnewrhs(lrule, dsl_expr);
#ifdef ENGINE_DSL
						KB_Signs = loadkb_parse(dsl_expr, (compound_rec_ptr)0, KB_Signs);
#endif
					}
					break;

				case 255: // Error in parsing
					if (TRACE_ON) printf("Parsing error at line %d\n", lno);
					retcode = 255;
					goto end_exit;
					break;
				}
			}
		}
		lno += 1;
	}

end_exit:
	//fclose(fp);
	//if (line)
	//  free(line);  
	fs.close();

	sign_iter(KB_Signs, &sign_print);
	if (TRACE_ON) printf("%s----\t----\t----\n", S_val_color(_UNKNOWN));
	sign_iter(KB_Hypos, &hypo_print);
	if (TRACE_ON) printf("%s----\t----\t----\n", S_val_color(_UNKNOWN));
	sign_iter((sign_rec_ptr)KB_Rules, (sign_op)&rule_print);

	if (0 == rule_count) retcode = 255;

	return retcode;
}
