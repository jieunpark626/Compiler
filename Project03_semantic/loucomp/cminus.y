/****************************************************/
/* File: tiny.y                                     */
/* The TINY Yacc/Bison specification file           */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/
%{
#define YYPARSER /* distinguishes Yacc output from other code files */

#include "globals.h"
#include "util.h"
#include "scan.h"
#include "parse.h"

#define YYSTYPE TreeNode *
static char * savedName; /* for use in assignments */
static int savedNum;
static int savedLineNo;  /* ditto */
static TreeNode * savedTree; /* stores syntax tree for later return */


static int yylex(void);
%}

%token IF WHILE RETURN INT VOID
%nonassoc RPAREN
%nonassoc ELSE 
%token ID NUM
%token EQ NE LT LE GT GE LPAREN LBRACE RBRACE LCURLY RCURLY COMMA SEMI
%token ERROR 
%left PLUS MINUS 
%left TIMES OVER 
%right ASSIGN


%% /* Grammar for C-Minus */

program : declaration_list { savedTree = $1;};

declaration_list : declaration_list declaration
                    {YYSTYPE t = $1;
                    if (t != NULL)
                    { while (t->sibling != NULL)
                        t = t->sibling;
                        t->sibling = $2;
                        $$ = $1; 
                    } else $$ = $2;
                    }
                    | declaration {$$ = $1;}
                    ;

declaration : var_declaration { $$ = $1; }
            | fun_declaration {$$ = $1;} ;
            ;

identifier : ID 
            {
                $$ = newExpNode(IdK);
                $$->attr.name = copyString(tokenString);
                $$->lineno = lineno;
            }
            ;

number : NUM 
        {
            $$ = newExpNode(ConstK); 
            $$->attr.val = atoi(tokenString);
            $$->type = Integer;
        }
        ;

type_specifier : INT 
                {
                    $$ = newExpNode(TypeK);
                    $$->type = Integer;
                }
                |VOID 
                {
                    $$ = newExpNode(TypeK);
                    $$->type = Void;
                }
                ;

var_declaration : type_specifier identifier SEMI
                {
                    $$ = newDeclNode(VarK);                                             
                    $$->attr.name = copyString($2->attr.name);
                    $$->type = $1->type;
                    $$->lineno = lineno;
                    free($1); free($2);
                }
                | type_specifier identifier LBRACE number RBRACE SEMI
                {
                $$ = newDeclNode(VarK);
                $$->lineno = $2->lineno;
                $$->child[0] = $4;
                
                if ($1->type == Integer){
                    $$->type = IntegerArray;
                }
                else{
                    $$->type =VoidArray;  
                }

                $$->attr.name = $2->attr.name;

                free($1); free($2);
                }
                ;

fun_declaration : type_specifier identifier LPAREN params RPAREN compound_stmt
                  {
                    $$ = newDeclNode(FunK);
                    $$->child[0] = $4;
                    $$->child[1] = $6;
                    $$->lineno = lineno;
                    $$->type = $1->type;
                    $$->attr.name = copyString($2->attr.name);
                  }
                  ;
                

params : param_list {$$ = $1;}
        | VOID 
        {
          $$ = newDeclNode(ParamK);
          $$->type = Void;
          $$->lineno = lineno;
        }
        ;

param_list : param_list COMMA param 
            { YYSTYPE t = $1;
              if (t != NULL) {
                 while (t->sibling != NULL) { t = t->sibling; }
                 t->sibling = $3;
                 $$ = $1;
               } else {
                 $$ = $3;
               }
            }
            | param {$$ = $1;}
            ;

param : type_specifier identifier 
        {
          $$ = newDeclNode(ParamK);
          $$->type = $1->type;
          $$->attr.name = $2->attr.name;
          $$->lineno = lineno; 
        }
        | type_specifier identifier LBRACE RBRACE
        {
          $$ = newDeclNode(ParamK);
          $$->type = $1->type;
          $$->attr.name = $2->attr.name;
          $$->lineno = lineno;
        } 
        ;
compound_stmt
    : LCURLY local_declarations statement_list RCURLY
        {
            $$ = newStmtNode(CompK);
            $$->child[0] = $2; 
            $$->child[1] = $3;
            $$->lineno = lineno;
        }
    ;

local_declarations
    : local_declarations var_declaration
        {
            YYSTYPE t = $1;
            if (t != NULL) {
                while (t->sibling != NULL) 
                t = t->sibling;
                t->sibling = $2;
                $$ = $1;
            } else $$ = $2;
        }
    | /* empty */
        {
            $$ = NULL;
        }
    ;

statement_list : statement_list statement
            {
                 YYSTYPE temp = $1;
                   if(temp == NULL){
                        $$ = $2;
                   }
                   else{
                        while (temp->sibling != NULL)
                            temp = temp->sibling;
                        temp->sibling = $2;
                        $$ = $1; 
                   }
              }
            | { 
                $$ = NULL;
                }
            ;

statement
    : expression_stmt { $$ = $1; }
    | compound_stmt { $$ = $1; }
    | selection_stmt { $$ = $1; }
    | iteration_stmt { $$ = $1; }
    | return_stmt { $$ = $1; }
    ;

