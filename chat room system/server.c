//#include	"unp.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <sys/select.h>
#include <sys/time.h>
#include <errno.h>
#include <arpa/inet.h>
#define MAXLINE 4096
#define LISTENQ 1024
#define max(x,y) (x>y?x:y)
struct clienttt{
	int client_int;
	char client_string[14];
}clientt[FD_SETSIZE];


void err_sys(const char* x){
	perror(x);
	exit(EXIT_FAILURE);
}
ssize_t readline(int fd, void *vptr, size_t maxlen)
{
	ssize_t	n, rc;
	char	c, *ptr;

	ptr = vptr;
	for (n = 1; n < maxlen; n++) {
again:
		if ( (rc = read(fd, &c, 1)) == 1) {
			*ptr++ = c;
			if (c == '\n')
				break;	/* newline is stored, like fgets() */
		} else if (rc == 0) {
			*ptr = 0;
			return(n - 1);	/* EOF, n - 1 bytes were read */
		} else {
			if (errno == EINTR)
				goto again;
			return(-1);		/* error, errno set by read() */
		}
	}
	*ptr = 0;	/* null terminate like fgets() */
	return(n);
}
ssize_t Readline(int fd, void *ptr, size_t maxlen)
{
	ssize_t		n;

	if ( (n = readline(fd, ptr, maxlen)) < 0)
		err_sys("readline error");
	return(n);
}
/* end readline */
///////////////////////////////////////////////////////////////////
ssize_t						/* Read "n" bytes from a descriptor. */
readn(int fd, void *vptr, size_t n)
{
	size_t	nleft;
	ssize_t	nread;
	char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nread = read(fd, ptr, nleft)) < 0) {
			if (errno == EINTR)
				nread = 0;		/* and call read() again */
			else
				return(-1);
		} else if (nread == 0)
			break;				/* EOF */

		nleft -= nread;
		ptr   += nread;
	}
	return(n - nleft);		/* return >= 0 */
}
/* end readn */

ssize_t
Readn(int fd, void *ptr, size_t nbytes)
{
	ssize_t		n;

	if ( (n = readn(fd, ptr, nbytes)) < 0)
		err_sys("readn error");
	return(n);
}
///////////////////////////////////////////////////////////////
ssize_t						/* Write "n" bytes to a descriptor. */
writen(int fd, const void *vptr, size_t n)
{
	size_t		nleft;
	ssize_t		nwritten;
	const char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;		/* and call write() again */
			else
				return(-1);			/* error */
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}
void
Writen(int fd, void *ptr, size_t nbytes)
{
	if (writen(fd, ptr, nbytes) != nbytes)
		err_sys("writen error");
}
/* end writen */
////////////////////////////////////////////////////////////////////
void Listen (int fd, int backlog){
	char *ptr;
	if((ptr = getenv("LISTENQ")) != NULL)
		backlog = atoi (ptr);
	if(listen(fd,backlog)<0)
		err_sys("listen error");
}
///////////////////////////////////////////////////////

int main(int argc, char **argv)
{
	int					i, maxi, maxfd, listenfd, connfd, sockfd, sendoutfd;
	int					nready;
	ssize_t				n;
	fd_set				rset, allset;
	char				buf[MAXLINE];
	char				iamgod[10] = "[Server] ";
	char				finalsend[MAXLINE],middlesend[MAXLINE];
	char*				temp_client_string = malloc(sizeof(int)*14);

	socklen_t			clilen;
	struct sockaddr_in	cliaddr, servaddr,checkaddr;
	socklen_t checkaddr_len = sizeof(checkaddr);

	if (argc != 2)
		perror("usage: ./server <server_port>");

	listenfd = socket(AF_INET, SOCK_STREAM, 0);
  if(listenfd<0){
    perror("listenfd");
    exit (EXIT_FAILURE);
  }
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family      = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	int server_port = atoi(argv[1]);
	servaddr.sin_port = htons(server_port);

	if(bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr))<0){
    perror ("bind");
    exit (EXIT_FAILURE);
  }

	Listen(listenfd, LISTENQ);

	maxfd = listenfd;			/* initialize */
	maxi = -1;					/* index into client[] array */
	for (i = 0; i < FD_SETSIZE; i++){
		clientt[i].client_int = -1;
	}
					/* -1 indicates available entry */
	FD_ZERO(&allset);
	FD_SET(listenfd, &allset);
/* end fig01 */

