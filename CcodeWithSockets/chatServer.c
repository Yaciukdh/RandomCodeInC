#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <pthread.h>
#define BUFFER_SIZE 1024
#define MAX_CLIENTS 100

pthread_mutex_t mutexThread  = PTHREAD_MUTEX_INITIALIZER;
int client_count; // total number of clients

struct C_info 
{
    char * name; //name that the user logged in as
    char * string;
    pthread_t tid;
    int logged_in;//1 = logged in, 0 otherwise
    int self_index;//index where it is stored in cinfo
    int sock;
};

struct C_info cinfo[MAX_CLIENTS]; //
struct sockaddr_in client;

/**************************************************************************/


void sort_alph(char ** a, int k)// found at https://stackoverflow.com/questions/44982737/sort-word-in-alphabet-order-in-c
{ //sorts alphabetically
    int i = 0 , j = 0;
    char* temp = (char*)calloc(30,sizeof(char));

     while ( i < k)
     {
       j = i+1;
       while ( j < k)
       {
         if (strcmp(a[i], a[j]) > 0)
         {
           strcpy(temp, a[i]);
           strcpy(a[i],a[j]);
           strcpy(a[j], temp);
         }
          j++;
       }
        i++;
     }
  
}

/**************************************************************************/


int parse(char ** word,char * buffer, int i) //parses word from buffer
{
    char letter[12];
    letter[0]= '\0';
    letter[1]= '\0';
    if(buffer[i]==' ')
    {
        i++;
    }
    
    while(buffer[i]!=' ' && buffer[i]!='\n')
    {
        letter[0]=buffer[i];
        strcat(*word,letter);
        i++;
    }
    

    return i;

}

/**************************************************************************/

void login_func( struct C_info * set_info, char * buffer , int i) //this lets TCP connections log in
{
    int index = 0;
    char * name = (char*)calloc(30,sizeof(char));
    index = parse(&name,buffer,i);
    int error = 0;
    printf("CHILD %u: Rcvd LOGIN request for userid %s\n", (unsigned int)pthread_self(),name);
    if(((index-i<3))||(16< index - i ))
    {
        printf("CHILD %u: Sent ERROR (Invalid userid)\n", (unsigned int)pthread_self());
        error = strlen("ERROR Invalid userid\n");
        send(set_info->sock,"ERROR Invalid userid\n", error, 0);
        return;
    }
    int k = 0;
    while(k<client_count)
    { 
      if(1 == cinfo[k].logged_in)
      {
        if(strcmp(name,cinfo[k].name)==0)
        {
          printf("CHILD %u: Sent ERROR (Already connected)\n", (unsigned int)pthread_self());
          error = strlen("ERROR Already connected\n");
          send(set_info->sock,"ERROR Already connected\n", error, 0);
          return;
        }
      }
      k++;
    }
    strcpy(set_info->name,name);
    free(name);
    send(set_info->sock,"OK!\n", 4, 0);
    set_info->logged_in = 1;
    return;
}

/**************************************************************************/


void who_func_udp(struct C_info * set_info) //This function lets users see who else is logged on for UDP connections
{
  printf("CHILD %u: Rcvd WHO request\n", (unsigned int)pthread_self());
  char * to_send = (char*)calloc(500,sizeof(char));
  char ** to_sort = (char**)calloc(64,sizeof(char*));
  int i = 0, k = 0;
  while(i < client_count)
  {
    if(1 == cinfo[i].logged_in )
    {
      to_sort[k] = (char*)calloc(16,sizeof(char));
      strcpy(to_sort[k],cinfo[i].name);
      k++;
    }
    i++;
  }
  sort_alph(to_sort,k);
  
  i = 0;
  strcpy(to_send,"OK!\n");
  while(i<k)
  {
      strcat(to_send,to_sort[i]);
      strcat(to_send,"\n");
      i++;
  }
  sendto( set_info->sock, to_send, strlen(to_send) , 0, (struct sockaddr *) &client, sizeof( client ) );
  
  return; 
  
}

/**************************************************************************/

void who_func( struct C_info * set_info ) //This function lets users see who else is logged on
{
  printf("CHILD %u: Rcvd WHO request\n", (unsigned int)pthread_self());
  char * to_send = (char*)calloc(500,sizeof(char));
  char ** to_sort = (char**)calloc(64,sizeof(char*));
  int i = 0, k = 0;
  while(i < client_count)
  {
    if(1 == cinfo[i].logged_in )
    {
      to_sort[k] = (char*)calloc(16,sizeof(char));
      strcpy(to_sort[k],cinfo[i].name);
      k++;
    }
    i++;
  }
  sort_alph(to_sort,k);
  
  i = 0;
  strcpy(to_send,"OK!\n");

  while(i<k)
  {
      strcat(to_send,to_sort[i]);
      strcat(to_send,"\n");
      i++;
  }
  send( set_info->sock, to_send, strlen(to_send), 0);
  
  return; 
  
}

