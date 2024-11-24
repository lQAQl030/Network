/* include fig01 */
#include "../unp.h"
#include <bits/stdc++.h>
using namespace std;

bool isNumber(const string& str) {
  return str.find_first_not_of("0123456789") == string::npos;
}

int Read(int fd, string &s)
{
	char buffer[MAXLINE];
	int count = read(fd, buffer, MAXLINE);
	if (count == 0)
		return 0;
	s = string(buffer);
	return s.length();
}

void Write(int fd, string s)
{
	s.insert(0, "@clr");
	if (s.back() == '\n')
		s.back() = '\0';
	else
		s += "\0";

	write(fd, s.c_str(), s.length());
	s.clear();
	return;
}

string playerlist(string user[], string state[], int maxi, int myself)
{
	bool isZero = true;
	string list = "@╔════════════════════╦════════════════════╗\n";
	list += "║player              ║status              ║\n";
	list += "╠════════════════════╬════════════════════╣\n";
	for (int i = 0; i <= maxi; i++)
	{
		if (user[i].empty() || i == myself)
			continue;
		isZero = false;
		string username = user[i], status = state[i];
		username.resize(20, ' ');
		status.resize(20, ' ');
		list += "║" + username + "║" + status + "║\n";
	}
	if (isZero)
		list += "║             no player online            ║\n";
	list += "╚════════════════════╩════════════════════╝\n";
	return list;
}

void notifyPlayerlist(int client[], string isLogin[], string state[], int maxi, int i){
	for(int j = 0 ; j <= maxi ; j++){
		if(client[j] < 0 || client[j] == client[i] || state[j] != "LOBBY") continue;
		Write(client[j], playerlist(isLogin, state, maxi, j) + "@lobby");
	}
}

