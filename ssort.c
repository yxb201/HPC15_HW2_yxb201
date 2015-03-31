/* Parallel sample sort
 */
#include <stdio.h>
#include <unistd.h>
#include <mpi.h>
#include <stdlib.h>


static int compare(const void *a, const void *b)
{
  int *da = (int *)a;
  int *db = (int *)b;

  if (*da > *db)
    return 1;
  else if (*da < *db)
    return -1;
  else
    return 0;
}

int main( int argc, char *argv[])
{
  int rank, numProc;
  int i, N;
  int *vec;
  int *Splitter, *AllSplitter;
  int Root = 0;

  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numProc);
  
  /* Number of random numbers per processor (this should be increased
   * for actual tests or could be passed in through the command line */
  N = 100;

  vec = calloc(N, sizeof(int));
  /* seed random number generator differently on every core */
  srand((unsigned int) (rank + 393919));

  /* fill vector with random integers */
  for (i = 0; i < N; ++i) {
    vec[i] = rand();
  }
  printf("rank: %d, first entry: %d\n", rank, vec[0]);


  /* STEP1: sort locally */
  qsort(vec, N, sizeof(int), compare);
  
  /* 
  if (rank == 0){
    for (i = 0; i < N; i++){
        printf("v[%d] = %d \n", i,  vec[i]);	
    }
  }
  */

  /* STEP2: draw S=P-1 local splitters  */	
  Splitter = (int *) malloc( sizeof(int) * (numProc-1) );
  for (i = 0; i < numProc-1 ; i ++){
      Splitter[i] = vec[N/numProc * (i+1)];
  } 
  
  /*
  if(rank == 0){
      for(i=0; i<numProc-1 ; i++){
          printf("Splitter[%d] = %d \n", i, Splitter[i] );
      }
  }
  */

  /* STEP3: every processor send local splitters to root */
  AllSplitter = (int *) malloc( sizeof(int) * (numProc-1) * numProc );
  MPI_Gather( Splitter, numProc-1, MPI_INT, AllSplitter, numProc-1,
                                   MPI_INT, Root, MPI_COMM_WORLD  );

  /* STEP4: Root sort all splitters and pick global splitters */
  if (rank == Root){
     qsort(AllSplitter, numProc*(numProc-1), sizeof(int), compare);
     
     /*
     for (i = 0; i < numProc*(numProc-1) ; i++  ){
         printf("AllSplitter[%d] = %d\n", i, AllSplitter[i]); 
     }
     */
     
     
     for(i = 0; i < numProc-1; i++){
         Splitter[i] = AllSplitter[ numProc/2 + numProc * i  ];
     }

     /*
     if(rank == 0){
         for(i=0; i<numProc-1 ; i++){
              printf("Global Splitter[%d] = %d \n", i, Splitter[i] );
         }
     }    
     */

  }

  /* STEP 5: Broadcast Global Splitter to every processor */
  MPI_Bcast(Splitter, numProc-1, MPI_INT, Root, MPI_COMM_WORLD);

  /* randomly sample s entries from vector or select local splitters,
   * i.e., every N/P-th entry of the sorted vector */

  /* every processor communicates the selected entries
   * to the root processor; use for instance an MPI_Gather */

  /* root processor does a sort, determinates splitters that
   * split the data into P buckets of approximately the same size */

  /* root process broadcasts splitters */

  /* every processor uses the obtained splitters to decide
   * which integers need to be sent to which other processor (local bins) */

  /* send and receive: either you use MPI_AlltoallV, or
   * (and that might be easier), use an MPI_Alltoall to share
   * with every processor how many integers it should expect,
   * and then use MPI_Send and MPI_Recv to exchange the data */

  /* do a local sort */

  /* every processor writes its result to a file */

  free(vec);
  MPI_Finalize();
  return 0;
}
