/**********************************************************/
/*  Edited By:   Brendan Lilly                            */
/*  Course:      CSC552                                   */
/*  Professor:   Spiegel                                  */
/*  Assignment:  3                                        */
/*  Due Date:    5/11/2017                                */
/*  Start Date:  4/25/2017                                */
/*  Filename:    client.cpp			                      */
/**********************************************************/

/**
                Client file that communicates to the server via
                TCP and uses it to look up information on all other connected
                clients. Then uses that information to communicate with them
                over UDP.
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
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <sys/shm.h>
#include <sys/sem.h>

using namespace std;

//Constant to set the buffer size as needed
#define BUFFER_SIZE 200
#define MAX_USERS 10
#define SERV_HOST_ADDR "156.12.127.18"
#define SHARED_MEM_SEM 0
//Structs
struct local_info
{
	char name[20];
	struct timeval startTime;
	struct timeval lastMsgTime;
	int numMsg;
	pid_t pid;
};

struct local_dir
{
	local_info localInfo[MAX_USERS];
	int numClients;
	int totalMsgs;

};

struct msg_packet
{
	int portNum;
	int numClients;
	struct sockaddr_in socket_info;
	char buffer[BUFFER_SIZE];
};

//Prototypes
int openPort(struct sockaddr_in client);
void initiateLocalInfo(struct local_dir *localDirectory);
void readWrite(int clientSockfdUDP, int serverSockfdTCP,string name,struct local_dir *localDirectory,int semid,int shmid);
void writeMsgToClient(struct sockaddr_in client_info,int clientSockfdUDP,string author,string message,struct local_dir *localDirectory,int semid);
void writeMsgToServer(int serverSockfdTCP,int clientSockfdUDP,string outboundMsg,string ,struct local_dir *localDirectory,int semid);
void getAllClients(int serverSockfdTCP);
void addClientToSharedMemory(struct local_dir *localDirectory,int semid,string name);
void getAccess(int semid,int semNum);
void giveAccess(int semid,int semNum);
void listLocalClients(struct local_dir *localDirectory,int semid);
void updateLocalInfo(struct local_dir *localDirectory,int semid);
string getTime(struct timeval tv);
void cleanUpClient(struct local_dir *localDirectory,int semid,int shmid,int serverSockfdTCP);

/**Main method that starts the application
         @return 0 in success
                */
