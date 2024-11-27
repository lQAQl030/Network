#include "../unp.h"
#include <bits/stdc++.h>
using namespace std;

bool isNumber(const string &str)
{
	return str.find_first_not_of("0123456789") == string::npos;
}

void clr_scr()
{
	printf("\x1B[2J");
};

void set_scr()
{ // set screen to 80 * 25 color mode
	printf("\x1B[=3h");
};

int Read(int fd, string &s)
{
	int n = MAXLINE;
	char buffer[MAXLINE];
	memset(buffer, 0, sizeof(buffer));
	int count = read(fd, buffer, n);
	// printf("buff: %s\n", buffer);
	s = string(buffer);
	return (count) ? s.length() : 0;
}

int Write(int fd, string s)
{
	s.insert(0, "$");

	if (s.back() == '\n')
		s.back() = '\0';
	else
		s += "\0";

	return write(fd, s.c_str(), s.length());
}

void show_hand(vector<pair<int,int>> &hand){
	for(auto [color, number] : hand){
		cout << "\x1b[1;" + to_string(color+31) + "m" + to_string(number) + "\x1b[0m ";
	}
	cout << endl;
}

void show_curr_card(pair<int,int> &currcard){
	if(currcard.first == -1){
		cout << "current card: (empty)" << endl;
		return;
	}
	cout << "current card: \x1b[1;" + to_string(currcard.first+31) + "m" + to_string(currcard.second) + "\x1b[0m" << endl;
}

void display(vector<pair<int,int>> &hand, pair<int,int> &currcard){
	show_hand(hand);
	show_curr_card(currcard);
}

void menu(FILE *fp, int sockfd)
{
	set_scr();
	clr_scr();
	// FUNCTION init
	string sendline, recvline, student, cliip, outmsg;
	string state = "MENU";
	map<string,string> gui;
	gui["menu"] = "===========================\nMENU\n- login\n- register\n- exit\n";
	gui["lobby"] = "===========================\nLOBBY\n- create\n- join <roomid>\n- logout\n- exit\n";
	gui["register"] = "===========================\nRegister\n- back\n- <username> <password>\n- exit\n";
	gui["login"] = "===========================\nLogin\n- back\n- <username> <password>\n- exit\n";
	gui["room"] = "===========================\nROOM\n- back\n- start (only host can do this)\n- exit\n";
	gui["exit"] = "===========================\n--Server has disconnect you\n--Bye~ :D\n";
	gui["game"] = "===========================\nGAME\n";

	// GAME init
	#define RED 0
	#define GREEN 1
	#define YELLOW 2
	#define BLUE 3
	#define WILD 4
	#define SKIP 10
	#define TURN 11
	#define ADD2 12
	#define COLOR 13
	#define ADD4 14
	vector<pair<int,int>> hand;
	pair<int,int> currcard;
	bool recvHand = false;
	bool recvCard = false;

	// SELECT init
	int maxfdp1, stdineof;
	fd_set rset;

	stdineof = 0;
	FD_ZERO(&rset);
	for (;;)
	{
		if (stdineof == 0)
			FD_SET(fileno(fp), &rset);
		FD_SET(sockfd, &rset);
		maxfdp1 = max(fileno(fp), sockfd) + 1;
		select(maxfdp1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(sockfd, &rset))
		{ /* socket is readable */
			recvline.clear();
			if (Read(sockfd, recvline) == 0)
			{
				if (stdineof == 1)
					return; /* normal termination */
				else
					printf("Server Close\n");
				return;
			}
			else
			{
				stringstream ss(recvline);
				string command;
				while (getline(ss, command, '@'))
				{
					if (state == "GAME"){
						if(command == "clr"){
							clr_scr();
						}else if(command == "lobby"){
							state = "LOBBY";
							hand.clear();
							cout << gui[command];
						}else if(command == "hand"){
							recvHand = true;
						}else if(command == "card"){
							recvCard = true;
						}else if(command == "disp"){
							display(hand, currcard);
						}else{
							if(command.empty()) continue;
							if(recvHand){
								stringstream sshand(command);
								int color, number;
								while(sshand >> color >> number){
									hand.push_back({color, number});
								}
								recvHand = false;
								continue;
							}
							if(recvCard){
								stringstream sscard(command);
								sscard >> currcard.first >> currcard.second;
								recvCard = false;
								continue;
							}
							cout << command << endl;
						}

						continue;
					}

					if (command == "clr")
					{
						clr_scr();
					}
					else if (command == "exit")
					{
						cout << gui[command];
						close(sockfd);
						return;
					}
					else if (command == "game"){
						cout << gui[command];
						state = "GAME";
					}
					else if (gui.find(command) != gui.end())
					{
						cout << gui[command];
						transform(command.begin(), command.end(), command.begin(), ::toupper);
						state = command;
						
					}
					else
					{
						if (command.empty())
							continue;
						cout << command << '\n';
					}
				}
			}
		}

		if (FD_ISSET(fileno(fp), &rset))
		{ /* input is readable */
			char buffer[MAXLINE];
			if (fgets(buffer, MAXLINE, fp) == NULL)
			{
				printf("(leaving...)\n");
				stdineof = 1;
				shutdown(sockfd, SHUT_WR);
			}
			sendline = string(buffer);
			if (state == "GAME" && sendline.size() > 0 && sendline[0] == '#'){
				stringstream sscard(sendline.substr(1));
				pair<int,int> sentcard;
				sscard >> sentcard.first >> sentcard.second;
				auto it = std::find(hand.begin(), hand.end(), sentcard);
				if(it == hand.end()){
					cout << "You don't have that card" << endl;
					continue;
				}else if(currcard.first == -1 || sentcard.first == WILD || sentcard.first == currcard.first || sentcard.second == currcard.second){
					hand.erase(it);
				}
			}

			Write(sockfd, sendline);
		}
	}
}

int main(int argc, char **argv)
{
	int sockfd;
	struct sockaddr_in servaddr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(15023);
	inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
	// inet_pton(AF_INET, "140.113.235.151", &servaddr.sin_addr);

	connect(sockfd, (SA *)&servaddr, sizeof(servaddr));

	menu(stdin, sockfd); /* do it all */

	exit(0);
}
