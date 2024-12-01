/****************************************************/
/* File: symtab.c                                   */
/* Symbol table implementation for the TINY compiler*/
/* (allows only one symbol table)                   */
/* Symbol table is implemented as a chained         */
/* hash table                                       */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "symtab.h"
#include "util.h"



/* SHIFT is the power of two used as multiplier
   in hash function  */
#define SHIFT 4

/* the hash function */
static int hash ( char * key )
{ int temp = 0;
  int i = 0;
  while (key[i] != '\0')
  { temp = ((temp << SHIFT) + key[i]) % SIZE;
    ++i;
  }
  return temp;
}


/* the hash table */
//static BucketList hashTable[SIZE]; -> delete global hashTable

static Scope scopeStack[SIZE];
static int stackTop = 0;

static Scope scopeList[SIZE];
static int listTop = 0;

/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */
// void st_insert( char * name, int lineno, int loc )
// { int h = hash(name);
//   BucketList l =  hashTable[h];
//   while ((l != NULL) && (strcmp(name,l->name) != 0))
//     l = l->next;
//   if (l == NULL) /* variable not yet in table */
//   { l = (BucketList) malloc(sizeof(struct BucketListRec));
//     l->name = name;
//     l->lines = (LineList) malloc(sizeof(struct LineListRec));
//     l->lines->lineno = lineno;
//     l->memloc = loc;
//     l->lines->next = NULL;
//     l->next = hashTable[h];
//     hashTable[h] = l; }
//   else /* found in table, so just add line number */
//   { LineList t = l->lines;
//     while (t->next != NULL) t = t->next;
//     t->next = (LineList) malloc(sizeof(struct LineListRec));
//     t->next->lineno = lineno;
//     t->next->next = NULL;
//   }
// } /* st_insert */

// scope's symbol table 에 symbol insert
BucketList st_insert(Scope scope, SymbolKind kind, ExpType type, char * name, int lineno, int loc){
  int h = hash(name);
   BucketList l =  scope->symbolTable[h];
   while ((l != NULL) && (strcmp(name,l->name) != 0))
    l = l->next;
   if (l == NULL) /* variable not yet in table */ { 
    l = (BucketList) malloc(sizeof(struct BucketListRec));
    l->name = name;
    l->lines = (LineList) malloc(sizeof(struct LineListRec));
    l->lines->lineno = lineno;
    l->memloc = loc;
    l->lines->next = NULL;
    l->next = scope->symbolTable[h];
    l->type = type; // symbol type (func: return type)
    l->kind = kind; // symbol kind
    scope->symbolTable[h] = l; 
    }
    else return NULL;
  return l;
}
/* Function st_lookup returns the memory 
 * location of a variable or -1 if not found
 */
// int st_lookup ( char * name )
// { int h = hash(name);
//   BucketList l =  hashTable[h];
//   while ((l != NULL) && (strcmp(name,l->name) != 0))
//     l = l->next;
//   if (l == NULL) return -1;
//   else return l->memloc;
// }

/* Procedure printSymTab prints a formatted 
 * listing of the symbol table contents 
 * to the listing file
 */
// void printSymTab(FILE * listing)
// { int i;
//   fprintf(listing,"Variable Name  Location   Line Numbers\n");
//   fprintf(listing,"-------------  --------   ------------\n");
//   for (i=0;i<SIZE;++i)
//   { if (hashTable[i] != NULL)
//     { BucketList l = hashTable[i];
//       while (l != NULL)
//       { LineList t = l->lines;
//         fprintf(listing,"%-14s ",l->name);
//         fprintf(listing,"%-8d  ",l->memloc);
//         while (t != NULL)
//         { fprintf(listing,"%4d ",t->lineno);
//           t = t->next;
//         }
//         fprintf(listing,"\n");
//         l = l->next;
//       }
//     }
//   }
// } /* printSymTab */

