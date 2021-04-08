// Test Bits ADT

#include <stdio.h>
#include "defs.h"
#include "reln.h"
#include "tuple.h"
#include "bits.h"

int main(int argc, char **argv)
{
    Bits b = newBits(60);
    printf("t=0: "); showBits(b); printf("\n");
    setBit(b, 5);
    printf("t=1: "); showBits(b); printf("\n");
    setBit(b, 0);
    setBit(b, 1);
    setBit(b, 2);
    setBit(b, 7);
    setBit(b, 8);
    setBit(b,59);
    setBit(b,56);
    setBit(b,49);
    printf("t=2: "); showBits(b); printf("\n");
    if (bitIsSet(b,7)) printf("Bit 7 is set\n");
    if (bitIsSet(b,8)) printf("Bit 8 is set\n");

    printf("t=3: "); showBits(b); printf("\n");

    shiftBits(b,9);
    shiftBits(b,-10);

    printf("t=4: "); showBits(b); printf("\n");
    shiftBits(b,8);
    //shiftBits(b,-8);

    printf("t=5: "); showBits(b); printf("\n");
    shiftBits(b,10);
    //shiftBits(b,-8);

    printf("t=6: "); showBits(b); printf("\n");

/*
	Bits b1 = newBits(10);
	Bits b2 = newBits(10);
	setBit(b1,1);setBit(b1,2);
	setBit(b1,3);setBit(b1,5);
	setBit(b2,3);setBit(b2,4);
	orBits(b1,b2);
          printf("t=6: "); showBits(b1); printf("\n");
          shiftBits(b1,-2);
          printf("t=7: "); showBits(b1); printf("\n");
*/
	return 0;
}
