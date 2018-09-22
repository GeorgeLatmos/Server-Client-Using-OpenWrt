#include <stdio.h>
#include <string.h>    //strlen
#include <stdlib.h>    //strlen
#include <sys/socket.h>
#include <arpa/inet.h> //inet_addr
#include <unistd.h>    //write
#include <pthread.h> //for threading , link with lpthread
#include <sys/time.h>
#include <signal.h>


#define MAX_NUMBER_OF_CONNECTIONS 5
#define MESSAGE_LENGTH 2000
#define LSH_TOK_BUFSIZE 64
#define LSH_TOK_DELIM " \t\r\n\a &"

char **split_line(char *line);
unsigned int alarm2 (unsigned int seconds, unsigned int useconds);
void catch_alarm (int sig);
void *connection_handler(void *);
void DataRate(int sock);

volatile sig_atomic_t keep_going = 1;

char connected[MAX_NUMBER_OF_CONNECTIONS][MESSAGE_LENGTH];
char messages[MAX_NUMBER_OF_CONNECTIONS][MESSAGE_LENGTH];
char clientID[MESSAGE_LENGTH];
int count = 0;
 
int main(int argc , char *argv[])
{
    int socket_desc , client_sock, c, *new_sock;
    struct sockaddr_in server , client;
    int port;

    if(argc != 2){
      printf("Invalid number of arguments! Aborting...\n");
      exit(1);
    }
     
    port = atoi(argv[1]);
     
    //Create socket
    socket_desc = socket(AF_INET , SOCK_STREAM , 0);
    if (socket_desc == -1)
    {
        printf("Could not create socket");
    }
    puts("Socket created");
     
    //Prepare the sockaddr_in structure
    server.sin_family = AF_INET;
    server.sin_addr.s_addr = INADDR_ANY;
    server.sin_port = htons(port);
     
    //Bind
    if( bind(socket_desc,(struct sockaddr *)&server , sizeof(server)) < 0)
    {
        //print the error message
        perror("bind failed. Error");
        return 1;
    }
    puts("bind done");
     
    //Listen
    listen(socket_desc , MAX_NUMBER_OF_CONNECTIONS);
     
    //Accept and incoming connection
    puts("Waiting for incoming connections...");
    c = sizeof(struct sockaddr_in);
    while( (client_sock = accept(socket_desc, (struct sockaddr *)&client, (socklen_t*)&c)) )
    {
        puts("Connection accepted");
         
        pthread_t sniffer_thread;
        new_sock = malloc(1);
        *new_sock = client_sock;
        int flag = 1;
         
        if( pthread_create( &sniffer_thread , NULL ,  connection_handler , (void*) new_sock) < 0)
        {
            perror("could not create thread");
            return 1;
        }
         
        //Now join the thread , so that we dont terminate before the thread
        pthread_join( sniffer_thread , NULL);

        int i;
        for(i=0; i<=count; i++){
          if(strcmp(clientID,connected[i]) == 0){
            send(client_sock, messages[i], strlen(messages[i]), 0);
            flag = 0;
          }
        }

        if(flag){
          send(client_sock, "No Message", strlen("No Message"), 0);
        }

        system("uptime"); //Consumption

        count++;

        if (count == MAX_NUMBER_OF_CONNECTIONS){
          memset(connected,0,MAX_NUMBER_OF_CONNECTIONS*MESSAGE_LENGTH);
          memset(messages,0,MAX_NUMBER_OF_CONNECTIONS*MESSAGE_LENGTH);
          memset(clientID,0,MESSAGE_LENGTH);
          count = 0;
          close(client_sock);
        }
    }
     
    if (client_sock < 0)
    {
        perror("accept failed");
        return 1;
    }
     
    return 0;
}
 
/*
 * This will handle connection for each client
 * */
void *connection_handler(void *socket_desc)
{
    //Get the socket descriptor
    int sock = *(int*)socket_desc;
    int read_size;
    char from[MESSAGE_LENGTH],to[MESSAGE_LENGTH],message[MESSAGE_LENGTH], buffer[MESSAGE_LENGTH];
    char **temp;

    recv(sock , buffer , MESSAGE_LENGTH , 0);

    temp = split_line(buffer);

    strcpy(clientID,temp[0]);
    strcpy(connected[count],temp[1]);
    strcpy(messages[count],temp[2]);

    DataRate(sock);
         
    //Free the socket pointer
    free(socket_desc);
     
    return 0;
}

/*This function calculates the Data Rate*/
void DataRate(int sock){

  char buffer[MESSAGE_LENGTH];

  signal (SIGALRM, catch_alarm);
  alarm2(1,0);

  printf("CALCULATE DATA RATE...\n");

  FILE *fp;

  fp = fopen("time_send.txt", "w+");
    
  while(keep_going){
    recv(sock , buffer , MESSAGE_LENGTH , 0);
    fprintf(fp,"%s",buffer);
  }

  fseek(fp,0,SEEK_END);

  fprintf(stdout,"DATA RATE: %ld Bytes/sec\n", ftell(fp));

  keep_going = 1;

  fclose(fp);
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

/*This functions splits one string into pieces. Tokens are used*/
char **split_line(char *line)
{
  int bufsize = LSH_TOK_BUFSIZE, position = 0;
  char **tokens = malloc(bufsize * sizeof(char*));
  char *token, **tokens_backup;

  token = strtok(line, LSH_TOK_DELIM);
  while (token != NULL) {
    tokens[position] = token;
    position++;

    token = strtok(NULL, LSH_TOK_DELIM);
  }
  tokens[position] = NULL;
  return tokens;
}
