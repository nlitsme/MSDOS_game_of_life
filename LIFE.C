/*   attributes
????1??? -> high
1??????? -> flash
?????001 -> underscore
?111?000 -> reverse video
?000?000 -> hidden
*/

#include <conio.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dos.h>
#include <stdarg.h>
#include <graphics.h>

int XMAX;
int YMAX;

int debug=0;
int graphics=0;

struct cell {
  char state;
  char nneigh;
  int x,y;
  struct cell *n;
};

struct cell *newlist,*oldlist, **field;

unsigned int seg[4]={0xb000, 0xb200, 0xb400, 0xb600};
unsigned int ofsy[346];

unsigned char mask[]={0x80,0x40,0x20,0x10,0x08,0x04,0x02,0x01};
unsigned char andmask[]={0x7f,0xbf,0xdf,0xef,0xf7,0xfb,0xfd,0xfe};

int grdrv, grmod;

void setpoint(int x,int y)
{
  char far *p;
  p=MK_FP(seg[y&3],ofsy[y>>2]+(x>>3));
  *p|=mask[x&7];
}

void hline(int y)
{
	int x;
	for (x=0 ; x<=XMAX+1 ; x++)
    setpoint(x,y);
}

void vline(int x)
{
  int y;
	for (y=0 ; y<=YMAX+1 ; y++)
    setpoint(x,y);
}

void putcharxy(char far *scr,char far *map)
{
  *scr=*map++;
  *(scr+0x2000)=*map++;
  *(scr+0x4000)=*map++;
  *(scr+0x6000)=*map++;
  scr+=90;
  *scr=*map++;
  *(scr+0x2000)=*map++;
  *(scr+0x4000)=*map++;
  *(scr+0x6000)=*map++;
}

void graftextxy(int x, int y, char *s)
{
  /* ascii codes 0 .. ??   at f000:fa6e */
  char far *p;
  char far *chrs;
  p=MK_FP(0xb000,ofsy[y]+x);
  while (*s)
  {
    chrs=MK_FP(0xf000,0xfa6e+(*s<<3));
    putcharxy(p++,chrs);
    s++;
  }
}

void gprintf(int x,int y, char *s, ...)
{
  char line[80];
  va_list ap;
  va_start(ap,s);
  vsprintf(line,s,ap);
  if (graphics)
    graftextxy(x,y/4+1,line);
  else
  {
    gotoxy(x,y/2);
    cputs(line);
  }
  va_end(ap);
}

void graphinit(void)
{
	int i;

	grdrv=HERCMONO; grmod=HERCMONOHI;

  for (i=0 ; i<346 ; i++)  ofsy[i]=i*90;
	if (getenv("BGI"))
		initgraph(&grdrv,&grmod,getenv("BGI"));
	else
		initgraph(&grdrv,&grmod,"");
  cleardevice();
	hline(0); vline(0); hline(YMAX+1); vline(XMAX+1);
}

  /* dc df db ff    =  l u l+u - */

void setxy(int x, int y)
{
  int far *p;
  char far *q;
  if (graphics)
  {
    q=MK_FP(seg[y&3],ofsy[y>>2]+(x>>3));
    *q|=mask[x&7];
  }
  else
  {
    x--; y--;
    if (x>80 || y>50) return;
    p=MK_FP(0xb000,(y/2)*160+x*2);
    if (y&1)
    if (*p==0x0720) *p=0x07dc;
      else *p=0x07db;
    else
      if (*p==0x0720) *p=0x07df;
      else *p=0x07db;
  }
}

void clearxy(int x, int y)
{
  int far *p;
  char far *q;
  if (graphics)
  {
    q=MK_FP(seg[y&3],ofsy[y>>2]+(x>>3));
    *q&=andmask[x&7];
  }
  else
  {
    x--; y--;
    if (x>80 || y>50) return;
    p=MK_FP(0xb000,(y/2)*160+x*2);
    if (y&1)
      if (*p==0x07dc) *p=0x0720;
      else *p=0x07df;
    else
      if (*p==0x07df) *p=0x0720;
      else *p=0x07dc;
  }
}

void xputc(int x, int y, int c)
{
  int far *p;
  p=MK_FP(0xb000,y*160+x*2);
  *p=c;
}

int pass1(void)
{
  struct cell *p,*f;
  int dx,dy;
  int ncells=0;
  p=oldlist;
  newlist=oldlist;
  while (p!=NULL)
  {
    for (dx=-1 ; dx<=1 ; dx++)
      for (dy=-1 ; dy<=1 ; dy++)
      {
        f=&field[p->x+dx][p->y+dy];
        f->nneigh++;
        if (f->state==0)
        {
          f->n=newlist;
          newlist=f;
          f->state=2;
        }
      }
    p=p->n;   ncells++;
  }
  return(ncells);
}

