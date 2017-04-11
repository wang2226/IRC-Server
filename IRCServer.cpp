
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
	const char * blank = " ";
	vector<string> vec;
	
	const char * token = strtok(commandLine, blank);

	while(token != NULL){
		vec.push_back(string(token));
		token = strtok(NULL, blank);
	}
	
	const char * command = vec[0].c_str();
	const char * user = vec[1].c_str();
	const char * password = vec[2].c_str();
	
	string str;
	for(int i=3; i < vec.size(); i++){
		str += vec[i];
		
		if(i != vec.size()-1)
			str += " ";
	}

	const char * args = str.c_str();

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

	if(allUsers.find(string(user)) == allUsers.end()){
		allUsers.insert(pair<string, string>(string(user), string(password)));
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
	string room = string(args);

	if(!checkPassword(fd, user, password) ){
		msg =  "ERROR (Wrong password)\r\n";
	}else{
		vector<string>::iterator it;

		int flag = 0;
		for(it = chatRoom.begin(); it != chatRoom.end(); it++){
			if(*it == room){
				flag = 1;
				break;
			}
		}

        if(flag != 1){
			msg = "ERROR (No room)\r\n";
		}else{
			map<string, vector<string> >::iterator it = userInRoom.find(string(user)); 

			if(it == userInRoom.end()){
				vector<string> vec;
				vec.push_back(room);
				userInRoom.insert(pair <string,vector <string> > (string(user), vec));
			}else{
				int haveOne = 0;
				for(int i = 0; i < it->second.size(); i++){
					if(it->second[i] == room){
						haveOne = 1;
						break;
					}
				}

				if(haveOne != 1){
					it->second.push_back(room);
	//				userInRoom.erase(string(user));
	//				userInRoom.insert(pair <string,vector <string> > (string(user), it->second));
				}
			}
			msg =  "OK\r\n";
		}
	}

	write(fd, msg, strlen(msg));
	return;
}

void
IRCServer::leaveRoom(int fd, const char * user, const char * password, const char * args)
{
	const char * msg;	
	string room = string(args);

	if(!checkPassword(fd, user, password)){
		msg = "ERROR (Wrong password)\r\n";
	}else{
		map<string, vector<string> >::iterator it = userInRoom.find(string(user)); 
		
		if(it == userInRoom.end()){
			msg =  "ERROR (No user in room)\r\n";
		}else{
			vector<string> vec = it->second;
			vec.erase(remove(vec.begin(), vec.end(), room), vec.end());

			if(vec.empty())
				userInRoom.erase(string(user));
			else
				userInRoom.insert(pair <string,vector <string> > (string(user), vec));

			msg =  "OK\r\n";
		}
	}

	write(fd, msg, strlen(msg));
	return;
}

void
IRCServer::sendMessage(int fd, const char * user, const char * password, const char * args)
{
	const char * msg;	
	const char * blank = " ";
	vector<string> vec ;

	const char * token = strtok((char *)args, blank);

	while(token != NULL){
		vec.push_back(string(token));
		token = strtok(NULL, blank);
	}
	
	string room = vec[0];
	
	string message;
	for(int i=1; i < vec.size(); i++){
		message += vec[i];
		
		if(i != vec.size()-1)
			message += blank;
	}

	vector<string> msgVector;

	if(!checkPassword(fd, user, password) ){
		msg = "ERROR (Wrong password)\r\n";
	}else{
		map<string, vector<string> >::iterator it = userInRoom.find(string(user)); 
		
		if(it == userInRoom.end()){
			msg = "ERROR (user not in room)\r\n";
		}else{
			vector<string> vec = it->second;

			int inRoom = 0;
			for(int i = 0; i < vec.size(); i++){
				if(vec[i] == room){
					inRoom = 1;
					break;
				}
			}
			
			if(inRoom != 1){
				msg = "ERROR (user not in room)\r\n";
			}else{
				string str = string(user) + blank + message + "\r\n";

				map<string, vector<string> >::iterator it = msgInRoom.find(room); 

				if(it == msgInRoom.end()){
					msgVector.push_back(str);
				}else{
						msgVector = it->second;
						int size = msgVector.size();

						if(size >= 100)
								msgVector.erase(msgVector.begin());

						msgVector.push_back(str);
						msgInRoom.erase(room);
				  }

				msgInRoom.insert(pair <string,vector <string> > (room, msgVector));
				msg =  "OK\r\n";
			}
		}
	} 

	write(fd, msg, strlen(msg));
	return;
}

