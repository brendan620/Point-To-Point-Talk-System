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
                CLIENT
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

using namespace std;

//Constant to set the buffer size as needed
#define BUFFER_SIZE 500
#define MAX_USERS 10
#define SERV_HOST_ADDR "156.12.127.18"

//Structs
struct local_Info
{
	char name[20];
	struct timeval startTime;
	struct timeval lastMsgTime;
	int numMsg;
	pid_t pid;
};

struct local_Dir
{
	local_Info localInfo[MAX_USERS];
	int numClients;
	int totalMsgs;

};

struct packet{
	int portNum;
	int numClients;
};

//Prototypes
int openPort(struct sockaddr_in client);
struct local_Dir initiateLocalInfo(struct local_Dir localDirectory,string name,struct packet *port);
void readWrite(int clientSockfdUDP,int serverSockfdTCP);
void writeMsgToClient(struct sockaddr_in client_info,int clientSockfdUDP,string author,string message);
void writeMsgToServer(int serverSockfdTCP,int clientSockfdUDP,string outboundMsg);
/**Main method that starts the application
         @return 0 in success
                */
int main(int argc, char* argv[]){
	int serverSockfdTCP;
	struct sockaddr_in server,client;
	struct local_Dir localDirectory;
	string name;
	char buffer[BUFFER_SIZE];
	if(argc != 2)
	{
		cout<<"Usage " << argv[0] << " user_name" << endl;
		exit(0);
	}
	else
	{
		name = argv[1];
	}

	if(name.length()>10){
		perror("Name is longer than 10 chars");
		exit(0);
	}

	if((serverSockfdTCP=socket(AF_INET,SOCK_STREAM,0))<0){
		perror("SERVER CANNOT OPEN SOCKET");
		exit(0);
	}

	cout << "SOCKET CALL WORKED" << endl;

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
	//htons == Converts the unsigned short integer hostshort
	//from host byte order to network byte order
	server.sin_port = htons(15080);



	if((connect(serverSockfdTCP,(struct sockaddr *) &server,sizeof(server)))<0){
		perror("CONNECT CALL TO THE SERVER FAILED");
		exit(0);
	}
	else
	{
		cout << "CONNECTED TO THE SERVER" << endl;
	}

	strcpy(buffer,name.c_str());

	if((send(serverSockfdTCP,buffer,strlen(buffer),0))<0){
		perror("WRITE TO SERVER FAILED");
		exit(0);
	}
	else
	{

		cout << "WRITE TO SERVER WORKED" << endl;
		cout << "Waiting for a response" << endl;
		int input;

		void *ptr;

		if((input = recv(serverSockfdTCP,&ptr,BUFFER_SIZE,0))<0)
			{
				perror("READ CALL FAILED");
			}
		else if(input>0)
			{
				buffer[input]='\0';

			}
			struct packet *port=(struct packet*) &ptr;
			if(port->portNum==0)
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
				client.sin_port = (port->portNum);
				cout << "PORT GIVEN FROM SERVER " << port->portNum << endl;
				cout << "PORT IN THE CLIENT STRUCT" << client.sin_port << endl;
				void *addrPtr=(void *)&client;
				if((send(serverSockfdTCP,addrPtr,sizeof(client),0))<0){
						perror("WRITE TO SERVER FAILED");
						exit(0);
					}
				else{
						localDirectory=initiateLocalInfo(localDirectory,name,port);
						int clientSockfdUDP=openPort(client);
						readWrite(clientSockfdUDP,serverSockfdTCP);
				}
			}
	}
}



struct local_Dir initiateLocalInfo(struct local_Dir localDirectory,string name,struct packet *serverPacket)
{
	char tempName[20];
	strcpy(tempName, name.c_str());
	localDirectory.numClients=serverPacket->numClients;
	strcpy(localDirectory.localInfo[serverPacket->numClients-1].name,name.c_str());
	localDirectory.localInfo[serverPacket->numClients-1].pid=getpid();
	localDirectory.localInfo[serverPacket->numClients-1].numMsg=0;
	localDirectory.localInfo[serverPacket->numClients-1].lastMsgTime.tv_sec=0;
	gettimeofday (&localDirectory.localInfo[serverPacket->numClients-1].startTime,NULL);

