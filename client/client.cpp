#include	"../unp.h"
#include <bits/stdc++.h>
using namespace std;

int Read(int fd, string &s){
	int n = MAXLINE;
	char buffer[MAXLINE];
	int count = read(fd, buffer, n);
	buffer[count] = '\0';
	s = string(buffer);
	return count;
}

int Write(int fd, string s){
	return write(fd, s.c_str(), s.length());
}

void menu(FILE *fp, int sockfd)
{
    // FUNCTION init
	string sendline, recvline, student, cliip, outmsg;
	string dash = "===========================\n";
	string Menu = dash + "MENU\n- login\n- register\n- exit\n";
	string Lobby = dash + "LOBBY\n- refresh\n- logout\n- exit\n";
	string Register = dash + "Register\n- <username> <password>\n- menu\n- exit\n";
	string Login = dash + "Login\n- <username> <password>\n- menu\n- exit\n";
	string Exit = dash + "--Server has disconnect you\n--Bye~ :D\n";
    // SELECT init
	int			maxfdp1, stdineof;
	fd_set		rset;
	char		buf[MAXLINE];
	int		n;

    // GETHOST init
    char hostip[INET6_ADDRSTRLEN];
    struct hostent *hptr;

	stdineof = 0;
	FD_ZERO(&rset);
	for ( ; ; ) {
		if (stdineof == 0)
			FD_SET(fileno(fp), &rset);
		FD_SET(sockfd, &rset);
		maxfdp1 = max(fileno(fp), sockfd) + 1;
		select(maxfdp1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(sockfd, &rset)) {	/* socket is readable */
			if ( (n = Read(sockfd, recvline)) == 0) {
				if (stdineof == 1)
					return;		/* normal termination */
				else
					printf("Server Close\n");
					return;
			}

			stringstream ss(recvline);
			string command;
			while(getline(ss, command, '@')){
				if(command == "menu"){
					Write(fileno(fp), Menu);
				}else if(command == "lobby"){
					Write(fileno(fp), Lobby);
				}else if(command == "register"){
					Write(fileno(fp), Register);
				}else if(command == "login"){
					Write(fileno(fp), Login);
				}else if(command == "exit"){
					Write(fileno(fp), Exit);
					close(sockfd);
					return;
				}else{
					if(command.empty()) continue;
					Write(fileno(fp), command);
				}
			}
		}

		if (FD_ISSET(fileno(fp), &rset)) {  /* input is readable */
			if ( (n = Read(fileno(fp), recvline)) == 0) {
				stdineof = 1;
				shutdown(sockfd, SHUT_WR);	/* send FIN */
				FD_CLR(fileno(fp), &rset);
				continue;
			}
			Write(sockfd, recvline);
		}
	}
}

int
main(int argc, char **argv)
{
	int					sockfd;
	struct sockaddr_in	servaddr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_port = htons(15023);
	inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
	// inet_pton(AF_INET, "140.113.235.151", &servaddr.sin_addr);

	connect(sockfd, (SA *) &servaddr, sizeof(servaddr));

	menu(stdin, sockfd);		/* do it all */

	exit(0);
}