void
IRCServer::getMessages(int fd, const char * user, const char * password, const char * args)
{
	const char * msg;	
	vector<string> vec ;
	const char * blank = " ";

	const char * token = strtok((char *)args, blank);

	while(token != NULL){
		vec.push_back(string(token));
		token = strtok(NULL, blank);
	}
	
	//int lastMsgNum = atoi(vec[0].c_str());
	int lastMsgNum = stoi(vec[0]);
	string room = vec[1];

	if(!checkPassword(fd, user, password) ){
		msg = "ERROR (Wrong password)\r\n";
	}else{
		map<string, vector<string> >::iterator it = userInRoom.find(string(user)); 
		
		if(it == userInRoom.end()){
			msg = "ERROR (User not in room)\r\n";
		}else{
			vector<string> vec = it->second;

			int inRoom = 0;
			for(int i = 0; i < vec.size(); i++){
				if(vec[i] == room){
					inRoom = 1;
					break;
				}
			}
			
			if(inRoom != 1){
				msg = "ERROR (User not in room)\r\n";
			}else{
const char * buffer = "*********************************      1     ********************\n";
write(fd, buffer, strlen(buffer));
				map<string, vector<string> >::iterator itVec = msgInRoom.find(room); 
				vector<string> vec = itVec->second;
buffer = "*********************************      2     ********************\n";
write(fd, buffer, strlen(buffer));
				int size = vec.size();		
buffer = "*********************************      3     ********************\n";
write(fd, buffer, strlen(buffer));

				if(lastMsgNum+1 > size){
					msg =  "NO-NEW-MESSAGES\r\n";
				}else{
					for(int i = lastMsgNum+1; i < size; i++){
						string str = to_string(i) + blank + vec[i];
						msg =  str.c_str();
						write(fd, msg, strlen(msg));
					}	
					msg =  "\r\n";
				}
			}
		}
	} 

	write(fd, msg, strlen(msg));
	return;
}

void
IRCServer::getUsersInRoom(int fd, const char * user, const char * password, const char * args)
{
	const char * msg;
	if(!checkPassword(fd, user, password)){
		msg = "ERROR (Wrong password)\r\n";
		/*
	}else if(userInRoom.find(string(user)) == userInRoom.end()){
		msg =  "ERROR (No user in room)\r\n";
		*/
	}else{
		map<string,vector<string> >::iterator it;
		for(it = userInRoom.begin(); it != userInRoom.end(); it++){
			vector <string> vec;
			vec = it->second;

			for(int i = 0; i < vec.size(); i++){
				if(!(vec[i].compare(string(args)))){
					string str = it->first + "\r\n";	
					msg = str.c_str();
					write(fd, msg, strlen(msg));
					break;
				}
			}
		}
		msg = "\r\n";
	}

	write(fd, msg, strlen(msg));
	return;
}

void
IRCServer::getAllUsers(int fd, const char * user, const char * password,const  char * args)
{
	const char * msg;

	if(!checkPassword(fd, user, password)){
		msg = "ERROR (Wrong password)\r\n";
	}else{
		map<string,string>::iterator it;

		for(it = allUsers.begin(); it != allUsers.end(); it++){
			string str = it->first + "\r\n";
			msg = str.c_str();
			write(fd, msg, strlen(msg));
		} 

		msg = "\r\n";
	}

	write(fd, msg, strlen(msg));
	return;
}

void
IRCServer::createRoom(int fd, const char * user, const char * password,const  char * args)
{
	const char * msg;
	if(!checkPassword(fd, user, password)){
		msg = "DENIED\r\n";
	}else{
		vector<string>::iterator it;
		for(it = chatRoom.begin(); it != chatRoom.end(); it++){
			if(*it == string(args)){
				msg = "OK\r\n";
				write(fd, msg, strlen(msg));
				return;
			}
		}
		chatRoom.push_back(string(args));
		msg = "OK\r\n";
	} 
	write(fd, msg, strlen(msg));
	return;
}

void
IRCServer::listRooms(int fd, const char * user, const char * password,const  char * args)
{

}

