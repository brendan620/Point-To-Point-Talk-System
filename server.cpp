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
#include <string>
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
#include <arpa/inet.h>

using namespace std;

//Constant to set the buffer size as needed
#define BUFFER_SIZE 500
#define MAX_USERS 10
#define SERV_HOST_ADDR "156.12.127.18"
#define SHARED_MEM_SEM 0
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
	int portNums[MAX_USERS];
	bool portAvailable[MAX_USERS];
};

struct packet
{
	int portNum;
	int numClients;
};

//Prototypes
bool addCliToDir(int cliSockFd,char buffer[],struct dir *directory,int semid);
bool nameNotTaken(char buffer[],struct dir *directory);
void getAccess(int semid,int semNum);
void giveAccess(int semid,int semNum);
void addCliNet(int cliSockFd,struct sockaddr_in *client_info,struct dir *directory,int semid);
int getFreePort(int semid,int semNum,struct dir *directory);
string intToString(int num);
void waitForClientInput(int cliSockFd,struct dir *directory,int semid);
string charBuffertoString(char buffer[]);
bool checkForClientInDir(struct dir *directory,int semid,struct sockaddr_in &lookupResult,string name);
/**Main method that starts the application
         @return 0 in success
                */
int main(int argc, char* argv[]){

	int sockfd,cliSockFd,pid,shmid,semid;
	void *shmptr;
	socklen_t cliLen;
	struct sockaddr_in server,client;
	struct packet local;
	char buffer[BUFFER_SIZE];
	//struct dir directory;

	shmid=shmget(getuid(),1000,IPC_CREAT|0600);
	shmptr=shmat(shmid,0,0);
	struct dir *directory=(struct dir*) shmptr;


	//Create semid
 	semid = semget(getuid(), 1, 0600|IPC_CREAT);
	//Set the semaphore to open
	semctl(semid,SHARED_MEM_SEM,SETVAL,1);

	//Initialize some values in dir
	directory->numClients=0;
	int index=0;
	for(int i=15081;i<15090;i++)
	{

		directory->portNums[index]=i;
		directory->portAvailable[index]=true;
		index++;
	}

	if((sockfd=socket(AF_INET,SOCK_STREAM,0))<0){
		perror("SERVER CANNOT OPEN SOCKET");
		exit(0);
	}


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
			cout << "Accepted connection from: " << inet_ntoa(client.sin_addr) << endl;
		}

		//Child
		if((pid=fork())==0)
		{

			if(addCliToDir(cliSockFd,buffer,directory,semid))
			{
				//CLIENT IS OKAY
				cout << "Client added to directory asking for client sockaddr" << endl;

				int port=getFreePort(semid,SHARED_MEM_SEM,directory);
				local.portNum = port;
				local.numClients=directory->numClients;
				void *addrPtr=(void *) &local;
				cout << "PORT" << port << endl;
				if((send(cliSockFd,addrPtr,sizeof(local),0))<0){
					perror("WRITE TO CLIENT FAILED");
					exit(0);
				}
				else
				{

					//Getting struct from client
					int input;
					bzero(buffer,BUFFER_SIZE);
					void *ptr;
					if((input = recv(cliSockFd,&ptr,BUFFER_SIZE,0))<0)
					{
						perror("READ CALL FAILED");
					}

					struct sockaddr_in *client_info=(struct sockaddr_in*) &ptr;
					cout << client_info->sin_port << endl;
					addCliNet(cliSockFd,client_info,directory,semid);
					waitForClientInput(cliSockFd,directory,semid);

				}

			}
			else
			{
				local.portNum = 0;
				local.numClients=directory->numClients;
				void *addrPtr=(void *) &local;

				cout << "Client not added to directory" << endl;

				if((send(cliSockFd,addrPtr,sizeof(local),0))<0){
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
			//Get access to the semaphore

			getAccess(semid,SHARED_MEM_SEM);
			cout << "Parent got in semaphore : num clients incremented in parent" << endl;
			directory->numClients=directory->numClients+1;

			//Give access to the semaphore
			giveAccess(semid,SHARED_MEM_SEM);


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

		//Get access to the semaphore
		getAccess(semid,SHARED_MEM_SEM);

		cout << "Child got into semaphore" << endl;
		if(directory->numClients<10 && nameNotTaken(buffer,directory))
		{
			strcpy(directory->clientInfo[directory->numClients-1].name,buffer);
			//directory->numClients=directory->numClients+1;
			cout << "CLIENT " << directory->clientInfo[directory->numClients-1].name << " Number " << directory->numClients << endl;
			//Give access to the semaphore
			giveAccess(semid,SHARED_MEM_SEM);
			return true;
		}
		else
		{
			directory->numClients=directory->numClients-1;
		}


	//Give access to the semaphore
	giveAccess(semid,SHARED_MEM_SEM);
	return false;
}

void addCliNet(int cliSockFd,struct sockaddr_in *client_info,struct dir *directory,int semid)
{
	getAccess(semid,SHARED_MEM_SEM);
	cout << "Adding net info for client " << directory->clientInfo[directory->numClients-1].name << endl;
	cout << client_info->sin_port << endl;
	directory->clientInfo[directory->numClients-1].serverAddr.sin_family = client_info->sin_family;
	directory->clientInfo[directory->numClients-1].serverAddr.sin_addr.s_addr = client_info->sin_addr.s_addr;
	directory->clientInfo[directory->numClients-1].serverAddr.sin_port = client_info->sin_port;
	giveAccess(semid,SHARED_MEM_SEM);
}

bool nameNotTaken(char buffer[],struct dir *directory)
{
	cout << "number of clients when checking for names:" << directory->numClients <<endl;
	for(int i=0;i<directory->numClients-1;i++)
	{

		if(strcasecmp(buffer,directory->clientInfo[i].name)==0)
		{
			cout << "NAME ALREADY TAKEN" << endl;
			return false;
		}

	}
	cout << "NAME AVAILABLE" << endl;
	return true;
}


int getFreePort(int semid,int semNum,struct dir *directory)
{
	cout << "Entered getFreePort" << endl;
	getAccess(semid,SHARED_MEM_SEM);
	cout << "getFreePort: Got access" << endl;
	for(int i=0;i<MAX_USERS;i++)
	{
		cout << "getFreePort: Loop" << i << endl;
		if(directory->portAvailable[i])
		{
			directory->portAvailable[i]=false;
			giveAccess(semid,semNum);
			cout << "Leaving getFreePort" << endl;
			return directory->portNums[i];
		}
	}
	giveAccess(semid,SHARED_MEM_SEM);

}

//If sem_op is negative then a value is subtracted from the semaphore
//If semaphore is 0 then it is locked
void getAccess(int semid,int semNum){
	struct sembuf flag;
	flag.sem_num = semNum;
	flag.sem_op = -1;
	flag.sem_flg = 0;

	if(semop(semid, &flag,1) == -1)
	{
		perror("getAccess to semaphore failed");
	}

}

//If sem_op is positive then a value is added to the semaphore
//If semaphore is 1 then it is open
void giveAccess(int semid,int semNum){
	struct sembuf flag;
	flag.sem_num = semNum;
	flag.sem_op = 1;
	flag.sem_flg = 0;

	if(semop(semid, &flag,1) == -1)
	{
		perror("giveAccess to semaphore failed");
	}
}

string intToString(int num)
{
		 ostringstream stream;
     stream << num;
     return stream.str();
}

string charBuffertoString(char buffer[])
{
	string message=buffer;
	return message;
}


void waitForClientInput(int cliSockFd,struct dir *directory,int semid)
{
	char buffer[BUFFER_SIZE];
	while(true)
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

			cout << charBuffertoString(buffer) << endl;
			struct sockaddr_in lookupResult;

			if(checkForClientInDir(directory,semid,lookupResult,charBuffertoString(buffer)))
			{
				cout << "CLI FOUND" << endl;
				cout << "port" << lookupResult.sin_port << endl;

				void *addrPtr=(void *)&lookupResult;
				//struct sockaddr_in *client_info=(struct sockaddr_in*) &addrPtr;
				//cout << "port after assignment" << client_info->sin_port << endl;
				if((send(cliSockFd,addrPtr,sizeof(lookupResult),0))<0){
						perror("WRITE TO SERVER FAILED");
						exit(0);
					}
				else{

				}
			}
			else
			{
				cout << "Cli Not Found" << endl;
				lookupResult.sin_port=0;
				void *addrPtr=(void *)&lookupResult;
				if((send(cliSockFd,addrPtr,sizeof(lookupResult),0))<0){
						perror("WRITE TO SERVER FAILED");
						exit(0);
					}
				else{

				}
			}
	}
}

bool checkForClientInDir(struct dir *directory,int semid,struct sockaddr_in &lookupResult,string name)
{
	cout << "checkForClientInDir" << endl;
	getAccess(semid,SHARED_MEM_SEM);
	cout << "checkForClientInDir GOT ACCESS" << endl;
	cout << "Num clients:" << directory->numClients << endl;
	for(int i=0;i<directory->numClients;i++)
	{
		cout << "names["<<i<< "]:" << directory->clientInfo[i].name << endl;
		if(strcasecmp(name.c_str(),directory->clientInfo[i].name)==0)
		{
			lookupResult=directory->clientInfo[i].serverAddr;
			giveAccess(semid,SHARED_MEM_SEM);
			return true;
		}

	}

	giveAccess(semid,SHARED_MEM_SEM);
	return false;
}
