#include <stdio.h> 
#include <stdlib.h>
#include <string.h>    
#include <sys/socket.h>    
#include <arpa/inet.h> 
#include <unistd.h>
#include <pthread.h>
#include <assert.h>
#include <sys/time.h>
#include <signal.h>

#define MESSAGE_LENGTH 2000

unsigned int alarm2 (unsigned int seconds, unsigned int useconds);
void catch_alarm (int sig);
static void *thread(void *xa);

volatile sig_atomic_t keep_going = 1;

int id = 0;

pthread_mutex_t mutex;

struct sockaddr_in server;
 
int main(int argc , char *argv[])
{
    int port;

    int nthread;
    void *value;
    long i;
    pthread_t *tha;

    if(argc != 4){
       printf("Invalid number of arguments! Aborting...\n");
       exit(1);
    }

    nthread = atoi(argv[3]);
    tha = malloc(sizeof(pthread_t) * nthread);

    port = atoi(argv[2]);
     
    server.sin_addr.s_addr = inet_addr(argv[1]);
    server.sin_family = AF_INET;
    server.sin_port = htons(port);

    for(i = 0; i < nthread; i++) {
      assert(pthread_create(&tha[i], NULL, thread, (void *) i) == 0);
    }

    for(i = 0; i < nthread; i++) {
      assert(pthread_join(tha[i], &value) == 0);
    } 
    
    return 0;
}

static void *
thread(void *xa)
{  
  long n = (long) xa;
  char reply[MESSAGE_LENGTH],buffer[MESSAGE_LENGTH];
  int sock;

  pthread_mutex_lock(&mutex);

  //Create socket
  sock = socket(AF_INET , SOCK_STREAM , 0);
  if (sock == -1)
  {
      printf("Could not create socket");
  }
  puts("Socket created");

  //Connect to remote server
  if (connect(sock , (struct sockaddr *)&server , sizeof(server)) < 0)
  {
    perror("connect failed. Error");
    exit(1);
  }else{
    //printf("%ld: Connected \n", n);
  }

  id++;

  sprintf(buffer,"192.168.1.%d&192.168.1.%d&hello",id,id+1);
  
  send(sock, buffer, strlen(buffer), 0);

  signal (SIGALRM, catch_alarm);
  alarm2(1,0);

  while(keep_going){
    send(sock, buffer, strlen(buffer), 0);
  }

  recv(sock , reply , MESSAGE_LENGTH , 0);
  printf("Message: %s\n", reply);

  close(sock);

  keep_going = 1;

  pthread_mutex_unlock(&mutex);
  
}

/*This function resets the internal Real-Time Timer*/
unsigned int
alarm2 (unsigned int seconds, unsigned int useconds)
{
  struct itimerval old, new;
  new.it_interval.tv_usec = 0;
  new.it_interval.tv_sec = 0;
  new.it_value.tv_usec = (long int) useconds;
  new.it_value.tv_sec = (long int) seconds;
  if (setitimer (ITIMER_REAL, &new, &old) < 0)
    return 0;
  else
    return old.it_value.tv_sec;
}

/* The signal handler just clears the flag and re-enables itself. */
void
catch_alarm (int sig)
{

  keep_going = 0;

  signal(SIGALRM, catch_alarm);
}