/* include fig01 */
#include	"../unp.h"
#include <bits/stdc++.h>
using namespace std;

int Read(int fd, string &s){
	s.clear();
	int n = MAXLINE;
	char buffer[MAXLINE];
	int count = read(fd, buffer, n);
	if(count-1 > 0 && buffer[count-1] == '\n'){
		buffer[count-1] = '\0';
	}else{
		buffer[count] = '\0';
	}
	s = string(buffer);
	return count;
}

int Write(int fd, string s){
	return write(fd, s.c_str(), s.length());
}

string playerlist(string user[], string state[], int maxi, int myself){
	bool isZero = true;
	string list = "╔════════════════════╦════════════════════╗\n";
	list += "║player              ║status              ║\n";
	list += "╠════════════════════╬════════════════════╣\n";
	for(int i = 0 ; i <= maxi ; i++){
		if(user[i].empty() || i == myself) continue;
		isZero = false;
		string username = user[i], status = state[i];
		username.resize(20,' ');
		status.resize(20, ' ');
		list += "║" + username + "║" + status + "║\n";
	}
	if(isZero) list += "║             no player online            ║\n";
	list += "╚════════════════════╩════════════════════╝\n";
	return list;

}

int
main(int argc, char **argv)
{
	int					i, maxi, maxfd, listenfd, connfd, sockfd;
	int					nready, client[FD_SETSIZE];
	ssize_t				n;
	fd_set				rset, allset;
	char				buf[MAXLINE];
	socklen_t			clilen;
	struct sockaddr_in	cliaddr, servaddr;

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
	struct linger ling;
	ling.l_linger = 0;
	ling.l_onoff = 1;
	setsockopt(listenfd, SOL_SOCKET, SO_LINGER, &ling, sizeof(ling));

	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port        = htons(15023);

	if(bind(listenfd, (SA *) &servaddr, sizeof(servaddr)) < 0){
		printf("bind fail...\n");
		exit(0);
	}

	listen(listenfd, LISTENQ);

	maxfd = listenfd;			/* initialize */
	maxi = -1;					/* index into client[] array */
	for (i = 0; i < FD_SETSIZE; i++)
		client[i] = -1;			/* -1 indicates available entry */
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);
/* end fig01 */

	printf("server is running...\n");
	string sendline, recvline, cliip[FD_SETSIZE];
	string state[FD_SETSIZE] = {"MENU"};
	string isLogin[FD_SETSIZE] = {""};
	map<string,string> user;
	string Console = "@Console -> ";

/* include fig02 */
	for ( ; ; ) {
		rset = allset;		/* structure assignment */
		nready = select(maxfd+1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(listenfd, &rset)) {	/* new client connection */
			clilen = sizeof(cliaddr);
			connfd = accept(listenfd, (SA *) &cliaddr, &clilen);

			for (i = 0; i < FD_SETSIZE; i++)
				if (client[i] < 0) {
					client[i] = connfd;	/* save descriptor */
					break;
				}
			if (i == FD_SETSIZE)
				printf("too many clients\n");

			cliip[i] = string(inet_ntoa(cliaddr.sin_addr));
			Write(fileno(stdout), "--player from [" + cliip[i] +"] connected--\n");
			Write(connfd, "@menu" + Console);

			FD_SET(connfd, &allset);	/* add new descriptor to set */
			
			if (connfd > maxfd)
				maxfd = connfd;			/* for select */
			if (i > maxi)
				maxi = i;				/* max index in client[] array */

			if (--nready <= 0)
				continue;				/* no more readable descriptors */
		}

		for (i = 0; i <= maxi; i++) {	/* check all clients for data */
			if ( (sockfd = client[i]) < 0)
				continue;
			if (FD_ISSET(sockfd, &rset)) {
				if ( (n = Read(sockfd, recvline)) == 0) {
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
				}

				if(recvline == "terminate"){
					for(i = 0; i <= maxi; i++){
						if(client[i] < 0 || client[i] == sockfd) continue;
						close(client[i]);
						FD_CLR(client[i], &allset);
					}
					close(listenfd);
					FD_CLR(listenfd, &allset);
					close(sockfd);
					FD_CLR(sockfd, &allset);
					exit(0);
				}
				
				if(recvline == "exit"){
					Write(sockfd, "@exit");
					Write(fileno(stdout), "--player from [" + cliip[i] +"] disconnected--\n");
					close(sockfd);
					FD_CLR(sockfd, &allset);
					client[i] = -1;
					state[i] = "MENU";
					isLogin[i].clear();
				}else if(recvline == "menu"){
					state[i] = "MENU";
					Write(sockfd, (!isLogin[i].empty()) ?"@lobby" :"@menu");
				}else if(recvline == "register"){
					state[i] = (!isLogin[i].empty()) ? "MENU" : "REGISTER";
					Write(sockfd, (!isLogin[i].empty()) ?"@please logout first\n" :"@register");
				}else if(recvline == "login"){
					state[i] = "LOGIN";
					Write(sockfd, (!isLogin[i].empty()) ?"@already login\n" :"@login");
				}else if(recvline == "logout"){
					state[i] = "MENU";
					if(!isLogin[i].empty()){
						isLogin[i].clear();
						Write(sockfd, "@logout successful\n");
						Write(sockfd, "@menu");
					}else{
						Write(sockfd, "@you haven't login yet\n");
						Write(sockfd, "@menu");
					}
				}else if(recvline == "refresh"){
					if((isLogin[i].empty())){
						Write(sockfd, "@you haven't login yet\n");
						Write(sockfd, "@menu");
						state[i] = "MENU";
					}
					Write(sockfd, playerlist(isLogin, state, maxi, i));
					Write(sockfd, "@lobby");
				}else{
					if(state[i] == "REGISTER"){
						stringstream ss(recvline);
						string username, password;
						ss >> username >> password;

						if(user.find(username) != user.end()){
							Write(sockfd, "@username existed\n");
						}else{
							user[username] = password;
							Write(sockfd, "@register succeed\n");
							Write(sockfd, "@menu");
							state[i] = "MENU";
						}
					}else if(state[i] == "LOGIN"){
						stringstream ss(recvline);
						string username, password;
						ss >> username >> password;

						if(user.find(username) == user.end()){
							Write(sockfd, "@username not exist\n");
						}else if(user[username] == password){
							state[i] = "LOBBY";
							isLogin[i] = username;
							Write(sockfd, "@login successful\n");
							Write(sockfd, playerlist(isLogin, state, maxi, i));
							Write(sockfd, "@lobby");
						}
					}else{
						Write(sockfd, "@\"" + recvline + "\" is not a valid command\n");
					}
				}

				Write(sockfd, Console);

				if (--nready <= 0)
					break;				/* no more readable descriptors */
			}
		}
	}
}
/* end fig02 */
