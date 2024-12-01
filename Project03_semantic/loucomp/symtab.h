/****************************************************/
/* File: symtab.h                                   */
/* Symbol table interface for the TINY compiler     */
/* (allows only one symbol table)                   */
/* Compiler Construction: Principles and Practice   */
/* Kenneth C. Louden                                */
/****************************************************/

#ifndef _SYMTAB_H_
#define _SYMTAB_H_
#include "globals.h"

/* SIZE is the size of the hash table */
#define SIZE 211


/* the list of line numbers of the source 
 * code in which a variable is referenced
 */
typedef struct LineListRec
   { int lineno;
     struct LineListRec * next;
   } * LineList;

/* The record in the bucket lists for
 * each variable, including name, 
 * assigned memory location, and
 * the list of line numbers in which
 * it appears in the source code
 */
typedef struct BucketListRec
   { char * name;
     LineList lines;
     int memloc ; /* memory location for variable */
     struct BucketListRec * next;
     ExpType type; // symbol type
     SymbolKind kind; // symbol kind
     char * scopeName;
   } * BucketList;

// add scope 
typedef struct ScopeListRec {
  char *name; // scope name
  int nestedLevel; // nested level
  struct ScopeListRec *parent; // parent scope
  BucketList symbolTable[SIZE]; // scope's symbol table
  int curloc; // scope 내부의 location
} * Scope;



/* Procedure st_insert inserts line numbers and
 * memory locations into the symbol table
 * loc = memory location is inserted only the
 * first time, otherwise ignored
 */
BucketList st_insert(Scope scope, SymbolKind kind, ExpType type, char * name, int lineno, int loc);

/* Function st_lookup returns the memory 
 * location of a variable or -1 if not found
 */
int st_lookup ( char * name );

/* Procedure printSymTab prints a formatted 
 * listing of the symbol table contents 
 * to the listing file
 */
void printSymTab(FILE * listing);

Scope getCurScope();
Scope createScope(char *name);
void pushScopeToStack(Scope scope);
BucketList checkScope(char *name);
void addSymbol(TreeNode *t, SymbolKind kind);
BucketList findSymbol(char * name);
void addSymbolImplict(TreeNode *t, SymbolKind kind);
void insertLineno(BucketList symbol, int lineno);
Scope findScope(char *name) ;
BucketList findSymbolinCheck(Scope curScope, char *name);

#endif

