/****************************************************/
/* File: analyze.c                                  */
/* Semantic analyzer implementation                 */
/* for the TINY compiler                            */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include "globals.h"
#include "symtab.h"
#include "analyze.h"
#include "util.h"

/* counter for variable memory locations */
static int location = 0;

//for catch function compound
static int enterFunc = 0; // 0: flase, 1: true


// postProc : exit scope
static void exitScope(TreeNode *t){
  if (t->nodekind == StmtK && t->kind.stmt == CompK) {
    popScopeInStack(); // Compound Statement 종료 시 스코프 pop
  }

  // if (t->nodekind == DeclK && t->kind.decl == FunK) {
  //   fprintf(listing,"cur scope is %s\n",getCurScope()->name);
  //   popScopeInStack(); // 함수 정의 종료 시 스코프 pop
  // }

}

/* Procedure traverse is a generic recursive 
 * syntax tree traversal routine:
 * it applies preProc in preorder and postProc 
 * in postorder to tree pointed to by t
 */
static void traverse( TreeNode * t,
               void (* preProc) (TreeNode *),
               void (* postProc) (TreeNode *) )
{ if (t != NULL)
  { preProc(t);
    { int i;
      for (i=0; i < MAXCHILDREN; i++)
        traverse(t->child[i],preProc,postProc);
    }
    postProc(t);
    traverse(t->sibling,preProc,postProc);
  }
}

/* nullProc is a do-nothing procedure to 
 * generate preorder-only or postorder-only
 * traversals from traverse
 */
static void nullProc(TreeNode * t)
{ if (t==NULL) return;
  else return;
}

/* Procedure insertNode inserts 
 * identifiers stored in t into 
 * the symbol table 
 */
static void insertNode(TreeNode * t)
{ switch (t->nodekind)
  { 
    case DeclK:{
       BucketList symbol = NULL;
      switch(t->kind.decl)
      {
        case FunK: {
          symbol = checkScope(t->attr.name);
  
          if (symbol != NULL){ // already exist in current scope
            printRedefinedError(symbol);
            break; // 
          } else{

            addSymbol(t, Function); //add function symbol in current scope
            insertScope(t->attr.name); // make new scope for function
            enterFunc = 1;
          }
          break;
        }
        case VarK: {
          BucketList symbol = checkScope(t->attr.name);
         
          if (symbol != NULL){ // already exist in current scope
            printRedefinedError(symbol);
            insertLineno(symbol, t->lineno);
          } 

          if(t->type == Void || t->type == VoidArray) { //void & voidarray type can not be declared
            fprintf(listing, "Error: The void-type variable is declared at line %d (name : \"%s\")\n", t->lineno, t->attr.name);
          }

          addSymbol(t, Variable);
          break;
        }

        case ParamK: {
          if(t->type==0){ // if param is void
            break;
          } 
          
          BucketList symbol = checkScope(t->attr.name);
          if (symbol != NULL){ // already exist in current scope
            printRedefinedError(symbol);
          }
          addSymbol(t, Variable);

          break;
        }
        default:
          break;
      }
      break;
    }
    case StmtK:
      switch (t->kind.stmt)
      { 
        case CompK:{
          if(enterFunc == 0){ // not function compound, inner compound
            insertScope(getCurScope()->name); // create compound scope with same name of func
          }
          enterFunc = 0; //if function compound -> change to false
          break;
        }
        case IfK:
        case WhileK:
        case ReturnK:
        default:
          break;
      }
      break;
    case ExpK:
      switch (t->kind.exp)
      {BucketList symbol = NULL;
        case IdK:
          //undeclared check
          symbol = findSymbol(t->attr.name);
          if(symbol==NULL){
            fprintf(listing, "Error: undeclared variable \"%s\" is used at line %d\n", t->attr.name, t->lineno);
            addSymbolImplict(t,Variable);
          } else{
            t->type = symbol->type;
            insertLineno(symbol, t->lineno);
          }
          break;
        case CallK:
          //undeclared check
          symbol = findSymbol(t->attr.name);
          if(symbol==NULL){
            fprintf(listing, "Error: undeclared function \"%s\" is called at line %d\n", t->attr.name, t->lineno);
            addSymbolImplict(t,Function);
          } else{
            t->type = symbol->type;
            insertLineno(symbol, t->lineno);
          }
          break;
        case AssignK:
        case OpK:
        case ConstK:
        default:
          break;
      }
      break;

    default:
      break;
  }
}

/* Function buildSymtab constructs the symbol 
 * table by preorder traversal of the syntax tree
 */
void buildSymtab(TreeNode * syntaxTree)
{ 
  
  Scope globalScope = createScope("Global"); // have to make global scope first
  pushScopeToStack(globalScope); // push global scope to top of stack
  addBuiltinFunc(globalScope); // add built in function in lobal scope
  traverse(syntaxTree,insertNode,exitScope);
  popScopeInStack();

  if (TraceAnalyze)
  { fprintf(listing,"\nSymbol table:\n\n");
    printSymTab(listing);
  }
}