	return localDirectory;
}



int openPort(struct sockaddr_in client)
{
	int clientSockfdUDP;

  if((clientSockfdUDP = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		perror("server: cannot open stream socket");
		exit(1);
	}

	if(bind(clientSockfdUDP, (struct sockaddr *) &client, sizeof(client))< 0)
    {
			perror("server: cannot bind local address");
			exit(2);
		}

		return clientSockfdUDP;

}

void readWrite(int clientSockfdUDP, int serverSockfdTCP)
{
	cout << ">";
	fd_set readSet,writeSet;
	string outboundMsg;
	int selectStatus;
	FD_ZERO(&readSet);
	//FD_ZERO(&writeSet);
	int numFDs=2;

	while(true)
	{
		FD_SET(clientSockfdUDP,&readSet);
		FD_SET(0,&readSet);
		//FD_SET(1,&writeSet);
		//if(select((max(clientSockfdUDP,0)+1),&readSet,&writeSet,NULL,NULL)<0)
		if(select((max(clientSockfdUDP,0)+1),&readSet,NULL,NULL,NULL)<0)
		{
			perror("Select call failed");
			exit(0);
		}

		if(FD_ISSET(0,&readSet))
		{
			getline(cin,outboundMsg);

			if(strcasecmp(outboundMsg.c_str(),"list")==0)
			{
				cout << "LIST WAS ENTERED" << endl;
			}
			else if(strcasecmp(outboundMsg.c_str(),"all")==0)
			{
				cout << "All WAS ENTERED" << endl;
			}
			else if(strcasecmp(outboundMsg.c_str(),"exit")==0)
			{
				cout << "EXIT WAS ENTERED" << endl;

			}
			else
			{
				if(outboundMsg.find(" ")==string::npos)
				{
					cout << "Please enter a message after the users name" << endl;
				}
				else
				{
					writeMsgToServer(serverSockfdTCP,clientSockfdUDP,outboundMsg);

				}
			}

		}
		if(FD_ISSET(clientSockfdUDP,&readSet))
			{
			cout << "Inside FD_ISSET"<<clientSockfdUDP << endl;
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


void writeMsgToServer(int serverSockfdTCP,int clientSockfdUDP,string outboundMsg)
{
		string author = outboundMsg.substr(0,outboundMsg.find_first_of(" "));
		string message = outboundMsg.substr(outboundMsg.find_first_of(" "),outboundMsg.size());

		if((send(serverSockfdTCP,author.c_str(),author.size(),0))<0){
			perror("WRITE TO SERVER FAILED");
			exit(0);
		}
		else
		{
			int input;

			void *ptr;
			if((input = recv(serverSockfdTCP,&ptr,sizeof(&ptr),0))<0)
			{
			perror("READ CALL FAILED");
			}

			struct sockaddr_in *client_info=(struct sockaddr_in*) &ptr;
			cout << "CLI AFTER" << endl;
			cout << client_info->sin_port << endl;
			struct sockaddr_in temp_info;
			cout << "client_info" << client_info->sin_port << endl;


			//bzero((char *) &temp_info,sizeof(temp_info));
			temp_info.sin_port=client_info->sin_port;
			temp_info.sin_family=client_info->sin_family;
			temp_info.sin_addr.s_addr=client_info->sin_addr.s_addr;



			if(client_info->sin_port==0)
			{
				cout << "Not found" << endl;
			}
			else
			{
				writeMsgToClient(temp_info,clientSockfdUDP,author,message);
			}

		}
}

void writeMsgToClient(struct sockaddr_in client_info,int clientSockfdUDP,string author,string message)
{
	string fullMessage = "Message From Blank:\n"+message+"\n";
	if(sendto(clientSockfdUDP, fullMessage.c_str(), fullMessage.size(),0,(struct sockaddr *)&client_info, sizeof(client_info))<0)
	{
		perror("SENDTO FAILED");
	}

}
