#include "../unp.h"
#include <bits/stdc++.h>
using namespace std;

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

void menu(FILE *fp, int sockfd)
{
	set_scr();
	clr_scr();
	// FUNCTION init
	string sendline, recvline, student, cliip, outmsg;
	map<string,string> gui;
	gui["menu"] = "===========================\nMENU\n- login\n- register\n- exit\n";
	gui["lobby"] = "===========================\nLOBBY\n- create\n- join <roomid>\n- logout\n- exit\n";
	gui["register"] = "===========================\nRegister\n- back\n- <username> <password>\n- exit\n";
	gui["login"] = "===========================\nLogin\n- back\n- <username> <password>\n- exit\n";
	gui["room"] = "===========================\nROOM\n- back\n- start (only host can do this)\n- exit\n";
	gui["exit"] = "===========================\n--Server has disconnect you\n--Bye~ :D\n";

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
					else if (gui.find(command) != gui.end())
					{
						cout << gui[command];
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
			recvline = string(buffer);
			Write(sockfd, recvline);
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
