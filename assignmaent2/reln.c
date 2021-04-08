// reln.c ... functions on Relations
// part of signature indexed files
// Written by John Shepherd, March 2019

#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "defs.h"
#include "reln.h"
#include "page.h"
#include "tuple.h"
#include "tsig.h"
#include "bits.h"
#include "hash.h"
#include "psig.h"

// open a file with a specified suffix
// - always open for both reading and writing

File openFile(char *name, char *suffix)
{
	char fname[MAXFILENAME];
	sprintf(fname,"%s.%s",name,suffix);
	File f = open(fname,O_RDWR|O_CREAT,0644);
	assert(f >= 0);
	return f;
}

// create a new relation (five files)
// data file has one empty data page

Status newRelation(char *name, Count nattrs, float pF, char sigtype,
                   Count tk, Count tm, Count pm, Count bm)
{
	Reln r = malloc(sizeof(RelnRep));
	RelnParams *p = &(r->params);
	assert(r != NULL);
	p->nattrs = nattrs;
	p->pF = pF,
	p->sigtype = sigtype;
	p->tupsize = 28 + 7*(nattrs-2);
	Count available = (PAGESIZE-sizeof(Count));
	p->tupPP = available/p->tupsize;
	p->tk = tk; 
	if (tm%8 > 0) tm += 8-(tm%8); // round up to byte size
	p->tm = tm; p->tsigSize = tm/8; p->tsigPP = available/(tm/8);
	if (pm%8 > 0) pm += 8-(pm%8); // round up to byte size
	p->pm = pm; p->psigSize = pm/8; p->psigPP = available/(pm/8);
	if (p->psigPP < 2) { free(r); return -1; }
	if (bm%8 > 0) bm += 8-(bm%8); // round up to byte size
	p->bm = bm; p->bsigSize = bm/8; p->bsigPP = available/(bm/8);
	if (p->bsigPP < 2) { free(r); return -1; }
	r->infof = openFile(name,"info");
	r->dataf = openFile(name,"data");
	r->tsigf = openFile(name,"tsig");
	r->psigf = openFile(name,"psig");
	r->bsigf = openFile(name,"bsig");
	addPage(r->dataf); p->npages = 1; p->ntups = 0;
	addPage(r->tsigf); p->tsigNpages = 1; p->ntsigs = 0;
	addPage(r->psigf); p->psigNpages = 1; p->npsigs = 0;
	addPage(r->bsigf); p->bsigNpages = 1; p->nbsigs = 0; // replace this
	// Create a file containing "pm" all-zeroes bit-strings,
    // each of which has length "bm" bits
	//TODO
	// have a loop from 0 to pm-1. To insert bm-bits into bsig_page
	// add new page when page is full
          Bits new_sig = newBits(p->bm);
	for(p->nbsigs=0;p->nbsigs<p->pm;p->nbsigs++){
	    PageID bit_slice_id = p->bsigNpages-1;
	    Page bit_slice_page = getPage(r->bsigf,bit_slice_id);

	    if(pageNitems(bit_slice_page)==p->bsigPP){
	        //printf("prev page is :%d\n",p->bsigNpages);
	        addPage(r->bsigf);
	        p->bsigNpages++;
	        bit_slice_id++;
	        bit_slice_page=newPage();
	        if(bit_slice_page==NULL) return NO_PAGE;
	    }
	    putBits(bit_slice_page,pageNitems(bit_slice_page),new_sig);
	    addOneItem(bit_slice_page);
	    putPage(r->bsigf,bit_slice_id,bit_slice_page);

	}
          freeBits(new_sig);
	//printf("whole bsig page :%d\n",p->bsigNpages);
	closeRelation(r);
	return 0;
}

// check whether a relation already exists

Bool existsRelation(char *name)
{
	char fname[MAXFILENAME];
	sprintf(fname,"%s.info",name);
	File f = open(fname,O_RDONLY);
	if (f < 0)
		return FALSE;
	else {
		close(f);
		return TRUE;
	}
}

// set up a relation descriptor from relation name
// open files, reads information from rel.info

Reln openRelation(char *name)
{
	Reln r = malloc(sizeof(RelnRep));
	assert(r != NULL);
	r->infof = openFile(name,"info");
	r->dataf = openFile(name,"data");
	r->tsigf = openFile(name,"tsig");
	r->psigf = openFile(name,"psig");
	r->bsigf = openFile(name,"bsig");
	read(r->infof, &(r->params), sizeof(RelnParams));
	return r;
}

// release files and descriptor for an open relation
// copy latest information to .info file
// note: we don't write ChoiceVector since it doesn't change

void closeRelation(Reln r)
{
	// make sure updated global data is put in info file
	lseek(r->infof, 0, SEEK_SET);
	int n = write(r->infof, &(r->params), sizeof(RelnParams));
	assert(n == sizeof(RelnParams));
	close(r->infof); close(r->dataf);
	close(r->tsigf); close(r->psigf); close(r->bsigf);
	free(r);
}

// insert a new tuple into a relation
// returns page where inserted
// returns NO_PAGE if insert fails completely

