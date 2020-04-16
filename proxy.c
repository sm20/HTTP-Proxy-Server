/* 

Author: SAJID CHOUDHRY

A proxy server for both receiving a client's(browser- sending HTTP requests) requests,
forwarding that request to the actual server, receiving the response from the server,
and finally forwarding that response to the original client.

This program initializes the sockets needed for the communication, and it takes client requests,
and forwards server responses manipulated. It manipulates some text on sites, and images shown, both
embedded and linked.

Limitations: It only changes one image on a site with multiple embedded images. It would have been better 
to learn and use regex string manipulation rather than using the manual method I used for find and replace.


Usage:

    1.Set your broewser(firefox) to read from proxy: port 8080 and localhost IP address 127.0.0.1
        Alternatively you can set your system to go through that proxy in network settings.
    2. compile using: gcc proxy.c -o proxy
    3. Go to a HTTP website such as a ucalgary prof site, turn off cache in dev settings(learn how to)
    4. Run: ./proxy
    5. interact act with different elements on the site.
    

REFERENCES:
TA- Rachel Mclean - CPSC 441 - Winter 2020 - University of Calgary
https://stackoverflow.com/questions/22184403/how-to-cast-the-size-t-to-double-or-int-c
https://stackoverflow.com/questions/13293226/initializing-array-element-to-null
https://en.wikibooks.org/wiki/C_Programming/String_manipulation#The_strstr_function
https://en.wikibooks.org/wiki/C_Programming/string.h/strstr
https://stackoverflow.com/questions/5169562/finding-all-instances-of-a-substring-in-a-string
https://webmasters.stackexchange.com/questions/25342/headers-to-prevent-304-if-modified-since-head-requests
http://beej.us/guide/bgnet/html/
https://www.w3schools.com/tags/ref_httpmethods.asp

*/

/* standard libraries*/
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

/* libraries for socket programming */
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

/*libraries for parsing strings*/
#include <string.h>
#include <strings.h>

/*h_addr address*/
#include <netdb.h>

/*clean exit*/
#include <signal.h>

/* port numbers */
#define HTTP_PORT 80
#define PROXY_PORT 8080

/* string sizes */
#define MESSAGE_SIZE 2048

int lstn_sock, data_sock, web_sock;

void cleanExit(int sig){
	close(lstn_sock);
	close(data_sock);
	close(web_sock);
	exit(0);
}

