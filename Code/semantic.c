#include "semantic.h"
#include "node.h"

FieldList hashTable[HASH_SIZE];

unsigned int hash_pjw(char *name){
	unsigned int val = 0, i;
	for(;*name;++name){
		val = (val << 2) + *name;
		if(i=val & ~0x3fff){
			val = (val ^ (i>>12)) & 0x3fff;
		}
		return val % HASH_SIZE;
	}
}


void initHashtable(){
	for(int i=0; i<HASH_SIZE; i++){
		hashTable[i] = NULL;
	}
}



int insertSymbol(FieldList f){
	if(f==NULL)
		return 0;
	if(f->name==NULL)
		return 0;
	f->collision = 0;
	unsigned int key = hash_pjw(f->name);
	if(hashTable[key] == NULL){
		hashTable[key] = f;
		return 1;
	}
	
	if(strcmp(hashTable[key]->name, f->name)==0){
		return 1;	//redefine
	}

	while(true){
		key = (++key) % HASH_SIZE;
		f->collision = f->collision + 1;
		if(hashTable[key] == NULL){
			hashTable[key] = f;
			return 1;
		}
	}
	return 0;
}

bool TypeEqual(TypePtr type1,TypePtr type2){
	if((type1==NULL)||(type2==NULL))
		return false;
	if(type1->kind!=type2->kind)
		return false;
	else switch(type1->kind){
		case BASIC:{
			if(type1->u.basic_==type2->u.basic_)
				return true;
			else return false;
		}break;
		case ARRAY:{
			if(TypeEqual(type1->u.array_.elem,type2->u.array_.elem))
				return true;
			else return false;
		}break;
		case STRUCTURE:{
			FieldList field1=type1->u.structure_;
			FieldList field2=type2->u.structure_;
			if((field1!=NULL)&&(field2!=NULL)){
				while((field1!=NULL)&&(field2!=NULL)){
					if(!TypeEqual(field1->type,field2->type)){
						return false;
					}
					field1=field1->tail;
					field2=field2->tail;
				}
				if((field1==NULL)&&(field2==NULL))
					return true;
			}
			return false;
		}break;
		case FUNCTION:{
			if(type1->u.function_.paramNum!=type2->u.function_.paramNum)
				return false;
			FieldList param1=type1->u.function_.params;
			FieldList param2=type2->u.function_.params;
			for(int i=0;i<type1->u.function_.paramNum;i++){
				if(!TypeEqual(param1->type,param2->type))
					return false;
				param1=param1->tail;
				param2=param2->tail;
			}
			return true;
		}break;
		default:{
			return false;
		}break;
	}
}

FieldList lookupSymbol(char *name,bool function){
	if(name == NULL){
		return NULL;
	}
	unsigned int key=hash_pjw(name);
	FieldList p=hashTable[key];	
	
	while(p!=NULL){
		if(strcmp(name,p->name)==0){
			if((function)&&(p->type->kind==FUNCTION))
				return p;
			if((!(function))&&(p->type->kind!=FUNCTION))
				return p;
		}
		int key=(++key)%HASH_SIZE;
		p=hashTable[key];
	}
	return NULL;
}

void AllSymbol(){
	for(int i=0;i<HASH_SIZE;i++)
		if(hashTable[i]!=NULL)
			printf("name:%s,kind:%d\n",hashTable[i]->name,hashTable[i]->type->kind);
}


