#include "../unp.h"
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

bool isNumber(const string &str)
{
	return str.find_first_not_of("0123456789") == string::npos;
}

bool isNumberS(const string &str)
{
	return str.find_first_not_of("0123456789 ") == string::npos;
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

void show_hand(vector<pair<int,int>> &hand){
	cout << "Your Hand======================" << endl;
	// id
	for(int cnt = 0 ; cnt < hand.size() ; cnt++){
		if(cnt <= 9){
			cout << cnt << "  ";
		}else{
			cout << cnt << " ";
		}
	}
	cout << endl;
	// card
	for(auto [color, number] : hand){
		cout << "\x1b[1;" + to_string(color+30) + "m" + num2sym(number) + "\x1b[0m ";
	}
	cout << endl;
}

void show_curr_card(pair<int,int> &currcard){
	if(currcard.first == -1){
		cout << "current card: (empty)" << endl;
		return;
	}
	cout << "current card: \x1b[1;" + to_string(currcard.first+30) + "m" + num2sym(currcard.second) + "\x1b[0m" << endl;
}

void display(vector<pair<int,int>> &hand, pair<int,int> &currcard){
	show_hand(hand);
	show_curr_card(currcard);
	cout << "Type anything in the chat to chat, or \"leave\" to leave this game" << endl;
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
	gui["win"] = "===========================\nRESULT\n- back\n- exit\n";

	// GAME init
	vector<pair<int,int>> hand;
	pair<int,int> currcard;
	bool recvHand = false;
	bool recvCard = false;
	bool myTurn = false;
	bool beingAdded = false;

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
						}else if(command == "win"){
							state = "WIN";
							hand.clear();
							cout << gui[command];
						}else if(command == "hand"){
							recvHand = true;
						}else if(command == "card"){
							recvCard = true;
						}else if(command == "turn"){
							myTurn = true;
							cout << "It's your turn now\n- use #<card_id> (<color>\x1b[1;31m1\x1b[1;32m2\x1b[1;33m3\x1b[1;34m4\x1b[0m) to play a card\n- #draw to draw" << endl;
						}else if(command == "add"){
							beingAdded = true;
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
			memset(buffer, 0, MAXLINE);
			if (fgets(buffer, MAXLINE, fp) == NULL)
			{
				printf("(leaving...)\n");
				stdineof = 1;
				shutdown(sockfd, SHUT_WR);
			}
			sendline.clear();
			sendline = string(buffer);
			if (state == "GAME" && myTurn && !sendline.empty() && sendline[0] == '#'){
				string cardcommand = sendline.substr(1,sendline.size()-2);
				// cout << "cardcommand: " << cardcommand << endl;
				if(cardcommand == "draw"){
					if(currcard.first == -1){
						cout << "You are the first one. Why you draw a card(?" << endl;
						continue;
					}
					sendline = "#draw";
					beingAdded = false;
					myTurn = false;
				}else if(isNumberS(cardcommand)){
					stringstream sscard(cardcommand);
					int cardpos, color;
					sscard >> cardpos >> color;
					if(cardpos >= hand.size()){
						cout << "You don't have that many cards" << endl;
						continue;
					}else{
						pair<int,int> sentcard = hand[cardpos];
						if(beingAdded){
							if(sentcard.second == currcard.second || sentcard.second == ADD4){
								if(sentcard.second == ADD4){
									if(color == 0){
										cout << "Wrong color input" << endl;
										continue;
									}else{
										sendline = "#" + to_string(sentcard.first) + " " + to_string(sentcard.second) + " " + to_string(color);
										beingAdded = false;
									}
								}else{
									sendline = "#" + to_string(sentcard.first) + " " + to_string(sentcard.second);
									beingAdded = false;
								}
							}else{
								if(sentcard.second == ADD2 && currcard.second == ADD4){
									cout << "Current card is +4, but you play +2" << endl;
								}else{
									cout << "You are being plus, try to play +2 or +4" << endl;
								}
								continue;
							}
						}else if(currcard.first == -1 || sentcard.first == WILD || sentcard.first == currcard.first || sentcard.second == currcard.second){
							if(sentcard.first == WILD){
								if(color == 0){
									cout << "Wrong color input" << endl;
									continue;
								}else{
									sendline = "#" + to_string(sentcard.first) + " " + to_string(sentcard.second) + " " + to_string(color);
									beingAdded = false;
								}
							}else{
								sendline = "#" + to_string(sentcard.first) + " " + to_string(sentcard.second);
							}
						}else{
							cout << "You cannot send this card" << endl;
							continue;
						}
						hand.erase(hand.begin() + cardpos);
						myTurn = false;
					}

					// // debug
					// stringstream sscard(cardcommand);
					// pair<int,int> sentcard;
					// sscard >> sentcard.first >> sentcard.second;
					// auto it = std::find(hand.begin(), hand.end(), sentcard);
					// if(it == hand.end()){
					// 	cout << "You don't have that card" << endl;
					// 	continue;
					// }else if(currcard.first == -1 || sentcard.first == WILD || sentcard.first == currcard.first || sentcard.second == currcard.second){
					// 	hand.erase(it);
					// }
					// myTurn = false;
				}else{
					cout << "invalid command" << endl;
					continue;
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
