#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include "node.h"
#include "semantic.h"

Node* createNode(char* name, char* text){
    Node *pnode = (Node*)malloc(sizeof(Node));
    strcpy(pnode->name, name);
    strcpy(pnode->text, text);

    pnode->lineno = yylineno;
    for(int i=0;i<MAX_CHILD_NUM;i++){
        pnode->child[i] = NULL;
    }
    pnode->childsum = 0;
    return pnode;
}

void addChild(int childsum, Node* parent, ...){
    va_list ap; 
    va_start(ap,parent);
    
    for(int i=0; i<childsum; i++){
        parent->child[i] = va_arg(ap, Node*);
    }
    parent->lineno = parent->child[0]->lineno;
    parent->childsum = childsum;
    va_end(ap);
}

void printTree(Node *parent, int blank){
    if(parent == NULL){
        return;
    }
    for(int i=0;i<blank;i++){
        printf(" ");
    }
    if(parent->childsum != 0){
        printf("%s (%d)\n", parent->name, parent->lineno);
        for(int i=0; i< parent->childsum; i++){
            printTree(parent->child[i],blank+2);
        }
    }else{
        if(strcmp(parent->name, "INT")==0){
            printf("%s: %d\n", parent->name, atoi(parent->text));
        }
        else if(strcmp(parent->name, "FLOAT")==0){
            printf("%s: %f\n", parent->name, atof(parent->text));
        }
        else if(strcmp(parent->name, "ID")==0 || strcmp(parent->name, "TYPE")==0){
            printf("%s: %s\n", parent->name, parent->text);
        }
        else{
            printf("%s\n", parent->name);
        }
    }
}

FieldList VarDec(Node *root,TypePtr basictype){
    Node *temp=root;
    int i=0;
    while(strcmp(temp->child[0]->name,"ID")!=0){
        temp=temp->child[0];
        i++;
    }
    char *s=temp->child[0]->text;

    FieldList field=(FieldList )malloc(sizeof(FieldList_));
    field->name=s;

    if(strcmp(root->child[0]->name,"ID")==0){
        field->type=basictype;
        return field;
    }
    
    switch(i){//allow 2 row array
        case 1:{
            TypePtr var1=(TypePtr)malloc(sizeof(Type_));
            var1->kind=ARRAY;
            var1->u.array_.size=atoi(root->child[2]->text);
            var1->u.array_.elem=basictype;
            field->type=var1;
            return field;
        }break;
        case 2:{
            TypePtr var1=(TypePtr)malloc(sizeof(Type_));
            var1->kind=ARRAY;
            var1->u.array_.size=atoi(root->child[2]->text);
            var1->u.array_.elem=basictype;
            TypePtr var2=(TypePtr)malloc(sizeof(Type_));
            var2->kind=ARRAY;
            var2->u.array_.size=atoi(root->child[0]->child[2]->text);
            var2->u.array_.elem=var1;
            field->type=var2;
            return field;
        }break;
        default:printf("error in VarDec");break;
    }
}

