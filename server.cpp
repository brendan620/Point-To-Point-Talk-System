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
                Lookup server that is used by clients to find 
                the sockaddr_in information of the person they
                are requestion to communicate with.
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
#include <signal.h>
#include <sys/time.h>

using namespace std;

//Constant to set the buffer size as needed
#define BUFFER_SIZE 200
#define MAX_USERS 10
#define SERV_HOST_ADDR "156.12.127.18"
#define SHARED_MEM_SEM 0
//15080-15089

//Structs
//Contains all of the client information
struct client
{
	char name[20];
	struct sockaddr_in serverAddr;
	struct timeval startTime;
	struct timeval lastLookupTime;

};

//Holds all of the client structs and general
//info about the server
struct dir
{
	client clientInfo[MAX_USERS];
	int numClients;
	int portNums[MAX_USERS];
	bool portAvailable[MAX_USERS];
};

//Holds the packet information that is sent to and from the server
struct msg_packet
{
	int portNum;
	int numClients;
	struct sockaddr_in socket_info;
	char buffer[BUFFER_SIZE];
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
void sendClientList(int cliSockFd,struct dir *directory,int semid);
void exitHandler(int signal);
string getTime(struct timeval tv); 
/**Main method that starts the application
         @return 0 in success
                */
int main(int argc, char* argv[]){

	//Set up the variables
	int sockfd,cliSockFd,pid,shmid,semid;
	void *shmptr;
	socklen_t cliLen;
	struct sockaddr_in server,client;
	char buffer[BUFFER_SIZE];
	
	//Set up shared memory so each of the child processes can access
	//the stucts 
	shmid=shmget(15080,sizeof(dir),IPC_CREAT|0666);
	shmptr=shmat(shmid,0,0);
	struct dir *directory=(struct dir*) shmptr;


	//Create semid
 	semid = semget(15080, 1, 0666|IPC_CREAT);
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


	//Set up signal handler
	signal(SIGINT,exitHandler);


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

		//Child
		if((pid=fork())==0)
		{

			if(addCliToDir(cliSockFd,buffer,directory,semid))
			{
				//CLIENT IS OKAY
				//Get the next free port
				int port=getFreePort(semid,SHARED_MEM_SEM,directory);

				//Create a msg_packet and assign the port to it
				//before sending it back to the client
				msg_packet packet;
				packet.portNum = port;
				packet.numClients=directory->numClients;
				void *addrPtr=(void *) &packet;

				if((send(cliSockFd,addrPtr,sizeof(msg_packet),0))<0){
					perror("WRITE TO CLIENT FAILED");
					exit(0);
				}
				else
				{

					//Getting struct from client
					//Read the clients sockaddr_in struct
					int input;
					void *ptr=malloc(sizeof(struct sockaddr_in));
					if((input = recv(cliSockFd,&ptr,sizeof(struct sockaddr_in),0))<0)
					{
						perror("READ CALL FAILED");
					}

					struct sockaddr_in *client_info=(struct sockaddr_in*) &ptr;
					//Add the clients network information
					addCliNet(cliSockFd,client_info,directory,semid);
					//Wait for more input from the client
					waitForClientInput(cliSockFd,directory,semid);

				}

			}
			else
			{
				//If the client cannot be added because the
				//server is full or the name was already taken
				//send them a msg_packet with a port of 0
				msg_packet packet;
				packet.portNum = 0;
				packet.numClients=directory->numClients;
				void *addrPtr=(void *) &packet;

				if((send(cliSockFd,addrPtr,sizeof(msg_packet),0))<0){
					perror("WRITE TO CLIENT FAILED");
					exit(0);
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

			//Increment the number of clients
			directory->numClients=directory->numClients+1;

			//Give access to the semaphore
			giveAccess(semid,SHARED_MEM_SEM);

			int status;
			//Call waitpid and pass the status
			/*pid_t done = waitpid(-1, &status, WUNTRACED); 
			if(status==0)
			{
				getAccess(semid,SHARED_MEM_SEM);
				directory->numClients=directory->numClients-1;
				if(directory->numClients==0)
				{
					exitHandler(shmid);
				}
				giveAccess(semid,SHARED_MEM_SEM);
			}*/


		}



	}

	

}


/**Adds the client to the dir struct
 	@param cliSockFd The clients sockfd number
 	@param buffer[] The character buffer from the client
 	@param *directory The dir struct stored in shared memory
 	@param semid The semiphore id
	@return True if the client was added successfully false otherwise
*/
bool addCliToDir(int cliSockFd,char buffer[],struct dir *directory,int semid)
{
	//Get the packet from the client
	int input;
	void *ptr = malloc(sizeof(msg_packet));

	if((input = recv(cliSockFd,ptr,sizeof(msg_packet),0))<0)
		{
			perror("READ CALL FAILED");
		}
	else if(input>0)
		{
			//Copy the buffer from the packet
			msg_packet *packet=(struct msg_packet*) ptr;
			buffer=packet->buffer;
		}

		//Get access to the semaphore
		getAccess(semid,SHARED_MEM_SEM);

		//Check if the name hasnt been taken and that there are not 10 clients
		if(directory->numClients<10 && nameNotTaken(buffer,directory))
		{
			//Copy the buffer into the name attribute of the clientInfo struct
			strcpy(directory->clientInfo[directory->numClients-1].name,buffer);
			gettimeofday (&directory->clientInfo[directory->numClients-1].startTime,NULL);
			gettimeofday (&directory->clientInfo[directory->numClients-1].lastLookupTime,NULL);
			//Give access to the semaphore
			giveAccess(semid,SHARED_MEM_SEM);
			return true;
		}
		else
		{
			//If the client cannot be added remove a client from shared memory
			directory->numClients=directory->numClients-1;
		}


	//Give access to the semaphore
	giveAccess(semid,SHARED_MEM_SEM);
	return false;
}


/**Add the clients sockaddr_in to shared memory
 	@param cliSockFd The client socket file descriptor
 	@param *client_info The clients sockaaddr_in struct
 	@param *directory the dir struct in shared memory
 	@param semid The semaphore id
*/
void addCliNet(int cliSockFd,struct sockaddr_in *client_info,struct dir *directory,int semid)
{
	getAccess(semid,SHARED_MEM_SEM);
	//Add the clients sockaddr_in to the shared memory so other clients can contact it
	directory->clientInfo[directory->numClients-1].serverAddr.sin_family = client_info->sin_family;
	directory->clientInfo[directory->numClients-1].serverAddr.sin_addr.s_addr = client_info->sin_addr.s_addr;
	directory->clientInfo[directory->numClients-1].serverAddr.sin_port = client_info->sin_port;
	giveAccess(semid,SHARED_MEM_SEM);
}

/**Checks if the name has already been registered
 	@param buffer[] the clients name
 	@param *directory the dir struct in shared memory
 	@return True if the name is available false otherwise
*/
bool nameNotTaken(char buffer[],struct dir *directory)
{


	for(int i=0;i<directory->numClients-1;i++)
	{
		//Compare the buffer to each clients name
		if(strcasecmp(buffer,directory->clientInfo[i].name)==0)
		{
			//If its found return false
			return false;
		}

	}
	//If its available return true
	return true;
}


/**Get the next available port
	@param semNum
 	@param *directory the dir struct in shared memory
 	@param semid The semaphore id
 	@return The port number
*/
int getFreePort(int semid,int semNum,struct dir *directory)
{
	getAccess(semid,SHARED_MEM_SEM);
	//Loop 10 times
	for(int i=0;i<MAX_USERS;i++)
	{
		//Check if the port is available
		if(directory->portAvailable[i])
		{	
			//Set it to false
			directory->portAvailable[i]=false;
			giveAccess(semid,semNum);
			//Return the port number
			return directory->portNums[i];
		}
	}
	giveAccess(semid,SHARED_MEM_SEM);

}

//If sem_op is negative then a value is subtracted from the semaphore
//If semaphore is 0 then it is locked
/**Gets access from the semaphore
	@param semNum the semaphore number
 	@param semid The semaphore id
*/
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
/**Gives access back to the semaphore
	@param semNum the semaphore number
 	@param semid The semaphore id
*/
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

/**Converts an int to a string
	@param num The number to convert
 	@return The number as a string
*/
string intToString(int num)
{
		 ostringstream stream;
     stream << num;
     return stream.str();
}

/**Converts a char array to a string
	@param buffer[] The character array
 	@return The array as a string
*/
string charBuffertoString(char buffer[])
{
	string message=buffer;
	return message;
}


/**Does all of the client interaction after the client is set up
	@param cliSockfd The clients file descriptor
	@param *directory The dir struct in shared memory
 	@param semid The semaphore id
*/
void waitForClientInput(int cliSockFd,struct dir *directory,int semid)
{
	char buffer[BUFFER_SIZE];
	//Loop for input
	while(true)
	{

		int input;
		//Receive the command from the client
		if((input = recv(cliSockFd,buffer,BUFFER_SIZE,0))<0)
			{
				perror("READ CALL FAILED");
			}
		else if(input>0)
			{

				buffer[input]='\0';
			}

			//If the client sends the all command then get the client list
			if(strcasecmp(buffer,"all")==0)
			{
				sendClientList(cliSockFd,directory,semid);
			}
			else if(strcasecmp(buffer,"exit")==0)
			{
				exit(0);
			}
			else
			{
				//If the client sends a word that isnt a command then it is a 
				//users name
					
					struct sockaddr_in lookupResult;
					getAccess(semid,SHARED_MEM_SEM);
					for(int i=0;i<directory->numClients;i++)
					{
						if(charBuffertoString(buffer)==directory->clientInfo[i].name)
						{
							gettimeofday (&directory->clientInfo[i].lastLookupTime,NULL);
						}
					}
					giveAccess(semid,SHARED_MEM_SEM);
					//Check if the client exists 
					if(checkForClientInDir(directory,semid,lookupResult,charBuffertoString(buffer)))
					{

						//Write the users sockaddr_in struct back to 
						//the requesting client via a msg_packet
						msg_packet packet;
						packet.socket_info=lookupResult;
						void *addrPtr=(void *)&packet;
						if((send(cliSockFd,addrPtr,sizeof(msg_packet),0))<0){
								perror("WRITE TO SERVER FAILED");
								exit(0);
							}
						else{

						}
					}
					else
					{
						//If the client does not exist
						//Write a sockaddr_in struct to the client with
						//a port of 0
						lookupResult.sin_port=0;
						msg_packet packet;
						packet.socket_info=lookupResult;
						void *addrPtr=(void *)&packet;
						if((send(cliSockFd,addrPtr,sizeof(msg_packet),0))<0){
								perror("WRITE TO SERVER FAILED");
								exit(0);
						}
						else{

							
						}

						}
				}
	}
}

/**Checks if a client with that name exists in the directory
	@param *directory The dir struct stored in shared memory
 	@param semid The semaphore id
 	@param &lookupResult The sockaddr_in of the client being looked for
 	@param name The name being searched for
 	@return True if found false otherwise
*/
bool checkForClientInDir(struct dir *directory,int semid,struct sockaddr_in &lookupResult,string name)
{
	
	getAccess(semid,SHARED_MEM_SEM);
	
	//Loop for the number of connected clients
	for(int i=0;i<directory->numClients;i++)
	{
		//Check if the names match
		if(strcasecmp(name.c_str(),directory->clientInfo[i].name)==0)
		{
			//Copt the clients sockaddr_in struct to the lookupResult
			lookupResult=directory->clientInfo[i].serverAddr;
			giveAccess(semid,SHARED_MEM_SEM);
			//Return true
			return true;
		}

	}

	giveAccess(semid,SHARED_MEM_SEM);
	return false;
}


/**Sends the list of clients
	@param cliSockFd The clients file descriptor
	@param *directory The dir struct in shared memory
 	@param semid The semaphore id
*/
void sendClientList(int cliSockFd,struct dir *directory,int semid)
{
	getAccess(semid,SHARED_MEM_SEM);
	string names;
	//Create a string of all the names in the directory
	for(int i=0;i<directory->numClients;i++)
	{
		names+=("Client Name:"+charBuffertoString(directory->clientInfo[i].name)+"\n");
		names+=("Start Time:" + getTime(directory->clientInfo[i].startTime)+"\n");
		names+=("Last Lookup Time" + getTime(directory->clientInfo[i].lastLookupTime)+"\n");
	}
	giveAccess(semid,SHARED_MEM_SEM);

	//Add the names to the msg_packet and send it over
	msg_packet packet;
	strcpy(packet.buffer,names.c_str());
	void *addrPtr=(void *)&packet;
	if((send(cliSockFd,addrPtr,sizeof(msg_packet),0))<0){
			perror("WRITE TO SERVER FAILED");
			exit(0);
	}
}


/**Handles the exiting of the program
	@param signal The signal received 
*/
void exitHandler(int signal){
	string response;
	cout << "Do you wish to close the server(Y/N)?" << endl;
	cin >> response;
	if(strcasecmp(response.c_str(),"y")==0)
	{
		if(signal!=0)
		{
			if ((shmctl(signal,IPC_RMID,0))==-1){
				perror("Error removing shared memory segment");
    		}
    	}
		exit(0);
	}
}


/**Converts the timeval struct into a readable string
	This function was modified using the example that Andrew posted on the 
	forum. 
	@param tv The timeval struct to be converted
	@return The converted string
*/
string getTime(struct timeval tv) 
{
    time_t nowtime = tv.tv_sec;
    struct tm *nowtm;
    char tmbuf[64];

    nowtm = localtime(&nowtime);
    strftime(tmbuf, sizeof(tmbuf), "%Y-%m-%d %H:%M:%S", nowtm);
    string result = tmbuf;
    return result;
}