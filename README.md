# Point-To-Point-Talk-System
CSC-552 Implement a Distributed Point-to-Point talk system. Uses both kinds of sockets, semaphores, shared memory, and signal processing.

Due to the scope of this project I am going to break it up into sections.
	These two programs must be run on acad for them to work. 
	I wasn't able to get them to work from other machines.

	LookupServer
	----------------------------------------------
	Can communicate successfully with connected clients and return either
	the list of all clients on the server or the requested clients sockaddr_in
	struct.

	It has shared memory set up and stores all of the required information.
	I did have issues with releasing the shared memory in the server so
	that is not done. 

	When clients connect the server first verifies that there is room in the 
	server and that the name requested was not already taken.

	The lookup server can be exited via the CTRL+C command. This prompts the 
	user to confirm that they want to exit. I had this set up to also 
	work when all clients disconnect but ran into issues with the waitpid call
	blocking.

	Client
	----------------------------------------------
	Can successfully register itself with the server and issue all four possible
	commands. Typing "All" will cause all of the clients connected to the 
	server to be displayed to the screen. It also shows their start time and
	last lookup time. Typing "List" will cause all of the local users to 
	be listed along with their information. Typing "Exit" will cause
	the clients program to terminate and notify the server.

	The clients successfully store their information in shared memory. When 
	the last client exits on a machine that shared memory is then released 
	properly.

	The select command was used to allow input from both a UDP socket and
	stdin which allows for a client to receive messages as well as send them.
	As instructed the SIGINT signal is disabled on the client.

	
	Semaphores
	----------------------------------------------
	I ran out of time to implement the reader/write algorithm as requested
	in the spec sheet. I do have a single set of semaphores in both 
	the client and server to protect the critical sections. This isn't the most
	efficient way to guard the shared memory but it does the job.