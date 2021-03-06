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
#include <time.h>
#include <sys/stat.h>
#include <errno.h>

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

/* append() 
 *
 * Takes a destination string, a source string, appends them, and returns the
 * result. Returns NULL on failure.
 * */
char *append(char *dest, char *src) {
    char* result = dest;
    result = realloc(dest, strlen(dest) + strlen(src));
    if (!result) {
        printf("REALLOC FAILED\n");
        return NULL;
    } else {
        result = strcat(result, src);
        return result;
    }
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
    char *response = (char*) malloc(0);
 
    //read in request and print it to the console
    bzero(buffer,512);
    n = read(sock,buffer,511);
    if (n < 0) error("ERROR reading from socket");
    else printf("\nno error!\n%s\n",buffer);
    
    //tokenize request into headers
    char *headers = NULL;
    char *filename = NULL;
    char *fileExt = NULL;
    
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
        if(filename){
            strncpy(filename, path, end-path);
       		// printf("\nFilename: %s\n", filename);
           	//Find file extension to make mime/type
		    fileExt = strstr(filename, ".");
		    if(fileExt)
		    {
		        fileExt += 1;
		        size_t num = filename - fileExt;
		        //printf("fileExtension : %s\n",fileExt);

		        //conditionally determine mimeType    
			    char *mimeType = NULL;
			    if(!strcmp(fileExt, "html"))
			    { 
			        mimeType="text/html";
			    }
			    else if(!strcmp(fileExt, "txt"))
			    {
			        mimeType="text/plain";
			    }
			    else if((!strcmp(fileExt, "jpg")) || (!strcmp(fileExt,"jpeg"))  )
			    {
			        mimeType="image/jpeg";
			    }
			    //add bmp, gif, png and binarY    
			    else if(!strcmp(fileExt, "bmp")) 
			    {
			        mimeType="image/bmp";
			    }
			    else if(!strcmp(fileExt, "png"))
			    {
			        mimeType="image/png";
			    }
			    else if(!strcmp(fileExt, "gif"))
			    {
			        mimeType="image/gif";
			    }
			    else
			    {
			        mimeType="application/octet-stream";
			    }
			    printf("mimeType : %s\n",mimeType);
		    }
		    else
		    {
		        
		        //TODO: No extension?
		    }
		    
		    
        }
        else{
        	//TODO: no file? serve index.html?
        }
    
    }
    else
    {
        //handle case when GET or HTTP is not found. Probably corrupt request
    }
    
    headers = strtok(buffer, "\n");
    while(headers){
        //printf("Current header: %s\n",headers);
        
        //get next header
        headers = strtok(NULL, "\n");
    }        
    
    //printf("Content-type: %s\n",mimeType);    
    // Try reading from a file and printing to the console
    int c;
    FILE *file;

    // open the file for reading
    file = fopen(filename, "r");

    char* body;
    int file_len;
    char *responseHeaders = NULL;

    responseHeaders = strtok(buffer, " ");

    char *statusHeader;

    if (file) {
        
        statusHeader = "HTTP/1.1 200 OK\n";

        // get the size of the file so we can copy it into a string
        fseek(file, 0L, SEEK_END);
        file_len = ftell(file);
        // seek back to the beginning
        fseek(file, 0L, SEEK_SET);

        //printf("\nFILE SIZE: %i\n", file_len);
        body = (char*) malloc(file_len*sizeof(char));

        // copy the requested file to a local buffer
        int i = 0;
        while  ((c = getc(file)) != EOF) {
            body[i] = c;
            ++i;
        }

            char tbuffer [80];

        fclose(file);
        statusHeader = "HTTP/1.1 200 OK\n";

    }
    else {
        
        statusHeader = "HTTP/1.1 404 Not Found\n";

        responseHeaders = "";
        // return 404 if the file doesn't exist
        body = "<!DOCTYPE html><html><body><h1>404 - Page Not Found</h1></body></html>";
        statusHeader = "HTTP/1.1 404 Not Found\n";
    }
    printf("Status Header: %s\n",statusHeader);

    char *dateHeader = "";
    time_t rawtime;
    struct tm * timeinfo;
    char tbuffer [80];
    time (&rawtime);
    timeinfo = gmtime (&rawtime);
    strftime (tbuffer,80,"Date: %a, %d %b %G %T GMT\n",timeinfo);
    dateHeader = tbuffer;

    char *serverHeader = "Server: BBServer/2.0\n";

    char *lastModifiedHeader = "Last-Modified: Wed, 18 Feb 2004 13:21:45 GMT\n";
    
    
    //char *reply = 
    //    "HTTP/1.1 200 OK\n" done
    //    "Date: Thu, 19 Feb 2009 12:27:04 GMT\n" done
    //    "Server: Apache/2.2.3\n"
    //    "Last-Modified: Wed, 18 Jun 2003 16:05:58 GMT\n"
    //    "ETag: \"56d-9989200-1132c580\"\n"
    //    "Content-Type: text/html\n"
    //    "Content-Length: 15\n"
    //    "Accept-Ranges: bytes\n"
    //    "Connection: close\n"
    //    "\n"
    //    "<!DOCTYPE html><html><body><h1>404 - Page Not Found</h1></body></html>";
     
    // build the response
    response = append(response, statusHeader);
    response = append(response, "\n");
    response = append(response, body);

    // return the request file to the client, or a 404 if it doesn't exist
    printf("writing to socket: \n%s", response);
    // sometimes garbage is at the end, so the magic number gets rid of that
    n = write(sock, response, strlen(response) - 3); 
    if (n < 0) error("ERROR writing to socket");

}