/* include fig02 */
	for ( ; ; ) {
		rset = allset;		/* structure assignment */
		nready = select(maxfd+1, &rset, NULL, NULL, NULL);

		if (FD_ISSET(listenfd, &rset)) {	/* new client connection */
			clilen = sizeof(cliaddr);
			connfd = accept(listenfd, (struct sockaddr *) &cliaddr, &clilen);
			if(connfd<0){
				perror ("accept error");
				exit(EXIT_FAILURE);
			}
//#ifdef	NOTDEF
			char haha[INET_ADDRSTRLEN];
			sprintf(finalsend, "[Server] Hello, anonymous! From: %s:%d\n",inet_ntop(AF_INET, &cliaddr.sin_addr, haha, INET_ADDRSTRLEN),ntohs(cliaddr.sin_port));
			Writen(connfd, finalsend, strlen(finalsend));
			memset(finalsend,'\0',sizeof(finalsend));

//#endif
			int current_client_num;
			for (i = 0; i < FD_SETSIZE; i++){   //just set one client
				if (clientt[i].client_int < 0) {
					clientt[i].client_int = connfd;	/* save descriptor */
					strcpy(clientt[i].client_string,"anonymous");
					break;
				}
			}
			if (i == FD_SETSIZE)
				err_sys("too many clients");

			FD_SET(connfd, &allset);	/* add new descriptor to set */
			if (connfd > maxfd)
				maxfd = connfd;			/* for select */
			if (i > maxi)
				maxi = i;				/* max index in client[] array */

							/* no more readable descriptors */
			for(int j=0; j<=maxi; j++){
				sendoutfd = clientt[j].client_int;
				if ((sendoutfd < 0)||(connfd==sendoutfd))	continue;
				else{
					sprintf(finalsend, "[Server] Someone is coming!\n");
					Writen(sendoutfd, finalsend, strlen(finalsend));
					memset(finalsend,'\0',sizeof(finalsend));
				}
			}
			if (--nready <= 0)
				continue;
		}

		for (i = 0; i <= maxi; i++) {
			memset(buf,'\0',sizeof(buf));
			if ( (sockfd = clientt[i].client_int) < 0){
				continue;
			}
			if (FD_ISSET(sockfd, &rset)) {
				n = Readline(sockfd, buf, MAXLINE);
        if ( n == 0) {
						/*4connection closed by client */
					if(close(sockfd)){
						perror("close error");
						exit(EXIT_FAILURE);
					}
					FD_CLR(sockfd, &allset);
					clientt[i].client_int = -1;
					memset(clientt[i].client_string,'\0',sizeof(clientt[i].client_string));
					goto loop_continue;
				}
				else if((buf[0]=='n')&&(buf[1]=='a')&&(buf[2]=='m')&&(buf[3]=='e')&&(buf[4]==' ')){
					temp_client_string = strtok(buf," ");
					temp_client_string = strtok(NULL," ");
					temp_client_string[strlen(temp_client_string)]=0;
					temp_client_string[strlen(temp_client_string)-1]='\0';
					if(strcmp(temp_client_string,"anonymous")==0){
						sprintf(finalsend, "[Server] ERROR: Username cannot be anonymous.\n");
						Writen(sockfd, finalsend, strlen(finalsend));
						memset(finalsend,'\0',sizeof(finalsend));
						goto loop_continue;
					}
					if((strlen(temp_client_string)<2)||(strlen(temp_client_string)>12)){
						sprintf(finalsend, "[Server] ERROR: Username can only consists of 2~12 English letters.\n");
						Writen(sockfd, finalsend, strlen(finalsend));
						memset(finalsend,'\0',sizeof(finalsend));
						goto loop_continue;
					}
					for(int k=0; k<strlen(temp_client_string)-1; k++){
						if( ((temp_client_string[k]<65)||(temp_client_string[k]>122)) || ((temp_client_string[k]>90)&&(temp_client_string[k]<97)) ){
							sprintf(finalsend, "[Server] ERROR: Username can only consists of 2~12 English letters.\n");
							Writen(sockfd, finalsend, strlen(finalsend));
							memset(finalsend,'\0',sizeof(finalsend));
							goto loop_continue;
						}
					}

					for(int k=0; k<=maxi; k++){
						if(k==i)	continue;
						else if(clientt[k].client_int < 0 )	continue;
						else if(strcmp(temp_client_string,clientt[k].client_string)==0){
							sprintf(finalsend, "[Server] ERROR: %s has been used by others.\n",clientt[k].client_string);
							Writen(sockfd, finalsend, strlen(finalsend));
							memset(finalsend,'\0',sizeof(finalsend));
							goto loop_continue;
						}
					}
					sprintf(finalsend, "[Server] You're now known as %s.\n",temp_client_string);
					Writen(sockfd, finalsend, strlen(finalsend));
					memset(finalsend,'\0',sizeof(finalsend));

					for(int j=0; j<=maxi; j++){
						sendoutfd = clientt[j].client_int;
						if ((sendoutfd < 0)||(sockfd==sendoutfd))	continue;
						else {
							sprintf(finalsend, "[Server] %s is now known as %s.\n",clientt[i].client_string,temp_client_string);
							Writen(sendoutfd, finalsend, strlen(finalsend));
							memset(finalsend,'\0',sizeof(finalsend));
						}
					}
					strcpy(clientt[i].client_string,temp_client_string);
					memset(temp_client_string,'\0',sizeof(temp_client_string));
				}
				else if((buf[0]=='w')&&(buf[1]=='h')&&(buf[2]=='o')&&(buf[3]=='\n')){
					for(int j=0; j<=maxi; j++){
						if(clientt[j].client_int > 0){
							int getpeer = getpeername(clientt[j].client_int,(struct sockaddr *) &checkaddr, &checkaddr_len);
							if( getpeer < 0 ){
								perror("getpeername failed");
								exit(EXIT_FAILURE);
							}
							if(j==i){
								sprintf(finalsend, "[Server] %s %s:%d ->me\n",clientt[j].client_string,inet_ntoa(checkaddr.sin_addr),(int)ntohs(checkaddr.sin_port));
								Writen(sockfd, finalsend, strlen(finalsend));
								memset(finalsend,'\0',sizeof(finalsend));
							}
							else{
								sprintf(finalsend, "[Server] %s %s:%d\n",clientt[j].client_string,inet_ntoa(checkaddr.sin_addr),(int)ntohs(checkaddr.sin_port));
								Writen(sockfd, finalsend, strlen(finalsend));
								memset(finalsend,'\0',sizeof(finalsend));
							}
						}
					}
				}
				else if((buf[0]=='t')&&(buf[1]=='e')&&(buf[2]=='l')&&(buf[3]=='l')&&(buf[4]==' ')){
					temp_client_string = strtok(buf," ");
					temp_client_string = strtok(NULL," ");
					temp_client_string[strlen(temp_client_string)]=0;
					if(strcmp(temp_client_string,"anonymous")==0){
						sprintf(finalsend, "[Server] ERROR: The client to which you sent is anonymous.\n");
						Writen(sockfd, finalsend, strlen(finalsend));
						memset(finalsend,'\0',sizeof(finalsend));
					}else if(strcmp(clientt[i].client_string,"anonymous")==0){
						sprintf(finalsend, "[Server] ERROR: You are anonymous.\n");
						Writen(sockfd, finalsend, strlen(finalsend));
						memset(finalsend,'\0',sizeof(finalsend));
					}else{
						for(int j=0; j<=maxi; j++){
							if ((sendoutfd = clientt[j].client_int) < 0) continue;
							else if(strcmp(clientt[j].client_string, temp_client_string)==0){
								sprintf(finalsend, "[Server] SUCCESS: Your message has been sent.\n");
								Writen(sockfd, finalsend, strlen(finalsend));
								memset(finalsend,'\0',sizeof(finalsend));

								strcpy(middlesend, buf+strlen(temp_client_string)+6);
								sprintf(finalsend, "[Server] %s tell you %s",clientt[i].client_string,middlesend);
								Writen(sendoutfd, finalsend, strlen(finalsend));
								memset(finalsend,'\0',sizeof(finalsend));
								goto loop_continue;
							}
						}
						sprintf(finalsend, "[Server] ERROR: The receiver doesn't exist.\n"); //didn't find out
						Writen(sockfd, finalsend, strlen(finalsend));
						memset(finalsend,'\0',sizeof(finalsend));
					}
					memset(temp_client_string,'\0',sizeof(temp_client_string));
				}
				else if((buf[0]=='y')&&(buf[1]=='e')&&(buf[2]=='l')&&(buf[3]=='l')&&(buf[4]==' ')){
					for(int j=0; j<=maxi; j++){ //broadcast
						if ( (sendoutfd = clientt[j].client_int) < 0)
							continue;
						else{
							sprintf(finalsend,"[Server] %s ",clientt[i].client_string);
							strcat(finalsend,buf);
							Writen(sendoutfd, finalsend, strlen(finalsend));
							memset(finalsend,'\0',sizeof(finalsend));
						}
					}
				}
				else if((buf[0]=='e')&&(buf[1]=='x')&&(buf[2]=='i')&&(buf[3]=='t')&&(buf[4]=='\n')){
					for(int j=0; j<=maxi; j++){ //broadcast
						if (((sendoutfd = clientt[j].client_int) < 0)|| (i==j))
							continue;
						else{
							sprintf(finalsend,"[Server] %s is offline.\n",clientt[i].client_string);
							Writen(sendoutfd, finalsend, strlen(finalsend));
							memset(finalsend,'\0',sizeof(finalsend));
							}
					}
					FD_CLR(sockfd, &allset);
					clientt[i].client_int = -1;
					memset(clientt[i].client_string,'\0',sizeof(clientt[i].client_string));
				}
				else{
					sprintf(finalsend, "[Server] ERROR: Error command.\n"); //error
					Writen(sockfd, finalsend, strlen(finalsend));
					memset(finalsend,'\0',sizeof(finalsend));
				}
loop_continue:
				if (--nready <= 0)
					break;				/* no more readable descriptors */
			}
		}
	}
	free(temp_client_string);
}
