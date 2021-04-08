// psig.c ... functions on page signatures (psig's)
// part of signature indexed files
// Written by John Shepherd, March 2019

#include "defs.h"
#include "reln.h"
#include "query.h"
#include "hash.h"
#include "psig.h"



//referenced from Page 4/12, Lecture Slides Week 5 Signature-based Indexing
Bits codeword_p(char *attr_value,int m,int k){
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

Bits codeword_p_c(char *attr_value,int m,int k,int tuple_length){
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

Bits makePageSig(Reln r, Tuple t)
{         assert(r->params.sigtype=='c'|| r->params.sigtype=='s');
	assert(r != NULL && t != NULL);
	//TODO
	int m = r->params.pm; //width of tuple signature (#bits)
	int k =r->params.tk;  // bits set per attribute
	int attr_number = r->params.nattrs;
	int shift_bits =0 ;
	Bits desc = newBits(m);
	char **attr_val=NULL;
	attr_val = tupleVals(r,t);
	if(r->params.sigtype=='s') {
              for (int i = 0; i < attr_number; i++) {
                  Bits attr_bits = codeword_p(attr_val[i], m, k);
                  orBits(desc, attr_bits);
                  freeBits(attr_bits);
              }
          } else {
              for (int i = 0; i < attr_number; i++) {
                  int len_attr = m / attr_number;
                  int tuplePP = r->params.tupPP;
                  if (i == 0) len_attr = len_attr + m % attr_number;
                  Bits attr_bits = codeword_p_c(attr_val[i], len_attr, len_attr / (2*tuplePP), m);
                  shiftBits(attr_bits, shift_bits);
                  shift_bits = shift_bits + len_attr;
                  orBits(desc, attr_bits);
                  freeBits(attr_bits);
              }
          }
	freeVals(attr_val,r->params.nattrs);
	free(attr_val);
	return desc;
}

void findPagesUsingPageSigs(Query q)
{
	assert(q != NULL);
	//TODO
	//scan psig_page to find match
	Bits query_signature = makePageSig(q->rel,q->qstring);
	PageID sig_page_id = 0;
	PageID max_sig_page = q->rel->params.psigNpages; //max psig_page
          unsigned int max_sigid_num = q->rel->params.psigPP;
          File data = psigFile(q->rel);
          for(sig_page_id=0;sig_page_id<max_sig_page;sig_page_id++){
              Page sig_page = getPage(data,sig_page_id);
              q->nsigpages++;
              for(Offset ele_idx=0;ele_idx<pageNitems(sig_page);ele_idx++) {
                  Bits p_sig = newBits(psigBits(q->rel));
                  getBits(sig_page,ele_idx,p_sig);
                  q->nsigs++;
                  if(isSubset(query_signature,p_sig)){
                      PageID cur_data_page = sig_page_id*max_sigid_num+ele_idx;
                      setBit(q->pages,cur_data_page);
                  }
                  freeBits(p_sig);
              }
              free(sig_page);
          }
	freeBits(query_signature);
}

