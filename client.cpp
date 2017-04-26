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
	struct sockaddr_in server;
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

	if(strlen(buffer)>10){
		perror("Name is longer than 10 chars");
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
		cout << "WRITE TO SERVER WORKED" << endl;
	}


}