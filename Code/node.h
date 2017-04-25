#ifndef _MY_NODE_H_
#define _MY_NODE_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <stdarg.h>
#include "semantic.h"

#define MAX_CHILD_NUM 7
extern int yylineno;
//抽象语法树
typedef struct Abstract_Tree{
    char name[32];  
    char text[32];
    int lineno;
    int childsum;
    struct Abstract_Tree *child[MAX_CHILD_NUM];
}Node;

Node* createNode(char* name, char* text);
void addChild(int childsum, Node* parent, ...);
void printTree(Node *root, int blank);


void traverseTree(Node *root);
FieldList VarDec(Node *root,TypePtr basictype);
TypePtr Specifier(Node *root);
void ExtDefList(Node *root);
void CompSt(Node *root,TypePtr funcType);
void DefList(Node *root);
void Stmt(Node *root,TypePtr funcType);
TypePtr Exp(Node* root);
#endif