TypePtr Specifier(Node *root){
    TypePtr spe=(TypePtr)malloc(sizeof(Type_));
    if(strcmp(root->child[0]->name,"TYPE")==0){//TYPE
        spe->kind=BASIC;
        if(strcmp(root->child[0]->text,"int")==0)
            spe->u.basic_=INT_TYPE;
        else spe->u.basic_=FLOAT_TYPE;
        return spe;
    }
    else{
        spe->kind=STRUCTURE;
        if(root->child[0]->childsum<3){//STRUCT Tag
            char *s=root->child[0]->child[1]->child[0]->text;
            FieldList field=lookupSymbol(s,false);
            if(field==NULL){
                printf("Error type 17 at Line %d: Undefined structure \"%s\".\n",root->lineno,s);
                return NULL;
            }
            else if(field->type!=NULL)
                return field->type;
        }
        else{//STRUCT OptTag LC DefList RC
            Node* DefList=root->child[0]->child[3];
            spe->u.structure_=NULL;
            //DefList in STRUCT is different from that outside
            while(DefList!=NULL){//Def DefList
                Node *Def=DefList->child[0];
                TypePtr basictype=Specifier(Def->child[0]);
                
                Node* DecList=Def->child[1];
                while(DecList->childsum==3){//Dec COMMA DecList
                    FieldList field=VarDec(DecList->child[0]->child[0],basictype);
                    if(DecList->child[0]->childsum!=1)
                        printf("Error type 15 at Line %d: Variable %s in struct is initialized.\n",Def->lineno,field->name);
                    FieldList temp=spe->u.structure_;
                    while(temp!=NULL){
                        if(strcmp(temp->name,field->name)==0){
                            printf("Error type 15 at Line %d: Redefined field \"%s\".\n",Def->lineno,field->name);
                            break;
                        }
                        temp=temp->tail;
                    }
                    if(temp==NULL){
                        field->tail=spe->u.structure_;
                        spe->u.structure_=field;
                    }
                    DecList=DecList->child[2];
                }
                FieldList field=VarDec(DecList->child[0]->child[0],basictype);
                if(DecList->child[0]->childsum!=1)
                    printf("Error type 15 at Line %d: Variable \"%s\" in struct is initialized.\n",Def->lineno,field->name);
                FieldList temp=spe->u.structure_;
                while(temp!=NULL){
                    if(strcmp(temp->name,field->name)==0){
                        printf("Error type 15 at Line %d: Redefined field \"%s\".\n",Def->lineno,field->name);
                        break;
                    }
                    temp=temp->tail;
                }
                if(temp==NULL){
                    field->tail=spe->u.structure_;
                    spe->u.structure_=field;
                }

                DefList=DefList->child[1];
            }
            if(root->child[0]->child[1]!=NULL){//OptTag exist
                FieldList field=(FieldList )malloc(sizeof(FieldList_));
                field->type=spe;
                char *s=root->child[0]->child[1]->child[0]->text;//get the name of OptTag
                field->name=s;
                if(lookupSymbol(field->name,false)!=NULL)
                    printf("Error type 16 at Line %d: Duplicated name \"%s\".\n",root->lineno,field->name);
                else insertSymbol(field);
            }
            return spe;
        }
    }
}

void ExtDefList(Node *root){
    Node* ExtDefList=root;
    while(ExtDefList->childsum!=0){//ExtDef ExtDefList
        Node* ExtDef=ExtDefList->child[0];
        TypePtr basictype=Specifier(ExtDef->child[0]);
        
        if(strcmp(ExtDef->child[1]->name,"ExtDecList")==0){//Specifier ExtDecList SEMI
            Node* temp=ExtDef->child[1];//ExtDecList
            FieldList field;
            while(temp->childsum==3){
                field=VarDec(temp->child[0],basictype);
                if(lookupSymbol(field->name,false)!=NULL)
                    printf("Error type 3 at Line %d: Redefined variable \"%s\".\n",ExtDef->lineno,field->name);
                else insertSymbol(field);
                temp=temp->child[2];
            }
            field=VarDec(temp->child[0],basictype);
            if(lookupSymbol(field->name,false)!=NULL)
                printf("Error type 3 at Line %d: Redefined variable \"%s\".\n",ExtDef->lineno,field->name);
            else insertSymbol(field);
        }
        else if(strcmp(ExtDef->child[1]->name,"FunDec")==0){//Specifier FunDec CompSt
            FieldList field=(FieldList )malloc(sizeof(FieldList_));
            field->name=ExtDef->child[1]->child[0]->text;
            TypePtr typ=(TypePtr)malloc(sizeof(Type_));
            typ->kind=FUNCTION;
            typ->u.function_.funcType=basictype;
            //ID LP RP already done
            typ->u.function_.paramNum=0;
            typ->u.function_.params=NULL;

            if(strcmp(ExtDef->child[1]->child[2]->name,"VarList")==0){//ID LP VarList RP
                Node *VarList=ExtDef->child[1]->child[2];
                while(VarList->childsum!=1){//ParamDec COMMA VarList
                    TypePtr tempType=Specifier(VarList->child[0]->child[0]);
                    FieldList tempField=VarDec(VarList->child[0]->child[1],tempType);
                    if(lookupSymbol(tempField->name,false)!=NULL)
                        printf("Error type 3 at Line %d: Redefined variable \"%s\".\n",ExtDef->lineno,tempField->name);
                    else insertSymbol(tempField);
                    typ->u.function_.paramNum++;
                    tempField->tail=typ->u.function_.params;
                    typ->u.function_.params=tempField;

                    VarList=VarList->child[2];
                }//ParamDec
                TypePtr tempType=Specifier(VarList->child[0]->child[0]);
                FieldList tempField=VarDec(VarList->child[0]->child[1],tempType);
                if(lookupSymbol(tempField->name,false)!=NULL)
                    printf("Error type 3 at Line %d: Redefined variable \"%s\".\n",ExtDef->lineno,tempField->name);
                else insertSymbol(tempField);
                typ->u.function_.paramNum++;
                tempField->tail=typ->u.function_.params;
                typ->u.function_.params=tempField;
            }
            field->type=typ;
            if(lookupSymbol(field->name,true)!=NULL)
                printf("Error type 4 at Line %d: Redefined function \"%s\".\n",ExtDef->lineno,field->name);
            else insertSymbol(field);

            //CompSt->LC DefList StmtList RC
            CompSt(ExtDef->child[2],basictype);
        }
        else{//Specifier SIMI
            //do nothing
        }

        if(ExtDefList->child[1]==NULL)//ExtDef
            return;
        ExtDefList=ExtDefList->child[1];
    }
}

