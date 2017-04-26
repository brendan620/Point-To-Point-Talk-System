/**********************************************************/
/*  Edited By:   Brendan Lilly                            */
/*  Course:      CSC552                                   */
/*  Professor:   Spiegel                                  */
/*  Assignment:  3                                        */
/*  Due Date:    5/11/2017                                */
/*  Start Date:  4/25/2017                                */
/*  Filename:    server.cpp			                      */
/**********************************************************/

/**
                LOOKUP_SERVER
                @file
    @author Brendan Lilly
    @date March 2017
*/

#include <iostream>
#include <sys/wait.h>
#include <stdlib.h>
#include <stdio.h>
#include <sstream>
#include <string.h>
#include <sys/types.h>
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fcntl.h>
using namespace std;

//Constant to set the buffer size as needed
#define BUFFER_SIZE 500
#define MAX_USERS 10
#define SERV_HOST_ADDR "156.12.127.18"
//15080-15089

//Structs
struct client
{
	char name[20];
	struct sockaddr_in serverAddr;
	struct timeval startTime;
	struct timeval lastLookupTime;
	
};

struct dir
{
	client clientInfo[MAX_USERS];
	int numClients;
};
//Prototypes


/**Main method that starts the application
         @return 0 in success
                */
int main(int argc, char* argv[]){

	int sockfd,cliSockFd,pid,shmid;
	void *shmptr;
	socklen_t cliLen;
	struct sockaddr_in server,client;
	char buffer[BUFFER_SIZE];
	//struct dir directory;

	shmid=shmget(getuid(),1000,IPC_CREAT|0600);
	shmptr=shmat(shmid,0,0);
	struct dir *directory=(struct dir*) shmptr;



	directory->numClients=0;

	if((sockfd=socket(AF_INET,SOCK_STREAM,0))<0){
		perror("SERVER CANNOT OPEN SOCKET");
		exit(0);
	}

	cout << "SOCKET CALL WORKED" << endl;

	//bzero(void *s, size_t n)
	//erases the data in the n bytes of memory starting
	//at the location pointed to by s, by writing zeros
	//to that area
	bzero((char *) &server,sizeof(server));

	server.sin_family = AF_INET;
	//htonl == Converts the unsigned integer hostlong from host
	//byte order to network byte order
	server.sin_addr.s_addr = htonl(INADDR_ANY);
	//htons == Converts the unsigned short integer hostshort
	//from host byte order to network byte order
	server.sin_port = htons(15080);

	if((bind(sockfd,(struct sockaddr *) &server,sizeof(server)))<0)
	{
		perror("BIND CALL FAILED");
		exit(0);
	}
	cout << "BIND CALL WORKED" << endl;

	listen(sockfd,MAX_USERS);
	cliLen = sizeof(client);

	while(true){

		if((cliSockFd=accept(sockfd, (struct sockaddr *) &client, &cliLen))<0)
		{
			perror("ACCEPT ERROR IN SERVER");
		}
		else
		{
			cout << "Accepted connection from: " << cliSockFd << endl;
		}

		//Child 
		if((pid=fork())==0)
		{
			cout << "Entering the child" << endl;
			//close(sockfd);

			int input;
			if((input = recv(cliSockFd,buffer,BUFFER_SIZE,MSG_WAITALL))<0)
				{
					perror("READ CALL FAILED");
				}
			else if(input>0)
				{
					buffer[input]='\0';
					
				}
			//if(directory->numClients==0)
			//{

				strcpy(directory->clientInfo[directory->numClients].name,buffer);
				directory->numClients=directory->numClients+1;	
				cout << "CLIENT " << directory->clientInfo[directory->numClients-1].name << " Number" << directory->numClients << endl;

			//}	
			
		}
		else if(pid<0)
		{
			perror("FORK CALLED FAILED");
		}
		close(cliSockFd);

	}

}