int main(int argc, char* argv[]){

    //Arrays storing the requests the proxy will receive & responses it will send out
	char client_request[MESSAGE_SIZE],
        server_request[MESSAGE_SIZE],
        server_response[1000*MESSAGE_SIZE],
        client_response[1000*MESSAGE_SIZE];

	char url[MESSAGE_SIZE],
        host[MESSAGE_SIZE],
        path[MESSAGE_SIZE];

	int clientBytes, serverBytes, i;


    /* to handle ungraceful exits */
    signal(SIGTERM, cleanExit);
    signal(SIGINT, cleanExit);

    //1A initialize proxy address
	printf("Initializing proxy address...\n");
	struct sockaddr_in proxy_addr;                      //Create a struct object called proxy_addr; fill in its attributes below
	proxy_addr.sin_family = AF_INET;                    //IPv4 protocol
	proxy_addr.sin_port = htons(PROXY_PORT);            //port 8080
	proxy_addr.sin_addr.s_addr = htonl(INADDR_ANY);     //address is any available address on the interface

	//1B create listening socket
	printf("Creating lstn_sock...\n");
	lstn_sock = socket(AF_INET, SOCK_STREAM, 0);        //IPv4, TCP socket type, 0 is TCP protocol
	if (lstn_sock <0){
		perror("socket() call failed");
		exit(-1);
	}

	//2 bind listening socket (lstn_socket) to proxy_addr(port # and IP #)
	printf("Binding lstn_sock...\n");
	if (bind(lstn_sock, (struct sockaddr*)&proxy_addr, sizeof(struct sockaddr_in)) < 0){
		perror("bind() call failed");
		exit(-1);
	}


	//3 listen for client connection requests- put the socket in a listening state
	printf("Listening on lstn_sock...\n");
	if (listen(lstn_sock, 20) < 0){             //20 people can wait for a connection if the server is busy
		perror("listen() call failed...\n");
		exit(-1);
	}

/*proxy socket is ready to listen for clients connection now*/
int n = 0;

	//infinite while loop for listening
	while (1){
		printf("Accepting connections...\n");

		/*
        accept client connection request:

        accept is a blocking call that waits until a connection request is made.

        Once the listening socket receives a connection request from the client,
        a new socket (data_sock) is created that is NOT in the listening state, and is
        for the purpose of sending and receiving data from the client.

        The original lstn_socket remains in the listening state and is unaffected by the accept().

        data_sock has a new socket id is that is dynamically assigned (via accept() ), HOWEVER,
        it still uses the same PORT as lstn_socket. This is fine because a TCP SERVER port can have
        multiple clients connect to the same port.

        When there are multiple connections to the same SERVER port, then that means that either
        the client's IP or client's port are different, or both.
        https://bytes.com/topic/net/answers/155864-determine-dynamically-assigned-port-accepted-server-socket
        
        */
		data_sock = accept(lstn_sock, NULL, NULL);  //create new socket once lstn socket(will it have new port?)
		if (data_sock < 0){
			perror("accept() call failed\n");
			exit(-1);
		}
        else {
            printf("Connected...\n");
        }

		//while loop to receive client requests
		while ((clientBytes = recv(data_sock, client_request, MESSAGE_SIZE, 0)) > 0){
			printf("\n~~Client Request~~\n%s\n", client_request);

			//copy to server_request to preserve contents (client_request will be damaged in strtok())
			strcpy(server_request, client_request);

			//tokenize to get a line at a time
			char *line = strtok(client_request, "\r\n");
			
			//extract url 
			sscanf(line, "GET http://%s", url);
			
			//separate host from path
			for (i = 0; i < strlen(url); i++){
				if (url[i] =='/'){
					strncpy(host, url, i);
					host[i] = '\0';
					break;
				}
			}
			bzero(path, MESSAGE_SIZE);
			strcat(path, &url[i]);

			printf("~~Host: %s, Path: %s~~\n", host, path);

			//initialize server address
			printf("~~Initializing server address...~~\n");
			struct sockaddr_in server_addr;
			struct hostent *server;
			server = gethostbyname(host);

			bzero((char*)&server_addr, sizeof(struct sockaddr_in));
			server_addr.sin_family = AF_INET;
			server_addr.sin_port = htons(HTTP_PORT);
			bcopy((char*)server->h_addr, (char*)&server_addr.sin_addr.s_addr, server->h_length);

			//create web_socket to communicate with web_server
			web_sock = socket(AF_INET, SOCK_STREAM, 0);
			if (web_sock < 0){
				perror("socket() call failed\n");
			}

			//send connection request to web server
			if (connect(web_sock, (struct sockaddr*)&server_addr, sizeof(struct sockaddr_in))<0){
				perror("connect() call failed\n");
			}

			//send http request to web server
			if (send(web_sock, server_request, MESSAGE_SIZE, 0) < 0){
				perror("send(0 call failed\n");
			}

			//receive http response from server
			serverBytes = 0;

			while((serverBytes = recv(web_sock, server_response, 1000*MESSAGE_SIZE, 0)) > 0){
				
				////////////////////////
				// Modify response... //
				////////////////////////
                
                int pos;
                char *ptr = server_response;
                char rep[] = "trollface";
                int repLen = strlen(rep);
                char backup[1000*MESSAGE_SIZE];

                //replace embedded images
                if( (strstr(ptr, "<img src") != NULL) && (strstr(server_response, "trollface") == NULL) )
                {
                        while( ptr != NULL )
                        {
                                //search for image path
                                ptr = strstr(ptr,"<img src");
                                if(ptr != NULL)
                                {
                                    
                                    
                                    //image path first char at 'i'+12
                                    pos = ptr - server_response;
                                    pos = pos+12;
                                    

                                    //find '.' position
                                    ptr = strstr(ptr,".jpg");
                                    int pos2 = ptr - server_response;

                                    //copy everything up to img name start
                                    for (int i = 0; i < pos; i++)
                                    {
                                        backup[i] = server_response[i];
                                    }
                                    //replace image name with new image name
                                    int j = 0;
                                    while(j < repLen)
                                    {
                                        backup[pos] = rep[j];
                                        j++;
                                        pos++;
                                    }
                                    //copy remainder of original server response
                                    while(pos2 < strlen(server_response))
                                    {
                                        backup[pos] = server_response[pos2];
                                        pos++;
                                        pos2++;
                                    }


                                }

                        }
                    strcpy(server_response,backup);
                }


//////
                
                //replace hyperlinked images
                bzero(backup, 1000*MESSAGE_SIZE);
                ptr = server_response;
                if( (strstr(ptr, "<a href") != NULL) && (strstr(server_response, "trollface") == NULL) )
                {
                        while( ptr != NULL )
                        {
                                //search for URL
                                ptr = strstr(ptr,"<a href");
                                if(ptr != NULL)
                                {
                                    
                                    
                                    //URL jpg extension start character 55th past 'a'
                                    pos = ptr - server_response;
                                    pos = pos+54;
                                    

                                    //find '.' position
                                    ptr = strstr(ptr,".jpg");
                                    int pos2 = ptr - server_response;

                                    //copy everything up to jpg extension start
                                    for (int i = 0; i < pos; i++)
                                    {
                                        backup[i] = server_response[i];
                                    }
                                    //replace jpg extension with new extension
                                    int j = 0;
                                    while(j < repLen)
                                    {
                                        backup[pos] = rep[j];
                                        j++;
                                        pos++;
                                    }

                                    while(pos2 < strlen(server_response))
                                    {
                                        backup[pos] = server_response[pos2];
                                        pos++;
                                        pos2++;
                                    }


                                }

                        }
                    strcpy(server_response,backup);
                }

                

                //find and replace all Floppy
                ptr = server_response;
                while( ptr != NULL )
                {
                        ptr = strstr(ptr,"Floppy");
                        if(ptr != NULL)
                        {
                            pos = ptr - server_response;
                            server_response[pos] = 'T';
                            server_response[pos+1] = 'r';
                            server_response[pos+2] = 'o';
                            server_response[pos+3] = 'l';
                            server_response[pos+4] = 'l';
                            server_response[pos+5] = 'y';

                            ptr += strlen("Floppy");
                        }

                }
                //find and replace all Italy
                ptr = server_response;
                while( ptr != NULL )
                {
                        ptr = strstr(ptr,"Italy");
                        if(ptr != NULL)
                        {
                            pos = ptr - server_response;
                            server_response[pos] = 'J';
                            server_response[pos+1] = 'a';
                            server_response[pos+2] = 'p';
                            server_response[pos+3] = 'a';
                            server_response[pos+4] = 'n';

                            ptr += strlen("Italy");
                        }

                }                



                
				//we are not modifying here, just passing the response along
				printf("\n~~Server Response%d~~\n%s\n", n,server_response);
				bcopy(server_response, client_response, serverBytes);
n++;
				//send http response to client
				if (send(data_sock, client_response, serverBytes, 0) < 0){
					perror("send() call failed...\n");
				}
				bzero(client_response, 1000*MESSAGE_SIZE);
				bzero(server_response, 1000*MESSAGE_SIZE);
			}//while recv() from server
		}//while recv() from client
		close(data_sock);
	}//infinite loop
	return 0;
}