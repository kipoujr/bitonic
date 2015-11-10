#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>
#include <omp.h>

#define MINPOW 22
#define MINK (1<<MINPOW)
#define SIZE (1<<(MINPOW-1))

struct timeval startwtime, endwtime;
double seq_time;

int N,n;          // data array size
int *a;         // data array to be sorted

const int ASCENDING  = 1;
const int DESCENDING = 0;


void init(void);
void print(void);
void test(void);
inline void exchange(int i, int j);
void compare(int i, int j, int dir);

void PimpBitonicSort( void );

/** compare for qsort **/
int desc( const void *a, const void *b ){
  int* arg1 = (int *)a;
  int* arg2 = (int *)b;
  if( *arg1 > *arg2 ) return -1;
  else if( *arg1 == *arg2 ) return 0;
  return 1;
}
int asc( const void *a, const void *b ){
  int* arg1 = (int *)a;
  int* arg2 = (int *)b;
  if( *arg1 < *arg2 ) return -1;
  else if( *arg1 == *arg2 ) return 0;
  return 1;
}

/** the main program **/
int main( int argc, char **argv ) {

  if (argc != 3 || atoi( argv[ 2 ] ) > 256 ) {
    printf( "Usage: %s q t\n  where N=2^q is problem size (power of two), and n=2^t is the number of threads, <=256, to use.\n", argv[ 0 ] );
    exit( 1 );
  }

  N = 1 << atoi( argv[ 1 ] );
  n = 1 << atoi(argv[2]);

  a = (int *) malloc( N * sizeof( int ) );
  init();
  gettimeofday( &startwtime, NULL );
  qsort( a, N, sizeof( int ), asc );
  gettimeofday( &endwtime, NULL );
  seq_time = (double)( ( endwtime.tv_usec - startwtime.tv_usec ) / 1.0e6 + endwtime.tv_sec - startwtime.tv_sec );
  printf( "Qsort wall clock time = %f\n", seq_time );

  init();
  gettimeofday( &startwtime, NULL );
  PimpBitonicSort();
  gettimeofday( &endwtime, NULL );
  seq_time = (double)( ( endwtime.tv_usec - startwtime.tv_usec ) / 1.0e6 + endwtime.tv_sec - startwtime.tv_sec );
  printf( "Bitonic parallel imperative with %i threads wall clock time = %f\n", n,  seq_time );
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

void PimpBitonicSort() {
  int itonos, i1, i, ij, j,k, maskj, maskk, fourthN, halfN, chunks;
  fourthN = N>>2, halfN = N>>1;

  if ( N < MINK ) {
    qsort( a, N, sizeof( int ), asc );
    return;
  }

  omp_set_num_threads( n );
  chunks = N / SIZE;
#pragma omp parallel for
  for (i=0; i<chunks; ++i) {
    qsort( a + i*SIZE, SIZE, sizeof( int ), i%2?desc:asc );
  }
  
  for (k = MINK; k < N; k *= 2 ) {
    maskk = k-1;
    for (j=k>>1; j>0; j=j>>1) {
      maskj = j-1;
#pragma omp parallel for private(i,ij,i1)
      for (itonos=0; itonos<fourthN; itonos++) {
	i1 = (itonos<<1) - (itonos&maskj);
	//Number with 0 at j-lsb

	//Number with 0 at k
	i = (i1<<1) - (i1&maskk);
	ij=i^j;
	if (a[i] > a[ij]) {
	  exchange(i,ij);
	}

	//Number with 1 at k
	i |= k;
	ij = i^j;
	if (a[i] < a[ij]) {
	  exchange(i,ij);
	}
      }
    }
  }

  //The last execution of the for(k...) deserves its own space :D
  //That's because the on-bit of N is out of scope, so the code is simpler
  for (j=N>>1; j>0; j >>= 1) {
    maskj = j-1;
#pragma omp parallel for private(i,ij)
    for (itonos=0; itonos<halfN; ++itonos) {
      i = (itonos<<1) - (itonos&maskj);
      ij = i^j;
      if (a[i] > a[ij]) {
	exchange(i,ij);
      }
    }
  }
} 
