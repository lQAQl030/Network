#include "../unp.h"
#include <bits/stdc++.h>
#include <ncurses.h>
#include <unistd.h>
#include <fcntl.h>
#include <locale.h>
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

string input_buffer = "";

bool isNumber(const string &str)
{
    return str.find_first_not_of("0123456789") == string::npos;
}

bool isNumberS(const string &str)
{
    return str.find_first_not_of("0123456789 ") == string::npos;
}

int Read(int fd, string &s)
{
    int n = MAXLINE;
    char buffer[MAXLINE];
    memset(buffer, 0, sizeof(buffer));
    int count = read(fd, buffer, n);
    s = string(buffer, count);
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
        symbol = "╰╮ ";
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

void show_hand(WINDOW *hand_win, vector<pair<int,int>> &hand){
    werase(hand_win);
    box(hand_win, 0, 0);
    mvwprintw(hand_win, 1, 1, "Your Hand\n");
    // id
    string id_line = "";
    for(int cnt = 0 ; cnt < hand.size() ; cnt++){
        if(cnt <= 9){
            id_line += to_string(cnt) + "  ";
        }else{
            id_line += to_string(cnt) + " ";
        }
    }
    mvwprintw(hand_win, 2, 2, id_line.c_str());

    // card
    int x = 2;
    for(auto [color, number] : hand){
        string card = num2sym(number) + " ";
        // Apply color
        if(color >= 1 && color <= 5){
            wattron(hand_win, COLOR_PAIR(color));
            mvwprintw(hand_win, 3, x, card.c_str());
            wattroff(hand_win, COLOR_PAIR(color));
        } else {
            mvwprintw(hand_win, 3, x, card.c_str());
        }
        x += card.length();
    }
    wrefresh(hand_win);
}

void show_curr_card(WINDOW *card_win, pair<int,int> &currcard){
    werase(card_win);
    box(card_win, 0, 0);
    mvwprintw(card_win, 1, 2, "Current Card");
    if(currcard.first == -1){
        mvwprintw(card_win, 2, 2, "(empty)");
    }
    else{
        string card = num2sym(currcard.second) + " ";
        if(currcard.first >=1 && currcard.first <= 5){
            wattron(card_win, COLOR_PAIR(currcard.first));
            mvwprintw(card_win, 2, 2, card.c_str());
            wattroff(card_win, COLOR_PAIR(currcard.first));
        } else {
            mvwprintw(card_win, 2, 2, card.c_str());
        }
    }
    wrefresh(card_win);
}

void display(WINDOW *hand_win, WINDOW *card_win, vector<pair<int,int>> &hand, pair<int,int> &currcard){
    show_hand(hand_win, hand);
    show_curr_card(card_win, currcard);

}

void menu(int sockfd)
{
    // Initialize ncurses
    initscr();            // Start curses mode
    cbreak();             // Disable line buffering
    noecho();             // Don't echo() while we do getch
    keypad(stdscr, TRUE); // Enable function keys
    curs_set(1);          // Show cursor

    // Initialize colors
    if (has_colors() == FALSE)
    {	
        endwin();
        printf("Your terminal does not support color\n");
        exit(1);
    }
    start_color();
    // Define color pairs
    init_pair(RED, COLOR_RED, COLOR_BLACK);
    init_pair(GREEN, COLOR_GREEN, COLOR_BLACK);
    init_pair(YELLOW, COLOR_YELLOW, COLOR_BLACK);
    init_pair(BLUE, COLOR_BLUE, COLOR_BLACK);
    init_pair(WILD, COLOR_WHITE, COLOR_BLACK);

    // Create windows
    int height, width;
    getmaxyx(stdscr, height, width);

    WINDOW *hand_win;
    WINDOW *card_win;
    WINDOW *msg_win = newwin(height - 15, width - 10, 13, 2);
    WINDOW *room_win;
    WINDOW *input_win = newwin(3, width - 10, height - 4, 2);
    WINDOW *err_win;

    // Draw borders
    box(msg_win, 0, 0);
    box(input_win, 0, 0);

    // Refresh windows
    wrefresh(msg_win);
    wrefresh(input_win);

    // FUNCTION init
    string sendline, recvline;
    string state = "MENU";
    map<string,string> gui;
    gui["menu"] = "MENU\n  - login\n  - register\n  - exit\n";
    gui["lobby"] = "LOBBY\n  - create\n  - join <roomid>\n  - logout\n  - exit\n";
    gui["register"] = "REGISTER\n  - back\n  - <username> <password>\n  - exit\n";
    gui["login"] = "LOGIN\n  - back\n  - <username> <password>\n  - exit\n";
    gui["room"] = "ROOM\n  - back\n  - start (only host can do this)\n  - exit\n";
    gui["exit"] = "Server has disconnected you\n--Bye~ :D\n";
    gui["game"] = "GAME\n";
    gui["win"] = "RESULT\n  - back\n  - exit\n";

    // GAME init
    vector<pair<int,int>> hand;
    pair<int,int> currcard;
    bool recvHand = false;
    bool recvCard = false;
    bool myTurn = false;
    bool beingAdded = false;


    // Display initial menu
    werase(msg_win);
    box(msg_win, 0, 0);
    mvwprintw(msg_win, 1, 2, gui["menu"].c_str());
    wrefresh(msg_win);

    // Main loop
    for(;;)
    {
        // Check if socket has data
        fd_set readfds;
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 100000; // 100ms

        int ret = select(sockfd + 1, &readfds, NULL, NULL, &tv);
        if (ret > 0 && FD_ISSET(sockfd, &readfds))
        { 
            // Socket has data to read
            recvline.clear();
            if (Read(sockfd, recvline) == 0)
            {
                // Server closed connection
                werase(msg_win);
                box(msg_win, 0, 0);
                mvwprintw(msg_win, 1, 2, "--Server has disconnected you");
                mvwprintw(msg_win, 2, 2, "--Bye~ :D");
                wrefresh(msg_win);
                break;
            }
            else
            {
                stringstream ss(recvline);
                string command;
                while (getline(ss, command, '@'))
                {
                    if (command.empty())
                        continue;

                    if (state == "GAME"){
                        
                        if(command == "clr"){
                            // Clear windows
                            werase(msg_win);
                            werase(input_win);
                            werase(hand_win);
                            werase(card_win);
                            werase(err_win);
                            box(msg_win, 0, 0);
                            box(input_win, 0, 0);
                            box(hand_win, 0, 0);
                            box(card_win, 0, 0);
                            box(err_win, 0, 0);
                            wrefresh(msg_win);
                            wrefresh(input_win);
                            wrefresh(hand_win);
                            wrefresh(card_win);
                            wrefresh(err_win);
                        }
                        else if(command == "lobby"){
                            state = "LOBBY";
                            hand.clear();
                            werase(msg_win);
                            box(msg_win, 0, 0);
                            mvwprintw(msg_win, 1, 4, gui["lobby"].c_str());
                            wrefresh(msg_win);
                            wrefresh(room_win);
                        }
                        else if(command == "win"){
                            state = "WIN";
                            hand.clear();
                            werase(msg_win);
                            box(msg_win, 0, 0);
                            mvwprintw(msg_win, 1, 4, gui["win"].c_str());
                            wrefresh(msg_win);
                        }
                        else if(command == "hand"){
                            recvHand = true;
                        }
                        else if(command == "card"){
                            recvCard = true;
                        }
                        else if(command == "turn"){
                            myTurn = true;
                            werase(msg_win);
                            box(msg_win, 0, 0);
                            mvwprintw(msg_win, 1, 2, "It's your turn now");
                            mvwprintw(msg_win, 2, 2, "- Use #<card_id> to play a card");
                            mvwprintw(msg_win, 3, 2, "- #draw to draw a card");
                            wrefresh(msg_win);
                        }
                        else if(command == "add"){
                            beingAdded = true;
                        }
                        else if(command == "disp"){
                            display(hand_win, card_win, hand, currcard);
                        }
                        else{
                            if(command.empty()) continue;
                            if(recvHand){
                                stringstream sshand(command);
                                int color, number;
                                while(sshand >> color >> number){
                                    hand.emplace_back(color, number);
                                }
                                recvHand = false;
                                display(hand_win, card_win, hand, currcard);
                                continue;
                            }
                            if(recvCard){
                                stringstream sscard(command);
                                sscard >> currcard.first >> currcard.second;
                                recvCard = false;
                                show_curr_card(card_win, currcard);
                                continue;
                            }
                            // Display other game messages
                            werase(msg_win);
                            box(msg_win, 0, 0);
                            mvwprintw(msg_win, 1, 3, command.c_str());
                            wrefresh(msg_win);
                        }

                        continue;
                    }

                    if (command == "clr")
                    {
                        werase(stdscr);
                        refresh();
                    }
                    else if (command == "exit")
                    {
                        werase(msg_win);
                        box(msg_win, 0, 0);
                        mvwprintw(msg_win, 1, 4, gui["exit"].c_str());
                        wrefresh(msg_win);
                        close(sockfd);
                        goto cleanup;
                    }
                    else if (command == "game"){
                        werase(msg_win);
                        box(msg_win, 0, 0);
                        mvwprintw(msg_win, 1, 4, gui["game"].c_str());
                        wrefresh(msg_win);
                        state = "GAME";

                        // Initialize hand_win and card_win
                        hand_win = newwin(5, width - 50, 1, 2);
                        card_win = newwin(5, width - 50, 7, 2);
                        err_win = newwin(11, 35, 1, width - 48);
                        box(hand_win, 0, 0);
                        box(card_win, 0, 0);
                        wrefresh(hand_win);
                        wrefresh(card_win);
                        wrefresh(err_win);

                        continue;
                    }
                    else if (gui.find(command) != gui.end())
                    {
                        werase(msg_win);
                        box(msg_win, 0, 0);
                        mvwprintw(msg_win, 1, 4, gui[command].c_str());
                        wrefresh(msg_win);
                        transform(command.begin(), command.end(), command.begin(), ::toupper);
                        state = command;
                        continue;
                    }
                    else
                    {
                        if (command.empty())
                            continue;
                        

                        room_win = newwin(12, width-10, 1, 2);
                        werase(room_win);
                        box(room_win, 0, 0);
                        mvwprintw(room_win, 2, 2, command.c_str());
                        wrefresh(room_win);
                    }
                }
            }
        }

        nodelay(input_win, TRUE);

        // Handle user input
        werase(input_win);
        box(input_win, 0, 0);
        mvwprintw(input_win, 1, 2, "Input: %s", input_buffer.c_str());
        wrefresh(input_win);

        echo();
        int ch = wgetch(input_win);
        noecho();
        
        if (ch != ERR) {
            if (ch == '\n' || ch == '\r') {
                if (!input_buffer.empty()) {
                    sendline = input_buffer;
                    input_buffer.clear();
                    if (state == "GAME" && myTurn && sendline[0] == '#'){
                        string cardcommand = sendline.substr(1, sendline.size()-1);
                        // Remove newline character
                        if(cardcommand.find('\n') != string::npos){
                            cardcommand.erase(cardcommand.find('\n'));
                        }
                        if(cardcommand == "draw"){
                            if(currcard.first == -1){
                                werase(err_win);
                                box(err_win, 0, 0);
                                mvwprintw(err_win, 1, 2, "You are the first player. Why are you drawing a card?");
                                wrefresh(err_win);
                                continue;
                            }
                            sendline = "#draw";
                            beingAdded = false;
                            myTurn = false;
                        }
                        else if(isNumberS(cardcommand)){
                            stringstream sscard(cardcommand);
                            int cardpos, color;
                            sscard >> cardpos >> color;
                            
                            if(cardpos >= hand.size()){
                                werase(err_win);
                                box(err_win, 0, 0);
                                mvwprintw(err_win, 1, 2, "You don't have that many cards");
                                wrefresh(err_win);
                                continue;
                            }
                            else{
                                pair<int,int> sentcard = hand[cardpos];
                                if(beingAdded){
                                    if(sentcard.second == currcard.second || sentcard.second == ADD4){
                                        if(sentcard.second == ADD4){
                                            if(color == 0){
                                                werase(err_win);
                                                box(err_win, 0, 0);
                                                mvwprintw(err_win, 1, 2, "Wrong color input");
                                                wrefresh(err_win);
                                                continue;
                                            }
                                            else{
                                                sendline = "#" + to_string(sentcard.first) + " " + to_string(sentcard.second) + " " + to_string(color);
                                                beingAdded = false;
                                            }
                                        }
                                        else{
                                            sendline = "#" + to_string(sentcard.first) + " " + to_string(sentcard.second);
                                            beingAdded = false;
                                        }
                                    }
                                    else{
                                        if(sentcard.second == ADD2 && currcard.second == ADD4){
                                            werase(err_win);
                                            box(err_win, 0, 0);
                                            mvwprintw(err_win, 1, 2, "Current card is +4, but you played +2");
                                            wrefresh(err_win);
                                        }
                                        else{
                                            werase(err_win);
                                            box(err_win, 0, 0);
                                            mvwprintw(err_win, 1, 2, "You are being added cards, try to play +2 or +4");
                                            wrefresh(err_win);
                                        }
                                        continue;
                                    }
                                }
                                else if(currcard.first == -1 || sentcard.first == WILD || sentcard.first == currcard.first || sentcard.second == currcard.second){
                                    if(sentcard.first == WILD){
                                        if(color == 0){
                                            werase(err_win);
                                            box(err_win, 0, 0);
                                            mvwprintw(err_win, 1, 2, "Wrong color input");
                                            wrefresh(err_win);
                                            continue;
                                        }
                                        else{
                                            sendline = "#" + to_string(sentcard.first) + " " + to_string(sentcard.second) + " " + to_string(color);
                                            beingAdded = false;
                                        }
                                    }
                                    else{
                                        sendline = "#" + to_string(sentcard.first) + " " + to_string(sentcard.second);
                                    }
                                }
                                else{
                                    werase(err_win);
                                    box(err_win, 0, 0);
                                    mvwprintw(err_win, 1, 2, "You cannot play this card");
                                    wrefresh(err_win);
                                    continue;
                                }
                                hand.erase(hand.begin() + cardpos);
                                myTurn = false;
                                display(hand_win, card_win, hand, currcard);
                            }
                        }
                        else{
                            werase(err_win);
                            box(err_win, 0, 0);
                            mvwprintw(err_win, 1, 2, "Invalid command");
                            wrefresh(err_win);
                            continue;
                        }
                    }

                    Write(sockfd, sendline);
                    werase(input_win);
                    box(input_win, 0, 0);
                    mvwprintw(input_win, 1, 2, "Input: ");
                    wrefresh(input_win);
                }
            } else if (ch == KEY_BACKSPACE || ch == 127 || ch == '\b') {
                if (!input_buffer.empty()) {
                    input_buffer.pop_back();
                }

                werase(input_win);
                box(input_win, 0, 0);
                mvwprintw(input_win, 1, 2, "Input: %s", input_buffer.c_str());
                wrefresh(input_win);
            } else {
                input_buffer += static_cast<char>(ch);
                werase(input_win);
                box(input_win, 0, 0);
                mvwprintw(input_win, 1, 2, "Input: %s", input_buffer.c_str());
                wrefresh(input_win);
            }
        }    
    }

cleanup:
    // Cleanup ncurses
    delwin(hand_win);
    delwin(card_win);
    delwin(msg_win);
    delwin(input_win);
    endwin();
}

int main(int argc, char **argv)
{
    setlocale(LC_ALL, "");

    int sockfd;
    struct sockaddr_in servaddr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);

    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(15023);
    inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
    // inet_pton(AF_INET, "140.113.235.151", &servaddr.sin_addr);

    if(connect(sockfd, (SA *)&servaddr, sizeof(servaddr)) < 0){
        perror("Connect failed");
        exit(1);
    }

    menu(sockfd); /* Execute main function */

    close(sockfd);
    exit(0);
}
