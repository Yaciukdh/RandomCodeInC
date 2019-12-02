#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <stdlib.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>


/*
 * General comments:
 *
 * Written very early in my C career.
 *
 * Any operations that seem redundant are there to get rid of compiler errors
 *
 * I.E. unused variable errors
 *
 *
 *
 */

#define BUFFER_SIZE 100


pthread_mutex_t mutexThread  = PTHREAD_MUTEX_INITIALIZER;

/********************************************************************************************************/

struct Stuff
{
    char* buff;
    int data;
    int new;
    struct sockaddr_in clie;
} ;

/**************************************************************************************************/

int dumbSort(char** names, int numOfNames, char** list) //alphabetically sort
{
//FOUND HERE : https://stackoverflow.com/questions/40033310/sorting-a-list-of-strings-in-alphabetical-order-c
    int i = 0, j = 0;
    char temp[30];
    while(i < numOfNames)
    {
        j=0;
        while( j < numOfNames)
        {
            if(strcmp(names[i], names[j]) < 0)
            {
                strcpy(temp, names[i]);
                strcpy(names[i], names[j]);
                strcpy(names[j], temp);
            }
            j++;
        }
        i++;
    }
    i=0;
    while( i < numOfNames)
    {
        strcat(*list," ");
        strcat(*list,names[i]);
        i++;
    }
    return 0;
}
/************************************************************************************************/

int chFileName(char* filenamez);

/************************************************************************************************/
int parse(char **word,char * buffer, int i) //gets the next word in the buffer
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
/************************************************************************************************/

void * mesFun( void * arg );

/************************************************************************************************/

void directoryCheck() //checks if directory exists.
{
    DIR * dir = opendir( "storage" );   /* open the current working directory */
    if(dir==NULL){
        perror("Invalid directory.\n");
        exit(-1);
    }
    closedir(dir);
}

/********************************************************************************************************/

int textCheck(char* filename) //checks if there are numbers or periods in filename
{

    int l = 0;
    l = strlen(filename);
    int i = 0;
    int error = 0;
    while(i<l)
    {
        if(isalnum(filename[i])|| filename[i]=='.')
        {
            error=-5000;
        }
        i++;
    }

    return error;
}

/********************************************************************************************************/
int numOfByt(char* filename) //counts bytes
{

    DIR * dir = opendir( "." );
    if(dir==NULL)
    {
        perror("Invalid directory.\n");
        fflush( stdout );
        exit(-11);
    }
    struct dirent * file;
    while ( ( file = readdir( dir ) ) != NULL )
    {
        if (file->d_type == DT_REG)
        {
            if(strcmp(filename,file->d_name)==0)
            {
                    struct stat buf;
                    int rc = lstat( (filename), &buf );  /* e.g., "xyz.txt" */
                    if ( rc == -1 )
                    {
                          perror( "lstat() failed" );
                          return EXIT_FAILURE;
                    }
                    int bytes =(int)buf.st_size;
                    closedir(dir);
                    return bytes;

            }
        }
    }

    closedir(dir);
    return 0;
}


/********************************************************************************************************/

