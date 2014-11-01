/* A simple server in the internet domain using TCP
   The port number is passed as an argument 
   This version runs forever, forking off a separate 
   process for each connection
*/
#include <stdio.h>
#include <sys/types.h>   // definitions of a number of data types used in socket.h and netinet/in.h
#include <sys/socket.h>  // definitions of structures needed for sockets, e.g. sockaddr
#include <netinet/in.h>  // constants and structures needed for internet domain addresses, e.g. sockaddr_in
#include <stdlib.h>
#include <strings.h>
#include <sys/wait.h>	/* for the waitpid() system call */
#include <signal.h>	/* signal name macros, and the kill() prototype */


void sigchld_handler(int s)
{
    while(waitpid(-1, NULL, WNOHANG) > 0);
}

void dostuff(int); /* function prototype */
void error(char *msg)
{
    perror(msg);
    exit(1);
}

int main(int argc, char *argv[])
{
     int sockfd, newsockfd, portno, pid;
     socklen_t clilen;
     struct sockaddr_in serv_addr, cli_addr;
     struct sigaction sa;          // for signal SIGCHLD

     if (argc < 2) {
         fprintf(stderr,"ERROR, no port provided\n");
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
        error("ERROR opening socket");
     bzero((char *) &serv_addr, sizeof(serv_addr));
     portno = atoi(argv[1]);
     serv_addr.sin_family = AF_INET;
     serv_addr.sin_addr.s_addr = INADDR_ANY;
     serv_addr.sin_port = htons(portno);
     
     if (bind(sockfd, (struct sockaddr *) &serv_addr,
              sizeof(serv_addr)) < 0) 
              error("ERROR on binding");
     
     listen(sockfd,5);
     
     clilen = sizeof(cli_addr);
     
     /****** Kill Zombie Processes ******/
     sa.sa_handler = sigchld_handler; // reap all dead processes
     sigemptyset(&sa.sa_mask);
     sa.sa_flags = SA_RESTART;
     if (sigaction(SIGCHLD, &sa, NULL) == -1) {
         perror("sigaction");
         exit(1);
     }
     /*********************************/
     
     while (1) {
         newsockfd = accept(sockfd, (struct sockaddr *) &cli_addr, &clilen);
         
         if (newsockfd < 0) 
             error("ERROR on accept");
         
         pid = fork(); //create a new process
         if (pid < 0)
             error("ERROR on fork");
         
         if (pid == 0)  { // fork() returns a value of 0 to the child process
             close(sockfd);
             dostuff(newsockfd);
             exit(0);
         }
         else //returns the process ID of the child process to the parent
             close(newsockfd); // parent doesn't need this 
     } /* end of while */
     return 0; /* we never get here */
}

/******** DOSTUFF() *********************
 There is a separate instance of this function 
 for each connection.  It handles all communication
 once a connnection has been established.
 *****************************************/
void dostuff (int sock)
{
    int n;
    char buffer[512];
    
    //read in request and print it to the console
    bzero(buffer,512);
    n = read(sock,buffer,511);
    if (n < 0) error("ERROR reading from socket");
    else printf("\n%s\n",buffer);
    
    //tokenize request into headers
    char *headers = NULL;
    char *filename = NULL;
    char *fileExtension = NULL;
   //find path of file requested
    char *path = strstr(buffer, "GET /");
    char *httpLoc = strstr(buffer, " HTTP");
    if(path && httpLoc)
    {
        //printf("Found GET / and HTTP\n");    
        path += 5;
        char *end = path;
       // printf("String: %s\n", end);
        while(strncmp(end," ",1))
        {
            end++;
        }
        //printf("Size of filename: %d\n",end-path);

        filename = (char*)malloc((end - path)*sizeof(char));
        strncpy(filename, path, end-path);
        //printf("\nFilename: %s\n", filename);
    }
    else
    {
        //handle case when GET or HTTP is not found. Probably corrupt request
    }

    //Find file extension to make mime/type
    char *extLoc = strstr(filename, ".");
    if(extLoc)
    {
        extLoc += 1;
        size_t num = filename - extLoc;
        //printf("After . : %s\n",extLoc);
    }
    else
    {
        
        //No extension?
    }
        
    headers = strtok(buffer, "\n");
    while(headers){
        //printf("Current header: %s\n",headers);
        
        //get next header
        headers = strtok(NULL, "\n");
    }        
    
    //construct response header
    char *content-type = NULL;
    if(!strcmp(extLoc, "html"))
    { 
        content-type="text/html";
    }
    else if(!strcmp(extLoc, "txt"))
    {
        content-type="text/plain";
    }
    else if((!strcmp(extLoc, "jpg")) || (!strcmp(extLoc,"jpeg"))  )
    {
        content-type="image/jpeg";
    }
    //add bmp, gif, png and binarY    
    else
    {
    }
    // Try reading from a file and printing to the console
    int c;
    FILE *file;

    // open the file for reading
    file = fopen(filename, "r");

    char* response;
    int response_length;
    char *responseHeaders = NULL;

    responseHeaders = strtok(buffer, " ");
    if (file) {
        
        // get the size of the file so we can copy it into a string
        fseek(file, 0L, SEEK_END);
        response_length = ftell(file);
        // seek back to the beginning
        fseek(file, 0L, SEEK_SET);

        //printf("\nFILE SIZE: %i\n", response_length);
        response = (char*) malloc(response_length*sizeof(char));

        // copy the requested file to a local buffer
        int i = 0;
        while  ((c = getc(file)) != EOF) {
            response[i] = c;
            ++i;
        }

        //printf("\nTHE HTML:\n%s", response);
        //puts(response);

        fclose(file);
    }
    else {
        responseHeaders = "";
        // return 404 if the file doesn't exist
        response = "<!DOCTYPE html><html><body><h1>404 - Page Not Found</h1></body></html>";
        response_length = strlen(response);
    }
   
    //printf("\nresponse:\n%s\n", response);

    // return the request file to the client, or a 404 if it doesn't exist
    n = write(sock, response, response_length);
    if (n < 0) error("ERROR writing to socket");

}
