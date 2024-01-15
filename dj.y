/*  DJ PARSER  */
/* I pledge my Honor that I have not cheated, and will not cheat, on this assignment. */
/* Raymond Gillies */

%code provides {
  #include "lex.yy.c"
  #include "ast.h"

  #define YYSTYPE ASTree *

  ASTree *pgmAST;

  /* Function for printing generic syntax-error messages */
  void yyerror(const char *str) {
    printf("Syntax error on line %d at token %s\n",yylineno,yytext);
    printf("(This version of the compiler exits after finding the first ");
    printf("syntax error.)\n");
    exit(-1);
  }
}

%token MAIN CLASS EXTENDS NATTYPE IF ELSE WHILE
%token PRINTNAT READNAT THIS NEW NUL NATLITERAL 
%token ID ASSIGN PLUS MINUS TIMES EQUALITY GREATER
%token AND NOT DOT SEMICOLON COMMA LBRACE RBRACE 
%token LPAREN RPAREN ENDOFFILE

%start pgm

%right ASSIGN
%left AND
%nonassoc GREATER EQUALITY
%left PLUS MINUS
%left TIMES
%right NOT
%left DOT

%%

pgm : instlst ENDOFFILE 
	    { pgmAST = $1; return 0; }
    ;

instlst : classdeflst MAIN LBRACE varlst exprlst RBRACE
          { $$ = newAST(PROGRAM, $1, 0, NULL, yylineno); 
          appendToChildrenList($$, $4); appendToChildrenList($$, $5); }
         ;

classdeflst : classdeflst classdef
              { if($1 == NULL) { $$ = newAST(CLASS_DECL_LIST, $2, 0, NULL, yylineno); }
              else { appendToChildrenList($1, $2); } }
            |  
              { $$ = newAST(CLASS_DECL_LIST, NULL, 0, NULL, yylineno); }
            ;

classdef : CLASS ast_id EXTENDS ast_id LBRACE varlst methlist RBRACE
           { $$ = newAST(CLASS_DECL, $2, 0, NULL, yylineno); 
             appendToChildrenList($$, $4);
             appendToChildrenList($$, $6); appendToChildrenList($$, $7); }
         | CLASS ast_id EXTENDS ast_id LBRACE varlst RBRACE
           { $$ = newAST(CLASS_DECL, $2, 0, NULL, yylineno);
             appendToChildrenList($$, $4); appendToChildrenList($$, $6);
             appendToChildrenList($$, newAST(METHOD_DECL_LIST, $1, 0, NULL, yylineno)); }
         ;

varlst : varlst vardec SEMICOLON
         { if($1 == NULL) { $$ = newAST(VAR_DECL_LIST, $2, 0, NULL, yylineno); }
           else { appendToChildrenList($1, $2); } }
       |  
         { $$ = newAST(VAR_DECL_LIST, NULL, 0, NULL, yylineno); }
       ;

vardec : nattype ast_id
         { $$ = newAST(VAR_DECL, $1, 0, NULL, yylineno); appendToChildrenList($$, $2); }
       | ast_id ast_id
         { $$ = newAST(VAR_DECL, $1, 0, NULL, yylineno); appendToChildrenList($$, $2); }
       ;

exprlst : exprlst expr SEMICOLON
          { appendToChildrenList($1, $2); }
        | expr SEMICOLON
          { $$ = newAST(EXPR_LIST, $1, 0, NULL, yylineno); }
        ;

