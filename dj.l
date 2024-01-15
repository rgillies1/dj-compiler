%option yylineno
%option noyywrap

%{
  #define DEBUG 1
  typedef YYTOKENTYPE Token;
  Token scanned(Token t);
%}

digit   [0-9]
id      [a-zA-Z][0-9a-zA-Z_]*
wspace  [ \t\r\n]+

%%

main            {return scanned(MAIN);}
class           {return scanned(CLASS);}
extends         {return scanned(EXTENDS);}
nat             {return scanned(NATTYPE);}
if              {return scanned(IF);}
else            {return scanned(ELSE);}
while           {return scanned(WHILE);}
printnat        {return scanned(PRINTNAT);}
readnat         {return scanned(READNAT);}
this            {return scanned(THIS);}
new             {return scanned(NEW);}
null            {return scanned(NUL);}
"-"?{digit}+    {return scanned(NATLITERAL);}
{id}            {return scanned(ID);}
"="             {return scanned(ASSIGN);}
"+"             {return scanned(PLUS);}
"-"             {return scanned(MINUS);}
"*"             {return scanned(TIMES);}
"=="            {return scanned(EQUALITY);}
">"             {return scanned(GREATER);}
"&&"            {return scanned(AND);}
"!"             {return scanned(NOT);}
"."             {return scanned(DOT);}
";"             {return scanned(SEMICOLON);}
","             {return scanned(COMMA);}
"{"             {return scanned(LBRACE);}
"}"             {return scanned(RBRACE);}
"("             {return scanned(LPAREN);}
")"             {return scanned(RPAREN);}
<<eof>>         {return scanned(ENDOFFILE);}
"//"                { char c = input();
                             while(c!='\n' && c!=EOF) c=input();
                   }
{wspace}   { /*skip whitespace*/ }
.          { if(DEBUG) printf("\n");
             printf("Lex error on line %d: Illegal character %s\n",
                    yylineno,yytext);
           }

%%

/* process and return scanned token t */
Token scanned(Token t) {
  if(DEBUG==0) return t;
  switch(t) {
    case MAIN: printf("MAIN "); return t;
    case CLASS: printf("CLASS "); return t;
    case EXTENDS: printf("EXTENDS "); return t;
    case NATTYPE: printf("NATTYPE "); return t;
    case IF: printf("IF "); return t;
    case ELSE: printf("ELSE "); return t;
    case WHILE: printf("WHILE "); return t;
    case PRINTNAT: printf("PRINTNAT "); return t;
    case READNAT: printf("READNAT "); return t;
    case THIS: printf("THIS "); return t;
    case NEW: printf("NEW "); return t;
    case NUL: printf("NUL "); return t;
    case NATLITERAL: printf("NATLITERAL(%s) ",yytext); return t;
    case ID: printf("ID(%s) ", yytext); return t;
    case ASSIGN: printf("ASSIGN "); return t;
    case PLUS: printf("PLUS "); return t;
    case MINUS: printf("MINUS "); return t;
    case TIMES: printf("TIMES "); return t;
    case EQUALITY: printf("EQUALITY "); return t;
    case GREATER: printf("GREATER "); return t;
    case AND: printf("AND "); return t;
    case NOT: printf("NOT "); return t;
    case DOT: printf("DOT "); return t;
    case SEMICOLON: printf("SEMICOLON "); return t;
    case COMMA: printf("COMMA "); return t;
    case LBRACE: printf("LBRACE "); return t;
    case RBRACE: printf("RBRACE "); return t;
    case LPAREN: printf("LPAREN "); return t;
    case RPAREN: printf("RPAREN "); return t;
    case ENDOFFILE: printf("ENDOFFILE\n"); return t;
    default: printf("ERROR: invalid token in scanned().\n"); exit(-1);
  }
}

