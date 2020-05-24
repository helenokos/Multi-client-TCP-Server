#include <iostream>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/mman.h>
#include <stdlib.h>
#include <time.h>
#include <ctime>

using namespace std;

int main() {
    //socket
	int server_fd;
	server_fd = socket(AF_INET, SOCK_STREAM, 0);
	if (server_fd < 0) {
		perror("server socket fail");
		exit(1);
	}
	
	//bind
	sockaddr_in srv;
	bzero(&srv, sizeof(srv));
	srv.sin_family = AF_INET;
	srv.sin_port = htons(10000);
	srv.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(server_fd, (sockaddr*)&srv, sizeof(srv)) < 0) {
		perror("bind fail");
		exit(1);
	}
	
	//listen
	if (listen(server_fd, 20) < 0) {
		perror("listen fail");
		exit(1);
	}
	
	//accept
	printf("No client connected yet\n");
	int newfd;
	sockaddr_in cli;
	socklen_t cli_len = sizeof(cli);

	//variable for inter-process communication
	int *que_idx = (int*)mmap(NULL, 3*sizeof(int), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	char (*msg_queue)[100][1000] = (char (*)[100][1000])mmap(NULL, 3*100*1000*sizeof(char), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	bool *online_member = (bool*)mmap(NULL, 3*sizeof(bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	bool *event_occur = (bool*)mmap(NULL, 3*sizeof(bool), PROT_READ | PROT_WRITE, MAP_SHARED | MAP_ANONYMOUS, -1, 0);
	memset(que_idx, 0, sizeof(que_idx));
	memset(msg_queue, '\0', sizeof(msg_queue));
	memset(online_member, 0, sizeof(online_member));
	memset(event_occur, 0, sizeof(event_occur));
	while(1) {
		newfd = accept(server_fd, (sockaddr*)&cli, &cli_len);
		if (newfd < 0) {
			perror("accept fail");
			exit(EXIT_FAILURE);
		}
		else {
			pid_t accept_pid = fork();
			switch (accept_pid) {
			case -1 : {
				perror("accept fork");
				break;
			}
			case 0 : {
				close(server_fd);
				//get user name from client
				char recv_msg[1000], send_msg[1000], user_name[20];
				memset(send_msg, '\0', sizeof(send_msg));
				memset(recv_msg, '\0', sizeof(recv_msg));
				memset(user_name, '\0', sizeof(user_name));
				recv(newfd, user_name, sizeof(user_name), 0);
				send(newfd, "ok", sizeof("ok"), 0);

				//convert user name to id
				int id;
				char id_to_name[3][20] = {"Alice", "Bill", "Caesar"};
				for (int i = 0; i < 3; ++i) {
					if ( !strcmp(user_name, id_to_name[i]) ) {
						id = i;
						break;
					}
				}

				//synchronize
				recv(newfd, recv_msg, sizeof(recv_msg), 0);

				//set online and inform other online members
				for (int i = 0; i < 3; ++i) {
					//not yet loggin
					if ( i == id && !online_member[i]) {
						id = i;
						online_member[i] = true;
						cout << "A client \"" << user_name << "\" has connected via \"" << inet_ntoa(cli.sin_addr) << ":"  << cli.sin_port << "\" using SOCK_STREAM(TCP)" << endl;
					}//already login => fail to login								 
					else if ( i == id && online_member[i]) {
						send(newfd, "fail", sizeof("fail"), 0);
						recv(newfd, recv_msg, sizeof(recv_msg), 0);
						close(newfd);
						exit(EXIT_FAILURE);
					}//sb login before you loggin, and you have to inform he/she
					else if (online_member[i] && i != id) {
						event_occur[i] = true;
						strcpy(send_msg, "<User ");
						strcat(send_msg, id_to_name[id]);
						strcat(send_msg, " is on-line, IP address: ");
						strcat(send_msg, inet_ntoa(cli.sin_addr));
						strcat(send_msg, ">");
						strcpy(msg_queue[i][que_idx[i]], send_msg);
						que_idx[i] = (que_idx[i]+1)%100;
					}
				}
				
				//if client login success, then send "login" to client
				send(newfd, "login", sizeof("login"), 0);
				recv(newfd, recv_msg, sizeof(recv_msg), 0);					

				//a loop to keep interacting to client
				bool bye = false;
				while(1) {
					//send msg to client
					if (event_occur[id]) {
						send(newfd, "start", sizeof("start"), 0);
						recv(newfd, recv_msg, sizeof(recv_msg), 0);
						for(int i = 0; i < que_idx[id]; ++i) {
							send(newfd, msg_queue[id][i], sizeof(msg_queue[id][i]), 0);
							recv(newfd, recv_msg, sizeof(recv_msg), 0);
						}
						send(newfd, "done", sizeof("done"), 0);
						recv(newfd, recv_msg, sizeof(recv_msg), 0);
						event_occur[id] = false;
						que_idx[id] = 0;
					}//no msg
					else {
						send(newfd, "no", sizeof("no"), 0);
						recv(newfd, recv_msg, sizeof(recv_msg), 0);
					}

					if (bye) {							
						strcpy(send_msg, "<User ");
						strcat(send_msg, id_to_name[id]);
						strcat(send_msg, " is off-line>");
						for (int i = 0; i < 3; ++i) {
							if (i != id && online_member[i]) {
								event_occur[i] = true;
								strcpy(msg_queue[i][que_idx[i]], send_msg);
								que_idx[i] = (que_idx[i]+1) % 100;
							}
						}
						online_member[id] = false;
						memset(msg_queue[id], '\0', sizeof(msg_queue[id]));
						que_idx[id] = 0;
						event_occur[id] = false;
						close(newfd);
						cout << "User " << id_to_name[id] << " is off-line" << endl;
						break;
					}

					//synchronize
					send(newfd, "wait", sizeof("wait"), 0);

					//recv msg from client
					recv(newfd, recv_msg, sizeof(recv_msg), 0);						
					send(newfd, "ok", sizeof("ok"), 0);						

					if ( !strcmp(recv_msg, "name") ) {
						char author[20];
						bool receiver[3];
						memset(author, '\0', sizeof(author));
						memset(receiver, 0, sizeof(receiver));
						recv(newfd, author, sizeof(author), 0);
						send(newfd, "ok", sizeof("ok"), 0);
						while (1) {
							recv(newfd, recv_msg, sizeof(recv_msg), 0);
							send(newfd, "ok", sizeof("ok"), 0);
							if ( !strcmp(recv_msg, "msg") ) break;
							else if ( !strcmp(recv_msg, "Alice") ) receiver[0] = true;
							else if ( !strcmp(recv_msg, "Bill") ) receiver[1] = true;
							else if ( !strcmp(recv_msg, "Caesar") ) receiver[2] = true;
						}
						recv(newfd, recv_msg, sizeof(recv_msg), 0);
						strcpy(send_msg, "<User ");
						strcat(send_msg, author);
						strcat(send_msg, " has send you a message ");
						strcat(send_msg, recv_msg);
						strcat(send_msg, " at ");
						time_t now;
						time(&now);
						strncat(send_msg, ctime(&now), strlen(ctime(&now))-1);
						char tmp[1000];
						memset(tmp, '\0', sizeof(tmp));
						strcpy(tmp, send_msg);
						strcat(tmp, ">");
						strcpy(send_msg, tmp);
						send(newfd, "ok", sizeof("ok"), 0);

						//synchronize
						recv(newfd, recv_msg, sizeof(recv_msg), 0);

						for (int i = 0; i < 3; ++i) {
							if (receiver[i]) {
								event_occur[i] = true;
								strcpy(msg_queue[i][que_idx[i]], send_msg);									
								que_idx[i] = (que_idx[i]+1) % 100;
								//online
								if (online_member[i]) {
									send(newfd, "on", sizeof("on"), 0);
									recv(newfd, recv_msg, sizeof(recv_msg), 0);
								}//offline
								else {
									send(newfd, "off", sizeof("off"), 0);
									recv(newfd, recv_msg, sizeof(recv_msg), 0);
									send(newfd, id_to_name[i], sizeof(id_to_name[i]), 0);
									recv(newfd, recv_msg, sizeof(recv_msg), 0);
								}
							}
						}
						send(newfd, "done", sizeof("done"), 0);
					}//bye
					else if ( !strcmp(recv_msg, "bye") ) {
						bye = true;
					}
					recv(newfd, recv_msg, sizeof(recv_msg), 0);
				}

				exit(EXIT_SUCCESS);
				break;
			}//another fork for accept
			default : {
				close(newfd);
				continue;			
			}
			}
		}
	}

	close(server_fd);
    return 0;
}
