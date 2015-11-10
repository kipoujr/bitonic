#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <cilk/cilk.h>
#include <cilk/cilk_api.h>

#define MINPOW 17
#define MINK (1<<MINPOW)
#define BASEFORPOW 16
#define BASEFOR (1<<BASEFORPOW)

struct timeval startwtime, endwtime;
double seq_time;

int N;          // data array size
int *a;         // data array to be sorted

const int ASCENDING  = 1;
const int DESCENDING = 0;


void init(void);
void print(void);
void sort(void);
void test(void);
inline void exchange(int i, int j);
void compare(int i, int j, int dir);
void bitonicMerge(int lo, int cnt, int dir);
void recBitonicSort(int lo, int cnt, int dir);

/** procedures asc-ending and desc-ending, used in qsort **/
int asc( const void *a, const void *b ){
  int* arg1 = (int *)a;
  int* arg2 = (int *)b;
  if( *arg1 < *arg2 ) return -1;
  return ( *arg1 != *arg2 );
}

int desc( const void *a, const void *b ){
  int* arg1 = (int *)a;
  int* arg2 = (int *)b;
  if( *arg1 > *arg2 ) return -1;
  return ( *arg1 != *arg2 );
} 

/** the main program **/
int main(int argc, char **argv) {

  if (argc != 3) {
    printf("Usage: %s q t\n  where n=2^q is problem size (power of two) and t is the number of threads\n",
	   argv[0]);
    exit(1);
  }

  N = 1<<atoi(argv[1]);
  a = (int *) malloc(N * sizeof(int));
  printf ("Number of elements %d, number of threads %s\n", N, argv[2]);
  __cilkrts_set_param("nworkers", argv[2]);

  init();
  gettimeofday (&startwtime, NULL);
  qsort( a, N, sizeof( int ), asc );                                                                    
  gettimeofday (&endwtime, NULL);
  seq_time = (double)((endwtime.tv_usec - startwtime.tv_usec)/1.0e6
		      + endwtime.tv_sec - startwtime.tv_sec);
  printf( "Qsort wall clock time = %f\n", seq_time );                                                    
  test();

  init();
  gettimeofday (&startwtime, NULL);
  sort();
  gettimeofday (&endwtime, NULL);
  seq_time = (double)((endwtime.tv_usec - startwtime.tv_usec)/1.0e6
		      + endwtime.tv_sec - startwtime.tv_sec);
  printf("Recursive wall clock time = %f\n", seq_time);
  test();
}

/** -------------- SUB-PROCEDURES  ----------------- **/

/** procedure test() : verify sort results **/
void test() {
  int pass = 1;
  int i;
  for (i = 1; i < N; i++) {
    pass &= (a[i-1] <= a[i]);
  }

  printf(" TEST %s\n",(pass) ? "PASSed" : "FAILed");
}


/** procedure init() : initialize array "a" with data **/
void init() {
  int i;
  for (i = 0; i < N; i++) {
    a[i] = rand() % N; // (N - i);
  }
}

/** procedure  print() : print array elements **/
void print() {
  int i;
  for (i = 0; i < N; i++) {
    printf("%d\n", a[i]);
  }
  printf("\n");
}


/** INLINE procedure exchange() : pair swap **/
inline void exchange(int i, int j) {
  int t;
  t = a[i];
  a[i] = a[j];
  a[j] = t;
}



/** procedure compare()
    The parameter dir indicates the sorting direction, ASCENDING
    or DESCENDING; if (a[i] > a[j]) agrees with the direction,
    then a[i] and a[j] are interchanged.
**/
inline void compare(int i, int j, int dir) {
  if (dir==(a[i]>a[j]))
    exchange(i,j);
}

void Split(int x, int cnt, int k,int dir) {
  if ( cnt > BASEFOR ) {
    cilk_spawn Split(x,cnt/2,k,dir);
    cilk_spawn Split(x+cnt/2,cnt/2,k,dir);
    cilk_sync;
    return;
  }
  for (int i=x; i<x+cnt; ++i) {
    compare(i,i+k,dir);
  }
}


/** Procedure bitonicMerge()
    It recursively sorts a bitonic sequence in ascending order,
    if dir = ASCENDING, and in descending order otherwise.
    The sequence to be sorted starts at index position lo,
    the parameter cbt is the number of elements to be sorted.
**/
void bitonicMerge(int lo, int cnt, int dir) {
  if (cnt>1) {
    int k=cnt/2;

    //cilk_for slows things down. very very much. very nice extra large.
    Split(lo,k,k,dir);
    /*for (int i=lo; i<lo+k; ++i) {
      compare(i, i+k,dir);
      }*/

    cilk_spawn bitonicMerge(lo, k, dir);
    cilk_spawn bitonicMerge(lo+k, k, dir);
    cilk_sync;
  }
}



/** function recBitonicSort()
    first produces a bitonic sequence by recursively sorting
    its two halves in opposite sorting orders, and then
    calls bitonicMerge to make them in the same order
**/
void recBitonicSort(int lo, int cnt, int dir) {
  if (cnt>=MINK) {
    int k=cnt/2;
    cilk_spawn recBitonicSort(lo, k, ASCENDING);
    cilk_spawn recBitonicSort(lo+k, k, DESCENDING);
    cilk_sync;
    bitonicMerge(lo, cnt, dir);
  }
  else {
    qsort(a+lo,cnt,sizeof(int),dir==ASCENDING?asc:desc);
  }
}


/** function sort()
    Caller of recBitonicSort for sorting the entire array of length N
    in ASCENDING order
**/
void sort() {
  recBitonicSort(0, N, ASCENDING);
}

/*
  imperative version of bitonic sort
*/
void impBitonicSort() {

  int i,j,k,superset;
  for (k=2; k<=N; k=2*k) {
    for (j=k>>1; j>0; j=j>>1) {
      superset = (N-1) ^ j;
      for (i=superset; i>0; i=(i-1)&superset) {
	int ij=i^j;
	if ((i&k)==0 && a[i] > a[ij])
	  exchange(i,ij);
	if ((i&k)!=0 && a[i] < a[ij])
	  exchange(i,ij);
      }
      if ( a[0] > a[j] ) {
	exchange(0,j);
      }
    }
  }
}
