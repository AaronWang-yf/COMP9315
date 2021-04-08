// bsig.c ... functions on Tuple Signatures (bsig's)
// part of signature indexed files
// Written by John Shepherd, March 2019

#include "defs.h"
#include "reln.h"
#include "query.h"
#include "bsig.h"
#include "psig.h"
#include "bits.h"

void findPagesUsingBitSlices(Query q)
{
	assert(q != NULL);
	//TODO
	RelnParams *rp = &(q->rel->params);
	Bits query_signature = makePageSig(q->rel,q->qstring);
	setAllBits(q->pages);
	//printf("query_sig is :");showBits(query_signature);printf("\n");
          //pm slice to scan
          // bm = width of bit-slice = max pages

          //printf("length of p->pages is :%d\n",bits_length(q->pages));
          //printf("length of rp->npages is %d\n",rp->npages);
          //printf("length of bm is:%d\n",rp->bm);

          PageID temp=0;
          Bool flag = TRUE; //flag to detect if it is first page to check
	for(int i=0;i<rp->pm;i++){
	    if(bitIsSet(query_signature,i)){
                  PageID bit_slice_id = i/rp->bsigPP;
                  Offset b_offset = i%rp->bsigPP;
                  Page bit_slice_page = getPage(q->rel->bsigf,bit_slice_id);
                  Bits bit_slice_sig = newBits(rp->bm);
                  getBits(bit_slice_page,b_offset,bit_slice_sig);
                  q->nsigs++;

                  if(flag==FALSE){
                      if(temp!=bit_slice_id){
                          temp=bit_slice_id;
                          q->nsigpages++;
                      }
                  }

                  if(flag==TRUE){
                      temp = bit_slice_id;
                      q->nsigpages++;
                      flag=FALSE;
                  }

                  //printf("pm bit is %d\n",i);
                  for(int j=0;j<rp->npages;j++){
                      if(!bitIsSet(bit_slice_sig,j)){

                          //printf("page is %d\n",j);
                          unsetBit(q->pages,j);
                      }
                  }
                  freeBits(bit_slice_sig);
                  free(bit_slice_page);
	    }
	}
	freeBits(query_signature);
}