void CompSt(Node *root,TypePtr funcType){
    Node *CompSt=root;
    DefList(CompSt->child[1]);
    Node *StmtList=CompSt->child[2];
    while(StmtList!=NULL){
        Node *Stmt_=StmtList->child[0];
        Stmt(Stmt_,funcType);
        StmtList=StmtList->child[1];
    }
}

void DefList(Node *root){
    Node* DefList=root;
    while(DefList!=NULL){//Def DefList
        Node* Def=DefList->child[0];
        TypePtr basictype=Specifier(Def->child[0]);

        Node *DecList=Def->child[1];
        while(DecList->childsum==3){//Dec COMMA DecList
            FieldList field=VarDec(DecList->child[0]->child[0],basictype);
            if(lookupSymbol(field->name,false)!=NULL){
                if(lookupSymbol(field->name,false)->type->kind!=STRUCTURE)
                   printf("Error type 3 at Line %d: Redefined variable \"%s\".\n",DecList->lineno,field->name);
                else printf("Error type 16 at Line %d: Duplicated name \"%s\".\n",DecList->lineno,field->name);
            }
            else insertSymbol(field);
            DecList=DecList->child[2];
        }
        FieldList field=VarDec(DecList->child[0]->child[0],basictype);
        if(lookupSymbol(field->name,false)!=NULL){
            if(lookupSymbol(field->name,false)->type->kind!=STRUCTURE)
               printf("Error type 3 at Line %d: Redefined variable \"%s\".\n",DecList->lineno,field->name);
            else printf("Error type 16 at Line %d: Duplicated name \"%s\".\n",DecList->lineno,field->name);
        }
        else insertSymbol(field);

        if(DefList->child[1]==NULL)//Def
            return;
        DefList=DefList->child[1];
    }
}

void Stmt(Node *root,TypePtr funcType){
    Node *Stmt_=root;
    if(strcmp(Stmt_->child[0]->name,"RETURN")==0){//RETURN Exp SEMI
        TypePtr returnType=Exp(Stmt_->child[1]);
        if(!TypeEqual(funcType,returnType))
            printf("Error type 8 at Line %d:Type mismatched for return.\n",Stmt_->lineno);
    }
    else if(strcmp(Stmt_->child[0]->name,"Exp")==0){//Exp
        Exp(Stmt_->child[0]);
    }
    else if(strcmp(Stmt_->child[0]->name,"CompSt")==0){//CompSt
        CompSt(Stmt_->child[0],funcType);
    }
    else if(strcmp(Stmt_->child[0]->name,"WHILE")==0){//WHILE LP Exp RP Stmt
        TypePtr typ=Exp(Stmt_->child[2]);
        if(!((typ->kind==BASIC)&&(typ->u.basic_==INT_TYPE)))
            printf("Error type 9 at Line %d:Only type INT could be used for judgement.\n",Stmt_->lineno);
        Stmt(Stmt_->child[4],funcType);
    }
    else if(Stmt_->childsum<6){//IF LP Exp RP Stmt
        TypePtr typ=Exp(Stmt_->child[2]);
        if(typ!=NULL)
            if(!((typ->kind==BASIC)&&(typ->u.basic_==INT_TYPE)))
                printf("Error type 9 at Line %d:Only type INT could be used for judgement.\n",Stmt_->lineno);

        Stmt(Stmt_->child[4],funcType);
    }
    else{//IF LP Exp RP Stmt ELSE Stmt
        TypePtr typ=Exp(Stmt_->child[2]);
        if(!((typ->kind==BASIC)&&(typ->u.basic_==INT_TYPE)))
            printf("Error type 9 at Line %d:Only type INT could be used for judgement.\n",Stmt_->lineno);
        Stmt(Stmt_->child[4],funcType);
        Stmt(Stmt_->child[6],funcType);
    }
}

