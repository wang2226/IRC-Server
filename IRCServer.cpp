
const char * usage =
"                                                               \n"
"IRCServer:                                                   \n"
"                                                               \n"
"Simple server program used to communicate multiple users       \n"
"                                                               \n"
"To use it in one window type:                                  \n"
"                                                               \n"
"   IRCServer <port>                                          \n"
"                                                               \n"
"Where 1024 < port < 65536.                                     \n"
"                                                               \n"
"In another window type:                                        \n"
"                                                               \n"
"   telnet <host> <port>                                        \n"
"                                                               \n"
"where <host> is the name of the machine where talk-server      \n"
"is running. <port> is the port number you used when you run    \n"
"daytime-server.                                                \n"
"                                                               \n";

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>

#include "IRCServer.h"

int QueueLength = 5;

//test

int
IRCServer::open_server_socket(int port) {

	// Set the IP address and port for this server
	struct sockaddr_in serverIPAddress; 
	memset( &serverIPAddress, 0, sizeof(serverIPAddress) );
	serverIPAddress.sin_family = AF_INET;
	serverIPAddress.sin_addr.s_addr = INADDR_ANY;
	serverIPAddress.sin_port = htons((u_short) port);
  
	// Allocate a socket
	int masterSocket =  socket(PF_INET, SOCK_STREAM, 0);
	if ( masterSocket < 0) {
		perror("socket");
		exit( -1 );
	}

	// Set socket options to reuse port. Otherwise we will
	// have to wait about 2 minutes before reusing the sae port number
	int optval = 1; 
	int err = setsockopt(masterSocket, SOL_SOCKET, SO_REUSEADDR, 
			     (char *) &optval, sizeof( int ) );
	
	// Bind the socket to the IP address and port
	int error = bind( masterSocket,
			  (struct sockaddr *)&serverIPAddress,
			  sizeof(serverIPAddress) );
	if ( error ) {
		perror("bind");
		exit( -1 );
	}
	
	// Put socket in listening mode and set the 
	// size of the queue of unprocessed connections
	error = listen( masterSocket, QueueLength);
	if ( error ) {
		perror("listen");
		exit( -1 );
	}

	return masterSocket;
}

void
IRCServer::runServer(int port)
{
	int masterSocket = open_server_socket(port);

	initialize();
	
	while ( 1 ) {
		
		// Accept incoming connections
		struct sockaddr_in clientIPAddress;
		int alen = sizeof( clientIPAddress );
		int slaveSocket = accept( masterSocket,
					  (struct sockaddr *)&clientIPAddress,
					  (socklen_t*)&alen);
		
		if ( slaveSocket < 0 ) {
			perror( "accept" );
			exit( -1 );
		}
		
		// Process request.
		processRequest( slaveSocket );		
	}
}

int
main( int argc, char ** argv )
{
	// Print usage if not enough arguments
	if ( argc < 2 ) {
		fprintf( stderr, "%s", usage );
		exit( -1 );
	}
	
	// Get the port from the arguments
	int port = atoi( argv[1] );

	IRCServer ircServer;

	// It will never return
	ircServer.runServer(port);
	
}

//
// Commands:
//   Commands are started y the client.
//
//   Request: ADD-USER <USER> <PASSWD>\r\n
//   Answer: OK\r\n or DENIED\r\n
//
//   REQUEST: GET-ALL-USERS <USER> <PASSWD>\r\n
//   Answer: USER1\r\n
//            USER2\r\n
//            ...
//            \r\n
//
//   REQUEST: CREATE-ROOM <USER> <PASSWD> <ROOM>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: LIST-ROOMS <USER> <PASSWD>\r\n
//   Answer: room1\r\n
//           room2\r\n
//           ...
//           \r\n
//
//   Request: ENTER-ROOM <USER> <PASSWD> <ROOM>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: LEAVE-ROOM <USER> <PASSWD>\r\n
//   Answer: OK\n or DENIED\r\n
//
//   Request: SEND-MESSAGE <USER> <PASSWD> <MESSAGE> <ROOM>\n
//   Answer: OK\n or DENIED\n
//
//   Request: GET-MESSAGES <USER> <PASSWD> <LAST-MESSAGE-NUM> <ROOM>\r\n
//   Answer: MSGNUM1 USER1 MESSAGE1\r\n
//           MSGNUM2 USER2 MESSAGE2\r\n
//           MSGNUM3 USER2 MESSAGE2\r\n
//           ...\r\n
//           \r\n
//
//    REQUEST: GET-USERS-IN-ROOM <USER> <PASSWD> <ROOM>\r\n
//    Answer: USER1\r\n
//            USER2\r\n
//            ...
//            \r\n
//