expression_stmt
    : expression SEMI
        {
            $$ = $1;
        }
    | SEMI
        {
            $$ = NULL; // Empty statement
        }
    ;

selection_stmt
    : IF LPAREN expression RPAREN statement ELSE statement
        {
            $$ = newStmtNode(IfK);
            $$->child[0] = $3; // Condition
            $$->child[1] = $5; // Then-Statement
            $$->child[2] = $7; // Else-Statement
            $$->lineno = lineno;
        }
    | IF LPAREN expression RPAREN statement
        {
            $$ = newStmtNode(IfK);
            $$->child[0] = $3;
            $$->child[1] = $5; 
            $$->child[2] = NULL;
            $$->lineno = lineno;
        }
    ;

iteration_stmt
    : WHILE LPAREN expression RPAREN statement
        {
            $$ = newStmtNode(WhileK);
            $$->child[0] = $3; // Condition
            $$->child[1] = $5; // Loop Body
            $$->lineno = lineno;
        }
    ;

return_stmt
    : RETURN SEMI
        {
            $$ = newStmtNode(ReturnK);
            $$->lineno = lineno;
        }
    | RETURN expression SEMI
        {
            $$ = newStmtNode(ReturnK);
            $$->child[0] = $2; // Return Value
            $$->lineno = lineno;
        }
    ;

expression
    : var ASSIGN expression
        {
            $$ = newExpNode(AssignK);
            $$->child[0] = $1; 
            $$->child[1] = $3; 
            $$->lineno = lineno;
        }
    | simple_expression
    {
        $$ = $1;
    }
    ;

var : identifier
        {
            $$ = newExpNode(IdK);
            $$->attr.name = $1->attr.name;
            $$->lineno = $1->lineno;
        }
        | identifier LBRACE expression RBRACE
        {
            $$ = newExpNode(IdK);
            $$->attr.name = copyString($1->attr.name);
            $$->child[0] = $3;
            $$->child[0]->attr.name = $3->attr.name;
            $$->lineno = lineno;
        }
        ;

simple_expression
    : additive_expression relop additive_expression
        {
            $$ = newExpNode(OpK);
            $$->attr.op = $2->attr.op; 
            $$->child[0] = $1; 
            $$->child[1] = $3; 
            $$->lineno = $1->lineno;
        }
    | additive_expression
        {
            $$ = $1;
        }
    ;

relop
    : EQ { 
        $$ = newExpNode(OpK);
        $$->attr.op = EQ;
        }
    | NE { 
        $$ = newExpNode(OpK);
        $$->attr.op = NE; }
    | LT { 
        $$ = newExpNode(OpK);
        $$->attr.op = LT; }
    | LE { 
        $$ = newExpNode(OpK);
        $$->attr.op = LE; }
    | GT{ 
        $$ = newExpNode(OpK);
        $$->attr.op = GT; }
    | GE { 
        $$ = newExpNode(OpK);
        $$->attr.op = GE; }
    ;

additive_expression
    : additive_expression addop term
        {
            $$ = newExpNode(OpK);
            $$->attr.op = $2; 
            $$->child[0] = $1; 
            $$->child[1] = $3; 
            $$->lineno = $1->lineno;
        }
    | term
        {
            $$ = $1;
        }
    ;

addop
    : PLUS { $$ = PLUS; }
    | MINUS { $$ = MINUS; }
    ;

term
    : term mulop factor
        {
            $$ = newExpNode(OpK);
            $$->attr.op = $2; // Multiplicative operator
            $$->child[0] = $1; // Left operand
            $$->child[1] = $3; // Right operand
            $$->lineno = lineno;
        }
    | factor
        {
            $$ = $1;
        }
    ;

mulop
    : TIMES { $$ = TIMES; }
    | OVER { $$ = OVER; }
    ;

factor
    : LPAREN expression RPAREN
        {
            $$ = $2;
        }
    | var
        {
            $$ = $1;
        }
    | call
        {
            $$ = $1;
        }
    | NUM
    {
        $$ = newExpNode(ConstK);
        $$->lineno = lineno;
        $$->attr.val = atoi(tokenString);
        $$->type = Integer;
    }
    ;

call
    : identifier LPAREN args RPAREN
        {
            $$ = newExpNode(CallK);
            $$->attr.name = $1->attr.name;
            $$->child[0] = $3; 
            $$->lineno = lineno;
        }
    ;

args
    : arg_list { $$ = $1; }
    | /* empty */
        {
            $$ = NULL;
        }
    ;

arg_list
    : arg_list COMMA expression
        {
            YYSTYPE t = $1;
            if (t != NULL) {
                while (t->sibling != NULL) t = t->sibling;
                t->sibling = $3;
                $$ = $1;
            } else $$ = $3;
        }
    | expression
        {
            $$ = $1;
        }
    ;





%%

int yyerror(char * message)
{ fprintf(listing,"Syntax error at line %d: %s\n",lineno,message);
  fprintf(listing,"Current token: ");
  printToken(yychar,tokenString);
  Error = TRUE;
  return 0;
}

/* yylex calls getToken to make Yacc/Bison output
 * compatible with ealier versions of the TINY scanner
 */
static int yylex(void)
{  
    return getToken();
}

TreeNode * parse(void)
{ yyparse();
  return savedTree;
}