int isborder(struct cell *p)
{
  return(p->x==0 || p->y==0 || p->x==XMAX || p->y==YMAX);
}

int pass2(void)
{
  struct cell *p,*q;
  int changes=0;
  q=NULL;
  p=newlist;
  while (p!=NULL)
  {
    if (((p->state!=1 || p->nneigh!=4) && p->nneigh!=3) || isborder(p))
    {
      p->nneigh=0;
      if (p->state==1 && !isborder(p))
      {
        clearxy(p->x+1,p->y+1);
        changes++;
      }
      p->state=0;
      if (q==NULL)
      {
        newlist=p->n;
        p->n=NULL;
        p=newlist;
      }
      else
      {
        q->n=p->n;
        p->n=NULL;
        p=q->n;
      }
    }
    else
    {
      p->nneigh=0;
      if (p->state==2)
      {
        p->state=1;
        setxy(p->x+1,p->y+1);
        changes++;
      }
      q=p;
      p=p->n;
    }
  }
  return(changes);
}

int initfield(void)
{
  int x,y;
  struct cell *f;
  field=(struct cell **)malloc((XMAX+2)*sizeof(struct cell *));
  if (field==NULL)
  {
    printf("Out of memory!\n");
    return(-1);
  }
  for (x=0 ; x<=XMAX+1 ; x++)
  {
    field[x]=(struct cell *)malloc((YMAX+2)*sizeof(struct cell));
    if (field[x]==NULL)
    {
      printf("Out of memory!\n");
      return(-1);
    }
    for (y=0 ; y<=YMAX+1 ; y++)
    {
      f=&field[x][y];
      f->x=x;
      f->y=y;
      f->n=NULL;
      f->state=0;
      f->nneigh=0;
    }
  }
  return(0);
}

int readstart(char *inputfile)
{
  FILE *f;
  int x=0,y=0;
  int error=0;
  int record=0;
  int len;
  char *line, *p;
  line=(char *)malloc(XMAX);
  if (line==NULL)
  {
    if (graphics) closegraph();
    printf("Out of memory!\n");
    error++;
  }
  f=fopen(inputfile,"r");
  if (f==NULL)
  {
    if (graphics) closegraph();
    perror(inputfile);
    error++;
  }
  while (!error && fgets(line,XMAX,f)!=NULL)
  {
    if (line[0]=='#') continue;  /* remark line */
    len=strlen(line);  line[--len]=0;
    while (len && line[len-1]==' ')
      line[--len]=0;
    if ((x||y) && !(len>0 && strchr(line,'*')==NULL) )
    {
      if (len>0)
      {
        p=strchr(line,'*');
        while (p!=NULL)
        {
          setxy(x+p-line+1,y+1);
          field[x+p-line][y].state=1;
          field[x+p-line][y].n=oldlist;
          oldlist=&field[x+p-line][y];
          p=strchr(p+1,'*');
        }
      }
      y++;
    }
    else
    {
      record++;
      if (sscanf(line,"%d %d",&x,&y)!=2)
      {
        if (graphics) closegraph();
        printf("Error in coordinate line for record %d\n",record);
        error++;
        break;
      }
    }
  }
  fclose(f);
  free(line);
  if (error)
    return(-1);
  else
    return(0);
}

int countlist(struct cell *p)
{
  int count=0;
  while (p!=NULL)
  {
    count++;
    p=p->n;
  }
  return(count);
}

void incrextension(char *name)
{
  int l;
  l=strlen(name)-1;
  while (name[l]=='f') name[l--]='0';
  name[l]++; if (name[l]=='9'+1) name[l]='a';
}

char *addsavextension(char *name)
{
  char *p;
  char *newname;
  p=strchr(name,'.');
  if (p==NULL)
    newname=(char *)malloc(strlen(name)+5);
  else
    newname=(char *)malloc(strlen(name)+4-strlen(p+1));
  if (newname==NULL)
  {
    printf("Out of memory allocating savename!\n");
    return(NULL);
  }
  strcpy(newname,name);
  p=strchr(newname,'.');
  if (p!=NULL) *p=0;
  strcat(newname,".000");
  return(newname);
}

