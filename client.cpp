#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <string>
#include <ctime>

using namespace std;

typedef struct in_addr in_addr;
typedef struct sockaddr_in sockaddr_in;
typedef struct sockaddr sockaddr;

int main(int argc , char *argv[]) {
	//basic imformation : ip, port, user name
	char ip[16], user_name[20];
	int port;
	memset(ip, '\0', sizeof(ip));
	memset(user_name, '\0', sizeof(user_name));
	cout << "IP : "; cin >> ip;
	cout << "PORT : "; cin >> port;
	cout << "USER : "; cin >> user_name;
	
	//socket
	int client_fd;
	client_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (client_fd < 0) {
		perror("client socket fail");
		exit(1);
	}

	//check wether the user exist
	while ( strcmp("Alice", user_name) && strcmp("Bill", user_name) && strcmp("Caesar", user_name) ) {
		cout << "<User not exist, please try again>" << endl;
		cout << "USER : ";
		cin >> user_name;
	}

	//connect
	sockaddr_in cli;
	bzero(&cli, sizeof(cli));
	cli.sin_family = AF_INET;
	cli.sin_port = htons(port);
	cli.sin_addr.s_addr = inet_addr(ip);
	if (connect(client_fd, (sockaddr*)&cli, sizeof(cli)) < 0) {
		perror("connect fail");
		exit(1);
	}

	//send user name to server
	char recv_msg[1000], send_msg[1000];
	memset(recv_msg, '\0', sizeof(recv_msg));
	memset(send_msg, '\0', sizeof(send_msg));
	send(client_fd, user_name, sizeof(user_name), 0);
	recv(client_fd, recv_msg, sizeof(recv_msg), 0);

	//synchronize
	send(client_fd, "wait", sizeof("wait"), 0);

	//check wether login success
	recv(client_fd, recv_msg, sizeof(recv_msg), 0);
	send(client_fd, "ok", sizeof("ok"), 0);
	
	//you are logging
	if ( !strcmp(recv_msg, "login") ) {
		cout << "<login success>" << endl;
	}//you already login
	else if ( !strcmp(recv_msg, "fail") ) {
		cout << "fail to loggin, maybe you already loggin in other places" << endl;
		close(client_fd);
		exit(EXIT_FAILURE);
	}

	//keep looping to check if new event occurs and recv
	bool bye = false;
	while (1) {
		//recv information type from server
		recv(client_fd, recv_msg, sizeof(recv_msg), 0);
		send(client_fd, "ok", sizeof("ok"), 0);
		//no new msg
		cout << "===========================" << endl;
		if ( !strcmp(recv_msg, "no") ) {
			cout << "<no new msg>" << endl;
		}//reveive msg
		else if ( !strcmp(recv_msg, "start") ) {
			while (1) {
				recv(client_fd, recv_msg, sizeof(recv_msg), 0);
				send(client_fd, "ok", sizeof("ok"), 0);
				if ( !strcmp(recv_msg, "done") ) break;
				else cout << recv_msg << endl;
			}
		}

		if (bye) break;

		//user action
		int action;
		cout << "===========================" << endl;
		cout << "0 : chat\n1 : bye\n2 : load msg\ninput : ";
		cin >> action;
		while(action != 0 && action != 1 && action != 2) {
			cout << "invalid input\ninput : ";
			cin >> action;
		}

		//synchronize
		recv(client_fd, recv_msg, sizeof(recv_msg), 0);

		//chat
		if (!action) {
			//get where is the end of user name
			int idx;
			string input;
			cout << "user(s) \"msg\" : ";
			getchar();
			getline(cin, input);
			for (int i = 0; i < input.length(); ++i) {
				if (input[i] == '"') {
					idx = i;
					break;
				}
			}

			//get all receiver's name and send msg
			send(client_fd, "name", sizeof("name"), 0);
			recv(client_fd, recv_msg, sizeof(recv_msg), 0);
			send(client_fd, user_name, sizeof(user_name), 0);
			recv(client_fd, recv_msg, sizeof(recv_msg), 0);

			char arr[100];
			memset(arr, '\0', sizeof(arr));
			strncpy(arr, input.c_str(), idx);
			char *tok = strtok(arr, " ");
			bool no_receiver = true;
			while(tok != NULL) {
				if ( strcmp(tok, "Alice") && strcmp(tok, "Bill") && strcmp(tok, "Caesar") ) {
					cout << "<user " << tok << " does not exist>" << endl;
				}
				else {
					no_receiver = false;
					send(client_fd, tok, sizeof(tok), 0);
					recv(client_fd, recv_msg, sizeof(recv_msg), 0);
				}
				tok = strtok(NULL, " ");
			}
			send(client_fd, "msg", sizeof("msg"), 0);
			recv(client_fd, recv_msg, sizeof(recv_msg), 0);
			strcpy(send_msg, input.c_str()+idx);
			send(client_fd, send_msg, sizeof(send_msg), 0);
			recv(client_fd, recv_msg, sizeof(recv_msg), 0);

			//synchronize
			send(client_fd, "wait", sizeof("wait"), 0);

			if (!no_receiver) {
				while (1) {
					recv(client_fd, recv_msg, sizeof(recv_msg), 0);
					send(client_fd, "ok", sizeof("ok"), 0);
					if ( !strcmp(recv_msg, "done") ) break;
					else if ( !strcmp(recv_msg, "off") ) {
						recv(client_fd, recv_msg, sizeof(recv_msg), 0);
						send(client_fd, "ok", sizeof("ok"), 0);
						cout << "<user " << recv_msg << " is off-line. The message will be passed when he comes back.>" << endl;
					}
				}
			}
			else {
				recv(client_fd, recv_msg, sizeof(recv_msg), 0);
				send(client_fd, "ok", sizeof("ok"), 0);
			}
		}//bye
		else if (action == 1) {
			send(client_fd, "bye", sizeof("bye"), 0);
			recv(client_fd, recv_msg, sizeof(recv_msg), 0);
			cout << "<loading unread msg before logout...>" << endl;
			bye = true;

			//synchronize
			send(client_fd, "wait", sizeof("wait"), 0);
		}//load
		else {
			send(client_fd, "load", sizeof("load"), 0);
			recv(client_fd, recv_msg, sizeof(recv_msg), 0);
			send(client_fd, "wait", sizeof("wait"), 0);
		}
	}

    close(client_fd);
    return 0;
}
