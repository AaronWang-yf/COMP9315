// query.c ... query scan functions
// part of signature indexed files
// Manage creating and using Query objects
// Written by John Shepherd, March 2019

#include "defs.h"
#include "query.h"
#include "reln.h"
#include "tuple.h"
#include "bits.h"
#include "tsig.h"
#include "psig.h"
#include "bsig.h"

// check whether a query is valid for a relation
// e.g. same number of attributes

int checkQuery(Reln r, char *q)
{
	if (*q == '\0') return 0;
	char *c;
	int nattr = 1;
	for (c = q; *c != '\0'; c++)
		if (*c == ',') nattr++;
	return (nattr == nAttrs(r));
}

// take a query string (e.g. "1234,?,abc,?")
// set up a QueryRep object for the scan

Query startQuery(Reln r, char *q, char sigs)
{
	Query new = malloc(sizeof(QueryRep));
	assert(new != NULL);
	if (!checkQuery(r,q)) return NULL;
	new->rel = r;
	new->qstring = q;
	new->nsigs = new->nsigpages = 0;
	new->ntuples = new->ntuppages = new->nfalse = 0;
	new->pages = newBits(nPages(r));
	switch (sigs) {
	case 't': findPagesUsingTupSigs(new); break;
	case 'p': findPagesUsingPageSigs(new); break;
	case 'b': findPagesUsingBitSlices(new); break;
	default:  setAllBits(new->pages); break;
	}
	new->curpage = 0;
	return new;
}

// scan through selected pages (q->pages)
// search for matching tuples and show each
// accumulate query stats

void scanAndDisplayMatchingTuples(Query q)
{
	assert(q != NULL);
	Count ndpage;  //number of data pages
	unsigned int pid=0;
	//showBits(q->pages); printf("\n");
	File data;  //data file
	Page p;
	int check_match =0;
	data = dataFile(q->rel);
	ndpage=q->rel->params.npages;
	for(pid=0;pid<ndpage;pid++){
	    if(bitIsSet(q->pages,pid)){  //p->page is a bit string, pid as offset
	        //read page
	        q->curpage=pid;
	        q->ntuppages++;
	        p=getPage(data,pid);
	        check_match=0;
	        q->curtup=0;
	        for(int j=0;j<pageNitems(p);j++){
	            q->curtup++;
	            q->ntuples++;
	            Tuple cur_tuple = getTupleFromPage(q->rel,p,j); // tuple char*
	            if (tupleMatch(q->rel,q->qstring,cur_tuple)){
	                showTuple(q->rel,cur_tuple);
	                check_match = 1;
	            }
	        }
                  if(check_match==0) q->nfalse++;
                  free(p);
	    }
	}
          //p = getPage(data,1);
	//printf("nT:%d\n",pageNitems(p));
}

// print statistics on query

void queryStats(Query q)
{
	printf("# sig pages read:    %d\n", q->nsigpages);
	printf("# signatures read:   %d\n", q->nsigs);
	printf("# data pages read:   %d\n", q->ntuppages);
	printf("# tuples examined:   %d\n", q->ntuples);
	printf("# false match pages: %d\n", q->nfalse);
}

// clean up a QueryRep object and associated data

void closeQuery(Query q)
{
	free(q->pages);
	free(q);
}

