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
#include <sys/sem.h>
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
bool addCliToDir(int cliSockFd,char buffer[],struct dir *directory,int semid);
bool nameNotTaken(char buffer[],struct dir *directory);
/**Main method that starts the application
         @return 0 in success
                */
int main(int argc, char* argv[]){

	int sockfd,cliSockFd,pid,shmid,semid;
	void *shmptr;
	socklen_t cliLen;
	struct sockaddr_in server,client;
	char buffer[BUFFER_SIZE];
	//struct dir directory;

	shmid=shmget(getuid(),1000,IPC_CREAT|0600);
	shmptr=shmat(shmid,0,0);
	struct dir *directory=(struct dir*) shmptr;


	//Create semid
 	semid = semget(getuid(), 1, 0600|IPC_CREAT);
	//Set the semaphore to open
	semctl(semid,0,SETVAL,1);

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
			cout << "Accepted connection from: " << client.sin_port << endl;
		}

		//Child
		if((pid=fork())==0)
		{
			cout << "Entering the child" << endl;

			if(addCliToDir(cliSockFd,buffer,directory,semid))
			{
				//CLIENT IS OKAY
				cout << "Client added to directory asking for client sockaddr" << endl;
				bzero(buffer,BUFFER_SIZE);
				strcpy(buffer,"OK");

				if((send(cliSockFd,buffer,strlen(buffer),0))<0){
					perror("WRITE TO CLIENT FAILED");
					exit(0);
				}
				else
				{
					cout << "Wrote client successfully registered message to client" << endl;

					//Getting struct from client
					int input;
					bzero(buffer,BUFFER_SIZE);
					void *ptr;
					if((input = recv(cliSockFd,&ptr,BUFFER_SIZE,0))<0)
						{
							perror("READ CALL FAILED");
						}
					else if(input>0)
						{
							buffer[input]='\0';
							cout << "Size of struct buffer " << sizeof(buffer) << endl;

						}

						//memcpy(buffer,(void*) &client,sizeof(&client));

						struct sockaddr_in *test=(struct sockaddr_in*) &ptr;
						//memcpy(&test,buffer, sizeof(sockaddr_in));
						cout << "SIZE OF CLIENT SOCKADD_IN" <<  sizeof(test) << endl;
						cout << test->sin_family << endl;
						cout << test->sin_addr.s_addr << endl;
						cout << test->sin_port << endl;
				}

			}
			else
			{
				cout << "Client not added to directory" << endl;
				bzero(buffer,BUFFER_SIZE);
				strcpy(buffer,"No");

				if((send(cliSockFd,buffer,strlen(buffer),0))<0){
					perror("WRITE TO CLIENT FAILED");
					exit(0);
				}
				else
				{
					cout << "WRITE TO CLIENT WORKED" << endl;
				}

			}


		}
		else if(pid<0)
		{
			perror("FORK CALLED FAILED");
		}
		else
		{

			close(cliSockFd);
			//If the semaphore is open
			if(semctl(semid,0,GETVAL))
			{
				//Set the semaphore to closed
				semctl(semid,0,SETVAL,0);
				cout << "Parent got in semaphore : num clients incremented in parent" << endl;
				directory->numClients=directory->numClients+1;


				//Set the semaphore to open
				semctl(semid,0,SETVAL,1);
			}

		}



	}

}

bool addCliToDir(int cliSockFd,char buffer[],struct dir *directory,int semid)
{
	int input;
	if((input = recv(cliSockFd,buffer,BUFFER_SIZE,0))<0)
		{
			perror("READ CALL FAILED");
		}
	else if(input>0)
		{
			buffer[input]='\0';

		}
		//If the semaphore is open
		if(semctl(semid,0,GETVAL))
		{
			cout << "Child got into semaphore" << endl;
			cout << "NUM CLIENTS:" << directory->numClients << endl;
			//Set the semaphore to closed
			semctl(semid,0,SETVAL,0);
			if(directory->numClients<10 && nameNotTaken(buffer,directory))
			{
				strcpy(directory->clientInfo[directory->numClients-1].name,buffer);
				//directory->numClients=directory->numClients+1;
				cout << "CLIENT " << directory->clientInfo[directory->numClients-1].name << " Number " << directory->numClients << endl;
				//Set the semaphore to open
				semctl(semid,0,SETVAL,1);
				return true;
			}
			else
			{
				directory->numClients=directory->numClients-1;
			}

		}
	//Set the semaphore to open
	semctl(semid,0,SETVAL,1);
	return false;
}

bool nameNotTaken(char buffer[],struct dir *directory)
{
	cout << "number of clients when checking for names:" << directory->numClients <<endl;
	for(int i=0;i<directory->numClients-1;i++)
	{
		cout << "Buffer:" << buffer << " like " << directory->clientInfo[i].name << endl;
		if(strcasecmp(buffer,directory->clientInfo[i].name)==0)
		{
			cout << "NAME ALREADY TAKEN" << endl;
			return false;
		}

	}
	cout << "NAME AVAILABLE" << endl;
	return true;
}
