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

//Prototypes


/**Main method that starts the application
         @return 0 in success
                */
int main(int argc, char* argv[]){
	int sockfd;
	struct sockaddr_in server,client;
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

	if((sockfd=socket(AF_INET,SOCK_STREAM,0))<0){
		perror("SERVER CANNOT OPEN SOCKET");
		exit(0);
	}

	cout << "SOCKET CALL WORKED" << endl;

	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr(SERV_HOST_ADDR);
	//htons == Converts the unsigned short integer hostshort
	//from host byte order to network byte order
	server.sin_port = htons(15080);


	//SETUP CLIENTS SOCKADDR IN
	client.sin_family = AF_INET;
	//htonl == Converts the unsigned integer hostlong from host
	//byte order to network byte order
	client.sin_addr.s_addr = htonl(INADDR_ANY);
	//htons == Converts the unsigned short integer hostshort
	//from host byte order to network byte order
	client.sin_port = htons(15082);

	cout << "FAMILY" << client.sin_family << endl;
	cout << "ADDR"<< client.sin_addr.s_addr << endl;
	cout << "PORT"<< client.sin_port << endl;


	if((connect(sockfd,(struct sockaddr *) &server,sizeof(server)))<0){
		perror("CONNECT CALL TO THE SERVER FAILED");
		exit(0);
	}
	else
	{
		cout << "CONNECTED TO THE SERVER" << endl;
	}

	strcpy(buffer,name.c_str());

	if((send(sockfd,buffer,strlen(buffer),0))<0){
		perror("WRITE TO SERVER FAILED");
		exit(0);
	}
	else
	{
		bzero(buffer,BUFFER_SIZE);
		cout << "WRITE TO SERVER WORKED" << endl;
		cout << "Waiting for a response" << endl;
		int input;
		if((input = recv(sockfd,buffer,BUFFER_SIZE,0))<0)
			{
				perror("READ CALL FAILED");
			}
		else if(input>0)
			{
				buffer[input]='\0';

			}
			if(strcasecmp(buffer,"Ok")==0)
			{

				cout << "TRYING TO WRITE STRUCT BACK TO THE SERVER" << endl;
				void *addrPtr=(void *)&client;
				if((send(sockfd,addrPtr,sizeof(client),0))<0){
						perror("WRITE TO SERVER FAILED");
						exit(0);
					}
			}
			else
			{
				cout << "Either your name is taken or the maximum number of users are on the server right now" << endl;
				exit(0);
			}
	}




}
