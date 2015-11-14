#include <stdio.h>
#include <stdlib.h>
#include <sys/time.h>

struct timeval startwtime, endwtime;
double seq_time;

int N;          // data array size
int *a;         // data array to be sorted

void test(void);
void init(void);
/** procedures asc-ending and desc-ending, used in qsort **/
int asc( const void *a, const void *b ){
  int* arg1 = (int *)a;
  int* arg2 = (int *)b;
  if( *arg1 < *arg2 ) return -1;
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

  init();
  gettimeofday (&startwtime, NULL);
  qsort( a, N, sizeof( int ), asc );                                                                    
  gettimeofday (&endwtime, NULL);
  seq_time = (double)((endwtime.tv_usec - startwtime.tv_usec)/1.0e6
		      + endwtime.tv_sec - startwtime.tv_sec);
  printf( "Qsort %s threads %s size %f time\n", argv[2], argv[1], seq_time );                                                    
  test();
}
void test() {
  int pass = 1, i;
  for ( i=1; i<N; ++i ) {
    pass&=(a[i-1] <= a[i]);
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
