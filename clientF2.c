#include <signal.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <stdio.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
int sockfd;

void error(char *s)
{
	perror(s);
	exit(1);
}
void handler(int sig)
{
	int bytesSent = send(sockfd,"exit\n",strlen("exit\n"),0);
	if(bytesSent<0)
		error("Error sending message\n");
	close(sockfd);
	exit(0);
}
void *readFunc(void *arg)
{
	char buff[256];
	int bytesRec;
	while(1)
	{
	memset(buff,'\0',256);
	bytesRec = read(sockfd,buff,255);
	if(bytesRec<0)
		error("Error reading from socket\n");
	if(buff[0]!='\0')
	{
	//fputs("\b\b\b>> ",stdout);
	write(1,buff,strlen(buff));
	}
	}
}
void *writeFunc(void *arg)
{
	char buff[256];
	int bytesRec,bytesSent,buffLen,rem;
	while(1)
	{
	//("\b\b\b<< ");
	memset(buff,'\0',256);
	read(0,buff,256);
	buffLen = strlen(buff);
	//buff[buffffLen-1] = '\0';
	bytesSent = send(sockfd,buff,buffLen,0);
	//printf("Bytes Send = %d\n",bytesSent);
	if(bytesSent <= 0)
		error("Error Sending message\n");
	while(bytesSent!=buffLen)
	{
		rem = buffLen-bytesSent;
		bytesSent = send(sockfd,buff+bytesSent,rem,0);
		if(bytesSent<0)
		{
			error("Error Sending message\n");
			break;
		}
		buffLen = rem;
	}
	}	
}
int main(int argc, char *argv[])
{
	if (argc < 3) {
       fprintf(stderr,"usage %s hostname port\n", argv[0]);
       exit(0);
    }
    pthread_t tid[2];
    char ch;
	char *destIP = argv[1];
	short port = atoi(argv[2]);
	char buff[200];
	int connFlag,bytesSent,buffLen,bytesRec,rem;
	struct sockaddr_in dest;
	sockfd = socket(AF_INET,SOCK_STREAM,0);
	if(sockfd<0)
		error("Error Opening Socket");
	dest.sin_family = AF_INET;
	dest.sin_port = htons(port);
	dest.sin_addr.s_addr = inet_addr(destIP);
	memset(&(dest.sin_zero),'\0',8);
	connFlag = connect(sockfd,(struct sockaddr *)&dest,sizeof(struct sockaddr));
	if(connFlag < 0)
		error("Connection Unsuccessful\n");
	signal(SIGINT,handler);
	pthread_create(&tid[1],NULL,readFunc,0);
	pthread_create(&tid[0],NULL,writeFunc,0);
	pthread_join(tid[0],NULL);
	pthread_join(tid[1],NULL);
	close(sockfd);
	return 0;
}