int main(int argc, char* argv[]){
	int serverSockfdTCP;
	struct sockaddr_in server,client;
	string name;
	char buffer[BUFFER_SIZE];
	int shmid;
	void *shmptr;
	struct local_dir *localDirectory;
	
	//Disable SIGINT
	signal(SIGINT, SIG_IGN);

	//Set up shared memory

	//If shared doesnt exist 
	if(shmget(15081,sizeof(local_dir),0)==-1)
	{	
		//Create it
		shmid=shmget(15081,sizeof(local_dir),IPC_CREAT|0666);
		shmptr=shmat(shmid,0,0);
		localDirectory=(struct local_dir*) shmptr;
		initiateLocalInfo(localDirectory);
	}

	//Attach to the shared memory
	shmid=shmget(15081,sizeof(local_dir),0666);
	shmptr=shmat(shmid,0,0);
	localDirectory=(struct local_dir*) shmptr;

	if(argc != 2)
	{
		cout<<"Usage " << argv[0] << " user_name" << endl;
		exit(0);
	}
	else
	{
		name = argv[1];
	}

	if(name.length()>20){
		perror("Name is longer than 10 chars");
		exit(0);
	}

	if((serverSockfdTCP=socket(AF_INET,SOCK_STREAM,0))<0){
		perror("SERVER CANNOT OPEN SOCKET");
		exit(0);
	}



	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
	//htons == Converts the unsigned short integer hostshort
	//from host byte order to network byte order
	server.sin_port = htons(15080);



	if((connect(serverSockfdTCP,(struct sockaddr *) &server,sizeof(server)))<0){
		perror("CONNECT CALL TO THE SERVER FAILED");
		exit(0);
	}
	

	//Create msg_packet with the name in it
	strcpy(buffer,name.c_str());
	
	msg_packet packet;
	strcpy(packet.buffer,name.c_str());
	void *addrPtr=(void *)&packet;

	//Send the name to the server
	if((send(serverSockfdTCP,addrPtr,sizeof(msg_packet),0))<0){
		perror("WRITE TO SERVER FAILED");
		exit(0);
	}
	else
	{
		int input;

		void *ptr = malloc(sizeof(msg_packet));

		//Read the response from the server
		if((input = recv(serverSockfdTCP,ptr,sizeof(msg_packet),0))<0)
			{
				perror("READ CALL FAILED");
			}


			msg_packet *packet=(struct msg_packet*) ptr;

			//If the port number is 0 then 
			//either the name is taken or the server is full
			if(packet->portNum==0)
			{
				cout << "Either your name is taken or the maximum number of users are on the server right now" << endl;
				exit(0);
			}
			else
			{
				//SETUP CLIENTS SOCKADDR IN
				client.sin_family = AF_INET;
				//htonl == Converts the unsigned integer hostlong from host
				//byte order to network byte order
				client.sin_addr.s_addr = htonl(INADDR_ANY);
				//htons == Converts the unsigned short integer hostshort
				//from host byte order to network byte order
				client.sin_port = (packet->portNum);

				//Send the clients sockaddr_in to the server
				void *addrPtr=(void *)&client;
				if((send(serverSockfdTCP,addrPtr,sizeof(client),0))<0){
						perror("WRITE TO SERVER FAILED");
						exit(0);
					}
				else{

						//TODO add your info to shared memory
						//Create semid
						int semid;
					 	semid = semget(getuid(), 1, 0666|IPC_CREAT);
						//Set the semaphore to open
						semctl(semid,SHARED_MEM_SEM,SETVAL,1);

						addClientToSharedMemory(localDirectory,semid,name);
						//Open the port for reading from other clients
						int clientSockfdUDP=openPort(client);

						//Start the read write loop to the server
						readWrite(clientSockfdUDP,serverSockfdTCP,name,localDirectory,semid,shmid);
				}
			}
	}
}


/**Adds user to local shared memory
	@param *localDirectory The local_dir struct in shared memory
*/
void addClientToSharedMemory(struct local_dir *localDirectory,int semid,string name)
{
	getAccess(semid,SHARED_MEM_SEM);
	//cout << "num clients " << localDirectory->numClients << endl;
	strcpy(localDirectory->localInfo[localDirectory->numClients].name,name.c_str());
	localDirectory->localInfo[localDirectory->numClients].numMsg=0;
	localDirectory->localInfo[localDirectory->numClients].pid=getpid();
	gettimeofday (&localDirectory->localInfo[localDirectory->numClients].startTime,NULL);
	//gettimeofday (&localDirectory->localInfo[localDirectory->numClients].lastMsgTime,NULL);
	localDirectory->numClients=localDirectory->numClients+1;
	giveAccess(semid,SHARED_MEM_SEM);
}

/**Sets up shared memory
	@param *localDirectory The local_dir struct in shared memory
*/
void initiateLocalInfo(struct local_dir *localDirectory)
{
	
	localDirectory->totalMsgs=0;
	localDirectory->numClients=0;

	
}


