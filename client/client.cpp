#include "../unp.h"
#include <bits/stdc++.h>
#include <SFML/Graphics.hpp>
using namespace std;

#define RED 1
#define YELLOW 2
#define GREEN 3
#define BLUE 4
#define WILD 5
#define SKIP 10
#define TURN 11
#define ADD2 12
#define COLOR 13
#define ADD4 14

class TileMap : public sf::Drawable, public sf::Transformable
{
public:

    bool load(const std::string& tileset, sf::Vector2u tileSize, const int* tiles, unsigned int width, unsigned int height)
    {
        // load the tileset texture
        if (!m_tileset.loadFromFile(tileset))
            return false;

        // resize the vertex array to fit the level size
        m_vertices.setPrimitiveType(sf::Quads);
        m_vertices.resize(width * height * 4);

        // populate the vertex array, with one quad per tile
        for (unsigned int i = 0; i < width; ++i)
            for (unsigned int j = 0; j < height; ++j)
            {
                // get the current tile number
                int tileNumber = tiles[i + j * width];

                // find its position in the tileset texture
                int tu = tileNumber % (m_tileset.getSize().x / tileSize.x);
                int tv = tileNumber / (m_tileset.getSize().x / tileSize.x);

                // get a pointer to the current tile's quad
                sf::Vertex* quad = &m_vertices[(i + j * width) * 4];

                // define its 4 corners
                quad[0].position = sf::Vector2f(i * tileSize.x, j * tileSize.y);
                quad[1].position = sf::Vector2f((i + 1) * tileSize.x, j * tileSize.y);
                quad[2].position = sf::Vector2f((i + 1) * tileSize.x, (j + 1) * tileSize.y);
                quad[3].position = sf::Vector2f(i * tileSize.x, (j + 1) * tileSize.y);

                // define its 4 texture coordinates
                quad[0].texCoords = sf::Vector2f(tu * tileSize.x, tv * tileSize.y);
                quad[1].texCoords = sf::Vector2f((tu + 1) * tileSize.x, tv * tileSize.y);
                quad[2].texCoords = sf::Vector2f((tu + 1) * tileSize.x, (tv + 1) * tileSize.y);
                quad[3].texCoords = sf::Vector2f(tu * tileSize.x, (tv + 1) * tileSize.y);
            }

        return true;
    }

private:

    virtual void draw(sf::RenderTarget& target, sf::RenderStates states) const
    {
        // apply the transform
        states.transform *= getTransform();

        // apply the tileset texture
        states.texture = &m_tileset;

        // draw the vertex array
        target.draw(m_vertices, states);
    }

