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
  int rank, numProc, newN;
  int i, j, N;
  int *vec, *newvec;
  int *Splitter, *CandidateSplitter;
  int *sBucket, *rBucket; 
  int *sdispls, *rdispls;
  int Root = 0;
  double T1, T2;  


  MPI_Init(&argc, &argv);
  MPI_Comm_rank(MPI_COMM_WORLD, &rank);
  MPI_Comm_size(MPI_COMM_WORLD, &numProc);
 
  if( argc != 2){
      fprintf(stderr, "mush have only one argument: the length of array on each processor!\n");
      MPI_Abort(MPI_COMM_WORLD, 1);
  }
  
 
  /* Number of random numbers per processor */
  N = atoi(argv[1]);

  vec = calloc(N, sizeof(int));
  /* seed random number generator differently on every core */
  srand((unsigned int) (rank + 393919));

  /* fill vector with random integers */
  for (i = 0; i < N; ++i) {
    vec[i] = rand();
  }

  /*  printf("rank: %d, first entry: %d\n", rank, vec[0]); */


  /* start the clock */
  T1 = MPI_Wtime(); 

  /* STEP 1: sort locally */
  qsort(vec, N, sizeof(int), compare);
  
  /* debugging msg
  if (rank == 1){
    for (i = 0; i < N; i++){
        printf("v[%d] = %d \n", i,  vec[i]);	
    }
  }
  */ 

  /* STEP 2: draw S=P-1 local splitters  */	
  Splitter = (int *) malloc( sizeof(int) * (numProc-1) );
  for (i = 0; i < numProc-1 ; i ++){
      Splitter[i] = vec[N/numProc * (i+1)];
  } 
  
  /* debugging msg */
  /*
  if(rank == 0){
      for(i=0; i<numProc-1 ; i++){
          printf("Splitter[%d] = %d \n", i, Splitter[i] );
      }
  }
  */

  /* STEP 3: every processor send local splitters to root */
  CandidateSplitter = (int *) malloc( sizeof(int) * (numProc-1) * numProc );
  MPI_Gather( Splitter, numProc-1, MPI_INT, CandidateSplitter, numProc-1,
                                   MPI_INT, Root, MPI_COMM_WORLD  );

  /* STEP 4: Root sort all splitters and pick global splitters */
  if (rank == Root){
     qsort(CandidateSplitter, numProc*(numProc-1), sizeof(int), compare);
     
     /* debugging msg
     for (i = 0; i < numProc*(numProc-1) ; i++  ){
         printf("CandidateSplitter[%d] = %d\n", i, CandidateSplitter[i]); 
     }
     */
     
     /* pick global splitters */ 
     for(i = 0; i < numProc-1; i++){
         Splitter[i] = CandidateSplitter[ numProc/2 + numProc * i  ];
     }

     /* debugging msg */
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

  /* STEP 6: Each processor uses splitters to decide which integer 
             to be sent to which processor */
  rBucket = (int *) calloc( numProc, sizeof(int) );
  sBucket = (int *) calloc( numProc, sizeof(int) );
 
  /* determine how many integers are sent to every processor */
  j = 0; 
  for (i = 0; i < N; i++){
      if (j < numProc - 1){
          if ( vec[i] < Splitter[j] ){
              sBucket[j]++;
          }
          else{
              j++;
              sBucket[j]++;
          }
      }
      else{
          sBucket[numProc-1]++;
      }
  }
  
  /* debugging msg
  if(rank == 2){
      for(i=0; i<numProc ; i++){
          printf("sBucket[%d] = %d \n", i, sBucket[i] );
      }
  } 
  */ 

  /* STEP 7: all-to-all communcation to fill the reciver bucket: 
             rBucket[j] = # of integers to expect from processor j  */

  MPI_Alltoall(sBucket, 1, MPI_INT, rBucket, 1, MPI_INT, MPI_COMM_WORLD);
  
  /* debugging msg 
  if(rank == 1){
      for(i=0; i<numProc ; i++){
           printf("rBucket[%d] = %d \n", i, rBucket[i] );
      }
  }
  */
  
  
  /* STEP 8: all-to-all communication to send entries to the relevant
             processor */
  sdispls = (int *) calloc(numProc, sizeof(int));
  rdispls = (int *) calloc(numProc, sizeof(int));

  /* calculate send/recv displacement */
  for (i = 1; i < numProc; i++ ){
      sdispls[i] = sdispls[i-1] +  sBucket[i-1];
      rdispls[i] = rdispls[i-1] +  rBucket[i-1]; 
  }
  

  /* debugging msg 
  if(rank == 2){
      for(i=0; i<numProc ; i++){
          printf("sdispls[%d] = %d \n", i, sdispls[i] );
          printf("rdispls[%d] = %d \n", i, rdispls[i] );
      }
  }
  */ 
 
  /* construct newvec for sorting */
  newN = 0;
  for(i = 0; i < numProc; i++){
     newN = newN + rBucket[i];
  }
  newvec = (int *) malloc( sizeof(int) * newN );

  MPI_Alltoallv( vec,    sBucket, sdispls, MPI_INT, 
                 newvec, rBucket, rdispls, MPI_INT, MPI_COMM_WORLD );

 
 
  /* STEP 9: finally locally sort newvec */
  qsort(newvec, newN, sizeof(int), compare  ); 
  
  /* debugging msg
  if(rank == 2){ 
      for(i=0; i< newN; i++)
         printf("newv[%d] = %d\n", i , newvec[i] ); 
  }
  */

  /* stop the clock */
  T2 = MPI_Wtime();

  printf("rank = %d, elapsed time is %f\n", rank, T2-T1);

  /* STEP 10: print to file */
  FILE* fd1 = NULL;
  FILE* fd2 = NULL;
  char sorted_output[256];
  char timing[256];
  snprintf(sorted_output, 256, "ssort%03d_%03d.txt",numProc, rank);
  snprintf(timing, 256, "ssort_timing%03d.txt", numProc);
  fd1 = fopen(sorted_output, "w+");
  fd2 = fopen(timing, "a+");

  if (NULL == fd1 || NULL== fd2 ){
      printf("Error opening file \n");
      return 1;
  }
  
/* 
  for ( i = 0 ; i < newN ; i++ ){
      fprintf(fd1, "v[%d] = %d\n", i+1, newvec[i]);
  }
*/  
  fprintf(fd2, "t(%d) = %f\n", rank, T2-T1);
  
  free(vec);
  free(newvec);
  free(Splitter);
  free(CandidateSplitter);
  free(sBucket);
  free(rBucket);
  free(sdispls);
  free(rdispls);
   
  MPI_Finalize();
  return 0;
}