/**************************************************************************/

void logout_func(struct C_info * set_info) // This function logs a user out
{
  printf("CHILD %u: Rcvd LOGOUT request\n", (unsigned int)pthread_self());
  set_info->logged_in = 0;
//  printf("%s has logged out\n",set_info->name);
  send(set_info->sock,"OK!\n", 4, 0);

}

/**************************************************************************/

void send_func(char * buffer , int i, struct C_info * set_info)
{
    int index = 0;
    char * name = (char*)calloc(17,sizeof(char));
    char * number = (char*)calloc(6,sizeof(char));
    char * message = (char*)calloc(991,sizeof(char));
    char * to_send = (char*)calloc(1024,sizeof(char));

    index = parse(&name,buffer,i);
    printf("CHILD %u: Rcvd SEND request to userid %s\n", (unsigned int)pthread_self(), name);

    index = parse(&number,buffer,index);
    int index_for_sending = -1;
    int length_message = atoi(number);
    strncpy(message,buffer+index+1,strlen(buffer)-index-2 );
    strcpy(to_send,"FROM ");
    strcat(to_send,set_info->name);
    strcat(to_send," ");
    strcat(to_send,number);
    strcat(to_send," ");
    strcat(to_send, message);
    strcat(to_send,"\n");

    printf("%s = %d\n",number, length_message);
    printf("%s\n",to_send);

    int k = 0;
    while(k<client_count)
    { 
//      printf("k = %d, name = %s, name at k = %s, logged in = %d\n",k,name,cinfo[k].name, cinfo[k].logged_in);
      if(1 == cinfo[k].logged_in)
      {
        if(strcmp(name,cinfo[k].name)==0)
        {
            index_for_sending = k;
        }
      }
      k++;
    }
    if(-1 == index_for_sending )
    {
      printf("CHILD %u: Sent ERROR (Unknown userid)\n",(unsigned int)pthread_self());
      int error = strlen("ERROR Unknown userid\n");
      send(set_info->sock,"ERROR Unknown userid\n", error, 0);
      error = error*error;
      return;
    }
    if(((length_message < 1))||(990 < length_message ))
    {
      
        printf("CHILD %u: Invalid msglen\n", (unsigned int)pthread_self());
        int error = strlen("ERROR Invalid msglen\n");
        send(set_info->sock,"ERROR Invalid msglen\n", error, 0);
        error = error*error;

        return;
        
    }
    send(set_info->sock, "OK!\n" , 4, 0);
    send(cinfo[index_for_sending].sock, to_send , strlen(to_send), 0);
    return;
}

/**************************************************************************/

void broadcast_func(char * buffer , int i)
{
  
}

/**************************************************************************/

void share_func(char * buffer , int i)
{
  
}


/**************************************************************************/


void * udp_command(void * arg)
{
  struct C_info * set_info;
  set_info = (struct C_info *) arg;
  if(strcmp(set_info->string,"WHO\n")==0)
  {
      who_func_udp(set_info);
  }
  pthread_exit(0);
  return 0;
}

/**************************************************************************/


void * tcp_serv(void * arg)
{
  struct C_info * set_info;
  set_info = (struct C_info *) arg;
  char buffer[2000];
  buffer[1]='\0';
  int lol = 0;
  int n = 1;
  while( 0 == lol)
  {
      n = recv( set_info->sock, buffer, 100, 0 );
      buffer[n]='\0';
      char * new_buff = (char*)calloc(n+1,sizeof(char));
      strcpy(new_buff,buffer);
      if ( -1==n )
      {
        perror( "recv() failed\n" );
        pthread_exit(0);
      }
      else if(0 == n)
      {
        printf( "CHILD %u: Client disconnected\n", (unsigned int)pthread_self() );
        close(set_info->sock);
        pthread_detach(pthread_self());
        pthread_exit(0);
      }
      else
      {
        char * command = NULL;
        command = (char*)calloc(30,sizeof(char));
        int index = 0;
        index = parse(&command, buffer, index);
      
        if(strcmp(command,"LOGIN")==0)
        {
          login_func(set_info, buffer, index);
        }
        else if(strcmp(command,"WHO")==0)
        {
          who_func(set_info);
        }
        else if(strcmp(command,"LOGOUT")==0)
        {
          logout_func(set_info);
        }
        else if(strcmp(command,"SEND")==0)
        {
          send_func(buffer, index, set_info);
        }
        else if(strcmp(command,"BROADCAST")==0)
        {
          broadcast_func(buffer, index);
        }
        else if(strcmp(command,"SHARE")==0)
        {
          share_func(buffer, index);
        }
        else
        {
          printf("Incorrect Command, Try Again!\n");
        }
        free(command);
        free(new_buff);
      }

  } 
  pthread_exit(0);
  return 0;
}