void save(char *outputfile, int ngen, int ncells, int state)
{
  int x,y;
  FILE *f;
  int xmin=XMAX,xmax=0,ymin=YMAX,ymax=0;
  for (x=1 ; x<=XMAX ; x++)
    for (y=1 ; y<=YMAX ; y++)
      if (field[x][y].state==1)
      {
        if (x<xmin) xmin=x;
        if (y<ymin) ymin=y;
        if (x>xmax) xmax=x;
        if (y>ymax) ymax=y;
      }
  f=fopen(outputfile,"w");
  fprintf(f,"# after %d generation, %d cells",ngen,ncells);
  switch(state)
  {
    case 0: fprintf(f,"  stable configuration\n"); break;
    case 1: fprintf(f,"  oscilating configuration\n"); break;
    case 2: fprintf(f,"  still changing\n"); break;
  }
  fprintf(f,"%d %d\n",xmin,ymin);
  for (y=ymin ; y<=ymax ; y++)
  {
    for (x=xmin ; x<=xmax ; x++)
      if (field[x][y].state==1)
        putc('*',f);
      else
        putc(' ',f);
    putc('\n',f);
  }
  fclose(f);
  incrextension(outputfile);
}

void display(int x0)
{
  int x,y;
  int count=0;
  int attr;
  for (y=0 ; y<=YMAX+1 ; y++)
  {
    for (x=0 ; x<=XMAX+1 ; x++)
    {
      switch(field[x][y].state)
      {
        case 0: attr=0x0700+'0'; break;
        case 1: attr=0x7000+'0'; break;
        case 2: attr=0x0100+'0'; break;
      }
      if (field[x][y].n)
        count++;
      else
        attr|=0x800;
      xputc(x0+x,y,attr|field[x][y].nneigh);
    }
  }
  if (debug)
  {
    gotoxy(x0,YMAX+3); cprintf("<pass1");
    gotoxy(x0,YMAX+4); cprintf("old = %3d",countlist(oldlist));
    gotoxy(x0,YMAX+5); cprintf("new = %3d",countlist(newlist));
    gotoxy(x0,YMAX+6); cprintf("%3d cells",count);
  }
}

void usage(void)
{
  printf("Usage: life [options]\n");
  printf("  -f<filename>  : input file, default life.dat\n");
  printf("  -x<xsize>     : default 80\n");
  printf("  -y<ysize>     : default 48\n");
  printf("  -a[a]         : autostop & save, [detect loops]\n");
  printf("  -d            : debug mode\n");
  printf("  -g            : force graphics mode\n");
  printf("  -t            : force textmode\n");
}

int main(int argc, char **argv)
{
  int stop=0;
  int gen=0;
  int autostop=0;
  int onestep=0;
  char *inputfile="life.dat";
  char *outputfile;
  int changes=0,ncells;
  int configstate=2;
  int chcnt=0,lastchange=-1;

  XMAX=80;
  YMAX=48;
  graphics=0;
  while (argc>1)
  {
    if (argv[1][0]=='-')
      switch(argv[1][1])
      {
        case 'x': XMAX=atoi(argv[1]+2);
          if (XMAX>80) graphics=1;
          break;
        case 'y': YMAX=atoi(argv[1]+2);
          if (YMAX>48) graphics=1;
          break;
        case 'f': inputfile=argv[1]+2; break;
        case 'd': debug++; onestep++; XMAX=10; YMAX=10; graphics=0; break;
        case 'a': autostop++;
          if (argv[1][2]) autostop++;
          break;
        case 'g': graphics=1; break;
        case 't': graphics=0; break;
        case 's': onestep++; break;
        default: usage(); return(0);
      }
    argc--;
    argv++;
  }
  if (initfield())
    return(1);
  oldlist=NULL; newlist=NULL;
  outputfile=addsavextension(inputfile);
  if (graphics)
    graphinit();
  else
    clrscr();
  if (readstart(inputfile))
    return(1);

  while (!stop)
  {
    if (debug) display(30);
    ncells=pass1();
    if (debug) display(45);
    changes=pass2();
    if (debug) display(60);
    gprintf(1,YMAX+2,"Generation %d, popultion %d, changes %d         ",
        gen,changes,ncells);
    oldlist=newlist;
    if (onestep) while (!kbhit()) ;
    if (kbhit())
      switch(getch())
      {
        case 0x1b: stop++; break;
        case 's':  save(outputfile,gen,ncells,configstate); break;
        case 'e':  save(outputfile,gen,ncells,configstate); stop++; break;
      }
    while (kbhit()) getch();

    if (changes==lastchange)
      chcnt++;
    else
      chcnt=0;
    lastchange=changes;
    if (changes==0) configstate=0;
    else if (chcnt>10) configstate=1;
    else configstate=2;
    if (configstate<autostop)
    {
      save(outputfile,gen,ncells,configstate);
      stop++;
    }
    gen++;
  }
  if (graphics)
    closegraph();
  else
    gotoxy(1,22);
  return(0);
}