PageID addToRelation(Reln r, Tuple t)
{
	assert(r != NULL && t != NULL && strlen(t) == tupSize(r));
	Page p;  PageID pid;
	RelnParams *rp = &(r->params);


	// add tuple to last page
	pid = rp->npages-1;
	p = getPage(r->dataf, pid);
	// check if room on last page; if not add new page
	if (pageNitems(p) == rp->tupPP) {
		addPage(r->dataf);
		rp->npages++;
		pid++;
		free(p);
		p = newPage();
		if (p == NULL) return NO_PAGE;
	}
	addTupleToPage(r, p, t);
	rp->ntups++;  //written to disk in closeRelation()
	putPage(r->dataf, pid, p);

	// compute tuple signature and add to tsigf
	// 1.get t-sig
	// get last page (if page is full, create new page)
	// 2.add t-sig to page
	// 3.putPage
	//TODO
	Bits tuple_sig = makeTupleSig(r,t);
	PageID sig_pid = rp->tsigNpages-1;
	Page tsig_page = getPage(r->tsigf,sig_pid);
	// if page is full, add a new page
	if(pageNitems(tsig_page)==rp->tsigPP){
	    addPage(r->tsigf);
	    rp->tsigNpages++;
	    sig_pid++;
              tsig_page = newPage();
              if(tsig_page==NULL) return NO_PAGE;
	}
	putBits(tsig_page,pageNitems(tsig_page),tuple_sig);
	addOneItem(tsig_page);
	rp->ntsigs++;
	putPage(r->tsigf,sig_pid,tsig_page);
	freeBits(tuple_sig);


	// set a flag to detect if data f increased
	// if true, add a new page_sig based on makePageSig
	// else, fetch the old page_sig, orBits with new one, rewrite into psig_page
	//TODO
          Bool data_page_flag = FALSE;
          PageID check_data_id = rp->npages - 1;
          Page check_data_page = getPage(r->dataf,check_data_id);
          if(pageNitems(check_data_page)==1) data_page_flag=TRUE;
          free(check_data_page);
          Bits page_sig = makePageSig(r, t); //will be used in bit-sliced
	if(data_page_flag == TRUE) {
              //printf("have a new tuple page\n");

              PageID psig_id = rp->psigNpages - 1;
              //printf("page id is:%d\n", psig_id);
	    Page psig_page = getPage(r->psigf,psig_id);
	    if(pageNitems(psig_page)==rp->psigPP){
	        addPage(r->psigf);
	        rp->psigNpages++;
	        psig_id++;
                  psig_page = newPage();
	        if(page_sig==NULL) return NO_PAGE;
	    }
	    putBits(psig_page,pageNitems(psig_page),page_sig);
	    addOneItem(psig_page);
	    rp->npsigs++;
	    putPage(r->psigf,psig_id,psig_page);
	}else{
	    //printf("update an existing psig\n");
              PageID psig_id = rp->psigNpages-1;
              Page psig_page = getPage(r->psigf,psig_id);
              Bits cur_sig = newBits(rp->pm);
              getBits(psig_page,pageNitems(psig_page)-1,cur_sig);
              orBits(cur_sig,page_sig);
              putBits(psig_page,pageNitems(psig_page)-1,cur_sig);
              putPage(r->psigf,psig_id,psig_page);
              freeBits(cur_sig);
	}

	// use page signature to update bit-slices

	//TODO
	// 1.get page ID and tuple page sig
	// 2.for each bit in tuple page sig
	// if i_th bit is 1 update bs_id i with page ID bit


	PageID page_id = rp->npages-1;
	for(int i=0;i<rp->pm;i++){
	    // i is the number of bit slice
	    if(bitIsSet(page_sig,i)){
	        PageID bit_slice_id = i/rp->bsigPP;
	        Offset b_offset = i%rp->bsigPP;
	        Page bit_slice_page = getPage(r->bsigf,bit_slice_id);
	        Bits new_sig = newBits(rp->bm);
	        getBits(bit_slice_page,b_offset,new_sig);
	        setBit(new_sig,page_id);
	        putBits(bit_slice_page,b_offset,new_sig);
	        putPage(r->bsigf,bit_slice_id,bit_slice_page);
	        freeBits(new_sig);
	    }
	}

	freeBits(page_sig);

	return nPages(r)-1;
}

// displays info about open Reln (for debugging)

void relationStats(Reln r)
{
	RelnParams *p = &(r->params);
	printf("Global Info:\n");
	printf("Dynamic:\n");
    printf("  #items:  tuples: %d  tsigs: %d  psigs: %d  bsigs: %d\n",
			p->ntups, p->ntsigs, p->npsigs, p->nbsigs);
    printf("  #pages:  tuples: %d  tsigs: %d  psigs: %d  bsigs: %d\n",
			p->npages, p->tsigNpages, p->psigNpages, p->bsigNpages);
	printf("Static:\n");
    printf("  tups   #attrs: %d  size: %d bytes  max/page: %d\n",
			p->nattrs, p->tupsize, p->tupPP);
	printf("  sigs   %s",
            p->sigtype == 'c' ? "catc" : "simc");
    if (p->sigtype == 's')
	    printf("  bits/attr: %d", p->tk);
    printf("\n");
	printf("  tsigs  size: %d bits (%d bytes)  max/page: %d\n",
			p->tm, p->tsigSize, p->tsigPP);
	printf("  psigs  size: %d bits (%d bytes)  max/page: %d\n",
			p->pm, p->psigSize, p->psigPP);
	printf("  bsigs  size: %d bits (%d bytes)  max/page: %d\n",
			p->bm, p->bsigSize, p->bsigPP);
}
