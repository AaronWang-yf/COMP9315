/*
 * src/tutorial/intset.c
 *
 ******************************************************************************
  This file contains routines that can be bound to a Postgres backend and
  called by the backend in the process of processing queries.  The calling
  format for these routines is dictated by Postgres architecture.
******************************************************************************/

#include "postgres.h"

#include "fmgr.h"
#include "libpq/pqformat.h"		/* needed for send/recv functions */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>

PG_MODULE_MAGIC;

typedef struct intSet
{
	int32 num;
	int list[FLEXIBLE_ARRAY_MEMBER];
}	intSet;


/*****************************************************************************
 * Input/Output functions
 *****************************************************************************/

static bool check_space_beteen(char *str){
    int first = 0; //number index
    int second =0; //number index
    int comma = 0; //cooma index
    int i =0; //index
    int len = strlen(str);
    for (i=0;i<len;i++){
        if(str[i]>='0' && str[i]<='9'){
            if(first>second){
                second = i;
            } else {
                first = i;
            }
        }
        
        if(str[i]==','){
            if(abs(first-second)>1 && first>i && second>i) 
                return true;
            comma = i;
        }
    }
    if(abs(first-second)>1 && first>comma && second>comma) return true;
    return false;

}



static char* trim(char *str){
    char *p1 = str;
    char *p2 = str;
    while(*p1!='\0'){
        while(*p1 == ' ') {
            p1++;
        }
        *p2 = *p1;
        p2++;
        p1++;
    }
    *p2='\0';
    return str;
}

static bool check_valid(char *str){
    int length = 0;
    int i =0;
    char check_str[]="{}1234567890,";
    length = strlen(str);
    if(length<2)
        return false;
    if(length!=strspn(str,check_str))
        return false;
    if(str[0]!='{' || str[length-1]!='}')
        return false;
    if(length>2){
        if(str[1]==',' || str[length-2]==',')
            return false;
        for(i=1;i<length-1;i++){
            if((str[i]<'0' || str[i]>'9') && str[i]!=',')
                return false;
            if(str[i]==','){
                if(str[i+1]==',')
                    return false;
            }

        }
    }
    return true;
}

static void input(char *input_list, intSet *result){
    int temp=0;
    int i=0;
    int count=1; //number has been inserted
    int m=0;
    if(strlen(input_list)==2){
        result->list[0]=0;
        return;
    }

    for(i=0;i<strlen(input_list);i++){
        if (input_list[i]>='0' && input_list[i]<='9'){
            if(input_list[i]!='0' ||temp!=0){
                temp=(int)(temp*10 +(input_list[i]-'0'));
            }

        } else if (input_list[i]==',' || input_list[i]=='}'){
            //check duplicate;
            m=1;
            while(result->list[m]!=temp && m<count){
                m++;
            }
            if(m==count) {
                result->list[count] = temp;
                count++;
                temp = 0;
            }else {temp=0;}
        }
    }
    result->list[0] = count-1;
    //sort
    for(i=1;i<result->list[0];i++){
        for(m=1;m<=result->list[0]-i;m++){
            if(result->list[m]>result->list[m+1]){
                temp=result->list[m];
                result->list[m]=result->list[m+1];
                result->list[m+1]=temp;
            }
        }
    }
}



PG_FUNCTION_INFO_V1(intset_in);

Datum
intset_in(PG_FUNCTION_ARGS)
{   
    int i = 0;
    int count=1;
    intSet *L=NULL;
    char	   *str = PG_GETARG_CSTRING(0);
    if(check_space_beteen(str)){
                ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                        errmsg("invalid input syntax for type %s: \"%s\"",
                                "intSet", str)));
    }

    str = trim(str);
    if(!check_valid(str)){
        ereport(ERROR,
                (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
                        errmsg("invalid input syntax for type %s: \"%s\"",
                                "intSet", str)));

    }
    
    
    for(i=0;i<strlen(str);i++){
        if (str[i]==',') count++;
    }
    L = (intSet *) palloc(VARHDRSZ + sizeof(int)*(count+1));
    SET_VARSIZE(L,VARHDRSZ + sizeof(int)*(count+1));
    L->list[0]=count;
    //L->list=(long int*) palloc(sizeof(long int)*(L->num));
    input(str,L);
//    ereport(ERROR,
//            (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
//                    errmsg("string is:%d,%d,%d,%d",
//                            L->list[0], L->list[1],L->list[2],L->list[3])));
    PG_RETURN_POINTER(L);
}

PG_FUNCTION_INFO_V1(intset_out);