static void typeError(TreeNode * t, char * message)
{ fprintf(listing,"Type error at line %d: %s\n",t->lineno,message);
  Error = TRUE;
}

/* Procedure checkNode performs
 * type checking at a single tree node
 */
static void checkNode(TreeNode * t)
{ switch (t->nodekind)
  { case DeclK:
      switch (t->kind.decl){
    case ParamK:
        // If parameter is void and no name is provided, it means no parameters
        if (t->type == Void && 
            (t->attr.name == NULL || strcmp(t->attr.name, "(null)") == 0)) {
            // No parameters case, do nothing
            break;
        }

        // If parameter has void type but has a name, it's an error
        if (t->type == Void) {
            fprintf(listing, 
                    "Error: The void-type variable is declared at line %d (name : \"%s\")\n",
                    t->lineno, t->attr.name);
        }
        break;

        case VarK:
        case FunK:
        default :
          break;
      }
      break;
    case ExpK:
      switch (t->kind.exp)
      { 
        BucketList symbol = NULL;
        case AssignK:
          if(t->child[0]->type == Integer){
            if (t->child[1]->type!=Integer)
              fprintf(listing, "Error: invalid assignment at line %d\n", t->child[1]->lineno);
            break;
          }
          else if(t->child[0]->type == IntegerArray){
            
            if(t->child[1]->type!=Integer)
            fprintf(listing, "Error: invalid assignment at line %d\n", t->child[1]->lineno);
          break;
          }
           t->type = t->child[0]->type;
          break;
        case OpK:
          if(t->child[0]->type != Integer || t->child[1]->type != Integer){
            // fprintf(listing, "LHS lineno is %d and RHS lineno is %d\n", t->child[0]->lineno,t->child[1]->lineno);
            fprintf(listing, "Error: invalid operation at line %d\n", t->child[0]->lineno);
          }
          t->type = Integer;
          break;
        case ConstK:
          t->type = Integer;
          break;
        
        case IdK:
          if (t->child[0] != NULL) { 
            if (t->type != IntegerArray) {
              fprintf(listing, "Error: Invalid array indexing at line %d (name : \"%s\"). indexing can only allowed for int[] variables\n", t->lineno, t->attr.name);
            } else if (t->child[0]->type != Integer) {
              fprintf(listing, "Error: Invalid array indexing at line %d (name : \"%s\"). indicies should be integer\n", t->child[0]->lineno, t->attr.name);
            }
          }
          break;
        case CallK: {
          
          Scope curScope = findScope(t->attr.name);
          BucketList symbol = findSymbolinCheck(curScope, t->attr.name);
          if (symbol == NULL) {
            fprintf(listing, "Error: Function \"%s\" is not defined at line %d\n", t->attr.name, t->lineno);
          } else {
            if (symbol->kind != Function) {
              fprintf(listing, "Error: \"%s\" is not a function, but called as one at line %d\n", t->attr.name, t->lineno);
            }
          }
          break;
        }
        default:
          break;
      }
      break;
    case StmtK:
      switch (t->kind.stmt)
      { 
        case IfK:
        case WhileK:
          if (t->child[0]->type != Integer) { // 조건문이 정수가 아닌 경우
            fprintf(listing, "Error: invalid condition at line %d\n", t->child[0]->lineno);
          }
          break;
        
        case ReturnK:
          if (t->child[0] != NULL && t->child[0]->type != Integer) { // 반환 타입이 잘못된 경우
            fprintf(listing, "Error: Invalid return at line %d\n", t->lineno);
          }
          break;
        case CompK:
        default:
          break;
      }
      break;
    default:
      break;

  }
}

/* Procedure typeCheck performs type checking 
 * by a postorder syntax tree traversal
 */
void typeCheck(TreeNode * syntaxTree)
{ traverse(syntaxTree,nullProc,checkNode);
}


void addBuiltinFunc(Scope globalScope){
  BucketList input = st_insert(globalScope, Function, Integer, "input", 0, globalScope->curloc++);
  BucketList output = st_insert(globalScope, Function, Void, "output", 0, globalScope->curloc++);
  st_insert(globalScope, Variable, Integer, "value", 0,0);
}

void printRedefinedError(BucketList symbol){
  fprintf(listing, "Error: Symbol \"%s\" is redefined at line (already defined at line ", symbol->name);
  // Program to sequentially print all the line numbers
  LineList line = symbol->lines; // 심볼의 LineList 가져오기
  while (line != NULL) {
      fprintf(listing, "%d", line->lineno); // 현재 라인 번호 출력
      if (line->next != NULL) {
          fprintf(listing, ", "); // 다음 라인이 있으면 콤마 추가
      }
      line = line->next; // 다음 라인으로 이동
  }
  fprintf(listing, ")\n");
}