/**************************************************************************/


void set_structs()
{
  int i = 0;
  while(i<MAX_CLIENTS)
  {
    cinfo[i].name = (char*)calloc(30,sizeof(char*));
    cinfo[i].logged_in = 0;
    cinfo[i].self_index = i;
    i++;
  }
  return;
}

/**************************************************************************/

int get_lowest_unused_server()
{
  int i = 0;
  while(i<MAX_CLIENTS)
  {
    if(0==cinfo[i].logged_in)
    {
      if(client_count<=i)
      {
        client_count = i+1;
      }
      return i;
    }
    i++;
  }
  return -1;
}

/**************************************************************************/

int main(int argc, char *argv[])
{
  setvbuf( stdout, NULL, _IONBF, 0 );
  if(2!=argc)
  {
      perror("no.");
      return EXIT_FAILURE;
  }
  unsigned short port = (unsigned short) atoi(argv[1]);
  fd_set readfds;
  printf("MAIN: Started server\n");  
  set_structs();
  
  /* next free spot */
  /* Create the listener socket as TCP socket */
  printf("MAIN: Listening for TCP connections on port: %d\n",port); 
  int sockT = socket( PF_INET, SOCK_STREAM, 0 );
  printf("MAIN: Listening for UDP datagrams on port: %d\n",port);
  int sockU = socket( PF_INET, SOCK_DGRAM, 0 );
    /* note that PF_INET is protocol family, Internet */
  if ( sockT < 0 || sockU <0 )
  {
    perror( "socket()" );
    exit( EXIT_FAILURE );
  }

  /* socket structures from /usr/include/sys/socket.h */
  struct sockaddr_in server;
//  struct sockaddr_in client;

  server.sin_family = PF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons( port );
  
  int len = sizeof( server );

  if ( bind( sockT, (struct sockaddr *)&server, len ) < 0 )
  {
    perror( "bind()" );
    exit( EXIT_FAILURE );
  }
  if ( bind( sockU, (struct sockaddr *)&server, len ) < 0 )
  {
    perror( "bind()" );
    exit( EXIT_FAILURE );
  }

  listen( sockT, 32 ); 
  //  printf( "Listener bound to port %d\n", port );

  int fromlen = sizeof( client );

  char buffer[ BUFFER_SIZE ];

  int  n;
  FD_ZERO( &readfds );
  while ( 1 )
  {


    FD_SET( sockT, &readfds ); 
    FD_SET( sockU, &readfds ); 
    int ready = select( FD_SETSIZE, &readfds, NULL, NULL, NULL );
    
    if(0 == ready)
    {
      
    }
    else if(FD_ISSET(sockT,&readfds))//TCP
    {
        pthread_mutex_lock(&mutexThread);
        int sind = get_lowest_unused_server();
        pthread_t tid;

        if(-1 ==sind)
        {
          printf("not enough threads available\n");
        }
        else
        {
          printf("MAIN: Rcvd incoming TCP connection from: %s\n",inet_ntoa( client.sin_addr ));
          cinfo[sind].sock = accept(sockT,(struct sockaddr *)&client, (socklen_t* )&fromlen);
          int rc = pthread_create(&tid,NULL,tcp_serv,(void*)&cinfo[sind]);
          rc = rc*rc;
    
        }
      
       pthread_mutex_unlock(&mutexThread);
       pthread_detach(tid);

    }
    else if(FD_ISSET(sockU,&readfds))//UDP
    { 
        printf("MAIN: Rcvd incoming UDP datagram from: %s\n",inet_ntoa( client.sin_addr ));
        n = recvfrom(sockU, buffer, BUFFER_SIZE , 0, ( struct sockaddr *) &client, (socklen_t* ) &len); 
        buffer[n] = '\0';
        char * word = (char*)calloc(BUFFER_SIZE,sizeof(char));
        strcpy(word,buffer);
        struct C_info udp;
        udp.sock = sockU;
        udp.string = word;
        int rc = pthread_create(&udp.tid,NULL,udp_command,(void*)&udp);
        rc = rc*rc;
        pthread_detach(udp.tid);
        //broadcast
    }
  }
  return EXIT_SUCCESS; /* we never get here */
}