Datum
intset_out(PG_FUNCTION_ARGS)
{
    intSet    *L = (intSet *) PG_GETARG_POINTER(0);
//    ereport(ERROR,
//            (errcode(ERRCODE_INVALID_TEXT_REPRESENTATION),
//                    errmsg("string is:%d,%d,%d,%d",
//                            L->list[0], L->list[1],L->list[2],L->list[3])));
    char	   *result;
    char *output;
    int i=0;
    char str[32];
    int num = L->list[0];
    result = (char *)malloc(sizeof(char)*3+sizeof(char)*12*num);

    if(L->list[0]==0){
        sprintf(result,"{");
        strcat(result,"}\0");
    }
    else {
        sprintf(result, "{");
        //printf("res = %s\n",result);
        //printf("str=%s\n",str);
        for (i = 1; i < L->list[0]; i++) {
            //printf("num=%ld ",L->list[i]);
            sprintf(str, "%d", L->list[i]);
            strcat(result, str);
            strcat(result, ",");
        }
        sprintf(str, "%d", L->list[i]);
        strcat(result, str);
        strcat(result, "}\0");
    }
    output = psprintf("%s", result);
    free(result);
    PG_RETURN_CSTRING(output);
}


/*****************************************************************************
 * New Operators
 *
 * For intSet.
 *****************************************************************************/

//check countain: i ? S

PG_FUNCTION_INFO_V1(intset_contain);

Datum
intset_contain(PG_FUNCTION_ARGS)
{
    int    a =  PG_GETARG_INT32(0);
    intSet    *S = (intSet *) PG_GETARG_POINTER(1);
    bool    result = false;
    int i=1;
    for(i=1;i<=S->list[0];i++){
        if(a==S->list[i])
            result=true;
    }

    PG_RETURN_BOOL(result);
}


//cardinality |S|: #S

PG_FUNCTION_INFO_V1(intset_card);

Datum
intset_card(PG_FUNCTION_ARGS)
{
    intSet    *S = (intSet *) PG_GETARG_POINTER(0);
    int result=0;
    result = S->list[0];
    PG_RETURN_INT32(result);
}


//belong_left B belong A : A >@ B

PG_FUNCTION_INFO_V1(belong_left);

Datum
belong_left(PG_FUNCTION_ARGS)
{
    intSet    *father = (intSet *) PG_GETARG_POINTER(0);
    intSet    *child = (intSet *) PG_GETARG_POINTER(1);
    bool result = false;
    int i=1;
    int j=1;

    if(child->list[0]==0)
        PG_RETURN_BOOL(true);

    for(i=1;i<=child->list[0];i++){
        result = false;
        for(j=1;j<=father->list[0];j++){
            if(child->list[i]==father->list[j])
                result = true;
        }
        if(result == false)
            PG_RETURN_BOOL(result);
    }

    PG_RETURN_BOOL(result);
}

//belong_right A belong B : A @< B

PG_FUNCTION_INFO_V1(belong_right);

Datum
belong_right(PG_FUNCTION_ARGS)
{
    intSet    *child = (intSet *) PG_GETARG_POINTER(0);
    intSet    *father = (intSet *) PG_GETARG_POINTER(1);
    bool result = false;
    int i=1;
    int j=1;

    if(child->list[0]==0)
        PG_RETURN_BOOL(true);

    for(i=1;i<=child->list[0];i++){
        result = false;
        for(j=1;j<=father->list[0];j++){
            if(child->list[i]==father->list[j])
                result = true;
        }
        if(result == false)
            PG_RETURN_BOOL(result);
    }
    PG_RETURN_BOOL(result);
}



//intset_eq A = B : A = B

PG_FUNCTION_INFO_V1(intset_eq);

Datum
intset_eq(PG_FUNCTION_ARGS)
{
    intSet    *A = (intSet *) PG_GETARG_POINTER(0);
    intSet    *B = (intSet *) PG_GETARG_POINTER(1);
    int i=1;
    bool result = false;
    if(A->list[0]!=B->list[0])
        PG_RETURN_BOOL(result);
    
    for(i=1;i<=A->list[0];i++){
        if(A->list[i]!=B->list[i])
            PG_RETURN_BOOL(result);
    }
    result=true;
    PG_RETURN_BOOL(result);
}

//intset_neq A <> B : A <> B

PG_FUNCTION_INFO_V1(intset_neq);

Datum
intset_neq(PG_FUNCTION_ARGS)
{   int i=1;
    intSet    *A = (intSet *) PG_GETARG_POINTER(0);
    intSet    *B = (intSet *) PG_GETARG_POINTER(1);
    if(A->list[0]!=B->list[0])
        PG_RETURN_BOOL(true);
    
    for(i=1;i<=A->list[0];i++){
        if(A->list[i]!=B->list[i])
            PG_RETURN_BOOL(true);
    }
    PG_RETURN_BOOL(false);
}


//intset_intersect A && B : A && B

PG_FUNCTION_INFO_V1(intset_intersect);