expr : expr PLUS expr
       { $$ = newAST(PLUS_EXPR, $1, 0, NULL, yylineno); appendToChildrenList($$, $3); }
     | expr MINUS expr
       { $$ = newAST(MINUS_EXPR, $1, 0, NULL, yylineno); appendToChildrenList($$, $3); }
     | expr TIMES expr
       { $$ = newAST(TIMES_EXPR, $1, 0, NULL, yylineno); appendToChildrenList($$, $3); }
     | expr EQUALITY expr
       { $$ = newAST(EQUALITY_EXPR, $1, 0, NULL, yylineno); appendToChildrenList($$, $3); }
     | expr GREATER expr
       { $$ = newAST(GREATER_THAN_EXPR, $1, 0, NULL, yylineno); appendToChildrenList($$, $3); }
     | expr AND expr
       { $$ = newAST(AND_EXPR, $1, 0, NULL, yylineno); appendToChildrenList($$, $3); }
     | NOT expr
       { $$ = newAST(NOT_EXPR, $2, 0, NULL, yylineno); }
     | NATLITERAL
       { $$ = newAST(NAT_LITERAL_EXPR, NULL, atoi(yytext), NULL, yylineno); }
     | NUL
       { $$ = newAST(NULL_EXPR, NULL, 0, NULL, yylineno); }
     | IF LPAREN expr RPAREN LBRACE varlst exprlst RBRACE ELSE LBRACE varlst exprlst RBRACE
       { $$ = newAST(IF_THEN_ELSE_EXPR, $3, 0, NULL, yylineno); appendToChildrenList($$, $6); appendToChildrenList($$, $7);
         appendToChildrenList($$, $11); appendToChildrenList($$, $12); }
     | WHILE LPAREN expr RPAREN LBRACE varlst exprlst RBRACE
       { $$ = newAST(WHILE_EXPR, $3, 0, NULL, yylineno); appendToChildrenList($$, $6); appendToChildrenList($$, $7); }
     | NEW ast_id LPAREN RPAREN
       { $$ = newAST(NEW_EXPR, $2, 0, NULL, yylineno); }
     | THIS
       { $$ = newAST(THIS_EXPR, NULL, 0, NULL, yylineno); }
     | PRINTNAT LPAREN expr RPAREN
       { $$ = newAST(PRINT_EXPR, $3, 0, NULL, yylineno); }
     | READNAT LPAREN RPAREN
       { $$ = newAST(READ_EXPR, NULL, 0, NULL, yylineno); }
     | ast_id
       { $$ = newAST(ID_EXPR, $1, 0, NULL, yylineno); }
     | expr DOT ast_id
       { $$ = newAST(DOT_ID_EXPR, $1, 0, NULL, yylineno); }
     | expr DOT ast_id ASSIGN expr
       { $$ = newAST(DOT_ASSIGN_EXPR, $1, 0, NULL, yylineno); appendToChildrenList($$, $3);
         appendToChildrenList($$, $5); }
     | methcal
       { $$ = $1; }
     | ast_id ASSIGN expr
       { $$ = newAST(ASSIGN_EXPR, $1, 0, NULL, yylineno); appendToChildrenList($$, $3); }
     | LPAREN expr RPAREN
       { $$ = $2; }
     ;

methcal : expr DOT ast_id LPAREN arglist RPAREN
          { $$ = newAST(DOT_METHOD_CALL_EXPR, $1, 0, NULL, yylineno); appendToChildrenList($$, $3);
            appendToChildrenList($$, $5); }
        | ast_id LPAREN arglist RPAREN
          { $$ = newAST(METHOD_CALL_EXPR, $1, 0, NULL, yylineno); appendToChildrenList($$, $3); }
        ;

methlist : methdec
           { $$ = newAST(METHOD_DECL_LIST, $1, 0, NULL, yylineno); }
         | methlist methdec
           { appendToChildrenList($1, $2); }
         ;

methdec : nattype ast_id LPAREN paramlst RPAREN LBRACE varlst exprlst RBRACE
          { $$ = newAST(METHOD_DECL, $1, 0, NULL, yylineno); appendToChildrenList($$, $2);
            appendToChildrenList($$, $4); appendToChildrenList($$, $7);
            appendToChildrenList($$, $8); }
        | ast_id ast_id LPAREN paramlst RPAREN LBRACE varlst exprlst RBRACE
          { $$ = newAST(METHOD_DECL, $1, 0, NULL, yylineno); appendToChildrenList($$, $2);
            appendToChildrenList($$, $4); appendToChildrenList($$, $7);
            appendToChildrenList($$, $8); }
        ;

paramlst : paramlst COMMA paramdec
           { if($1 == NULL) { $$ = newAST(PARAM_DECL_LIST, $3, 0, NULL, yylineno); }
           else { appendToChildrenList($1, $3); } }
         | paramlst paramdec
           { if($1 == NULL) { $$ = newAST(PARAM_DECL_LIST, $2, 0, NULL, yylineno); }
           else { appendToChildrenList($1, $2); } }
         |  
           { $$ = newAST(PARAM_DECL_LIST, NULL, 0, NULL, yylineno); }
         ;

paramdec : nattype ast_id
           { $$ = newAST(PARAM_DECL, $1, 0, NULL, yylineno); appendToChildrenList($$, $2); }
         | ast_id ast_id
           { $$ = newAST(PARAM_DECL, $1, 0, NULL, yylineno); appendToChildrenList($$, $2); }

arglist : expr
          { $$ = newAST(ARG_LIST, $1, 0, NULL, yylineno); }
        | arglist COMMA expr
          { appendToChildrenList($1, $3); }
        |  
          { $$ = newAST(ARG_LIST, NULL, 0, NULL, yylineno); }
        ;

ast_id : ID
         { $$ = newAST(AST_ID, NULL, 0, yytext, yylineno); }
       ;

nattype : NATTYPE
          { $$ = newAST(NAT_TYPE, NULL, 0, NULL, yylineno); }
        ;

%%

int main(int argc, char **argv) {
  //#ifdef YYDEBUG
  //  yydebug = 1;
  //#endif
  if(argc!=2) {
    printf("Usage: dj-parse filename\n");
    exit(-1);
  }
  yyin = fopen(argv[1],"r");
  if(yyin==NULL) {
    printf("ERROR: could not open file %s\n",argv[1]);
    exit(-1);
  }
  yyparse();
  printAST(pgmAST);

  return 0;
}

