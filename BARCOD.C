#include <stdio.h>
/*
131131311 2    0     *
111331311 2    1     0
111311313 2    2     7
311331111 2    3     5
311311113 2    4     1
111331311 2    5     0
111331113 2    6     4
113311311 2    7     9
113311113 2    8     2
111331311 2    9     0
113311311 2   10     9

113311311 2   11     9
313311111 2   12     3
111331311 2   13     0
311311311 2   14     8
131131311 2   15     *

*/
char ascii[]="1234567890ABCDEFGHIJKLMNOPQRSTUVWXYZ-. *$/+%";
char *codes[44]={
"100100001","001100001","101100000","000110001","100110000",
"001110000","000100101","100100100","001100100","000110100",
"100001001","001001001","101001000","000011001","100011000",
"001011000","000001101","100001100","001001100","000011100",
"100000011","001000011","101000010","000010011","100010010",
"001010010","000000111","100000110","001000110","000010110",
"110000001","011000001","111000000","010010001","110010000",
"011010000","010000101","110000100","011000100","010010100",
"010101000","010100010","010001010","000101010"};

int codetab[44];

char *tobin(int x)
{
	static char out[20];
	int i;
	for (i=0 ; i<16 ; i++, x>>=1)
		out[15-i]=(x&1)+'0';
	out[16]=0;
	return(out);
}

void tobinx(int x, int y)
{
	unsigned int mask=0x8000;
	int i;
	for (i=0 ; i<16 ; i++)
	{
		if (y&mask)
			if (x&mask) putchar('i');
			else putchar('o');
		else
			if (x&mask) putchar('I');
			else putchar('O');
		mask>>=1;
	}
	textattr(7);
	putchar('\n');
}

int xlat1(char *code)
{
  int x=0;
  int bit=1;
  while (*code)
  {
    x=(x<<1)|bit;
    if (*code=='1')
    {
      x=(x<<1)|bit;
      x=(x<<1)|bit;
    }
    bit=1-bit;
    code++;
  }
  return(x);
}

void xlat(void)
{
  int i;
  for(i=0 ; i<44 ; i++)
		codetab[i]=xlat1(codes[i]);
}

int weight(unsigned int x)
{
  int w=0;
  while (x)
  {
    if (x&1) w++;
    x>>=1;
	}
	return(w);
}

void xors(void)
{
  int i,j;
  int x;
  for (i=0 ; i<44 ; i++)
  {
		for (j=0 ; j<i ; j++)
    {
			x=codetab[i]^codetab[j];
			if (weight(x)<=2)
			{
				printf("%s(%c) - %s(%c) : ",
					 codes[i],ascii[i],codes[j],ascii[j],weight(x));
				tobinx(codetab[i],x);
			}


/*			printf("%x",weight(x)); */
    }
/*		putchar('\n'); */
  }
}

void dump(void)
{
	int i;
	for (i=0 ; i<44 ; i++)
		printf("%s(%c) : %04x : %s\n",
			codes[i],ascii[i],codetab[i],tobin(codetab[i]));
}


void main(void)
{
	xlat();
	dump();
  xors();
}