Datum
intset_intersect(PG_FUNCTION_ARGS)
{
    intSet    *A = (intSet *) PG_GETARG_POINTER(0);
    intSet    *B = (intSet *) PG_GETARG_POINTER(1);
    intSet *L=NULL;
    int i=1;
    int j=1;
    int num=0;
    int count=0;
    int max_A=A->list[0];  //number of A
    int max_B=B->list[0];  //number of B
    int temp_A=0;
    int temp_B=0;
    num=A->list[0];
    L = (intSet *) palloc(VARHDRSZ + sizeof(int)*(num+1));
    SET_VARSIZE(L,VARHDRSZ + sizeof(int)*(num+1));

    while(i<=max_A && j<=max_B){
        temp_A = A->list[i];
        temp_B = B->list[j];
        if(temp_A<temp_B){
            i++;
        }
        else if (temp_A>temp_B){
            j++;
        } else {
	        count++;
	        i++;
            j++;
            L->list[count]=temp_B;
	    }
    }
    L->list[0]=count;

    PG_RETURN_POINTER(L);
}


//intset_union A || B : A || B

PG_FUNCTION_INFO_V1(intset_union);

Datum
intset_union(PG_FUNCTION_ARGS)
{
    intSet    *A = (intSet *) PG_GETARG_POINTER(0);
    intSet    *B = (intSet *) PG_GETARG_POINTER(1);
    intSet *L=NULL;
    int i=1; //index of A
    int j=1; //index of B
    int num=0;
    int count=0;
    int max_A=A->list[0];  //number of A
    int max_B=B->list[0];  //number of B
    int temp_A=0;
    int temp_B=0;
    num=A->list[0]+B->list[0];
    L = (intSet *) palloc(VARHDRSZ + sizeof(int)*(num+1));
    SET_VARSIZE(L,VARHDRSZ + sizeof(int)*(num+1));
    while(i<=max_A && j<=max_B){
        temp_A = A->list[i];
        temp_B = B->list[j];
        if(temp_A<temp_B){
            count++;
            i++;
            L->list[count]=temp_A;
        }
        else if (temp_A>temp_B){
            count++;
            j++;
            L->list[count]=temp_B;
        } else {
	    count++;
	    i++;
            j++;
            L->list[count]=temp_B;
	}
    }
    if(i<=max_A){
        for(;i<=max_A;i++){
            temp_A = A->list[i];
            count++;
            L->list[count]=temp_A;
        }
    }
    if(j<=max_B){
        for(;j<=max_B;j++){
            temp_B = B->list[j];
            count++;
            L->list[count]=temp_B;
        }
    }


    L->list[0]=count;

    PG_RETURN_POINTER(L);
}

//intset_disjunction A !! B : A !! B

PG_FUNCTION_INFO_V1(intset_disjunction);

Datum
intset_disjunction(PG_FUNCTION_ARGS)
{
    intSet    *A = (intSet *) PG_GETARG_POINTER(0);
    intSet    *B = (intSet *) PG_GETARG_POINTER(1);
    intSet *L=NULL;
    int i=1;  //index of A
    int j=1;  //index of B
    int num=0;
    int count=0;
    int max_A=A->list[0]; // number of A
    int max_B=B->list[0]; // number of B
    int temp_A=0;
    int temp_B=0;
    num=A->list[0]+B->list[0];
    L = (intSet *) palloc(VARHDRSZ + sizeof(int)*(num+1));
    SET_VARSIZE(L,VARHDRSZ + sizeof(int)*(num+1));
    while(i<=max_A && j<=max_B){
        temp_A = A->list[i];
        temp_B = B->list[j];
        if(temp_A<temp_B){
            count++;
            i++;
            L->list[count]=temp_A;
        }
        else if (temp_A>temp_B){
            count++;
            j++;
            L->list[count]=temp_B;
        }
        else{
            i++;
            j++;
        }


    }
    if(i<=max_A){
        for(;i<=max_A;i++){
            temp_A = A->list[i];
            count++;
            L->list[count]=temp_A;
        }
    }
    if(j<=max_B){
        for(;j<=max_B;j++){
            temp_B = B->list[j];
            count++;
            L->list[count]=temp_B;
        }
    }


    L->list[0]=count;

    PG_RETURN_POINTER(L);
}


//intset_difference A - B : A - B

PG_FUNCTION_INFO_V1(intset_difference);

Datum
intset_difference(PG_FUNCTION_ARGS)
{
    intSet    *A = (intSet *) PG_GETARG_POINTER(0);
    intSet    *B = (intSet *) PG_GETARG_POINTER(1);
    intSet *L=NULL;
    int i=1;  //index of A
    int j=1;  //index of B
    int num=0;
    int count=0;
    int max_A=A->list[0]; // number of A
    int max_B=B->list[0]; // number of B
    int flag = 0;//0:=not in B; 1:=in B
    num=A->list[0];
    L = (intSet *) palloc(VARHDRSZ + sizeof(int)*(num+1));
    SET_VARSIZE(L,VARHDRSZ + sizeof(int)*(num+1));
    for(i=1;i<=max_A;i++){
        flag=0;
        for(j=1;j<=max_B;j++){
            if(A->list[i]==B->list[j]){
                flag=1;
                break;
            }

        }
        if(flag==0){
            count++;
            L->list[count]=A->list[i];
        }
    }
    L->list[0]=count;
    PG_RETURN_POINTER(L);
}