TypePtr Exp(Node* root){
    if(root==NULL)
        return NULL;
    else if((strcmp(root->child[0]->name,"ID")==0)&&(root->childsum==1)){//ID
        FieldList field=lookupSymbol(root->child[0]->text,false);
        if(field!=NULL)
            return field->type;
        else{
            printf("Error type 1 at Line %d: Undefined variable \"%s\".\n",root->lineno,root->child[0]->text);
            return NULL;
        }
    }
    else if(strcmp(root->child[0]->name,"INT")==0){//INT
        TypePtr typ=(TypePtr)malloc(sizeof(Type_));
        typ->kind=BASIC;
        typ->u.basic_=INT_TYPE;
        return typ;
    }
    else if(strcmp(root->child[0]->name,"FLOAT")==0){//FLOAT
        TypePtr typ=(TypePtr)malloc(sizeof(Type_));
        typ->kind=BASIC;
        typ->u.basic_=FLOAT_TYPE;
        return typ;
    }
    else if((strcmp(root->child[0]->name,"LP")==0)||(strcmp(root->child[0]->name,"MINUS")==0)||(strcmp(root->child[0]->name,"NOT")==0)){
        return Exp(root->child[1]);
    }
    else if((strcmp(root->child[1]->name,"PLUS")==0)||(strcmp(root->child[1]->name,"MINUS")==0)||(strcmp(root->child[1]->name,"STAR")==0)||(strcmp(root->child[1]->name,"DIV")==0)){
        TypePtr typ1=Exp(root->child[0]);
        TypePtr typ2=Exp(root->child[2]);
        if(!TypeEqual(typ1,typ2)){
            if((typ1!=NULL)&&(typ2!=NULL))
                printf("Error type 7 at Line %d: Type mismatched for operands.\n",root->lineno);
            return NULL;
        }
        else return typ1;
    }
    else if((strcmp(root->child[1]->name,"AND")==0)||(strcmp(root->child[1]->name,"OR")==0)||(strcmp(root->child[1]->name,"RELOP")==0)){
        TypePtr typ1=Exp(root->child[0]);
        TypePtr typ2=Exp(root->child[2]);
        if(!TypeEqual(typ1,typ2)){
            if((typ1!=NULL)&&(typ2!=NULL))
                printf("Error type 7 at Line %d: Type mismatched for operands.\n",root->lineno);
            return NULL;
        }
        else return typ1;
    }
    else if(strcmp(root->child[1]->name,"ASSIGNOP")==0){
        if(root->child[0]->childsum==1){
            if(!(strcmp(root->child[0]->child[0]->name,"ID")==0)){
                printf("Error type 6 at Line %d: The left-hand size of an assignment must be a variable.\n",root->lineno);
                return NULL;
            }
        }
        else if(root->child[0]->childsum==3){
            if(!((strcmp(root->child[0]->child[0]->name,"Exp")==0)&&(strcmp(root->child[0]->child[1]->name,"DOT")==0)&&(strcmp(root->child[0]->child[2]->name,"ID")==0))){
                printf("Error type 6 at Line %d: The left-hand size of an assignment must be a variable.\n",root->lineno);
                return NULL;
            }
        }
        else if(root->child[0]->childsum==4){
            if(!((strcmp(root->child[0]->child[0]->name,"Exp")==0)&&(strcmp(root->child[0]->child[1]->name,"LB")==0)&&(strcmp(root->child[0]->child[2]->name,"Exp")==0)&&(strcmp(root->child[0]->child[3]->name,"RB")==0))){
                printf("Error type 6 at Line %d: The left-hand size of an assignment must be a variable.\n",root->lineno);
                return NULL;
            }
        }
        TypePtr typ1=Exp(root->child[0]);
        TypePtr typ2=Exp(root->child[2]);
        if(!TypeEqual(typ1,typ2)){
            if((typ1!=NULL)&&(typ2!=NULL))
                printf("Error type 5 at Line %d: Type mismatched for assignment.\n",root->lineno);
            return NULL;
        }
        else return typ1;
    }
    else if(strcmp(root->child[0]->name,"ID")==0){//ID LP RP
        FieldList fie=lookupSymbol(root->child[0]->text,true);
        if(fie==NULL){
            FieldList fie2=lookupSymbol(root->child[0]->text,false);
            if(fie2!=NULL)
                printf("Error type 11 at Line %d: \"%s\" is not a function.\n",root->lineno,root->child[0]->text);
            else printf("Error type 2 at Line %d: Undefined function \"%s\".\n",root->lineno,root->child[0]->text);
            return NULL;
        }
        TypePtr definedType=fie->type;

        TypePtr typ=(TypePtr)malloc(sizeof(Type_));
        typ->kind=FUNCTION;
        typ->u.function_.paramNum=0;
        typ->u.function_.params=NULL;
        if(strcmp(root->child[2]->name,"RP")!=0){//ID LP Args RP
            Node* temp=root->child[2];
            while(temp->childsum!=1){//Exp COMMA Args
                TypePtr tempType=Exp(temp->child[0]);
                FieldList tempField=(FieldList )malloc(sizeof(FieldList_));
                tempField->name="no";
                typ->u.function_.paramNum++;
                tempField->tail=typ->u.function_.params;
                typ->u.function_.params=tempField;

                temp=temp->child[2];
            }//Exp
            TypePtr tempType=Exp(temp->child[0]);
            FieldList tempField=(FieldList )malloc(sizeof(FieldList_));
            tempField->name="no";//just for temp compare
            typ->u.function_.paramNum++;
            tempField->tail=typ->u.function_.params;
            typ->u.function_.params=tempField;
        }
        if(!TypeEqual(typ,definedType)){
            printf("Error type 9 at Line %d: Params wrong in function \"%s\".\n",root->lineno,root->child[0]->text);
            return NULL;
        }
        else return definedType->u.function_.funcType;
    }
    else if(strcmp(root->child[1]->name,"DOT")==0){//Exp DOT ID
        TypePtr typ1=Exp(root->child[0]);
        if(typ1->kind!=STRUCTURE){
            Node* temp=root->child[0];
            char *s;
            switch(temp->childsum){
                case 1:{
                    if(strcmp(temp->child[0]->name,"ID")==0)
                        s=temp->child[0]->text;
                }break;
                case 3:{
                    if(strcmp(temp->child[2]->name,"ID")==0)
                        s=temp->child[0]->text;
                }break;
                case 4:{
                    if(strcmp(temp->child[0]->name,"Exp")==0)
                        if(strcmp(temp->child[0]->child[0]->name,"ID")==0)
                            s=temp->child[0]->child[0]->text;
                }break;
                default:s="error";break;
            }
            if(lookupSymbol(s,false)!=NULL)
                printf("Error type 13 at Line %d: Illegal use of \".\".\n",root->lineno);
            return NULL;
        }
        char *s=root->child[2]->text;
        FieldList temp=typ1->u.structure_;
        while(temp!=NULL){
            if(strcmp(temp->name,s)==0)
                return temp->type;

            temp=temp->tail;
        }
        
        printf("Error type 14 at Line %d: Non-existent field \"%s\".\n",root->lineno,root->child[2]->text);
        return NULL;
    }
    else if(strcmp(root->child[1]->name,"LB")==0){//Exp LB Exp RB
        TypePtr typ1=Exp(root->child[0]);
        if(typ1->kind!=ARRAY){
            Node* temp=root->child[0];
            char *s;
            switch(temp->childsum){
                case 1:{
                    if(strcmp(temp->child[0]->name,"ID")==0)
                        s=temp->child[0]->text;
                }break;
                case 3:{
                    if(strcmp(temp->child[2]->name,"ID")==0)
                        s=temp->child[0]->text;
                }break;
                case 4:{
                    if(strcmp(temp->child[0]->name,"Exp")==0)
                        if(strcmp(temp->child[0]->child[0]->name,"ID")==0)
                            s=temp->child[0]->child[0]->text;
                }break;
                default:s="error";break;
            }
            if(lookupSymbol(s,false)!=NULL)
                printf("Error type 10 at Line %d: \"%s\" is not an array.\n",root->lineno,s);
            return NULL;
        }
        TypePtr temp=Exp(root->child[2]);
        if(temp->kind!=BASIC){
            printf("Error type 12 at Line %d: there is not a integer between \"[\" and \"]\".\n",root->lineno);
            return NULL;
        }
        else if(temp->u.basic_==FLOAT_TYPE){
            printf("Error type 12 at Line %d: there is not a integer between \"[\" and \"]\".\n",root->lineno);
            return NULL;
        }
        //no error
        TypePtr var1=(TypePtr)malloc(sizeof(Type_));
        var1->kind=ARRAY;
        var1->u.array_.size=1;
        var1->u.array_.elem=typ1;
        return var1;
    }
    else{
        printf("in\n");
        return NULL;
    }
}
void traverseTree(Node *root){
    if(root==NULL)
        return;

    if(strcmp(root->name,"ExtDefList")==0){
        ExtDefList(root);
        return;
    }

    if(strcmp(root->name,"DefList")==0){
        DefList(root);
        return;
    }

    if(root->childsum!=0){
        for(int i=0;i<root->childsum;i++)
            traverseTree(root->child[i]);
    }
}
