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
	vector<vector<pair<int,int>>> hands(4);
	vector<pair<int,int>> hand, deck, table;
	vector<vector<int>> hands_img(4);
	vector<int> hand_img, deck_img, table_img, handsCardsCnt(4);
	vector<string> names(4);
	int elapsed_time = 0, currentPlayer = 0;
	pair<int,int> currcard;
	string historyChat = "";
	bool direction = true;
	int counter = 0;
	bool recvHand = false;
	bool recvCard = false;
	bool recvHands = false;
	bool recvHandsCardsCnt = false;
	int recvHandsCnt = 0;
	bool recvDeck = false;
	bool recvTable = false;
	bool recvElapsedTime = false;
	bool recvNames = false;
	bool recvHistoryChat = false;
	bool myTurn = false;
	bool beingAdded = false;
	bool recvWinner = false;
	bool recvQuitter = false;
	bool recvCurrentplayer = false;
	bool recvDirection = false;

	// SELECT init
	int maxfdp1, stdineof;
	fd_set rset;

	stdineof = 0;
	FD_ZERO(&rset);
	struct timeval timeout;
	timeout.tv_sec = 0;
	timeout.tv_usec = 10;

	// Render init
	bool isUpdated, wrongpwd, voiduser, userlogged, fieldempty, userexist, recvRoomList, recvPlayerList, recvRoom;
	{	// Bool settings
		isUpdated = true;
		wrongpwd = false;
		voiduser = false;
		userlogged = false;
		fieldempty = false;
		userexist = false;
		recvRoomList = false;
		recvPlayerList = false;
		recvRoom = false;
	}
	
	sf::RenderWindow window(sf::VideoMode(960, 720), "My window");
	TileMap map, deckMap, tableMap, handsMap[4], handsCntMap[4];
	int rowcard = 22;
	{	// Map Settings
		map.setPosition(sf::Vector2f(60, 630));
		deckMap.setPosition(sf::Vector2f(0, 40));
		tableMap.setPosition(sf::Vector2f(0, 260));
		handsMap[0].setPosition(sf::Vector2f(0, 480));
		handsMap[1].setPosition(sf::Vector2f(0, 525));
		handsMap[2].setPosition(sf::Vector2f(0, 570));
		handsMap[3].setPosition(sf::Vector2f(0, 615));
		handsCntMap[0].setPosition(sf::Vector2f(100, 380));
		handsCntMap[1].setPosition(sf::Vector2f(100, 425));
		handsCntMap[2].setPosition(sf::Vector2f(100, 470));
		handsCntMap[3].setPosition(sf::Vector2f(100, 515));
	}

	sf::Texture bgTexture, currcardTexture, colorpickTexture, drawTexture, logoTexture, accountTexture, passwordTexture, headTexture, idTexture, chatTexture, downTexture, upTexture;
	sf::Texture loginBtnTexture, registerBtnTexture, exitBtnTexture, backBtnTexture, enterBtnTexture, createBtnTexture, joinBtnTexture, logoutBtnTexture, startBtnTexture, backSquareBtnTexture, sendBtnTexture;
	{	// Texture Settings
		// Decoration
		bgTexture.loadFromFile("./img/wood.jpg");
		currcardTexture.loadFromFile("./img/840pxUNO.png", sf::IntRect(0, 360, 60, 90));
		colorpickTexture.loadFromFile("./img/color.png", sf::IntRect(250, 250, 500, 500));
		drawTexture.loadFromFile("./img/draw.png");
		drawTexture.setSmooth(true);
		logoTexture.loadFromFile("./img/UNO_Logo.png");
		accountTexture.loadFromFile("./img/account.png");
		accountTexture.setSmooth(true);
		passwordTexture.loadFromFile("./img/password.png");
		passwordTexture.setSmooth(true);
		headTexture.loadFromFile("./img/head.png");
		idTexture.loadFromFile("./img/id.png");
		chatTexture.loadFromFile("./img/chat.png");
		chatTexture.setSmooth(true);
		downTexture.loadFromFile("./img/down.png");
		downTexture.setSmooth(true);
		upTexture.loadFromFile("./img/up.png");
		upTexture.setSmooth(true);

		// Btn
		loginBtnTexture.loadFromFile("./img/loginBtn.png");
		registerBtnTexture.loadFromFile("./img/registerBtn.png");
		exitBtnTexture.loadFromFile("./img/exitBtn.png");
		backBtnTexture.loadFromFile("./img/backBtn.png");
		enterBtnTexture.loadFromFile("./img/enterBtn.png");
		joinBtnTexture.loadFromFile("./img/joinBtn.png");
		logoutBtnTexture.loadFromFile("./img/logoutBtn.png");
		createBtnTexture.loadFromFile("./img/createBtn.png");
		startBtnTexture.loadFromFile("./img/startBtn.png");
		backSquareBtnTexture.loadFromFile("./img/backSquareBtn.png");
		backSquareBtnTexture.setSmooth(true);
		sendBtnTexture.loadFromFile("./img/sendBtn.png");
	}

	sf::Vector2f btnSize(200, 48), btnPos1(700, 180), btnPos2(700, 320), btnPos3(700, 460), btnPos4(700, 238);
	sf::Vector2f inputFieldSize(350, 30), inputFieldPos1(150, 400), inputFieldPos2(150, 450), inputFieldPos3(600, 248), chatboxPos(400, 530);
	sf::Vector2f listPos1(20, 20), listPos2(20, 320), listPos3(40, 180);
	sf::Sprite bgSprite, currcardSprite, colorpickSprite, drawSprite, logoSprite, accountSprite, passwordSprite, headSprite, idSprite, chatSprite, directionSprite;
	sf::Sprite btn1Sprite, btn2Sprite, btn3Sprite, btn4Sprite, backSquareBtnSprite, sendBtnSprite;
	{	// Sprite Settings
		// Decoration
		bgSprite.setTexture(bgTexture);
		currcardSprite.setTexture(currcardTexture);
		currcardSprite.setPosition(sf::Vector2f(100, 200));
		colorpickSprite.setTexture(colorpickTexture);
		colorpickSprite.setPosition(sf::Vector2f(170, 110));
		drawSprite.setTexture(drawTexture);
		drawSprite.setScale(sf::Vector2f(0.4, 0.4));
		drawSprite.setPosition(sf::Vector2f(840, 610));
		logoSprite.setTexture(logoTexture);
		logoSprite.setPosition(sf::Vector2f(80, 150));
		accountSprite.setTexture(accountTexture);
		accountSprite.setScale(sf::Vector2f(0.125, 0.125));
		accountSprite.setPosition(inputFieldPos1);
		passwordSprite.setTexture(passwordTexture);
		passwordSprite.setScale(sf::Vector2f(0.125, 0.125));
		passwordSprite.setPosition(inputFieldPos2);
		headSprite.setTexture(headTexture);
		headSprite.setPosition(sf::Vector2f(200, 100));
		idSprite.setTexture(idTexture);
		idSprite.setScale(sf::Vector2f(0.125, 0.125));
		idSprite.setPosition(inputFieldPos3);
		chatSprite.setTexture(chatTexture);
		chatSprite.setScale(sf::Vector2f(0.125, 0.125));
		chatSprite.setPosition(chatboxPos);
		directionSprite.setScale(sf::Vector2f(0.25, 0.25));
		directionSprite.setPosition(sf::Vector2f(220, 210));
		


		// Btn
		btn1Sprite.setPosition(btnPos1);
		btn2Sprite.setPosition(btnPos2);
		btn3Sprite.setPosition(btnPos3);
		btn4Sprite.setPosition(btnPos4);
		backSquareBtnSprite.setTexture(backSquareBtnTexture);
		backSquareBtnSprite.setScale(sf::Vector2f(0.25, 0.25));
		backSquareBtnSprite.setPosition(sf::Vector2f(20, 20));
		sendBtnSprite.setTexture(sendBtnTexture);
		sendBtnSprite.setPosition(chatboxPos + sf::Vector2f(320, -10));
	}

	sf::RectangleShape inputFieldBG1, inputFieldBG2, inputFieldBG3, chatboxBG;
	{	// input field background settings
		inputFieldBG1.setPosition(inputFieldPos1);
		inputFieldBG1.setSize(inputFieldSize);
		inputFieldBG1.setFillColor(sf::Color::White);
		inputFieldBG1.setOutlineThickness(4);
		inputFieldBG1.setOutlineColor(sf::Color::White);
		inputFieldBG2.setPosition(inputFieldPos2);
		inputFieldBG2.setSize(inputFieldSize);
		inputFieldBG2.setFillColor(sf::Color::White);
		inputFieldBG2.setOutlineThickness(4);
		inputFieldBG2.setOutlineColor(sf::Color::White);
		inputFieldBG3.setPosition(inputFieldPos3);
		inputFieldBG3.setSize(sf::Vector2f(90, 30));
		inputFieldBG3.setFillColor(sf::Color::White);
		inputFieldBG3.setOutlineThickness(4);
		inputFieldBG3.setOutlineColor(sf::Color::White);
		chatboxBG.setPosition(chatboxPos);
		chatboxBG.setSize(sf::Vector2f(300, 30));
		chatboxBG.setFillColor(sf::Color::White);
		chatboxBG.setOutlineThickness(4);
		chatboxBG.setOutlineColor(sf::Color::White);
	}
	bool inputField = true;
	sf::String roomListString, playerListString, roomString, namesString[4];
	sf::String inputField1, inputField2, inputField3, wrongpwdString, voiduserString, userloggedString, fieldemptyString, userexistString, deckString, tableString, handsString, chatboxString, historychatString;
	sf::String timeString, nameString;
	std::wstring_convert<std::codecvt_utf8_utf16<wchar_t>> converter;
	{	// String Literals
		wrongpwdString = "Wrong username/password !!!";
		voiduserString = "User not exist !!!";
		userloggedString = "User already logged in !!!";
		fieldemptyString = "You must filled all two fields !!!";
		userexistString = "Username already taken !!!";
		deckString = "Deck:";
		tableString = "Table:";
	}
	
	sf::Font font;
	font.loadFromFile("UbuntuMono-R.ttf");
	sf::Text dispText1, dispText2, dispText3, errorFieldText, roomListText, playerListText, roomText, deckText, tableText, handsText, namesText[4], chatboxText, historychatText, timeText, nameText;
	{	// Text display settings
		dispText1.setFont(font);
		dispText1.setPosition(inputFieldPos1 + sf::Vector2f(40, 0));
		dispText1.setFillColor(sf::Color::Black);
		dispText2.setFont(font);
		dispText2.setPosition(inputFieldPos2 + sf::Vector2f(40, 0));
		dispText2.setFillColor(sf::Color::Black);
		dispText3.setFont(font);
		dispText3.setPosition(inputFieldPos3 + sf::Vector2f(40, 0));
		dispText3.setFillColor(sf::Color::Black);
		
		errorFieldText.setFont(font);
		errorFieldText.setPosition(inputFieldPos2 + sf::Vector2f(0, 50));
		errorFieldText.setFillColor(sf::Color::Red);

		roomListText.setFont(font);
		roomListText.setCharacterSize(20);
		roomListText.setPosition(listPos1);
		playerListText.setFont(font);
		playerListText.setCharacterSize(20);
		playerListText.setPosition(listPos2);
		roomText.setFont(font);
		roomText.setPosition(listPos3);

		deckText.setFont(font);
		deckText.setString(deckString);
		deckText.setPosition(sf::Vector2f(0, 0));
		tableText.setFont(font);
		tableText.setString(tableString);
		tableText.setPosition(sf::Vector2f(0, 220));
		handsText.setFont(font);
		handsText.setPosition(sf::Vector2f(0, 440));

		namesText[0].setFont(font);
		namesText[0].setPosition(sf::Vector2f(0, 380));
		namesText[1].setFont(font);
		namesText[1].setPosition(sf::Vector2f(0, 425));
		namesText[2].setFont(font);
		namesText[2].setPosition(sf::Vector2f(0, 470));
		namesText[3].setFont(font);
		namesText[3].setPosition(sf::Vector2f(0, 515));

		chatboxText.setFont(font);
		chatboxText.setPosition(chatboxPos + sf::Vector2f(40, 0));
		chatboxText.setFillColor(sf::Color::Black);
		historychatText.setFont(font);
		historychatText.setPosition(sf::Vector2f(400, 10));
		historychatText.setFillColor(sf::Color::White);

		timeText.setFont(font);
		timeText.setPosition(sf::Vector2f(0, 660));
		nameText.setFont(font);
		nameText.setPosition(sf::Vector2f(400, 660));
	}

	while (window.isOpen())
	{
		// ===== IN GUI =====
		// check all the window's events that were triggered since the last iteration of the loop
        sf::Event event;
        while (window.pollEvent(event))
        {
            // "close requested" event: we close the window
            if (event.type == sf::Event::Closed){
				window.close();
			} 

			if (event.type == sf::Event::TextEntered){
				isUpdated = true;
				if(state == "LOGIN" || state == "REGISTER"){
					if(inputField){
						if(event.text.unicode == '\b'){
							if(!inputField1.isEmpty()) inputField1.erase(inputField1.getSize()-1, 1);
							dispText1.setString((inputField1.getSize() < 20) ?inputField1 :inputField1.substring(inputField1.getSize()-20, 20));
						}else if(event.text.unicode >= '!' && event.text.unicode <= '~'){
							inputField1 += event.text.unicode;
							dispText1.setString((inputField1.getSize() < 20) ?inputField1 :inputField1.substring(inputField1.getSize()-20, 20));
						}
					}else{
						if(event.text.unicode == '\b'){
							if(!inputField2.isEmpty()) inputField2.erase(inputField2.getSize()-1, 1);
							dispText2.setString((inputField2.getSize() < 20) ?inputField2 :inputField2.substring(inputField2.getSize()-20, 20));
						}else if(event.text.unicode >= '!' && event.text.unicode <= '~'){
							inputField2 += event.text.unicode;
							dispText2.setString((inputField2.getSize() < 20) ?inputField2 :inputField2.substring(inputField2.getSize()-20, 20));
						}
					}
				}
				else if(state == "LOBBY"){
					if(event.text.unicode == '\b'){
						if(!inputField3.isEmpty()) inputField3.erase(inputField3.getSize()-1, 1);
						dispText3.setString((inputField3.getSize() < 3) ?inputField3 :inputField3.substring(inputField3.getSize()-3, 3));
					}else if(event.text.unicode >= '0' && event.text.unicode <= '9'){
						inputField3 += event.text.unicode;
						dispText3.setString((inputField3.getSize() < 3) ?inputField3 :inputField3.substring(inputField3.getSize()-3, 3));
					}
				}
				else if(state == "GAME"){
					if(event.text.unicode == '\b'){
						if(!chatboxString.isEmpty()) chatboxString.erase(chatboxString.getSize()-1, 1);
						chatboxText.setString((chatboxString.getSize() < 17) ?chatboxString :chatboxString.substring(chatboxString.getSize()-17, 17));
					}else if(event.text.unicode >= ' ' && event.text.unicode <= '~'){
						chatboxString += event.text.unicode;
						chatboxText.setString((chatboxString.getSize() < 17) ?chatboxString :chatboxString.substring(chatboxString.getSize()-17, 17));
					}
				}
			}

			if (event.type == sf::Event::MouseButtonReleased){
				isUpdated = true;
				if(event.mouseButton.button == sf::Mouse::Left){
					if(state == "MENU"){
						if(event.mouseButton.x < btnPos1.x || event.mouseButton.x > btnPos1.x + btnSize.x) continue;
						sendline.clear();
						// Btn
						wrongpwd = false;
						voiduser = false;
						userlogged = false;
						fieldempty = false;
						userexist = false;
						inputField1.clear();
						inputField2.clear();
						dispText1.setString(inputField1);
						dispText2.setString(inputField2);
						if(event.mouseButton.y > btnPos1.y && event.mouseButton.y < btnPos1.y + btnSize.y) sendline = "login";
						if(event.mouseButton.y > btnPos2.y && event.mouseButton.y < btnPos2.y + btnSize.y) sendline = "register";
						if(event.mouseButton.y > btnPos3.y && event.mouseButton.y < btnPos3.y + btnSize.y) sendline = "exit";
						if(!sendline.empty()) Write(sockfd, sendline);
					}
					else if(state == "LOGIN" || state == "REGISTER"){
						// Input Field
						if(event.mouseButton.x > 150 && event.mouseButton.x < 500 && event.mouseButton.y > 400 && event.mouseButton.y < 430) inputField = true;
						else if(event.mouseButton.x > 150 && event.mouseButton.x < 500 && event.mouseButton.y > 450 && event.mouseButton.y < 480) inputField = false;

						// Btn
						if(event.mouseButton.x < btnPos1.x || event.mouseButton.x > btnPos1.x + btnSize.x) continue;
						sendline.clear();
						if(event.mouseButton.y > btnPos1.y && event.mouseButton.y < btnPos1.y + btnSize.y) sendline = "back";
						if(event.mouseButton.y > btnPos2.y && event.mouseButton.y < btnPos2.y + btnSize.y) sendline = inputField1.toAnsiString() + " " + inputField2.toAnsiString();
						if(event.mouseButton.y > btnPos3.y && event.mouseButton.y < btnPos3.y + btnSize.y) sendline = "exit";
						if(!sendline.empty()) Write(sockfd, sendline);
					}
					else if(state == "LOBBY"){
						// Btn
						if(event.mouseButton.x < btnPos1.x || event.mouseButton.x > btnPos1.x + btnSize.x) continue;
						sendline.clear();
						if(event.mouseButton.y > btnPos1.y && event.mouseButton.y < btnPos1.y + btnSize.y) sendline = "create";
						if(event.mouseButton.y > btnPos2.y && event.mouseButton.y < btnPos2.y + btnSize.y) sendline = "logout";
						if(event.mouseButton.y > btnPos3.y && event.mouseButton.y < btnPos3.y + btnSize.y) sendline = "exit";
						if(event.mouseButton.y > btnPos4.y && event.mouseButton.y < btnPos4.y + btnSize.y) sendline = "join " + inputField3.toAnsiString();
						if(!sendline.empty()) Write(sockfd, sendline);
					}
					else if(state == "ROOM"){
						// Btn
						if(event.mouseButton.x < btnPos1.x || event.mouseButton.x > btnPos1.x + btnSize.x) continue;
						sendline.clear();
						if(event.mouseButton.y > btnPos1.y && event.mouseButton.y < btnPos1.y + btnSize.y) sendline = "back";
						if(event.mouseButton.y > btnPos2.y && event.mouseButton.y < btnPos2.y + btnSize.y) sendline = "start";
						if(event.mouseButton.y > btnPos3.y && event.mouseButton.y < btnPos3.y + btnSize.y) sendline = "exit";
						if(!sendline.empty()) Write(sockfd, sendline);
					}
					else if(state == "GAME"){
						if(event.mouseButton.x > 20 && event.mouseButton.x < 84 && event.mouseButton.y > 20 && event.mouseButton.y < 84){
							sendline = "leave";
							Write(sockfd, sendline);
							continue;
						}
						if(event.mouseButton.x > 720 && event.mouseButton.x < 920 && event.mouseButton.y > 520 && event.mouseButton.y < 568){
							sendline = chatboxString.toAnsiString();
							chatboxString.clear();
							chatboxText.setString(chatboxString);
							Write(sockfd, sendline);
							continue;
						}
						if(myTurn){
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
							continue;
						}
						
					}
					else if(state == "WIN"){
						// Btn
						if(event.mouseButton.x < btnPos1.x || event.mouseButton.x > btnPos1.x + btnSize.x) continue;
						sendline.clear();
						if(event.mouseButton.y > btnPos1.y && event.mouseButton.y < btnPos1.y + btnSize.y) sendline = "back";
						if(event.mouseButton.y > btnPos3.y && event.mouseButton.y < btnPos3.y + btnSize.y) sendline = "exit";
						if(!sendline.empty()) Write(sockfd, sendline);
					}
				}
			}
        }

		if(isUpdated){
			// clear the window with black color
			window.clear(sf::Color::Black);

			// refresh frame
			window.draw(bgSprite);
			if(state == "MENU"){
				window.draw(logoSprite);
				btn1Sprite.setTexture(loginBtnTexture);
				btn2Sprite.setTexture(registerBtnTexture);
				btn3Sprite.setTexture(exitBtnTexture);
				window.draw(btn1Sprite);
				window.draw(btn2Sprite);
				window.draw(btn3Sprite);
			}
			else if(state == "LOGIN"){
				window.draw(headSprite);
				window.draw(inputFieldBG1);
				window.draw(accountSprite);
				window.draw(inputFieldBG2);
				window.draw(passwordSprite);
				window.draw(dispText1);
				window.draw(dispText2);
				if(wrongpwd || voiduser || userlogged){
					if(wrongpwd) errorFieldText.setString(wrongpwdString);
					if(voiduser) errorFieldText.setString(voiduserString);
					if(userlogged) errorFieldText.setString(userloggedString);
					window.draw(errorFieldText);
				}
				btn1Sprite.setTexture(backBtnTexture);
				btn2Sprite.setTexture(enterBtnTexture);
				btn3Sprite.setTexture(exitBtnTexture);
				window.draw(btn1Sprite);
				window.draw(btn2Sprite);
				window.draw(btn3Sprite);
			}
			else if(state == "REGISTER"){
				window.draw(headSprite);
				window.draw(inputFieldBG1);
				window.draw(accountSprite);
				window.draw(inputFieldBG2);
				window.draw(passwordSprite);
				window.draw(dispText1);
				window.draw(dispText2);
				if(fieldempty || userexist){
					if(fieldempty) errorFieldText.setString(fieldemptyString);
					if(userexist) errorFieldText.setString(userexistString);
					window.draw(errorFieldText);
				}
				btn1Sprite.setTexture(backBtnTexture);
				btn2Sprite.setTexture(enterBtnTexture);
				btn3Sprite.setTexture(exitBtnTexture);
				window.draw(btn1Sprite);
				window.draw(btn2Sprite);
				window.draw(btn3Sprite);
			}
			else if(state == "LOBBY"){
				window.draw(roomListText);
				window.draw(playerListText);
				window.draw(inputFieldBG3);
				window.draw(idSprite);
				window.draw(dispText3);
				btn1Sprite.setTexture(createBtnTexture);
				btn2Sprite.setTexture(logoutBtnTexture);
				btn3Sprite.setTexture(exitBtnTexture);
				btn4Sprite.setTexture(joinBtnTexture);
				window.draw(btn1Sprite);
				window.draw(btn2Sprite);
				window.draw(btn3Sprite);
				window.draw(btn4Sprite);
			}
			else if(state == "ROOM"){
				window.draw(roomText);
				btn1Sprite.setTexture(backBtnTexture);
				btn2Sprite.setTexture(startBtnTexture);
				btn3Sprite.setTexture(exitBtnTexture);
				window.draw(btn1Sprite);
				window.draw(btn2Sprite);
				window.draw(btn3Sprite);
			}
			else if(state == "GAME"){
				window.draw(map);
				window.draw(backSquareBtnSprite);
				if(direction) directionSprite.setTexture(downTexture);
				else directionSprite.setTexture(upTexture);
				window.draw(directionSprite);
				for(int i = 0 ; i < 4 ; i++){
					if(i == currentPlayer){
						namesText[i].setFillColor(sf::Color::Yellow);
					}else{
						namesText[i].setFillColor(sf::Color::White);
					}
					window.draw(namesText[i]);
					window.draw(handsCntMap[i]);
				}
				window.draw(historychatText);
				window.draw(chatboxBG);
				window.draw(chatSprite);
				window.draw(chatboxText);
				window.draw(sendBtnSprite);
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
			else if(state == "WIN"){
				window.draw(deckText);
				window.draw(deckMap);
				window.draw(tableText);
				window.draw(tableMap);
				window.draw(handsText);
				window.draw(handsMap[0]);
				window.draw(handsMap[1]);
				window.draw(handsMap[2]);
				window.draw(handsMap[3]);
				window.draw(timeText);
				window.draw(nameText);
				btn1Sprite.setTexture(backBtnTexture);
				btn3Sprite.setTexture(exitBtnTexture);
				window.draw(btn1Sprite);
				window.draw(btn3Sprite);
			}
			window.display();
			isUpdated = false;
		}
		

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
						}else if(command == "deck"){
							recvDeck = true;
						}else if(command == "table"){
							recvTable = true;
						}else if(command == "hands"){
							recvHands = true;
						}else if(command == "elapsedtime"){
							recvElapsedTime = true;
						}else if(command == "names"){
							recvNames = true;
						}else if(command == "handscardscnt"){
							recvHandsCardsCnt = true;
						}else if(command == "historychat"){
							recvHistoryChat = true;
						}else if(command == "winner"){
							recvWinner = true;
						}else if(command == "quitter"){
							recvQuitter = true;
						}else if(command == "currentplayer"){
							recvCurrentplayer = true;
						}else if(command == "direction"){
							recvDirection = true;
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
							map.load("./img/840pxUNO.png", sf::Vector2u(60, 90), hand_img.data(), hand_img.size(), 1);

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
							currcardTexture.loadFromFile("./img/840pxUNO.png", sf::IntRect(currcard_x*60, currcard_y*90, 60, 90));
						}else{
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
							if(recvDeck){
								stringstream ssdeck(command);
								int color, number;
								deck.clear();
								while(ssdeck >> color >> number){
									deck.push_back({color, number});
								}

								deck_img.clear();
								for(auto [color, number] : deck){
									if(number == ADD4){
										deck_img.push_back(69);
									}else if(number == COLOR){
										deck_img.push_back(COLOR);
									}else{
										deck_img.push_back((color-1)*14 + number);
									}
								}
								if(deck_img.size() % rowcard != 0) deck_img.resize(((deck_img.size()/rowcard)+1)*rowcard, 56);
								deckMap.load("./img/420pxUNO.png", sf::Vector2u(30, 45), deck_img.data(), rowcard, deck_img.size()/rowcard);
								
								recvDeck = false;
								continue;
							}
							if(recvTable){
								stringstream sstable(command);
								int color, number;
								table.clear();
								while(sstable >> color >> number){
									table.push_back({color, number});
								}

								table_img.clear();
								for(auto [color, number] : table){
									if(number == ADD4){
										table_img.push_back(69);
									}else if(number == COLOR){
										table_img.push_back(COLOR);
									}else{
										table_img.push_back((color-1)*14 + number);
									}
								}
								if(table_img.size() % rowcard != 0) table_img.resize(((table_img.size()/rowcard)+1)*rowcard, 56);
								tableMap.load("./img/420pxUNO.png", sf::Vector2u(30, 45), table_img.data(), rowcard, table_img.size()/rowcard);

								recvTable = false;
								continue;
							}
							if(recvHands){
								stringstream sshands(command);
								int color, number;
								hands[recvHandsCnt].clear();
								while(sshands >> color >> number){
									hands[recvHandsCnt].push_back({color, number});
								}

								hands_img[recvHandsCnt].clear();
								for(auto [color, number] : hands[recvHandsCnt]){
									if(number == ADD4){
										hands_img[recvHandsCnt].push_back(69);
									}else if(number == COLOR){
										hands_img[recvHandsCnt].push_back(COLOR);
									}else{
										hands_img[recvHandsCnt].push_back((color-1)*14 + number);
									}
								}
								if(hands_img[recvHandsCnt].size() % rowcard != 0) hands_img[recvHandsCnt].resize(((hands_img[recvHandsCnt].size()/rowcard)+1)*rowcard, 56);
								handsMap[recvHandsCnt].load("./img/420pxUNO.png", sf::Vector2u(30, 45), hands_img[recvHandsCnt].data(), rowcard, hands_img[recvHandsCnt].size()/rowcard);

								if(++recvHandsCnt == 4){
									recvHandsCnt = 0;
									recvHands = false;
								}
								continue;
							}
							if(recvElapsedTime){
								int time = 0;
								stringstream sstime(command);
								sstime >> time;
								elapsed_time = time;

								timeString = to_string(elapsed_time);
								timeText.setString("Time Elapsed: " + timeString + " sec");

								recvElapsedTime = false;
								continue;
							}
							if(recvNames){
								stringstream ssnames(command);
								string name;
								counter = 0;
								while(ssnames >> name){
									names[counter] = name;
									namesString[counter] = name;
									namesText[counter].setString(namesString[counter]);
									counter++;
								}

								handsString = "Hands: " + command;
								handsText.setString(handsString);

								recvNames = false;
								continue;
							}
							if(recvHandsCardsCnt){
								stringstream ss(command);
								int cnt = 0;
								counter = 0;
								while(ss >> cnt){
									handsCardsCnt[counter] = cnt;
									vector<int> temp(handsCardsCnt[counter], 56);
									handsCntMap[counter++].load("./img/420pxUNO.png", sf::Vector2u(30, 45), temp.data(), temp.size(), 1);
								}

								recvHandsCardsCnt = false;
								continue;
							}
							if(recvHistoryChat){
								historyChat = command;
								historychatString = historyChat;
								historychatText.setString(historychatString);
								recvHistoryChat = false;
							}
							if(recvWinner){
								nameString = command;
								nameText.setString("Winner is [" + nameString + "]");
								nameText.setFillColor(sf::Color::Yellow);
								recvWinner = false;
								continue;
							}
							if(recvQuitter){
								nameString = command;
								nameText.setString("[" + nameString + "] quit the game");
								nameText.setFillColor(sf::Color::Red);
								recvQuitter = false;
								continue;
							}
							if(recvCurrentplayer){
								stringstream ss(command);
								ss >> currentPlayer;
								recvCurrentplayer = false;
								continue;
							}
							if(recvDirection){
								stringstream ss(command);
								ss >> direction;
								recvDirection = false;
								continue;
							}
							if(command.empty()) continue;
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
						window.close();
						return;
					}
					else if (command == "game")
					{
						cout << gui[command];
						state = "GAME";
					}
					else if (command == "wrongpwd")
					{
						wrongpwd = true;
					}
					else if (command == "voiduser")
					{
						voiduser = true;
					}
					else if (command == "userlogged")
					{
						userlogged = true;
					}
					else if (command == "fieldempty")
					{
						fieldempty = true;
					}
					else if (command == "userexist")
					{
						userexist = true;
					}
					else if (command == "roomlist"){
						recvRoomList = true;
					}
					else if (command == "playerlist"){
						recvPlayerList = true;
					}
					else if (command == "gameroom"){
						recvRoom = true;
					}
					else if (gui.find(command) != gui.end())
					{
						cout << gui[command];
						transform(command.begin(), command.end(), command.begin(), ::toupper);
						state = command;
						
					}
					else
					{
						if (command.empty()) continue;
						if(recvRoomList){
							roomListString = converter.from_bytes(command);
							roomListText.setString(roomListString.toUtf32());
							recvRoomList = false;
						}
						if(recvPlayerList){
							playerListString = converter.from_bytes(command);
							playerListText.setString(playerListString.toUtf32());
							recvPlayerList = false;
						}
						if(recvRoom){
							roomString = converter.from_bytes(command);
							roomText.setString(roomString.toUtf32());
							recvRoom = false;
						}
						cout << command << '\n';
					}
				}
			}
			isUpdated = true;
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
	// inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
	inet_pton(AF_INET, "140.113.235.151", &servaddr.sin_addr);

	connect(sockfd, (SA *)&servaddr, sizeof(servaddr));

	menu(stdin, sockfd); /* do it all */

	exit(0);
}
