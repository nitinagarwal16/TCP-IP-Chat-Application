#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h> 
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include "sqliteHelper.h"
int sockfd;
void error(char *s);
void mysend(int sock,const void *buf,size_t len,int flags)
{
	int bytesSent;
	bytesSent = send(sock,buf,len,flags);
	if(bytesSent<0)
		error("Error Sending to Socket\n");
}
void myread(int sock,void *buf,size_t c)
{
	int bytesRec;
	bytesRec = read(sock,buf,c);
	if(bytesRec<0)
		error("Error reading from Socket\n");
}
void handler(int sig)
{
	close(sockfd);
	clearData();
	exit(0);
}
void error(char *s)
{
	clearData();
	perror(s);
	exit(1);
}
void *handleClient(void *arg)
{
	char uname[20]="",passwd[20]="",destUser[20]="",inString[100]="";
	char destMsg[90] = "";
	int bytesRec,bytesSent,status;
	int sock = *(int *)arg;
	printf("socket=%d\n",sock );
	int destSock;
	char choice[10] = "";
	char *msg = (char *)malloc(100);
	/*bzero(choice,2);
	bzero(uname,20);
	bzero(passwd,20);
	bzero(destUser,20);
	bzero(inString,100);*/
	//mysend(sock,"Welcome\n",strlen("Welcome\n"),0);
	char *menu = "1. SignUp\n2. Login\n";
	char *UserEnter = "Enter Username: ";
	char *passEnter = "Enter Password: ";
	mysend(sock,menu,strlen(menu),0);
	myread(sock,choice,2);
	choice[strlen(choice)-1]='\0';
	mysend(sock,UserEnter,strlen(UserEnter),0);
	myread(sock,uname,20);	
	uname[strlen(uname)-1]='\0';
	mysend(sock,passEnter,strlen(passEnter),0);
	myread(sock,passwd,20);
	passwd[strlen(passwd)-1]='\0';
	
	if(strcmp(choice,"1")==0)
	{
		/*printf("In SignUp\n");
		mysend(sock,"Enter Username: ",strlen("Enter Username: "),0);
		myread(sock,uname,20);
		
		mysend(sock,"Enter Password: ",strlen("Enter Password: "),0);
		
		myread(sock,passwd,20);*/
		printf("%s %s",uname,passwd);
		int retVal = insertNewUser(uname,passwd,sock);
		if(retVal==-1)
		{
			close(sock);
			pthread_exit(0);
		}
	}
	else
	{
		//mysend(sock,"Enter Username: ",strlen("Enter Username: "),0);
		//myread(sock,uname,20);
		//mysend(sock,"Enter Password: ",strlen("Enter Password: "),0);
		//myread(sock,passwd,20);
		checkLogin(uname,passwd,sock,msg);
		mysend(sock,msg,strlen(msg),0);
		if(strcmp(msg,"Login Not Successful\n")==0)
		{
			close(sock);
			pthread_exit(0);
		}
	}
	strcpy(destMsg,"At any time enter \"disp\" to display online users.\nMessage format: <RecepientName>: <Message>\n");
	//dispOnlineUsers(uname,sock);
	mysend(sock,destMsg,strlen(destMsg),0);
	bzero(destMsg,90);
	while(1)
	{
		memset(destUser,'\0',20);
		memset(inString,'\0',100);
    	bytesRec = read(sock,inString,100);
   		//inString[strlen(inString)-1]='\0';
    	if(bytesRec<0)
    		error("Error reading from socket\n");
    	if(strcmp(inString,"exit\n")==0)
    	{
    		clearUserData(uname);
    		close(sock);
    		pthread_exit(0);
    	}
    	if(strcmp(inString,"disp\n")==0)
    	{
    		dispOnlineUsers(uname,sock);
    		mysend(sock,"\n",1,0);
    		continue;
    	}
    	strncpy(destUser,inString,20);
    	int indexOfChar = strcspn(destUser,":");
    	destUser[indexOfChar] = '\0';
    	char *destinationmsg = inString;
    	destinationmsg=destinationmsg+indexOfChar+2;
    	printf("destinaion usr = %s\n destinationmsg = %s\n",destUser,destinationmsg);
    	status = checkOnline(destUser);
    	printf("status = %d\n",status);
    	char message[30] = {0};
    	if(status == 0)
    	{
    		snprintf(message,30,"User Not Online\n");
    		mysend(sock,message,strlen(message),0);
    	}
    	else if(status == 2)
    	{
    		snprintf(message,30,"No User named %s\n",destUser);
    		mysend(sock,message,strlen(message),0);
    	}
    	else
    	{
    		destSock = getSock(destUser);
    		printf("destSock = %d\n",destSock);
    		strcpy(destMsg,destinationmsg);
    		snprintf(destMsg,90,"%s>> %s",uname,destinationmsg);
    		mysend(destSock,destMsg,80,0);
    	}
	}
}
int main(int argc, char *argv[])
{
	 createTable();
	 int newsockfd, port,bindFlag,bytesRec,bytesSent,rem,buffLen,cliLen;
     struct sockaddr_in serv_addr, cli_addr;
     int n;
     int i = 0;
     pthread_t tid[10];
     if (argc != 2) {
         fprintf(stderr,"usage %s port\n", argv[0]);
         exit(1);
     }
     sockfd = socket(AF_INET, SOCK_STREAM, 0);
     if (sockfd < 0) 
     {
        fprintf(stderr, "ERROR opening socket\n");
        exit(1);
     }
    port = atoi(argv[1]);
    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(port);
    serv_addr.sin_addr.s_addr = INADDR_ANY; /*When INADDR_ANY is specified in the bind call,
    										  the socket will be bound to all local interfaces*/
    memset(&(serv_addr.sin_zero),'\0',8);
   
    bindFlag = bind(sockfd,(struct sockaddr *)&serv_addr,sizeof(struct sockaddr));
    if(bindFlag<0)
    {
    	error("Error binding socket\n");
    }
    listen(sockfd,4);
 	signal(SIGINT,handler);
    cliLen = sizeof(cli_addr);
    while(1)
    {
	    newsockfd = accept(sockfd,(struct sockaddr *) &cli_addr, &cliLen);
	    if (newsockfd < 0)
		    { 
		          fprintf(stderr, "ERROR on accept\n");
		          exit(1);
		    }
		fprintf(stdout,"\nClient %s connected at port %d\n",inet_ntoa(cli_addr.sin_addr), cli_addr.sin_port);    
		pthread_create(&tid[i++],NULL,handleClient,&newsockfd);	    
	}
	

}