/**Opens a udp port for client to client communication
	@param client the clients sockaddr_in struct
	@return the now opened socket file descriptor
*/
int openPort(struct sockaddr_in client)
{
	int clientSockfdUDP;

	//Create a udp socket
  if((clientSockfdUDP = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("server: cannot open stream socket");
		exit(1);
	}

	//Bind to that socket 
	if(bind(clientSockfdUDP, (struct sockaddr *) &client, sizeof(client))< 0)
    {
			perror("server: cannot bind local address");
			exit(2);
		}

	//Return the socketFd	
	return clientSockfdUDP;

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


/**Read write loop in the client
	@param clientSockfdUDP The udp sock fd
	@param serverSockfdTCP the tcp sockfd
	@param name The users name
	@param *localDirectory The local_dir struct in shared memory
	@param semid The semaphore id
	@param shmid The shared memory segment id
*/
void readWrite(int clientSockfdUDP, int serverSockfdTCP,string name,struct local_dir *localDirectory,int semid,int shmid)
{
	cout << ">";
	//Set up the read and write sets 
	fd_set readSet,writeSet;
	string outboundMsg;
	int selectStatus;

	//Zero the readset
	FD_ZERO(&readSet);
	//FD_ZERO(&writeSet);
	int numFDs=2;

	//Loop for input
	while(true)
	{
		//Set the readset up for 
		//both the client to client fd and stdin
		FD_SET(clientSockfdUDP,&readSet);
		FD_SET(0,&readSet);
		//FD_SET(1,&writeSet);
		
		//Use a select call to get input from either fd
		if(select((max(clientSockfdUDP,0)+1),&readSet,NULL,NULL,NULL)<0)
		{
			perror("Select call failed");
			exit(0);
		}

		//If input is from stdin
		if(FD_ISSET(0,&readSet))
		{
			//Get the input
			getline(cin,outboundMsg);

			if(strcasecmp(outboundMsg.c_str(),"list")==0)
			{
				listLocalClients(localDirectory,semid);
			}
			else if(strcasecmp(outboundMsg.c_str(),"all")==0)
			{
				getAllClients(serverSockfdTCP);
			}
			else if(strcasecmp(outboundMsg.c_str(),"exit")==0)
			{
				cleanUpClient(localDirectory,semid,shmid,serverSockfdTCP);

			}
			else
			{
				if(outboundMsg.find(" ")==string::npos)
				{
					cout << "Please enter a message after the users name" << endl;
				}
				else
				{
					writeMsgToServer(serverSockfdTCP,clientSockfdUDP,outboundMsg,name,localDirectory,semid);
				}
			}

		}
		//If the input is from another client
		if(FD_ISSET(clientSockfdUDP,&readSet))
			{
			//Get the message and print it
			char buffer[100];
			int result = recv(clientSockfdUDP, buffer, 100, 0);
			buffer[result]='\0';
			cout << ">"<<buffer << endl;
			}
		// if(FD_ISSET(1,&writeSet))
		// {
		//
		// 	flush(cout);
		// }

		}
}


/**Write to server before sending a message to a client
	@param serverSockfdTCP the TCP fd
	@param clientSockfdUDP the UDP fd
	@param outboundMsg The message to be sent
	@param author Who wrote the message
	@param *localDirectory The local_dir struct in shared memory
	@param semid The semaphore id

*/
void writeMsgToServer(int serverSockfdTCP,int clientSockfdUDP,string outboundMsg,string author,struct local_dir *localDirectory,int semid)
{
		//Split the message and get the user its going to and the content
		string user = outboundMsg.substr(0,outboundMsg.find_first_of(" "));
		string message = outboundMsg.substr(outboundMsg.find_first_of(" "),outboundMsg.size());

		//Send the users name to the server
		if((send(serverSockfdTCP,user.c_str(),user.size(),0))<0){
			perror("WRITE TO SERVER FAILED");
			exit(0);
		}
		else
		{
			//Getting struct from server
			int input;
			
			//void *ptr = malloc(sizeof(struct sockaddr_in));
			void *ptr = malloc(sizeof(msg_packet));
			//cout << "size of sockaddr_in" << sizeof(struct sockaddr_in) << endl;
			//if((input = recv(serverSockfdTCP,&ptr,sizeof(struct sockaddr_in),0))<0)

			//Get back the info from the server
			if((input = recv(serverSockfdTCP,ptr,sizeof(msg_packet),0))<0)
			{
				perror("READ CALL FAILED");
			}


			msg_packet *packet= (struct msg_packet*) ptr;
			struct sockaddr_in other_user_info=packet->socket_info;

			//If the port is 0 then the user wasnt found
			if(other_user_info.sin_port==0)
			{
				cout << "User Not Found" << endl;
			}
			else
			{	
				//Any other port means the user was found
				//So write the message to the client
				writeMsgToClient(other_user_info,clientSockfdUDP,author,message,localDirectory,semid);
			}

		}
}

/**Write to server before sending a message to a client
	@param client_info The information about the client receiving the message
	@param clientSockfdUDP the UDP fd
	@param author Who wrote the message
	@param message The message being sent
	@param *localDirectory The local_dir struct in shared memory
	@param semid The semaphore id

*/
void writeMsgToClient(struct sockaddr_in client_info,int clientSockfdUDP,string author,string message,struct local_dir *localDirectory,int semid)
{
	//Create the full message string
	string fullMessage = "Message From "+author+":\n"+message+"\n";
	//Send the message to the client
	if(sendto(clientSockfdUDP, fullMessage.c_str(), fullMessage.size(),0,(struct sockaddr *)&client_info, sizeof(client_info))<0)
	{
		perror("SENDTO FAILED");
	}
	updateLocalInfo(localDirectory,semid);

}

/**Get all of the clients registered on the server and print them
	@param serverSockfdTCP the TCP fd
*/
void getAllClients(int serverSockfdTCP)
{
	string all="all";
	//Send the keyword all to the server
	if((send(serverSockfdTCP,all.c_str(),all.size(),0))<0){
		perror("WRITE TO SERVER FAILED");
		exit(0);
	}
	else
	{	
		//Get the msg_packet back
		int input;
		char buffer[200];
		void *ptr=malloc(sizeof(msg_packet));

		if((input = recv(serverSockfdTCP,ptr,sizeof(msg_packet),0))<0)
			{
				perror("READ CALL FAILED");
			}
			else
			{
				//Get the buffer and print it
				msg_packet *packet=(struct msg_packet*)ptr;
				
				cout << packet->buffer << endl;
			}
			
	}

}

/**Print the information for each client in local shared memory
	@param *localDirectory The struct in shared memory
	@param semid The semaphore id 
*/
void listLocalClients(struct local_dir *localDirectory,int semid)
{
	getAccess(semid,SHARED_MEM_SEM);
	for(int i=0;i<localDirectory->numClients;i++)
	{
		cout << "Client Name: " << localDirectory->localInfo[i].name << endl;
		cout << "Start Time:"<< getTime(localDirectory->localInfo[i].startTime) << endl;
		cout << "Last Message Time:"<< getTime(localDirectory->localInfo[i].lastMsgTime) << endl;
		cout << "Number of Messages:"<<localDirectory->localInfo[i].numMsg << endl;
		cout << "PID:"<<localDirectory->localInfo[i].pid << endl;
		cout << "\n";
		cout << "\n";
	}
	cout << "Total Messages for all Clients" << localDirectory->totalMsgs << endl;
	giveAccess(semid,SHARED_MEM_SEM);
}

/**Update the local info once a message is sent successfully
	@param *localDirectory The struct in shared memory
	@param semid The semaphore id 
*/
void updateLocalInfo(struct local_dir *localDirectory,int semid)
{
	getAccess(semid,SHARED_MEM_SEM);
	for(int i=0;i<localDirectory->numClients;i++)
	{
		if(localDirectory->localInfo[i].pid==getpid())
		{
			localDirectory->localInfo[i].numMsg++;
			gettimeofday (&localDirectory->localInfo[i].lastMsgTime,NULL);
		}
	}
	localDirectory->totalMsgs++;
	giveAccess(semid,SHARED_MEM_SEM);
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


/**Releases the shared memory if there are no more clients on the machine
	@param *localDirectory The local_dir struct in shared memory
	@param semid The semaphore id
	@param shmid The shared memory id
	@param serverSockfdTCP The socket file descriptor for the server connection
*/
void cleanUpClient(struct local_dir *localDirectory,int semid,int shmid,int serverSockfdTCP)
{
	getAccess(semid,SHARED_MEM_SEM);
	localDirectory->numClients--;
	if(localDirectory->numClients==0)
	{
		if ((shmctl(shmid,IPC_RMID,0))==-1){
			perror("Error removing shared memory segment");
    	}
	}
	giveAccess(semid,SHARED_MEM_SEM);


	string msg="exit";
	//Send the keyword exit to the server
	if((send(serverSockfdTCP,msg.c_str(),msg.size(),0))<0){
		perror("WRITE TO SERVER FAILED");
		exit(0);
	}
	
	exit(0);
}