int main(int argc, char *argv[])
{
  /* Create the listener socket as TCP socket */
  directoryCheck();
  if(argc <2)
  {
      perror("Not enough arguments");
      return EXIT_FAILURE;
  }
   unsigned short port = (unsigned short) atoi(argv[1]);

  int sd = socket( PF_INET, SOCK_STREAM, 0 );
  int full = 0;
  full = full*full+1-1;
  pthread_t tid;
  struct Stuff stuff [100];

  if ( sd == -1 ) //checks socket
  {
    perror( "socket() failed" );
    return EXIT_FAILURE;
  }
  /* socket structures */
  struct sockaddr_in server;
  server.sin_family = PF_INET;
  server.sin_addr.s_addr = INADDR_ANY;
  server.sin_port = htons( port );
  int len = sizeof( server );
  if ( bind( sd, (struct sockaddr *)&server, len ) == -1 ) //bind() check
  {
    perror( "bind() failed" );
    return EXIT_FAILURE;
  }
  /* identify this port as a listener port */
  if ( listen( sd, 5 ) == -1 )
  {
    perror( "listen() failed" );
    return EXIT_FAILURE;
  }
  printf( "Started server\n" );
  fflush( stdout );
  printf( "Listening for TCP connections on port: %d\n", port );
  fflush( stdout );
  struct sockaddr_in client;
  int fromlen = sizeof( client );
  int i = 0;

  while ( 1 )
  {
    int newsd = accept( sd, (struct sockaddr *)&client, (socklen_t *)&fromlen );
    int len=20;
    char buffer[len];
    inet_ntop(AF_INET, &(client.sin_addr), buffer, len);

    printf( "Rcvd incoming TCP connection from: %s\n",buffer );
    fflush( stdout );
    stuff[i].clie = client;
    stuff[i].new  = newsd;
    int rc = pthread_create( &tid, NULL, mesFun, (void *)&stuff[i]);
    i++;
    if(rc != 0){

        perror("Pthread error\n");
    }
    pthread_detach(tid);
    if(i==100)
    {
        full = 1;
        i=0;
    }

  }
  return EXIT_SUCCESS;
}
/*************************************************************************************************/
int listFiles(int newsd)
{

    char * list = NULL;
    char buffer[100];
    DIR * dir = opendir( "storage" );
    char ** words = (char**)calloc(30,sizeof(char*));
    if(dir==NULL){
        perror("Invalid directory.\n");
        exit(-11);
    }
    int file_count = 0;
    struct dirent * file;

    while ( ( file = readdir( dir ) ) != NULL )
    {
        if (file->d_type == DT_REG)
        { /* If the entry is a regular file */
          file_count++;
        }
    }

    closedir(dir);
    sprintf(buffer,"%d",file_count);
    list = buffer;
    if(file_count == 0)
    {
        printf("%s\n",list);

    }
    else
    {

        DIR * dir = opendir( "storage" );   /* open the current working directory */
        if(dir==NULL){
            perror("Invalid directory.\n");
            fflush( stdout );
            exit(-11);
        }
        struct dirent * file;
        int i = 0;
        while ( ( file = readdir( dir ) ) != NULL )
        {

            if (file->d_type == DT_REG)
            {
 //               strcat(list," ");
 //               strcat(list,file->d_name);
                char * word   = (char*)calloc(1,sizeof(char));
                word = file->d_name;
                words[i]= word;
                i++;
            }


        }
        int k =dumbSort(words,i, &list);
        if(k==-1)//checks sort
        {
            printf("Sort Failed\n");
        }
        strcat(list,"\n");

        char bu[strlen(list)];
        int f = 0;
        while(f<strlen(list))
        {
            bu[f]=list[f];
            f++;
        }

        int g = send( newsd, bu, strlen(list), 0 );
        if(g!=strlen(list)){perror("Send failed\n");}
        printf("[child %u] Sent %s",(unsigned int)pthread_self(),list);
        fflush( stdout );
    }

    return 0;
}

/************************************************************************************************/
int putFiles(int newsd, char* buff)
{
    fflush( stdout );

    int b = 0;
    char buffer[BUFFER_SIZE];
    int ret = chdir( "storage" );   /* open the current working directory */
    fflush( stdout );

    if(ret!=0)
    {    //
        perror("Invalid directory.\n");
        exit(-11);
    }
        FILE * fp;
        char * file_name = (char*)calloc(1,4*sizeof(char));
        int g = parse(&file_name, buff, 3);
        int err1 = chFileName(file_name);
        int err2 = textCheck(file_name);
        if(err1 == 0)
        {
            char * file_b = (char*)calloc(1,4*sizeof(char));
            g = parse(&file_b,buff,g);
            int byt = atoi(file_b);
            if(byt<1 && err2 == -5000)
            {
                char * mh = "ERROR INVALID REQUEST\n";
                int b = send( newsd, mh, strlen(mh), 0 );//send
                if(b!=strlen(mh)){perror("Send failed\n");}

                chdir("..");

                return 0;
            }
            else
            {
                fflush( stdout );

                fp = fopen (file_name,"w");
                fflush( stdout );
                int sumt1 = 0;
                int sumt2 = 0;

                while(sumt1<byt)
                {
                    int n = recv( newsd, buffer,1, 0 );

                    if(n==0)//received message of length 0
                    {
                        printf( "[child %u] Client disconnected\n", (unsigned int)pthread_self() );
                        fflush( stdout );
                        pthread_mutex_unlock( &mutexThread );
                        pthread_exit(NULL);//quits thread if 0 bytes sent
                    }

                    sumt1 = sumt1+n;
                    int i = 0;
                    char letter[16];
                    letter[1] = '\0';
                    letter[0] = '\0';

                    while(i<n && (sumt2+i)<byt)
                    {
                        letter[0] = buffer[i];
                        fprintf (fp, "%c",letter[0]);//write letter
                        i++;
                    }
                    sumt2=sumt1;
                }

                    printf("[child %u] Stored file \"%s\" (%d bytes)\n", (unsigned int)pthread_self()  , file_name,byt);
                    fflush( stdout );
                    printf("[child %u] Sent ACK\n", (unsigned int)pthread_self());
                    fflush( stdout );
                    char ack[4];
                    ack[0] = 'A';
                    ack[1] = 'C';
                    ack[2] = 'K';
                    ack[3] = '\n';

                    b = send( newsd, ack, 4, 0 );
                   free(file_name);
                    if(b!=4){perror("Send failed\n");}
                    fclose (fp);//close
                    chdir("..");
                    return 0;
            }
        }
        else
        {
            printf("[child %u] Sent ERROR FILE EXISTS\n", (unsigned int)pthread_self());
            fflush( stdout );
            char * mh = "ERROR FILE EXISTS\n";
            char bf[strlen(mh)];
            int k = 0;
            while(k<strlen(mh))
            {
                bf[k]=mh[k];
                k++;
            }

            int b = send( newsd,bf, strlen(mh), 0 );//send
            if(b!=strlen(mh)){perror("Send failed\n");}

            chdir("..");
            return 0;
        }

}
/***********************************************************************************************/

