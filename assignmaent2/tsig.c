// tsig.c ... functions on Tuple Signatures (tsig's)
// part of signature indexed files
// Written by John Shepherd, March 2019

#include <unistd.h>
#include <string.h>
#include "defs.h"
#include "tsig.h"
#include "reln.h"
#include "hash.h"
#include "bits.h"


//referenced from Page 4/12, Lecture Slides Week 5 Signature-based Indexing
Bits codeword(char *attr_value,int m,int k){
    int nbits = 0;
    Bits cword = newBits(m);
    if(strcmp(attr_value,"?")==0) return cword;
    srandom(hash_any(attr_value,strlen(attr_value)));
    while(nbits<k){
        int i = random()%m;
        if(!bitIsSet(cword,i)){
            setBit(cword,i);
            nbits++;
        }
    }
    return cword;
}

Bits codeword_c(char *attr_value,int m,int k,int tuple_length){
    int nbits = 0;
    Bits cword = newBits(tuple_length);
    if(strcmp(attr_value,"?")==0) return cword;
    srandom(hash_any(attr_value,strlen(attr_value)));
    while(nbits<k){
        int i = random()%m;
        if(!bitIsSet(cword,i)){
            setBit(cword,i);
            nbits++;
        }
    }
    return cword;
}




// make a tuple signature
//referenced from Page 6/12, Lecture Slides Week 5 Signature-based Indexing
Bits makeTupleSig(Reln r, Tuple t)
{         assert(r->params.sigtype=='c'|| r->params.sigtype=='s');
	assert(r != NULL && t != NULL);
	//TODO
	int m = r->params.tm; //width of tuple signature (#bits)
	int k =r->params.tk;  // bits set per attribute
	int attr_number = r->params.nattrs;
	int shift_bits = 0;
	Bits desc = newBits(m);
	char **attr_val=NULL;
	attr_val = tupleVals(r,t);
	if(r->params.sigtype=='s') {    //stands for SIMC
              for (int i = 0; i < attr_number; i++) {
                  Bits attr_bits = codeword(attr_val[i], m, k);
                  orBits(desc, attr_bits);
                  freeBits(attr_bits);
              }
          }else{
	    for(int i =0;i<attr_number; i++){
	        int len_attr = m/attr_number;
	        if(i==0)  len_attr = len_attr +m%attr_number;
	        Bits attr_bits = codeword_c(attr_val[i],len_attr,len_attr/2,m);
	        shiftBits(attr_bits,shift_bits);
	        shift_bits = shift_bits + len_attr;
	        orBits(desc,attr_bits);
	        freeBits(attr_bits);
	    }
	}
	freeVals(attr_val,r->params.nattrs);
	free(attr_val);
	return desc;
}

// find "matching" pages using tuple signatures

void findPagesUsingTupSigs(Query q)
{
    assert(q != NULL);
    //TODO
    //find tsig page based on tsigNpages on rel
    //find page of the tuple based on the index of the tuple / max tuple numbers per page on rel
    Bits query_signature = makeTupleSig(q->rel,q->qstring);
    //printf("query tuple sigs:"); showBits(query_signature); putchar('\n');
    PageID sig_page_id = 0;
    PageID max_sig_page = q->rel->params.tsigNpages;
    unsigned int max_sigid_num = q->rel->params.tsigPP;
    unsigned int max_tuple_num = q->rel->params.tupPP;
    File data = tsigFile(q->rel);
    for(sig_page_id=0;sig_page_id<max_sig_page;sig_page_id++){
        Page sig_page = getPage(data,sig_page_id);
        q->nsigpages++;
        //printf("pageNitems:%d\n",pageNitems(sig_page));
        for(Offset tuple_id=0;tuple_id<pageNitems(sig_page);tuple_id++){
            q->nsigs++;
            Bits t_sig = newBits(tsigBits(q->rel));
            getBits(sig_page,tuple_id,t_sig);
            //printf("tuple sigs:"); showBits(t_sig); putchar('\n');
            if(isSubset(query_signature,t_sig)){
                PageID cur_data_page = (sig_page_id*max_sigid_num+tuple_id)/max_tuple_num;
                setBit(q->pages,cur_data_page);
            }
            freeBits(t_sig);
        }
        free(sig_page);
    }

    freeBits(query_signature);

    // The printf below is primarily for debugging
    // Remove it before submitting this function
    //printf("Matched Pages:"); showBits(q->pages); putchar('\n');
}
