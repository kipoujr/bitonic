#include <stdio.h>
#include <string.h>
#include <vector>

#define MAXW 50
#define MAXT 10
#define MAXS 30
#define pb push_back

using namespace std;

void printIt ( vector<double> yo[MAXS] ) {
  int i, q;
  double sum;

  for ( q=16; q<=24; ++q ) {
    sum = 0;
    for ( i=0; i<yo[q].size(); ++i ) {
      sum = sum + yo[q][i];
    }
    printf ("%lf ", sum/yo[q].size());
  }
  printf ("\n");
}

pair<double,int> printTimes( vector<double> yo[MAXT][MAXS]) {
  int p, pBest;
  double minim = 15;

  printf ("moxos\n");
  for ( p=1; p<=8; ++p ) {
    printIt ( yo[p] );
    for (int i=0; i<yo[p][24].size(); ++i) {
      if ( yo[p][24][i] < minim ) {
	pBest = p;
	minim = yo[p][24][i];
      }
    }
  }
  
  return make_pair(minim,pBest);
}

vector<double> pthreads[MAXT][MAXS], openmp[MAXT][MAXS], cilk[MAXT][MAXS], qusort[MAXS];
int main () {
  char word[MAXW];
  int threads, size;
  double ttime;

  while (scanf("%s", word) != EOF ) {
    if ( strcmp(word,"PASSed") == 0 ) {
      return 0;
    }
    if ( strcmp(word,"Pthreads") == 0 ) {
      scanf ("%d %*s %d %*s %lf %*s\n ", &threads, &size, &ttime);
      pthreads[threads][size].pb(ttime);
    }
    else if ( strcmp(word,"OpenMP") == 0 ) {
      scanf ("%d %*s %d %*s %lf %*s\n ", &threads, &size, &ttime);
      openmp[threads][size].pb(ttime);
    }
    else if ( strcmp(word,"Cilk") == 0 ) {
      scanf ("%d %*s %d %*s %lf %*s\n ", &threads, &size, &ttime);    
      cilk[threads][size].pb(ttime);
    }
    else if ( strcmp(word,"Qsort") == 0 ) {
      scanf ("%d %*s %d %*s %lf %*s\n ", &threads, &size, &ttime);      
      qusort[size].pb(ttime);
    }
    else if ( strcmp(word,"TEST") == 0 ) {
      scanf ("%s\n", word);
      if ( strcmp(word,"PASSed") != 0 ) {
	printf ("Error : %s\n", word);
      }
    }
  }

  pair<double,int> x;
  printf ("Pthreads\n");
  x = printTimes( pthreads );
  printf ("Best time for %d threads, %lf\n", x.second, x.first);
  printf ("OpenMP\n");
  x=printTimes( openmp );
  printf ("Best time for %d threads, %lf\n", x.second, x.first);
  printf ("Cilk\n");
  x=printTimes( cilk );
  printf ("Best time for %d threads, %lf\n", x.second, x.first);
  printf ("Qusort\n");
  printIt( qusort );
}