int chFileName(char* filenamez) //if file exists, send error
{
        int error = 0;
        DIR * dir = opendir( "." );   /* open the current working directory */

        if(dir==NULL){
            perror("Invalid directory.\n");
            exit(-11);
        }
        struct dirent * file;

        while ( ( file = readdir( dir ) ) != NULL )
        {

            if (file->d_type == DT_REG)
            {
                if(strcmp(filenamez,(file->d_name))==0)
                {
                    error = -5000;
                    return error;
                }
            }


        }
        return error;

}

/************************************************************************************************/
int isFileThere(char* filenamez) //checks if file is there
{
        int error = -5000;
        DIR * dir = opendir( "." );   /* open the current working directory */
        if(dir==NULL){
            perror("Invalid directory.\n");
            exit(-11);
        }
        struct dirent * file;

        while ( ( file = readdir( dir ) ) != NULL )
        {

            if (file->d_type == DT_REG)
            {
                if(strcmp(filenamez,(file->d_name))==0)
                {
                    error = 0;
                    return error;
                }
            }


        }
        return error;
}

/************************************************************************************************/
int getFiles(int newsd, char* buff)
{

 //   char buffer[BUFFER_SIZE];
    int f = 0;
    int ret = chdir( "storage" );   /* open the current working directory */
    if(ret!=0){
        perror("Invalid directory.\n");
        exit(-11);
    }
        FILE * fp;
        char * file_name = (char*)calloc(1,4*sizeof(char));
        int g = parse(&file_name, buff, 3);

        int err1 = isFileThere(file_name);
        int err2 = textCheck(file_name);
        if(err1 == 0)
        {
            char * file_len = (char*)calloc(1,4*sizeof(char));
            char * file_b = (char*)calloc(1,4*sizeof(char));

            g = parse(&file_b,buff,g);
            g = parse(&file_len,buff,g);

            int len = atoi(file_len);
            int byt = atoi(file_b);
            if(strlen(file_b)==0)
            {
            char * mh = "ERROR INVALID REQUEST\n";
            char bu[strlen(mh)];
            int f = 0;
            while(f<strlen(mh))
            {
                bu[f]=mh[f];
                f++;
            }
            int b = send( newsd,bu, strlen(mh), 0 );//send
            if(b!=strlen(mh)){perror("Send failed\n");}
            chdir("..");
            return 0;
            }
            else if( len<1 && err2 == -5000)
            {
            char * mh = "ERROR INVALID REQUEST\n";
            char bu[strlen(mh)];
            int f = 0;
            while(f<strlen(mh))
            {
                bu[f]=mh[f];
                f++;
            }
            int b = send( newsd, bu, strlen(mh), 0 );//send
            if(b!=strlen(mh)){perror("Send failed\n");}
            chdir("..");
            return 0;
            }
            else{

                fp = fopen (file_name,"r");
                int bytenum = numOfByt(file_name);
                int  start = byt;
                int  end   = byt + len;

                if(end<=bytenum)
                {
                    char letter[12];
                    letter[0]='\0';
                    letter[1]='\0';
                    char *  by = (char*)calloc(1,4*sizeof(char));
                    char * ack = "ACK ";
                    char * endl = "\n";
                    strcpy(by,ack);
                    strcat(by,file_len);
                    strcat(by,endl);
                    char bu[strlen(by)];
                    f = 0;
                    while(f<strlen(by))
                    {
                        bu[f]=by[f];
                        f++;
                    }
                    int  b     = send( newsd, bu, strlen(by), 0 );
                    printf("[child %u] Sent ACK %s\n", (unsigned int)pthread_self(), file_len);
                    fflush( stdout );


                    char* le = (char*)calloc(1,4*sizeof(char));
                    strcpy(le,"");
                    char piece =' ';
                    int i = 0;
                    while(i<start)
                    {
                        piece = fgetc(fp);
                        i++;
                    }
                    while(i<end)
                    {
                        piece     = fgetc(fp);
                        letter[0] = piece;
                        strcat(le,letter);
                        i++;
                    }
                    char bg[strlen(le)];
                    f = 0;
                    while(f<strlen(le))
                    {
                        bg[f]=le[f];
                        f++;
                    }
                    b = send( newsd, bg, strlen(le), 0 );
                    printf("[child %u] Sent %s bytes of \"%s\" from offset %s\n", (unsigned int)pthread_self(),file_len,file_name, file_b);
                    fflush( stdout );
                    if(b!=strlen(le)){perror("Send failed\n");}
 //                   free(le);
                    free(by);
                    free(file_b);
                    free(file_len);
                    free(file_name);
                    chdir("..");
                    return 0;
                }
                else
                {
                    printf("[child %u] Sent ERROR INVALID BYTE RANGE\n", (unsigned int)pthread_self());
                    fflush( stdout );
                    char * mh = "ERROR INVALID BYTE RANGE\n";
                    char bu[strlen(mh)];
                    int f = 0;
                    while(f<strlen(mh))
                    {
                        bu[f]=mh[f];
                        f++;
                    }

                    int b = send( newsd, bu, strlen(mh), 0 );//send
                    if(b!=strlen(mh)){perror("Send failed\n");}
                    chdir("..");
                    return 0;
                }
            }
        }
        else
        {
            printf("[child %u] Sent ERROR NO SUCH FILE\n", (unsigned int)pthread_self());
            fflush( stdout );
            char * mh = "ERROR NO SUCH FILE\n";
            char bu[strlen(mh)];
            int f = 0;
            while(f<strlen(mh))
            {
                bu[f]=mh[f];
                f++;
            }
            int b = send( newsd, bu, strlen(mh), 0 );//send
            if(b!=strlen(mh)){perror("Send failed\n");}
            chdir("..");
            return 0;
        }

}