int main(int argc, char **argv)
{
	int i, maxi, maxfd, listenfd, connfd, sockfd;
	int nready, client[FD_SETSIZE];
	ssize_t n;
	fd_set rset, allset;
	char buf[MAXLINE];
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	int one = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(15023);

	if (bind(listenfd, (SA *)&servaddr, sizeof(servaddr)) < 0)
	{
		printf("bind fail...\n");
		exit(0);
	}

	listen(listenfd, LISTENQ);

	signal(SIGCHLD, SIG_IGN);

	maxfd = listenfd; /* initialize */
	maxi = -1;		  /* index into client[] array */
	for (i = 0; i < FD_SETSIZE; i++)
		client[i] = -1; /* -1 indicates available entry */
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);
	/* end fig01 */

	printf("server is running...\n");
	string sendline, recvline, cliip[FD_SETSIZE];
	string state[FD_SETSIZE] = {"NONE"};
	string isLogin[FD_SETSIZE] = {""};
	map<string, string> user;
	vector<vector<int>> gameroom;
	int room_cnt = 0;

	/* include fig02 */
	for (;;)
	{
		rset = allset; /* structure assignment */
		nready = select(maxfd + 1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(listenfd, &rset))
		{ /* new client connection */
			clilen = sizeof(cliaddr);
			connfd = accept(listenfd, (SA *)&cliaddr, &clilen);

			for (i = 0; i < FD_SETSIZE; i++)
				if (client[i] < 0)
				{
					client[i] = connfd; /* save descriptor */
					break;
				}
			if (i == FD_SETSIZE)
				printf("too many clients\n");

			cliip[i] = string(inet_ntoa(cliaddr.sin_addr));
			cout << "--player from [" + cliip[i] + "] connected--\n";
			Write(connfd, "@menu");
			state[i] = "MENU";

			FD_SET(connfd, &allset); /* add new descriptor to set */

			if (connfd > maxfd)
				maxfd = connfd; /* for select */
			if (i > maxi)
				maxi = i; /* max index in client[] array */

			if (--nready <= 0)
				continue; /* no more readable descriptors */
		}

		for (i = 0; i <= maxi; i++)
		{ /* check all clients for data */
			if (client[i] < 0)
				continue;
			if (FD_ISSET(client[i], &rset))
			{
				recvline.clear();
				if ((n = Read(client[i], recvline)) == 0)
				{
					// user quit template
					cout << "--player from [" + cliip[i] + "] disconnected--\n";
					close(client[i]);
					FD_CLR(client[i], &allset);
					client[i] = -1;
					state[i] = "NONE";
					isLogin[i].clear();
					notifyPlayerlist(client, isLogin, state, maxi, i);
				}

				stringstream ss(recvline);
				string command;

				while (getline(ss, command, '$'))
				{
					if(command.empty()) continue;

					if(state[i] == "MENU"){
						if(command == "login"){
							state[i] = "LOGIN";
							Write(client[i], "@login");
						}else if(command == "register"){
							state[i] = "REGISTER";
							Write(client[i], "@register");
						}else if(command == "exit"){
							goto userexit;
						}else{
							Write(client[i], "\"" + command + "\" is not a valid command\n@menu");
						}
						continue;
					}

					if(state[i] == "REGISTER"){
						if(command == "back"){
							state[i] = "MENU";
							Write(client[i], "@menu");
						}else if(command == "exit"){
							goto userexit;
						}else{
							stringstream ss(command);
							string username, password, susername, spassword;
							ss >> username >> password;

							if(username.empty() || password.empty()){
								Write(client[i], "@username or password empty@register");
								continue;
							}

							fstream fuserin, fuserout;
							fuserin.open("user.txt", ios::in);
							fuserout.open("user.txt", ios::out | ios::app);
							bool isNewUser = true;
							while(fuserin >> susername >> spassword){
								if(username == susername){
									isNewUser = false;
									Write(client[i], "@username existed@register");
									break;
								}
							}
							if(!isNewUser){
								fuserin.close();
								fuserout.close();
								continue;
							}

							fuserout << username << " " << password << endl;
							Write(client[i], "@register succeed@menu");
							state[i] = "MENU";

							fuserin.close();
							fuserout.close();
						}
						continue;
					}

					if(state[i] == "LOGIN"){
						if(command == "back"){
							state[i] = "MENU";
							Write(client[i], "@menu");
						}else if(command == "exit"){
							goto userexit;
						}else{
							stringstream ss(command);
							string username, password, susername, spassword;
							ss >> username >> password;

							fstream fuserin;
							fuserin.open("user.txt", ios::in);
							bool isUserInData = false;
							while(fuserin >> susername >> spassword){
								if(username == susername){
									isUserInData = true;
									if(password == spassword){
										state[i] = "LOBBY";
										isLogin[i] = username;
										Write(client[i], "@login successful\n" + playerlist(isLogin, state, maxi, i) + "@lobby");
										notifyPlayerlist(client, isLogin, state, maxi, i);
									}else{
										Write(client[i], "@password wrong@login");
									}
									break;
								}
							}
							if(!isUserInData){
								Write(client[i], "@username not exist@login");
							}
						}
						continue;
					}

					if(state[i] == "LOBBY"){
						if(command == "create"){
							state[i] = "WAITING";
							room_cnt++;
							gameroom.push_back({i,-1,-1,-1});

						}else if(command == "join"){
							
						}else if(command == "logout"){
							isLogin[i].clear();
							Write(client[i], "@logout successful\n@menu");
							state[i] = "MENU";
							notifyPlayerlist(client, isLogin, state, maxi, i);
						}else if(command == "exit"){
							goto userexit;
						}else{
							Write(client[i], "\"" + command + "\" is not a valid command\n");
						}
						continue;
					}
				}

				if (--nready <= 0)
					break; /* no more readable descriptors */
				
				continue;

				userexit:
				// user exit template
				Write(client[i], "@exit");
				cout << "--player from [" + cliip[i] + "] disconnected--\n";
				close(client[i]);
				FD_CLR(client[i], &allset);
				client[i] = -1;
				state[i] = "NONE";
				isLogin[i].clear();
				notifyPlayerlist(client, isLogin, state, maxi, i);
			}
		}
	}
}
/* end fig02 */
