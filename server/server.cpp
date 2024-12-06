/* include fig01 */
#include "../unp.h"
#include "Timer.hpp"
#include <bits/stdc++.h>
using namespace std;

#define RED 1
#define GREEN 2
#define YELLOW 3
#define BLUE 4
#define WILD 5
#define SKIP 10
#define TURN 11
#define ADD2 12
#define COLOR 13
#define ADD4 14

// variables
int client[FD_SETSIZE];
int maxi;
string sendline, recvline;
string cliip[FD_SETSIZE];
string state[FD_SETSIZE] = {"NONE"};
string isLogin[FD_SETSIZE] = {""};
map<string, string> user;
map<int, vector<int>> gameroom;
map<int, vector<string>> gamechat;
int inRoom[FD_SETSIZE];

bool isNumber(const string &str)
{
	return str.find_first_not_of("0123456789") == string::npos;
}

int Read(int fd, string &s)
{
	char buffer[MAXLINE];
	memset(buffer, 0, MAXLINE);
	int count = read(fd, buffer, MAXLINE);
	if (count == 0)
		return 0;
	s = string(buffer);
	return s.length();
}

void Write(int fd, string s)
{
	if(s[0] != '%') s.insert(0, "@clr");
	else s.erase(s.begin());

	if (s.back() == '\n')
		s.back() = '\0';
	else
		s += "\0";

	write(fd, s.c_str(), s.length());
	s.clear();
	return;
}

string playerlist(int myself)
{
	bool isZero = true;
	string list = "@╔════════════════════╦════════════════════╗\n";
	list += "  ║player              ║status              ║\n";
	list += "  ╠════════════════════╬════════════════════╣\n";
	for (int i = 0; i <= maxi; i++)
	{
		if (isLogin[i].empty() || i == myself)
			continue;
		isZero = false;
		string username = isLogin[i], status = state[i];
		username.resize(20, ' ');
		status.resize(20, ' ');
		list += "  ║" + username + "║" + status + "║\n";
	}
	if (isZero)
		list += "  ║             no player online            ║\n";
	list += "  ╚════════════════════╩════════════════════╝\n";
	return list;
}

string roomlist()
{
	bool isZero = true;
	string list = "@╔════════════════════╦════════════════════╗\n";
	list += "  ║Room                ║status              ║\n";
	list += "  ╠════════════════════╬════════════════════╣\n";
	for (auto [host, room] : gameroom)
	{
		isZero = false;
		string username = "[" + to_string(host) + "] " + isLogin[host];
		string status = to_string(room.size()) + "/ 4 players";
		if(state[host] == "IN GAME") status += " [GAME]";
		username.resize(20, ' ');
		status.resize(20, ' ');
		list += "  ║" + username + "║" + status + "║\n";
	}
	if (isZero)
		list += "  ║             no room available           ║\n";
	list += "  ╚════════════════════╩════════════════════╝\n";
	return list;
}

void notifyLobby(int i)
{
	// if broadcast all
	if (i == -1)
	{
		for (int j = 0; j <= maxi; j++)
		{
			if (client[j] < 0 || state[j] != "LOBBY")
				continue;
			Write(client[j], roomlist() + playerlist(j) + "@lobby");
		}
	}
	else
	{
		for (int j = 0; j <= maxi; j++)
		{
			if (client[j] < 0 || client[j] == client[i] || state[j] != "LOBBY")
				continue;
			Write(client[j], roomlist() + playerlist(j) + "@lobby");
		}
	}
}

string displayGameroom(int gameroom_id)
{
	bool isZero = true;
	string list = "@╔════════════════════╦════════════════════╗\n";
	list += "  ║Player              ║status              ║\n";
	list += "  ╠════════════════════╬════════════════════╣\n";
	for (auto cli : gameroom[gameroom_id])
	{
		isZero = false;
		string username = isLogin[cli];
		string status = "Waiting";
		username.resize(20, ' ');
		status.resize(20, ' ');
		list += "  ║" + username + "║" + status + "║\n";
	}
	if (isZero)
		list += "  ║             no player in room           ║\n";
	list += "  ╚════════════════════╩════════════════════╝\n";
	return list;
}

void notifyGameroom(int gameroom_id)
{
	for (auto cli : gameroom[gameroom_id])
	{
		if(state[cli] == "WAITING") Write(client[cli], "@player joined\n" + displayGameroom(gameroom_id) + "@room");
	}
}

