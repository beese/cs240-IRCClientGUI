#include "IRCNetworking.h"
#include <sys/socket.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <time.h>
#include <fstream>
#include <string>
#include <iostream>


IRCResponse::IRCResponse(bool o) {
	okay = o;
	data = NULL;
}

IRCResponse::IRCResponse(bool o, vector<string> *y) {
	okay = o;
	data = y;
}

IRCMessage::IRCMessage() {

}

IRCMessageResponse::IRCMessageResponse(bool o, vector<string> *y) : IRCResponse(o) {
	okay = o;

	messages = new vector<IRCMessage>();

	for(int i = 0; i < y->size(); i++) {
		string message = (*y)[i];
		if(message.compare("NO-NEW-MESSAGES") == 0) {
			return;
		}
		int space = message.find(" ");
		string msgNum = message.substr(0, space);
		string rest = message.substr(space + 1);
		string user = rest.substr(0, space = rest.find(" "));
		string msg = rest.substr(space + 1);

		IRCMessage mymsg;
		mymsg.user = user;
		mymsg.message = msg;
		mymsg.messageNum = stoi(msgNum);

		messages->push_back(mymsg);
	}
}

IRCNetworking::IRCNetworking(int portN) {
	port = portN;
}

int open_client_socket(const char * host, int port) {
	// Initialize socket address structure
	struct  sockaddr_in socketAddress;
	
	// Clear sockaddr structure
	memset((char *)&socketAddress,0,sizeof(socketAddress));
	
	// Set family to Internet 
	socketAddress.sin_family = AF_INET;
	
	// Set port
	socketAddress.sin_port = htons((u_short)port);
	
	// Get host table entry for this host
	struct  hostent  *ptrh = gethostbyname(host);
	if ( ptrh == NULL ) {
		perror("gethostbyname");
		exit(1);
	}
	
	// Copy the host ip address to socket address structure
	memcpy(&socketAddress.sin_addr, ptrh->h_addr, ptrh->h_length);
	
	// Get TCP transport protocol entry
	struct  protoent *ptrp = getprotobyname("tcp");
	if ( ptrp == NULL ) {
		perror("getprotobyname");
		exit(1);
	}
	
	// Create a tcp socket
	int sock = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
	if (sock < 0) {
		perror("socket");
		exit(1);
	}
	
	// Connect the socket to the specified server
	if (connect(sock, (struct sockaddr *)&socketAddress,
		    sizeof(socketAddress)) < 0) {
		perror("connect");
		exit(1);
	}
	
	return sock;
}

void IRCNetworking::connect() {
	socket = open_client_socket("localhost", port);
}

void IRCNetworking::command(string commandName, vector<string> *args) {
	connect();
	dprintf(socket, "%s %s %s", commandName.c_str(), username.c_str(), password.c_str());
	if(args != NULL && args->size() > 0) {
		int i;
		for(i = 0; i < args->size(); i++) {
			dprintf(socket, " %s", (*args)[i].c_str());
		}
	}
	dprintf(socket, "\r\n");
}

string IRCNetworking::readLine() {
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
		read( socket, &newChar, 1) > 0 ) {
		if (newChar == '\n' && prevChar == '\r') {
			break;
		}
		
		commandLine[ commandLineLength ] = newChar;
		commandLineLength++;

		prevChar = newChar;
	}
	
	// Add null character at the end of the string
	// Eliminate last \r
	if(commandLineLength > 0) {
		commandLineLength--;
	}

    commandLine[ commandLineLength ] = 0;

	string response = string (commandLine);
	return response;
}

vector<string>* IRCNetworking::readResponse() {

	vector<string>* lines = new vector<string>();

	string currentLine = readLine();
	printf("current line: %s\n", currentLine.c_str());

	string error = "ERROR (";

	if(currentLine.compare(0, 7, error) == 0) {
		return NULL;
	}

	while(currentLine.compare("") != 0 && currentLine.compare("OK") != 0 && currentLine.compare(0, 7, error) != 0) {
		lines->push_back(currentLine);
		currentLine = readLine();
		printf("current line2: %s\n", currentLine.c_str());
		if(currentLine.compare("OK") == 0) {
			lines->push_back(currentLine);
			break;
		}
		if(currentLine.compare(0, 7, error) == 0) {
			return NULL;
		}
	}

	return lines;
}


IRCResponse* IRCNetworking::createRoom(string roomName) {
	vector<string>* roomN = new vector<string>();
	roomN->push_back(roomName);
	command(string("CREATE-ROOM"), roomN);

	vector<string>* responses = readResponse();

	return new IRCResponse(responses != NULL, responses);
}

IRCResponse* IRCNetworking::addUser(string user, string pass) {
	connect();
	dprintf(socket, "ADD-USER %s %s\r\n", user.c_str(), pass.c_str());

	vector<string>* responses = readResponse();

	return new IRCResponse(responses != NULL, responses);
}

IRCResponse* IRCNetworking::sendMessage(string roomName, string message) {
	vector<string>* info = new vector<string>();
	info->push_back(roomName);
	info->push_back(message);
	command(string("SEND-MESSAGE"), info);

	vector<string>* responses = readResponse();

	return new IRCResponse(responses != NULL, responses);
}

IRCResponse* IRCNetworking::getAllUsers() {
	vector<string>* info = new vector<string>();
	command(string("GET-ALL-USERS"), info);

	vector<string>* responses = readResponse();

	return new IRCResponse(responses != NULL, responses);
}

IRCResponse* IRCNetworking::getUsersInRoom(string roomName) {
	vector<string>* info = new vector<string>();
	info->push_back(roomName);
	command(string("GET-USERS-IN-ROOM"), info);

	vector<string>* responses = readResponse();

	return new IRCResponse(responses != NULL, responses);
}

IRCResponse* IRCNetworking::listRooms() {
	vector<string>* info = new vector<string>();
	command(string("LIST-ROOMS"), info);

	vector<string>* responses = readResponse();

	return new IRCResponse(responses != NULL, responses);
}

IRCResponse* IRCNetworking::leaveRoom(string roomName) {
	vector<string>* info = new vector<string>();
	info->push_back(roomName);
	command(string("LEAVE-ROOM"), info);

	vector<string>* responses = readResponse();

	return new IRCResponse(responses != NULL, responses);
}

IRCResponse* IRCNetworking::enterRoom(string roomName) {
	vector<string>* info = new vector<string>();
	info->push_back(roomName);
	command(string("ENTER-ROOM"), info);

	vector<string>* responses = readResponse();

	return new IRCResponse(responses != NULL, responses);
}

IRCMessageResponse* IRCNetworking::getMessages(string room, int lastMessageNum) {
	vector<string>* info = new vector<string>();
	info->push_back(to_string(lastMessageNum));
	info->push_back(room);
	
	command(string("GET-MESSAGES"), info);

	vector<string>* responses = readResponse();

	return new IRCMessageResponse(responses != NULL, responses);
}