void printSymTab(FILE *listing) {
    fprintf(listing, "< Symbol Table >\n");
    fprintf(listing, " %-12s %-12s %-12s %-12s %-10s %s\n", 
            "Symbol Name", "Symbol Kind", "Symbol Type", 
            "Scope Name", "Location", "Line Numbers");
    fprintf(listing, "-------------  -----------  -------------  ------------  --------  ------------\n");

    // 스코프 리스트 순회
    for (int i = 0; i < listTop; i++) {
        Scope scope = scopeList[i];
        for (int j = 0; j < SIZE; j++) {
            BucketList bucket = scope->symbolTable[j];
            while (bucket != NULL) {
                // 심볼 정보 출력
                fprintf(listing, " %-12s %-12s %-12s %-12s %-10d ", 
                        bucket->name, 
                        bucket->kind == Variable ? "Variable" : 
                        bucket->kind == Function ? "Function" : "Other",
                        bucket->type == Integer ? "int" : 
                        bucket->type == IntegerArray ? "int[]" : 
                        bucket->type == Void ? "void" :
                        bucket->type == Undetermined ? "undetermined" : "unknown",
                        
                        scope->name, 
                        bucket->memloc);

                // 라인 번호 출력
                LineList line = bucket->lines;
                while (line != NULL) {
                    fprintf(listing, "%d ", line->lineno);
                    line = line->next;
                }
                fprintf(listing, "\n");

                bucket = bucket->next;
            }
        }
    }
    fprintf(listing, "\n");
}



Scope getCurScope() {
  return scopeStack[stackTop-1];
}


// create new scope
Scope createScope(char *name){
  Scope newScope = (Scope) malloc(sizeof (struct ScopeListRec)); // create new scope
  newScope->name = name;
  newScope->nestedLevel = 0;
  newScope->curloc = 0;

  return newScope;
}

void pushScopeToStack(Scope scope){
   scopeStack[stackTop++] = scope;
}

void insertScope(char *name){
  Scope curScope = getCurScope(); 
  Scope newScope = createScope(name);
  newScope->parent = curScope;
  newScope->nestedLevel = (curScope->nestedLevel)+1;

  pushScopeToStack(newScope); // 스택에 현재 스코프 push
}



BucketList checkScope(char *name){
  Scope curScope = getCurScope();

  int h = hash(name);
  BucketList symbol = curScope->symbolTable[h];
  while ((symbol != NULL) && (strcmp(name, symbol->name) != 0)) {
        symbol = symbol->next;     
  } 
  return symbol; // NULL if not found
}

void addSymbol(TreeNode *t, SymbolKind kind){
  Scope curScope = getCurScope();
  //void parameter 면???????
  st_insert(curScope, kind, t->type, t->attr.name, t->lineno, curScope->curloc++);
}

void popScopeInStack(void){
  scopeList[listTop++] = scopeStack[stackTop-1];
  scopeStack[stackTop--] = NULL;

}

//해당하는 symbol 을 return 
BucketList findSymbol(char * name){
  Scope curScope = getCurScope();
  int h = hash(name);
  while(curScope != NULL){
    BucketList symbol = curScope->symbolTable[h];
    while((symbol != NULL) && (strcmp(name, symbol->name) != 0)){
      symbol = symbol->next;
    }
    if (symbol != NULL) return symbol;
    curScope = curScope->parent;
  }
  return NULL; // not found
}

void addSymbolImplict(TreeNode *t, SymbolKind kind){
  Scope curScope = getCurScope();
  st_insert(curScope, kind, Undetermined, t->attr.name, t->lineno, curScope->curloc++);
}

void insertLineno(BucketList symbol, int lineno){
  LineList t = symbol->lines;
  while (t->next != NULL) t = t->next;
  t->next = (LineList) malloc(sizeof(struct LineListRec));
  t->next->lineno = lineno;
  t->next->next = NULL;
}

Scope findScope(char *name) {
    int h = hash(name); 
    for (int i = 0; i < listTop; i++) { 
        Scope curScope = scopeList[i]; 
        BucketList symbol = curScope->symbolTable[h];

        while (symbol != NULL) { 
            if (strcmp(symbol->name, name) == 0) { 
                return curScope; 
            }
            symbol = symbol->next;
        }
    }
    
    return NULL;
}

BucketList findSymbolinCheck(Scope curScope, char *name) {
    int h = hash(name); 
    while (curScope != NULL) { 
        BucketList symbol = curScope->symbolTable[h];
        // 현재 스코프의 해시 버킷 탐색
        while (symbol != NULL) {
            
            if (strcmp(name, symbol->name) == 0) { // 이름이 일치하면 반환
                return symbol;
            }
            symbol = symbol->next; // 다음 심볼 확인
        }
        curScope = curScope->parent; // 부모 스코프로 이동
    }
    return NULL; // 심볼을 찾지 못한 경우
}
