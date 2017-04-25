#ifndef _SEMANTIC_H_
#define _SEMANTIC_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define HASH_SIZE 65536  

#define INT_TYPE 1
#define FLOAT_TYPE 2

//typdef enum Kind

typedef enum Kind_ {
	BASIC, ARRAY, STRUCTURE, FUNCTION
}Kind;

typedef struct Type_ *TypePtr;
typedef struct FieldList_ *FieldList;

struct Type_ {

	Kind kind;
	union{
		//basic type
		int basic_;

		//array type
		struct {
			int size;
			TypePtr elem;
		}array_;

		//structure type
		FieldList structure_;

		//function type
		struct{
			FieldList params;//parameters
			TypePtr funcType;
			//TypePtr returnType;
			int paramNum;//number of parameters
		}function_;

	}u;
};

struct FieldList_ {
	//Kind kind;
	char *name;
	TypePtr type;
	FieldList tail;
	//FieldList collision;
	int collision;
};


unsigned int hash_pjw(char *name);
void initHashtable();
int insertSymbol(FieldList f);
bool TypeEqual(TypePtr type1,TypePtr type2);
FieldList lookupSymbol(char *name,bool function);//function true,varible false
void AllSymbol();

#endif