void
IRCServer::processRequest( int fd )
{
	// Buffer used to store the comand received from the client
	const int MaxCommandLine = 1024;
	char commandLine[ MaxCommandLine + 1 ];
	int commandLineLength = 0;
	int n;
	
	// Currently character read
	unsigned char prevChar = 0;
	unsigned char newChar = 0;
	
	//
	// The client should send COMMAND-LINE\n
	// Read the name of the client character by character until a
	// \n is found.
	//

	// Read character by character until a \n is found or the command string is full.
	while ( commandLineLength < MaxCommandLine &&
		read( fd, &newChar, 1) > 0 ) {

		if (newChar == '\n' && prevChar == '\r') {
			break;
		}
		
		commandLine[ commandLineLength ] = newChar;
		commandLineLength++;

		prevChar = newChar;
	}
	
	// Add null character at the end of the string
	// Eliminate last \r
	commandLineLength--;
        commandLine[ commandLineLength ] = 0;

	printf("RECEIVED: %s\n", commandLine);
/*
	printf("The commandLine has the following format:\n");
	printf("COMMAND <user> <password> <arguments>. See below.\n");
	printf("You need to separate the commandLine into those components\n");
	printf("For now, command, user, and password are hardwired.\n");
*/
	const char * s = " ";
	const char * command = strtok(commandLine, s);
	const char * user = strtok(NULL, s);
	const char * password = strtok(NULL, s);
	const char * args = strtok(NULL, s);

	printf("command=%s\n", command);
	printf("user=%s\n", user);
	printf( "password=%s\n", password );
	printf("args=%s\n", args);

	if (!strcmp(command, "ADD-USER")) {
		addUser(fd, user, password, args);
	}
	else if (!strcmp(command, "ENTER-ROOM")) {
		enterRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "CREATE-ROOM")) {
		createRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "LIST-ROOM")) {
		listRooms(fd, user, password, args);
	}
	else if (!strcmp(command, "LEAVE-ROOM")) {
		leaveRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "SEND-MESSAGE")) {
		sendMessage(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-MESSAGES")) {
		getMessages(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-USERS-IN-ROOM")) {
		getUsersInRoom(fd, user, password, args);
	}
	else if (!strcmp(command, "GET-ALL-USERS")) {
		getAllUsers(fd, user, password, args);
	}
	else {
		const char * msg =  "UNKNOWN COMMAND\r\n";
		write(fd, msg, strlen(msg));
	}

	// Send OK answer
	//const char * msg =  "OK\n";
	//write(fd, msg, strlen(msg));

	close(fd);	
}

void
IRCServer::initialize()
{
	// Open password file
	ifstream is;
	is.open("PASSWORD_FILE");

	// Initialize users in room
	string user,password;
	while(is && getline(is,user) && getline(is,password)){
		allUsers.insert(pair<string, string>(user, password));	
	}
	// Initalize message list
	is.close();
}

bool
IRCServer::checkPassword(int fd, const char * user, const char * password) {
	// Here check the password
	bool match;
	string str (password);
	if(!allUsers[user].compare(str))
		match = true;
	else
		match = false;
	return match;
}

void
IRCServer::addUser(int fd, const char * user, const char * password, const char * args)
{
	// Here add a new user. For now always return OK.
	const char * msg;
	if(allUsers.find(user) == allUsers.end()){
		allUsers.insert(pair<string,string>(user,password));
		msg =  "OK\r\n";
	} else {
		msg =  "DENIED\r\n";
	}
	write(fd, msg, strlen(msg));
	return;		
}

void
IRCServer::enterRoom(int fd, const char * user, const char * password, const char * args)
{
	const char * msg;
	if(checkPassword(fd, user, password) && userInRoom.find(user) == userInRoom.end()){
		vector<string>::iterator it;
		int flag = 0;
		for(it = chatRoom.begin(); it < chatRoom.end(); it++){
			if(*it == args){
				flag = 1;
				break;
			}
		}
        if(flag != 1)
			createRoom(fd, user, password, args);
		userInRoom.insert(pair<string,string>(user, args));
		msg =  "OK\r\n";
	} else {
		msg =  "DENIED\r\n";
	}
	write(fd, msg, strlen(msg));
	return;
}

void
IRCServer::leaveRoom(int fd, const char * user, const char * password, const char * args)
{
	const char * msg;	
	if(checkPassword(fd, user, password) && userInRoom.find(user) != userInRoom.end()){
		userInRoom.erase(user);
		msg =  "OK\r\n";
	} else {
		msg =  "ERROR (No user in room)\r\n";
	}
	write(fd, msg, strlen(msg));
	return;
}

void
IRCServer::sendMessage(int fd, const char * user, const char * password, const char * args)
{
	const char * msg;	
	if(checkPassword(fd, user, password) && userInRoom.find(user) != userInRoom.end()){
		string room = userInRoom[user];
		vector <string> vec ;
		if(msgInRoom.find(room) == msgInRoom.end()){
			msgInRoom.insert(pair <string,vector <string> > (room, vec));
		}else{
			vec = msgInRoom[room];
		}

		int size = vec.size();

		if(size >= 100)
			vec.erase(vec.begin());

		string str = string(user) + " " + string(args) + "\r\n";

		vec.push_back(str);
		msg =  "OK\r\n";
	} else {
		msg =  "DENIED\r\n";
	}
	write(fd, msg, strlen(msg));
	for(string n : vec) {
	        cout << n << '\n';
			    }
	return;
}

void
IRCServer::getMessages(int fd, const char * user, const char * password, const char * args)
{
	const char * msg;	
	if(checkPassword(fd, user, password) && userInRoom.find(user) != userInRoom.end()){
		string room = userInRoom[user];
		vector <string> vec = msgInRoom[room];
		int size = vec.size();
		
		for(int i = 0; i < size; i++){
			string str = to_string(i+1) + string(" ") + vec[i];
			msg =  str.c_str();
			write(fd, msg, strlen(msg));
		}	

	} else {
		msg =  "DENIED\r\n";
		write(fd, msg, strlen(msg));
	} 
		msg =  "\r\n";
		write(fd, msg, strlen(msg));
	
	return;
}

void
IRCServer::getUsersInRoom(int fd, const char * user, const char * password, const char * args)
{
	const char * msg;
	if(checkPassword(fd, user, password) && userInRoom.find(user) != userInRoom.end()){
		map<string,string>::iterator it;
		for(it = userInRoom.begin(); it != userInRoom.end(); it++){
			if(!(it->second.compare(args))){
				string str = it->first + "\r\n";	
				msg = str.c_str();
				write(fd, msg, strlen(msg));
			}
		}
	} else {
		msg = "DENIED\r\n";
		write(fd, msg, strlen(msg));
	}
	msg = "\r\n";
	write(fd, msg, strlen(msg));
	return;
}

void
IRCServer::getAllUsers(int fd, const char * user, const char * password,const  char * args)
{
	map<string,string>::iterator it;
	const char * msg;
	if(checkPassword(fd, user, password)){
		for(it = allUsers.begin(); it != allUsers.end(); it++){
			string str = it->first + "\r\n";
			msg = str.c_str();
			write(fd, msg, strlen(msg));
		} 
	} else {
		msg = "DENIED\r\n";
		write(fd, msg, strlen(msg));
	}
	msg = "\r\n";
	write(fd, msg, strlen(msg));
	return;
}

void
IRCServer::createRoom(int fd, const char * user, const char * password,const  char * args)
{
	const char * msg;
	if(checkPassword(fd, user, password)){
		vector<string>::iterator it;
		for(it = chatRoom.begin(); it != chatRoom.end(); it++){
			if(*it == args){
				return;
			}
		}
		chatRoom.push_back(args);
		msg = "OK\r\n";
		write(fd, msg, strlen(msg));
	} else {
		msg = "DENIED\r\n";
		write(fd, msg, strlen(msg));
	}
}

void
IRCServer::listRooms(int fd, const char * user, const char * password,const  char * args)
{

}

