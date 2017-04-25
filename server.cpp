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
#include <sys/socket.h>
#include <netinet/in.h>
using namespace std;

//Constant to set the buffer size as needed
#define BUFFER_SIZE 500
#define MAX_USERS 10

//Structs
struct tagClient
{
	char name[20];
	struct sockaddr_in serverAddr;
	struct timeval startTime;
	struct timeval lastLookupTime;
	
};

struct tagDir
{
	tagClient clientInfo[MAX_USERS];
	int numClients;
};
//Prototypes


/**Main method that starts the application
         @return 0 in success
                */
int main(int argc, char* argv[]){

}

