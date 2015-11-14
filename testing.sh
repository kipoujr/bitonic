gcc -O3 qsort.c -o qsort.out
gcc -O3 -pthread threads.c -o threads.out
g++ -O3 -fopenmp openmp.cpp -o openmp.out
g++ -O3 -fcilkplus -lcilkrts cilk.cpp -o cilk.out
touch times
for q in {16..24}
do
  for i in {1..10}
  do
    for p in {1..8}
    do
      echo "Size $q, threads $p, loop $i"
      ./threads.out $q $p >> times
      ./openmp.out $q $p >> times
      ./cilk.out $q $p >> times
    done
    ./qsort.out $q 0 >> times
  done
done