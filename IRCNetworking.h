#include <string>
#include <vector>

using namespace std;

class IRCResponse {
private:

public:
	bool okay;
	vector<string> *data;
	IRCResponse(bool o);
	IRCResponse(bool o, vector<string> *y);
};

class IRCMessage {
private:

public:
	int messageNum;
	string user;
	string message;
	IRCMessage();
};

class IRCMessageResponse : public IRCResponse {
private:

public:
	vector<IRCMessage> *messages;
	IRCMessageResponse(bool o, vector<string> *y);

};

class IRCNetworking {
	// add any variables

private:
	// methods
	int socket;
	int port;
	void command(string commandName, vector<string> *args);
	string readLine();
	vector<string>* readResponse();

public:
	// methods
	IRCNetworking(int port);
	string username;
	string password;

	void connect();
	IRCResponse* createRoom(string roomName);
	IRCResponse* addUser(string user, string pass);
	IRCResponse* sendMessage(string roomName, string message);
	IRCResponse* getAllUsers();
	IRCResponse* getUsersInRoom(string roomName);
	IRCResponse* listRooms();
	IRCResponse* leaveRoom(string roomName);
	IRCResponse* enterRoom(string roomName);
	IRCMessageResponse* getMessages(string room, int lastMessageNum);

};

