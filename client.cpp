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
using namespace std;

//Constant to set the buffer size as needed
#define BUFFER_SIZE 500
#define MAX_USERS 10

//Structs
struct tagLocal_Info
{
	char name[20];
	struct timeval startTime;
	struct timeval lastMsgTime;
	int numMsg;
	pid_t pid;
};

struct tagLocal_Dir
{
	tagLocal_Info localInfo[MAX_USERS];
	int numClients;
	int totalMsgs;

};

//Prototypes


/**Main method that starts the application
         @return 0 in success
                */
int main(int argc, char* argv[]){

}