void hostExit(int i)
{
	if (inRoom[i] == i)
	{
		if(gameroom.find(i) == gameroom.end()) return;
		gameroom[i].erase(gameroom[i].begin()); // erase host to avoid write to void
		for (auto cli : gameroom[i])
		{
			if(cli == -1) continue;
			state[cli] = "LOBBY";
			inRoom[cli] = -1;
		}
		gameroom.erase(gameroom.find(i));
		gamechat.erase(gamechat.find(i));
		inRoom[i] = -1;
	}
}

void notifyResult(int i)
{
	for (auto cli : gameroom[i])
	{
		state[cli] = "RESULT";
		inRoom[cli] = -1;
	}
	gameroom.erase(gameroom.find(i));
	gamechat.erase(gamechat.find(i));
	inRoom[i] = -1;
}

string num2sym(int number){
	string symbol;
	if(number == SKIP){
		symbol = "Φ ";
	}else if(number == TURN){
		symbol = "╰╮";
	}else if(number == ADD2){
		symbol = "+2";
	}else if(number == COLOR){
		symbol = "⊕ ";
	}else if(number == ADD4){
		symbol = "+4";
	}else{
		symbol = to_string(number) + " ";
	}
	return symbol;
}

string gameInfoClient(int host, vector<pair<int,int>> table, vector<vector<pair<int,int>>> hands){
	string info = "@";

	// info += "Table:\n";
	// for(auto card : table){
	// 	info += "\x1b[1;" + to_string(card.first+30) + "m" + num2sym(card.second) + "\x1b[0m ";
	// }
	// info += "\n";

	info += "Player Hands:\n";
	for(int player = 0 ; player < 4 ; player++){
		info += "  (" + isLogin[gameroom[host][player]] + "): ";
		for(auto card : hands[player]){
			info += "■ ";
		}
		info += "(" + to_string(hands[player].size()) + ")\n";
	}

	return info;
}

string gameInfo(int host, vector<pair<int,int>> deck, vector<pair<int,int>> table, vector<vector<pair<int,int>>> hands, pair<int,int> currcard, int time_elapsed){
	string info = "@==========STATUS=========\n";
	info += "DECK:\n";
	for(auto card : deck){
		info += "\x1b[1;" + to_string(card.first+30) + "m" + num2sym(card.second) + "\x1b[0m ";
	}
	info += "\n";

	info += "Table:\n";
	for(auto card : table){
		info += "\x1b[1;" + to_string(card.first+30) + "m" + num2sym(card.second) + "\x1b[0m ";
	}
	info += "\n";

	info += "Player Hands:\n";
	for(int player = 0 ; player < 4 ; player++){
		info += "(" + isLogin[gameroom[host][player]] + "): ";
		for(auto card : hands[player]){
			info += "\x1b[1;" + to_string(card.first+30) + "m" + num2sym(card.second) + "\x1b[0m ";
		}
		info += "\n";
	}
	

	info += "Current Card:\n";
	info += "\x1b[1;" + to_string(currcard.first+30) + "m" + num2sym(currcard.second) + "\x1b[0m\n";

	info += "Elapsed time:\n";
	info += to_string(time_elapsed) + "\n";
	
	info += "=========================\n";

	return info;
}

