#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <sqlite3.h> 
#include <sys/types.h>
#include <sys/socket.h>
#include <string.h>
#include <unistd.h>
sqlite3 *db;
char *zErrMsg = 0;
pthread_t tid[10];
static int callback1(void *data, int argc, char **argv, char **azColName)
{
	printf("In callback1\n");
   int *p = (int *)data;
   printf("%s\n",argv[0]);
   *p=atoi(argv[0]);
   printf("%d\n",*p); 
   return 0;
}
static int callback2(void *data, int argc, char **argv, char **azColName)
{
	printf("In callback2\n");
	int sock = *(int *)data;	
    char buff[25];
    bzero(buff,25);
    if(argc==0)
    	send(sock,"No Online Users\n",strlen("No Online Users\n"),0);
    else
    {
    	snprintf(buff,25,"%s ",argv[0]);
    	send(sock,buff,strlen(buff),0);
    }
    return 0;
}
int executeSql(char *q,void *data)
{
	int tmp ;
	tmp = *(int *)data;
	int rc;
	if(tmp==0)
		rc = sqlite3_exec(db, q, 0, data, &zErrMsg);
	else
		rc = sqlite3_exec(db, q, callback1, data, &zErrMsg);
   	if( rc != SQLITE_OK )
   {
   	  fprintf(stderr, "SQL error: %s\n", zErrMsg);
   	  if(strcmp(zErrMsg," UNIQUE constraint failed: USERS.USERNAME"))
   	  	return -1;
      sqlite3_free(zErrMsg);
   }
   return 0; 
}
void createTable()
{
	int rc;
	rc = sqlite3_open("users.db", &db);
   if( rc )
   {
      fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
      sqlite3_free(zErrMsg);
   }
	char sql[256];
	 strcpy(sql,"CREATE TABLE IF NOT EXISTS USERS("  \
           "USERNAME TEXT PRIMARY KEY NOT NULL," \
           "PASSWORD TEXT NOT NULL,"\
           "SOCKET INT NOT NULL,"\
           "ACTIVE INT NOT NULL);"); 
	 int res = 0;
	 executeSql(sql,&res);
}
int insertNewUser(char *uname, char *passwd,int sockfd)
{
	int active = 1;
	printf("In insertNewUser\n");
	char sql[256];
	snprintf(sql,256,"INSERT INTO USERS VALUES ('%s','%s',%d,%d);",uname,passwd,sockfd,active);
	int res = 0;
	int ret = executeSql(sql,&res);
	if(ret==-1)
		send(sockfd,"Username not available\n",strlen("Username not available\n"),0);
	else
		send(sockfd,"Successfully Registered.\n",strlen("Successfully Registered.\n"),0);
	return ret;
}
void checkLogin(char *uname, char *passwd,int sockfd,char *result) /*checks if the user is valid and if so sets his 
status to active in the database. Also returns an apt message*/
{
	printf("In checkLogin\n");
	bzero(result,100);
	char sql[256];
	int res = -1;
	snprintf(sql,256,"SELECT COUNT(*) FROM USERS WHERE USERNAME='%s' AND PASSWORD='%s' AND ACTIVE=1;",uname,passwd);
	executeSql(sql,&res);
	if(res==1)
	{
		int same_sock = getSock(uname);
		char *msgg = "You have been logged out because you logged in from another location.\n";
		send(same_sock,msgg,strlen(msgg),0);
		close(same_sock);
		pthread_cancel(tid[same_sock-5]);
	}
	
	snprintf(sql,256,"SELECT COUNT(*) FROM USERS WHERE USERNAME='%s' AND PASSWORD='%s';",uname,passwd);
	res=-1;
	executeSql(sql,&res);
	if(res==0)
		snprintf(result,100,"Login Not Successful\n");
	else
	{
		snprintf(result,100,"Successfully Logged in\n");
		snprintf(sql,256,"UPDATE USERS SET SOCKET=%d,ACTIVE=1 WHERE USERNAME='%s';",sockfd,uname);
		res=0;
		executeSql(sql,&res);
	}
	printf("msg = %s\n",result);
}
void dispOnlineUsers(int sockfd)
{
	printf("In dispOnlineUsers\n");
	char sql[256];
	snprintf(sql,256,"SELECT USERNAME FROM USERS WHERE ACTIVE=1;");
	int rc;
	rc = sqlite3_exec(db,sql, callback2, &sockfd, &zErrMsg);
   	if( rc != SQLITE_OK )
   {
   	  fprintf(stderr, "SQL error: %s\n", zErrMsg);
      sqlite3_free(zErrMsg);
   }

}
void clearData()
{
	printf("In clearData\n");
	char sql[256];
	snprintf(sql,256,"UPDATE USERS SET SOCKET=-1,ACTIVE=0;");
	int res = 0;
	executeSql(sql,&res);
	sqlite3_close(db);
}
void clearUserData(char *uname)
{
	printf("In clearUserData\n");
	char sql[256];
	snprintf(sql,256,"UPDATE USERS SET SOCKET=-1,ACTIVE=0 WHERE USERNAME='%s';",uname);
	int res = 0;
	executeSql(sql,&res);
	//sqlite3_close(db);
}
int checkOnline(char *uname)
{
	printf("In checkOnline\n");
	char sql[256];
	int res = -1;
	snprintf(sql,256,"SELECT COUNT(*) FROM USERS WHERE USERNAME='%s';",uname);
	executeSql(sql,&res);
	if(res==1)
	{
		res = -1;
		snprintf(sql,256,"SELECT COUNT(*) FROM USERS WHERE USERNAME='%s' and ACTIVE=1;",uname);	
		executeSql(sql,&res);
		return res;
	}
	else
		return 2;
}
int getSock(char *uname)
{
	printf("In getSock\n");
	char sql[256];
	snprintf(sql,256,"SELECT SOCKET FROM USERS WHERE USERNAME='%s';",uname);	
	int res = -1;
	executeSql(sql,&res);
	return res;
}