    sf::VertexArray m_vertices;
    sf::Texture m_tileset;
};

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
	vector<int> hand_img;
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
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 10;

	// Render init
	sf::RenderWindow window(sf::VideoMode(960, 720), "My window");
	TileMap map;
	map.setPosition(sf::Vector2f(60, 630));

	sf::Texture bgTexture, currcardTexture, colorpickTexture, drawTexture;
	bgTexture.loadFromFile("wood.jpg");
	currcardTexture.loadFromFile("840pxUNO.png", sf::IntRect(0, 360, 60, 90));
	colorpickTexture.loadFromFile("color.png", sf::IntRect(250, 250, 500, 500));
	drawTexture.loadFromFile("draw.png");
	drawTexture.setSmooth(true);

	sf::Sprite bgSprite, currcardSprite, colorpickSprite, drawSprite;
	bgSprite.setTexture(bgTexture);
	currcardSprite.setTexture(currcardTexture);
	currcardSprite.setPosition(sf::Vector2f(390, 315));
	colorpickSprite.setTexture(colorpickTexture);
	colorpickSprite.setPosition(sf::Vector2f(170, 110));
	drawSprite.setTexture(drawTexture);
	drawSprite.setScale(sf::Vector2f(0.4, 0.4));
	drawSprite.setPosition(sf::Vector2f(840, 610));

	while (window.isOpen())
	{
		// ===== IN GUI =====
		// check all the window's events that were triggered since the last iteration of the loop
        sf::Event event;
        while (window.pollEvent(event))
        {
            // "close requested" event: we close the window
            if (event.type == sf::Event::Closed)
                window.close();

			if (event.type == sf::Event::MouseButtonReleased){
				if(event.mouseButton.button == sf::Mouse::Left){
					if(state == "GAME" && myTurn){
						// cout << "x = " << event.mouseButton.x << ", y = " << event.mouseButton.y << endl;
						if(event.mouseButton.y < 630) continue;
						int cardpos = -1, color = 0;
						for(int i = 0 ; i < hand.size() ; i++){
							if(event.mouseButton.x > 60*i+60 && event.mouseButton.x < 60*i+120){
								cardpos = i;
								break;
							}
						}
						if(event.mouseButton.x > 840 && event.mouseButton.x < 942){
							if(currcard.first == -1){
								cout << "You are the first one. Why you draw a card(?" << endl;
								continue;
							}
							sendline = "#draw";
							beingAdded = false;
							myTurn = false;
							Write(sockfd, sendline);
							continue;
						}
						if(cardpos == -1) continue;

						if(cardpos >= hand.size()){
							cout << "You don't have that many cards" << endl;
							continue;
						}else{
							pair<int,int> sentcard = hand[cardpos];
							if(beingAdded){
								if(sentcard.second == currcard.second || sentcard.second == ADD4){
									if(sentcard.second == ADD4){
										while(color == 0){
											// refresh frame
											window.clear(sf::Color::Black);
											window.draw(bgSprite);
											window.draw(map);
											window.draw(currcardSprite);
											window.draw(drawSprite);
											window.draw(colorpickSprite);
											window.display();
											if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)){
												sf::Vector2i position = sf::Mouse::getPosition(window);
												if(position.x > 170 && position.x < 405 && position.y > 110 && position.y < 345){
													color = RED;
												}else if(position.x > 435 && position.x < 670 && position.y > 110 && position.y < 345){
													color = GREEN;
												}else if(position.x > 170 && position.x < 405 && position.y > 375 && position.y < 610){
													color = BLUE;
												}else if(position.x > 435 && position.x < 670 && position.y > 375 && position.y < 610){
													color = YELLOW;
												}
											}
										}
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
									while(color == 0){
										// refresh frame
										window.clear(sf::Color::Black);
										window.draw(bgSprite);
										window.draw(map);
										window.draw(currcardSprite);
										window.draw(drawSprite);
										window.draw(colorpickSprite);
										window.display();
										if (sf::Mouse::isButtonPressed(sf::Mouse::Button::Left)){
											sf::Vector2i position = sf::Mouse::getPosition(window);
											if(position.x > 170 && position.x < 405 && position.y > 110 && position.y < 345){
												color = RED;
											}else if(position.x > 435 && position.x < 670 && position.y > 110 && position.y < 345){
												color = GREEN;
											}else if(position.x > 170 && position.x < 405 && position.y > 375 && position.y < 610){
												color = BLUE;
											}else if(position.x > 435 && position.x < 670 && position.y > 375 && position.y < 610){
												color = YELLOW;
											}
										}
									}
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
						Write(sockfd, sendline);
					}
				}
			}
        }

		// clear the window with black color
        window.clear(sf::Color::Black);

		// refresh frame
		window.draw(bgSprite);
		if(state == "GAME"){
			window.draw(map);
			if(currcard.second == COLOR || currcard.second == ADD4){
				if(currcard.first == RED) currcardSprite.setColor(sf::Color(255, 0, 0, 100));
				else if(currcard.first == YELLOW) currcardSprite.setColor(sf::Color(255, 255, 0, 100));
				else if(currcard.first == GREEN) currcardSprite.setColor(sf::Color(0, 255, 0, 100));
				else if(currcard.first == BLUE) currcardSprite.setColor(sf::Color(0, 0, 255, 100));
				else currcardSprite.setColor(sf::Color(255, 255, 255));
			}else currcardSprite.setColor(sf::Color(255, 255, 255));
			window.draw(currcardSprite);
			window.draw(drawSprite);
		}
		window.display();

		// ===== IN CONSOLE =====
		if (stdineof == 0)
			FD_SET(fileno(fp), &rset);
		FD_SET(sockfd, &rset);
		maxfdp1 = max(fileno(fp), sockfd) + 1;
		select(maxfdp1, &rset, NULL, NULL, &timeout);

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

							// hand disp
							hand_img.clear();
							for(auto [color, number] : hand){
								if(number == ADD4){
									hand_img.push_back(69);
								}else if(number == COLOR){
									hand_img.push_back(COLOR);
								}else{
									hand_img.push_back((color-1)*14 + number);
								}
							}
							if (!map.load("840pxUNO.png", sf::Vector2u(60, 90), hand_img.data(), hand_img.size(), 1)) return;

							// currcard disp
							int currcard_x, currcard_y;
							if(currcard.second == ADD4){
								currcard_x = 13;
								currcard_y = 4;
							}else if(currcard.second == COLOR){
								currcard_x = 13;
								currcard_y = 0;
							}else if(currcard.second == -1){
								currcard_x = 0;
								currcard_y = 4;
							}else{
								currcard_x = currcard.second;
								currcard_y = currcard.first-1;
							}
							currcardTexture.loadFromFile("840pxUNO.png", sf::IntRect(currcard_x*60, currcard_y*90, 60, 90));
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