void *gamethread(void *arg){
	int host = *((int*) arg);
	delete (int*)arg;
	pthread_detach(pthread_self());

	// RNG
	auto rd = std::random_device {}; 
	auto rng = std::default_random_engine { rd() };

	// local variables
	int currentPlayer = 0;
	vector<string> chatroom;
	vector<vector<pair<int,int>>> hands(4);
	vector<pair<int,int>> table;
	vector<pair<int,int>> deck;
	pair<int,int> currcard;
	bool direction = true;
	bool statusUpdate = false;
	int addCardBuff = 0;

	// initialize the deck
	for(int color = 1 ; color <= 4 ; color++){
		for(int number = 0 ; number <= 12 ; number++){
			deck.push_back({color, number});
			deck.push_back({color, number});
		}
	}
	for(int i = 0 ; i < 4 ; i++){
		deck.push_back({WILD, COLOR});
		deck.push_back({WILD, ADD4});
	}
	shuffle(deck.begin(), deck.end(), rng);

	// initilaize hands
	for(int player = 0 ; player < 4 ; player++){
		string hand_str = "@hand@";
		for(int i = 0 ; i < 7 ; i++){
			hands[player].push_back(deck.back());
			hand_str += to_string(deck.back().first) + " " + to_string(deck.back().second) + " ";
			deck.pop_back();
		}
		Write(client[gameroom[host][player]], hand_str);
	}

	// init msg
	for(int player = 0 ; player < 4 ; player++){
		Write(client[gameroom[host][player]], "@card@-1 -1" + gameInfoClient(host, table, hands) + "@disp");
	}
	Write(client[gameroom[host][0]], "%@turn");

	// timer set up
	F2::Timer t(1000); // ms every tick
	t.start(); // Starting the timer

	while(t.get_tick() >= 0 && gameroom.find(host) != gameroom.end()) // Running for [num] secs
	{
		if(t.updated()){
			for(int player = 0 ; player < gamechat[host].size() ; player++){
				if(!gamechat[host][player].empty()){
					statusUpdate = true;
					// cout << "recv msg: " << gamechat[host][player] << endl;
					string command = gamechat[host][player];
					gamechat[host][player].clear();

					if(player == currentPlayer){
						if(command[0] == '#'){
							string cardcommand = command.substr(1);
							if(cardcommand == "draw"){
								cout << "Player (" + isLogin[gameroom[host][player]] + ") draw " << ((addCardBuff) ?addCardBuff :1) << " cards" << endl;
								string hand_str = "%@hand@";
								int drawcnt = (addCardBuff) ?addCardBuff :1;
								for(int j = 0 ; j < drawcnt ; j++){
									if(deck.empty()){
										shuffle(table.begin(), table.end(), rng);
										deck.insert(deck.end(), table.begin(), table.end());
										table.clear();
									}
									hands[player].push_back(deck.back());
									hand_str += to_string(deck.back().first) + " " + to_string(deck.back().second) + " ";
									deck.pop_back();
								}
								addCardBuff = 0;
								Write(client[gameroom[host][player]], hand_str);
							}else{
								stringstream sscard(cardcommand);
								sscard >> currcard.first >> currcard.second;

								auto cardpos = std::find(hands[player].begin(), hands[player].end(), currcard);
								hands[player].erase(cardpos);
								table.push_back(currcard);
								cout << "Player (" + isLogin[gameroom[host][player]] + ") sent " << currcard.first << " " << currcard.second << endl;

								// end sequence
								if(hands[player].empty()){
									string info = gameInfo(host, deck, table, hands, currcard, t.get_tick());
									info += "Winner:\n" + isLogin[gameroom[host][player]] + "\n";

									for(auto cli : gameroom[host]){
										Write(client[cli], info + "@win");
									}

									notifyResult(host);
									notifyLobby(-1);

									return NULL;
								}

								// function card
								if(currcard.second == SKIP){
									if(direction){
										currentPlayer = (currentPlayer + 2) % 4;
									}else{
										currentPlayer -= 2;
										if(currentPlayer < 0) currentPlayer += 4;
									}
									continue;
								}else if(currcard.second == TURN){
									direction = !direction;
								}else if(currcard.second == ADD2){
									addCardBuff += 2;
								}else if(currcard.second == COLOR){
									sscard >> currcard.first;
								}else if(currcard.second == ADD4){
									addCardBuff += 4;
									sscard >> currcard.first;
								}
							}

							currentPlayer += (direction) ?1 :-1;
							if(currentPlayer < 0) currentPlayer += 4;
							currentPlayer %= 4;
							continue;
						}
					}

					if(command == "leave"){
						string info = gameInfo(host, deck, table, hands, currcard, t.get_tick());
						info += "Player Quit: " + isLogin[gameroom[host][player]] + "\n";

						for(auto cli : gameroom[host]){
							Write(client[cli], info + "@win");
						}

						notifyResult(host);
						notifyLobby(-1);

						return NULL;
					}

					chatroom.push_back("(" + isLogin[gameroom[host][player]] + "): " + command );
				}
			}

			// current status
			if(statusUpdate){
				// debug
				// cout << gameInfo(host, deck, table, hands, currcard, t.get_tick());

				// game
				statusUpdate = false;
				string historyChat = "@";
				string info = gameInfoClient(host, table, hands);
				for(auto chat : chatroom){
					historyChat += chat + "\n";
				}
				for(int player = 0 ; player < 4 ; player++){
					Write(client[gameroom[host][player]], "%@card@" + to_string(currcard.first) + " " + to_string(currcard.second));
				}
				for(int player = 0 ; player < 4 ; player++){
					Write(client[gameroom[host][player]], historyChat + info + "@disp");
				}
				string sentCurrent = "%@turn";
				if(addCardBuff){
					sentCurrent += "@add";
				}
				Write(client[gameroom[host][currentPlayer]], sentCurrent);
			}
		}
	}

	gameexit:
	// not normal
	state[host] = "LOBBY";
	hostExit(host);
	notifyLobby(-1);

	return NULL;
}

