// Chris O'Brien
// CS-311 (Concepts of Operating Systems)
// Traditional solution to the dining philosophers problem using semaphores

#include <stdio.h>  // acts like "import java.io.*;"
#include <stdlib.h> // for library function prototypes, including srand()
#include <errno.h> // for errors
#include <string.h> // for manipulating strings
#include <sys/types.h> // for queueing
#include <sys/ipc.h> // for queueing
#include <sys/sem.h> // for semaphores
#include <unistd.h> // for sleep function
#include <errno.h> // for errors
#define MAX_EATS 3
#define TRUE 1
#define FALSE 0

int main(int argc, char *argv[]) {
  int philNum, // passed as an argument
    forks, // ID for an array of 5 semaphores
    philCount, // number of philosophers at the table
    ret, // for error checking
    i, i2, timesAte[5],  // counters
    timeCount[5], // tracks the amount of time each philosopher sleeps for
    done; // boolean to say if the meal's over
  struct sembuf leftWait, rightWait, // P() operations
    leftSignal, rightSignal, // V() operations
    addPhil, // increment philCount
    subtractPhil; // decrement philCount

  philNum = atoi(argv[1]);
  if (philNum < 0 || philNum > 4) {
    printf("ERROR: invalid argument passed!\n");
    exit(1);
  }

  // create/attach to forks
  if ((forks = semget(370, 5, IPC_CREAT | 0600)) < 0) { 
    printf("ERROR creating forks in phil #%d!\n", philNum);
    exit(1);
  }
  // create/attach to philCount
  if ((philCount = semget(375, 1, IPC_CREAT | 0600)) < 0) { 
    printf("ERROR creating philCount in phil #%d!\n", philNum);
    exit(1);
  }
  //printf("Forks: %d, philCount: %d\n", forks, philCount);
  printf("Phil #%d started & attached.\n", philNum);

  // initialize sembuf structs
  leftWait.sem_flg = 0; // always 0
  leftWait.sem_num = (philNum+1)%5;
  leftWait.sem_op = -1; // decrement semaphore

  printf("leftWait.sem_num = %d (should be 1).\n", (philNum+1)%5);

  rightWait.sem_flg = 0;
  rightWait.sem_num = philNum;
  rightWait.sem_op = -1;

  leftSignal.sem_flg = 0;
  leftSignal.sem_num = (philNum+1)%5;
  leftSignal.sem_op = 1;

  rightSignal.sem_flg = 0;
  rightSignal.sem_num = philNum;
  rightSignal.sem_op = 1;

  addPhil.sem_flg = 0;
  addPhil.sem_num = 1;
  addPhil.sem_op = 1;

  subtractPhil.sem_flg = 0;
  subtractPhil.sem_num = 1;
  subtractPhil.sem_op = -1;

  //printf("Forks: %d, philCount: %d\n", forks, philCount);

  if (philNum == 0){
    for(i2 = 0; i2 < 5; i2++){
      // initialize forks to 1
      if (semctl(forks, i2, SETVAL, 1) < 0) {
        perror("ERROR initializing fork!");
        //printf("ERROR initializing fork #%d! ret: %d\n", philNum, ret);
        exit(1);
      }
    }
    // initialize philCount
    if (semctl(philCount, 0, SETVAL, 5) < 0) {
      perror("ERROR initializing fork!");
      exit(1);
    }
    printf("philCount set to 5.\n");
  }
  else { // other philosophers
    if (semop(philCount, &addPhil, 1) < 1) { // Increment philCount
      printf("ERROR incrementing philCount!\n");
      exit(1);
    }
  }

  timesAte[philNum] = 0; // initialize timesAte
  srand(10/* + getpid()*/); // seed for rand()
  timeCount[philNum] = (rand() % 30 + 60);
  printf("Phil #%d is sleeping for %d seconds.\n", philNum, timeCount[philNum]);
  fflush(stdout);
  sleep (timeCount[philNum]); // wait 60-90 seconds

  for(i = 0; i < MAX_EATS; i++) { // think, then grab left fork, then right, then eat, then put down left, then right
    if (timesAte[philNum] == 3) break;

    timeCount[philNum] = (rand() % 10 + 10)
    printf("Phil #%d is thinking for %d seconds.\n", philNum, timeCount[philNum]);
    fflush(stdout);
    sleep (timeCount[philNum]); // think for 10-20 seconds

    if ((semop(forks, &leftWait, 1)) < 1) { // P(Left Fork)
      perror("ERROR waiting for fork!");
      exit(1);
    }
    printf("Phil #%d grabs left fork.\n", philNum);
    fflush(stdout);

    if ((semop(forks, &rightWait, 1)) < 1) { // P(Right Fork)
      printf("ERROR waiting for fork!\n");
      exit(1);
    }
    printf("Phil #%d grabs right fork.\n", philNum);

    timeCount[philNum] = (rand() % 5 + 5)
    printf("Phil #%d is eating for %d seconds.\n", philNum, timeCount[philNum])
    fflush(stdout);
    sleep (timeCount[philNum]); // eat for 5-10 seconds
    timesAte[philNum]++;
    printf("Phil #%d has eaten %d times.\n", philNum, timesAte[philNum]);


    if ((semop(forks, &leftSignal, 1)) < 1) { // V(Left Fork)
      printf("ERROR signaling fork!\n");
      exit(1);
    }
    printf("Phil #%d drops left fork.\n", philNum);

    if ((semop(forks, &rightSignal, 1)) < 1) { // V(Right Fork)
      printf("ERROR signaling fork!\n");
      exit(1);
    }
    printf("Phil #%d drops right fork.\n", philNum);
  }
  
  ret = semctl(philCount, 0, GETVAL);
  if (ret < 0) {
    perror("ERROR checking philCount!");
    exit(1);
  }
  else if(ret == 1) { // last philosopher, clean up               **NEED MUTEX????**
    if (!done) {
      done = TRUE;
      if (semctl(forks, 1, IPC_RMID, (void *) 0) < 0) { // remove forks sem
        printf("ERROR removing forks!\n");
        exit(1);
      } 
      if (semctl(philCount, 1, IPC_RMID, (void *) 0) < 0) { // remove counter sem
        printf("ERROR removing philCount!\n");
        exit(1);
      } 
    }
  }
  else { // not the last philosopher
    if ((semop(philCount, &subtractPhil, 1)) < 1) { // decrement philCount
      printf("ERROR decrementing philCount!\n");
      exit(1);
    }
  }
  printf("Exiting phil #%d.\n", philNum);
  return 0;
}