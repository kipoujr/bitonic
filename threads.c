#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <sys/time.h>

#define BASEPOW 16
#define BASE (1<<BASEPOW)

struct timeval startwtime, endwtime;
double seq_time;

int N, n;          // data array size, threads
int *a, *b;         // data array to be sorted
pthread_mutex_t mutex1 = PTHREAD_MUTEX_INITIALIZER;
int activeThreads;

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
void *SortThreadFunction (void* arg);
void *MergeThreadFunction (void* arg);

typedef struct {
  int lo, cnt, dir;
} sortData;

int asc(const void* a,const void *b) {
  int* a1 = (int *)a;
  int* a2 = (int *)b;
  if ( *a1 < *a2 ) return -1;
  return *a1 != *a2;
}
int desc(const void *a, const void *b) {
  int* a1 = (int *)a;
  int* a2 = (int *)b;
  if ( *a1 > *a2 ) return -1;
  return *a1 != *a2;
}

/** the main program **/
int main(int argc, char **argv) {

  if (argc != 3) {
    printf("Usage: %s q t\n  where N=2^q is problem size(power of two) and n=2^t is number of threads (power of two)\n",
	   argv[0]);
    exit(1);
  }

  N = 1<<atoi(argv[1]);
  n = 1<<atoi(argv[2]);
  a = (int *) malloc(N * sizeof(int));
  b = (int *) malloc(N * sizeof(int));

  init();
  gettimeofday (&startwtime, NULL);
  sort();
  gettimeofday (&endwtime, NULL);
  seq_time = (double)((endwtime.tv_usec - startwtime.tv_usec)/1.0e6
		      + endwtime.tv_sec - startwtime.tv_sec);
  printf("Pthreads %s threads %s size %f time\n", argv[2], argv[1], seq_time);
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

/** Procedure bitonicMerge()
       It recursively sorts a bitonic sequence in ascending order,
          if dir = ASCENDING, and in descending order otherwise.
	     The sequence to be sorted starts at index position lo,
	        the parameter cbt is the number of elements to be sorted.
**/

void Merge(int lo, int cnt, int dir) {
  int i, maxim = lo, left, right, half=lo+cnt/2,lo2_plus_count_minus_1 = (lo<<1)+cnt-1;

  //Copy to b and find maximum
  for (i=0; i<cnt; ++i) {
    b[lo+i] = a[lo+i];
    if ( a[lo+i] > a[maxim] ) {
      maxim = lo+i;
    }
  }

  //init pointers
  left = maxim;
  right = maxim+1;
  if ( right == lo+cnt ) {
    right = lo;
  }

  //merge step, descending
  for (i=0; i<cnt; ++i) {
    if ( b[left] > b[right] ) {
      a[lo+i] = b[left--];
      if ( left < lo ) {
	left = lo+cnt-1;
      }
    }
    else {
      a[lo+i] = b[right++];
      if ( right == lo+cnt ) {
	right = lo;
      }
    }
  }

  //if you want ascending, reverse it
  if ( dir ) {
    for ( i=lo; i<half; ++i ) {
      exchange(i,lo2_plus_count_minus_1 - i);
    }
  }
}



void* CompareThreadFunction ( void* arg ) {
  int lo = ((sortData *)arg)->lo;
  int cnt = ((sortData *)arg)->cnt;
  int dir = ((sortData *)arg)->dir;
  int i;
  for ( i=lo; i<lo+cnt; i++ ) {
    compare(i,i+2*cnt,dir);
  }
}

void bitonicMergeSerial(int lo, int cnt, int dir) {
  if ( cnt > 1 ) {
    int i, k=cnt/2;
    for ( i=lo; i<lo+k; ++i ) {
      compare(i,i+k,dir);
    }
    bitonicMergeSerial(lo,k,dir);
    bitonicMergeSerial(lo+k,k,dir);
  }
}

void bitonicMerge(int lo, int cnt, int dir) {
  int i, k=(cnt>>1), kt = (cnt>>2);
  
  pthread_mutex_lock (&mutex1);
  if ( cnt > BASE && activeThreads < n ) {
    ++activeThreads;
    pthread_mutex_unlock(&mutex1);
      
    sortData comp;
    pthread_t threadComp;
    comp.lo = lo+k/2;
    comp.cnt = k/2;
    comp.dir = dir;
    
    pthread_create( &threadComp, NULL, CompareThreadFunction, &comp);
    for (i=lo; i<lo+kt; i++)
      compare(i, i+k, dir);
    pthread_join ( threadComp, NULL );
    
    sortData arg2;
    pthread_t thread2;
    arg2.lo = lo+k;
    arg2.cnt = k;
    arg2.dir = dir;
    
    pthread_create ( &thread2, NULL, MergeThreadFunction, &arg2 );
    bitonicMerge(lo, k, dir);
    pthread_join ( thread2, NULL );
    pthread_mutex_lock(&mutex1);
    --activeThreads;
    pthread_mutex_unlock(&mutex1);
  }
  else {
    pthread_mutex_unlock(&mutex1);
    Merge(lo,cnt,dir);
    //qsort (a+lo,cnt,sizeof(int),dir==ASCENDING?asc:desc);
    //bitonicMergeSerial(lo,cnt,dir);
  }
}

void* MergeThreadFunction ( void* arg ) {
  if ( ((sortData *)arg)-> cnt > 1 ) {
    bitonicMerge( ((sortData *)arg)-> lo, ((sortData *)arg)-> cnt, ((sortData *)arg)-> dir );
  }
}

/** function recBitonicSort()
        first produces a bitonic sequence by recursively sorting
	    its two halves in opposite sorting orders, and then
	        calls bitonicMerge to make them in the same order
**/

void recBitonicSort(int lo, int cnt, int dir) {
  if (cnt>1) {
    int k=cnt/2;
    pthread_mutex_lock (&mutex1);
    if ( activeThreads < n ) {
      ++activeThreads;
      pthread_mutex_unlock(&mutex1);

      sortData arg2;
      pthread_t thread2;

      arg2.lo = lo+k;
      arg2.cnt = k;
      arg2.dir = DESCENDING;

      pthread_create ( &thread2, NULL, SortThreadFunction, &arg2 );
      recBitonicSort(lo, k, ASCENDING);
      pthread_join ( thread2, NULL );

      pthread_mutex_lock(&mutex1);
      --activeThreads;
      pthread_mutex_unlock(&mutex1);
    }
    else {
      pthread_mutex_unlock(&mutex1);
      qsort (a+lo,cnt,sizeof(int),dir==ASCENDING?asc:desc);
      return;
    }
    bitonicMerge(lo, cnt, dir);
  }
}

void* SortThreadFunction (void *arg) {
  if ( ((sortData *)arg)-> cnt > 1 ) {
    recBitonicSort( ((sortData *)arg)-> lo, ((sortData *)arg)-> cnt, ((sortData *)arg)-> dir );
  }
}


/** function sort()
       Caller of recBitonicSort for sorting the entire array of length N
          in ASCENDING order
**/
void sort() {
  activeThreads = 1;
  recBitonicSort(0, N, ASCENDING);
}