/************************************************************************************************/
void * mesFun( void * arg)    //thread function, message functiom
{
   struct Stuff *t;
   t = (struct Stuff *) arg;
   char buffer[ BUFFER_SIZE ];
   int newsd = t->new;
   int n = 1 ;
   int whoa = 0;
   whoa = whoa * whoa + 0;

   do
   {
      fflush( stdout );
      char buffer2[2];
      buffer[1]='\0';
      int j = 0;
      int k = 0;
      while(k==0 && n>0)
      {
        n = recv( newsd, buffer2, 1, 0 );
        buffer[j]=buffer2[0];
        j++;
        if(buffer2[0]=='\n')
        {
            k=12;
        }
      }
      buffer[j]='\0';
      if ( n == -1 )
      {
        perror( "recv() failed\n" );
        exit(-1);
      }
      else if ( n == 0 )
      {
        printf( "[child %u] Client disconnected\n", (unsigned int)pthread_self() );
        fflush( stdout );

      }
      else /* n > 0 */
      {
        fflush( stdout );
        if(strlen(buffer)>3) // just for organization purposes
        {
            if(buffer[0]=='L'&&buffer[1]=='I'&&buffer[2]=='S'&&buffer[3]=='T')//LIST
            {
                 printf( "[child %u] Received %s",(unsigned int)pthread_self(), buffer );
                 pthread_mutex_lock( &mutexThread );
                 whoa =listFiles(newsd);
                 pthread_mutex_unlock( &mutexThread );

            }
        }
        if(strlen(buffer)>2)
        {
            if(buffer[0]=='P'&&buffer[1]=='U'&&buffer[2]=='T')//PUT
            {
               printf( "[child %u] Received %s",(unsigned int)pthread_self(), buffer );
               pthread_mutex_lock( &mutexThread );
               whoa = putFiles(newsd, buffer);
               pthread_mutex_unlock( &mutexThread );

            }
           if(buffer[0]=='G'&&buffer[1]=='E'&&buffer[2]=='T')//GET
            {
                printf( "[child %u] Received %s",(unsigned int)pthread_self(), buffer );
                pthread_mutex_lock( &mutexThread );
                whoa =getFiles(newsd, buffer);
                pthread_mutex_unlock( &mutexThread );


            }
        }
        int i = 0;
        while(i<strlen(buffer))
        {
            buffer[i] = '\0';
            i++;
        }

     }
   }while( n >0 );
    close( newsd );
    pthread_exit(NULL);

}