int main(int argc, char **argv)
{
	int i, maxfd, listenfd, connfd, sockfd;
	int nready;
	ssize_t n;
	fd_set rset, allset;
	socklen_t clilen;
	struct sockaddr_in cliaddr, servaddr;
	pthread_t p[FD_SETSIZE];

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
	memset(inRoom, -1, FD_SETSIZE*sizeof(int));

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
				n = Read(client[i], recvline);
				stringstream ss(recvline);
				string command;
				if (n == 0)
				{
					goto userexit;
				}
				
				// handle user command
				while (getline(ss, command, '$'))
				{
					if (command.empty())
						continue;
					
					// cout << state[i] << " " << command << endl; // enable debug

					if (state[i] == "MENU")
					{
						if (command == "login")
						{
							state[i] = "LOGIN";
							Write(client[i], "@login");
						}
						else if (command == "register")
						{
							state[i] = "REGISTER";
							Write(client[i], "@register");
						}
						else if (command == "exit")
						{
							Write(client[i], "@exit");
							goto userexit;
						}
						else
						{
							Write(client[i], "@\"" + command + "\" is not a valid command\n@menu");
						}
						continue;
					}

					if (state[i] == "REGISTER")
					{
						if (command == "back")
						{
							state[i] = "MENU";
							Write(client[i], "@menu");
						}
						else if (command == "exit")
						{
							Write(client[i], "@exit");
							goto userexit;
						}
						else
						{
							stringstream ss(command);
							string username, password, susername, spassword;
							ss >> username >> password;

							if (username.empty() || password.empty())
							{
								Write(client[i], "@username or password empty@register");
								continue;
							}

							fstream fuserin, fuserout;
							fuserin.open("user.txt", ios::in);
							fuserout.open("user.txt", ios::out | ios::app);
							bool isNewUser = true;
							while (fuserin >> susername >> spassword)
							{
								if (username == susername)
								{
									isNewUser = false;
									Write(client[i], "@username existed@register");
									break;
								}
							}
							if (!isNewUser)
							{
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

					if (state[i] == "LOGIN")
					{
						if (command == "back")
						{
							state[i] = "MENU";
							Write(client[i], "@menu");
						}
						else if (command == "exit")
						{
							Write(client[i], "@exit");
							goto userexit;
						}
						else
						{
							stringstream ss(command);
							string username, password, susername, spassword;
							ss >> username >> password;

							bool isUserOnline = false;
							for (int j = 0; j <= maxi; j++)
							{
								if (username == isLogin[j])
								{
									isUserOnline = true;
									Write(client[i], "@this account is already logged in@login");
									break;
								}
							}
							if (isUserOnline)
								continue;

							fstream fuserin;
							fuserin.open("user.txt", ios::in);
							bool isUserInData = false;
							while (fuserin >> susername >> spassword)
							{
								if (username == susername)
								{
									isUserInData = true;
									if (password == spassword)
									{
										state[i] = "LOBBY";
										isLogin[i] = username;
										Write(client[i], "@login successful\n" + roomlist() + playerlist(i) + "@lobby");
										notifyLobby(i);
									}
									else
									{
										Write(client[i], "@password wrong@login");
									}
									break;
								}
							}
							if (!isUserInData)
							{
								Write(client[i], "@username not exist@login");
							}
						}
						continue;
					}

					if (state[i] == "LOBBY")
					{
						if (command == "create")
						{
							state[i] = "WAITING";
							gameroom[i].push_back({i});
							gamechat[i].push_back({""});
							inRoom[i] = i;
							Write(client[i], "@gameroom create successful\n" + displayGameroom(i) + "@room");
							notifyLobby(i);
						}
						else if (command.find("join") != string::npos)
						{
							stringstream ss(command);
							string temp;
							int roomid = 0;
							ss >> temp >> roomid;

							if(roomid == 0){
								if(gameroom.find(roomid) == gameroom.end()){
									bool isRoomFind = false;
									for(auto [host, room] : gameroom){
										if(room.size() < 4){
											isRoomFind = true;
											roomid = host;
											break;
										}
									}
									if(!isRoomFind){
										Write(client[i], "@gameroom is not found!" + roomlist() + playerlist(i) + "@lobby");
										continue;
									}
								}
							}

							if (gameroom.find(roomid) == gameroom.end())
							{
								Write(client[i], "@gameroom is not exist!" + roomlist() + playerlist(i) + "@lobby");
							}
							else
							{
								if (gameroom[roomid].size() == 4)
								{
									Write(client[i], "@gameroom is full!" + roomlist() + playerlist(i) + "@lobby");
								}
								else
								{
									state[i] = "WAITING";
									gameroom[roomid].push_back(i);
									gamechat[roomid].push_back("");
									inRoom[i] = roomid;
									notifyGameroom(roomid);
								}
							}
							notifyLobby(i);
						}
						else if (command == "logout")
						{
							isLogin[i].clear();
							Write(client[i], "@logout successful\n@menu");
							state[i] = "MENU";
							notifyLobby(i);
						}
						else if (command == "exit")
						{
							Write(client[i], "@exit");
							goto userexit;
						}
						else
						{
							Write(client[i], "@\"" + command + "\" is not a valid command\n" + roomlist() + playerlist(i) + "@lobby");
						}
						continue;
					}

					if (state[i] == "WAITING")
					{
						if (command == "back")
						{
							// cout << "inRoom[i] = " << inRoom[i] << ", i = " << i << endl;
							state[i] = "LOBBY";
							if (inRoom[i] == i)
							{
								hostExit(i);
								// cout << "finish exit" << endl;
							}
							else
							{
								for (auto iter = gameroom[inRoom[i]].begin(); iter != gameroom[inRoom[i]].end(); iter++)
								{
									if (*iter == i)
									{
										gameroom[inRoom[i]].erase(iter);
										break;
									}
								}
								notifyGameroom(inRoom[i]);
								inRoom[i] = -1;
							}
							notifyLobby(-1);
						}
						else if (command == "start")
						{
							if (i != inRoom[i])
							{
								Write(client[i], "@you are not the host!\n" + displayGameroom(inRoom[i]) + "@room");
							}
							else
							{
								if(gameroom[i].size() < 4){
									Write(client[i], "@wait for more players to start...\n" + displayGameroom(inRoom[i]) + "@room");
								}else{
									for(auto cli : gameroom[i]){
										state[cli] = "IN GAME";
										Write(client[cli], "@game");
									}
									notifyLobby(-1);

									int *iptr = new int(i);
									pthread_create(&p[i], NULL, gamethread, iptr);
								}
							}
						}
						else if (command == "exit")
						{
							Write(client[i], "@exit");
							goto userexit;
						}
						else
						{
							Write(client[i], "@\"" + command + "\" is not a valid command\n" + displayGameroom(inRoom[i]) + "@room");
						}
						continue;
					}

					if (state[i] == "IN GAME"){
						for(int j = 0 ; j < 4 ; j++){
							if(gameroom[inRoom[i]][j] == i){
								gamechat[inRoom[i]][j] = command;
								break;
							}
						}
					}

					if (state[i] == "RESULT")
					{
						if (command == "back")
						{
							state[i] = "LOBBY";
							notifyLobby(-1);
						}
						else if (command == "exit")
						{
							Write(client[i], "@exit");
							goto userexit;
						}
						else
						{
							Write(client[i], "@\"" + command + "\" is not a valid command\n" + roomlist() + playerlist(i) + "@lobby");
						}
						continue;
					}
				}

				if (--nready <= 0)
					break; /* no more readable descriptors */

				continue;

				userexit:
				// user exit template
				cout << "--player from [" + cliip[i] + "] disconnected--\n";
				close(client[i]);
				FD_CLR(client[i], &allset);
				hostExit(i);
				if (inRoom[i] != -1)
				{
					if(state[i] == "IN GAME"){
						hostExit(inRoom[i]);
					}else{
						for (auto iter = gameroom[inRoom[i]].begin(); iter != gameroom[inRoom[i]].end(); iter++)
						{
							if (*iter == i)
							{
								gameroom[inRoom[i]].erase(iter);
								break;
							}
						}
						notifyGameroom(inRoom[i]);
						inRoom[i] = -1;
					}
				}
				client[i] = -1;
				state[i] = "NONE";
				isLogin[i].clear();
				notifyLobby(i);
			}
		}
	}
}
/* end fig02 */