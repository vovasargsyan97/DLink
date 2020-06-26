#include <sys/types.h>
#include <sys/socket.h>
#include <json-c/json.h>
#include <pthread.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <signal.h>
#include <time.h>
#include <stdarg.h>

#define PORT 1155
#define IP "127.5.8.49"
#define true 1

#define BUFFER_SZ 2048
#define NAME_LEN_1 32
#define TASK_LEN 2048

volatile sig_atomic_t flag=0;
volatile sig_atomic_t last_message = 1;
volatile sig_atomic_t  tt = 0;
int serv_time=-1;
int sockfd=0;
int socketfd=0;
char name[NAME_LEN_1];
char login[NAME_LEN_1];
char password[NAME_LEN_1];
char recipientname[NAME_LEN_1]="-1";
char last_task[TASK_LEN];
char * names[100];
int in=0;

void setSockopts(int *sock,int args_count,...  ){
    //struct timeval timeout;
    struct timeval timeout;
    //timeout parameters
    timeout.tv_sec = 5;
    timeout.tv_usec = 0;
    int optval = 1;
    va_list ap;
    va_start(ap, args_count);
    int i;
    for(i=0;i<args_count;++i){
         int arg = va_arg (ap, int);
         if(arg==SO_RCVTIMEO || arg==SO_SNDTIMEO){
             setsockopt (*sock, SOL_SOCKET, arg, (char *)&timeout,sizeof(timeout));
         }
         else if(arg==SO_REUSEADDR || arg==SO_REUSEPORT || arg==SO_KEEPALIVE){
             setsockopt (*sock, SOL_SOCKET, arg, &optval, sizeof(optval));
         }
    }
    va_end (ap);
}

void str_overwrite_stdout(){
	if(last_message==1){
		printf("\r%s","Enter the expression: ");
	}else if(last_message==0){
		printf("\r%s","Enter the expression: ");
		last_message = -1;
		goto p;
	}else{
		if(tt!=1){
	   	printf("\r%s",">" );
		}
	}
	p:
	fflush(stdout);
}
void str_trim_lf(char * arr,int len){
	for(int i=0;i<len;i++){
		if(arr[i]=='\n'){
			arr[i]='\0';
			break;
		}
	}
}
int is_task(const char * task){
		if(strstr(task,"+")!= NULL || strstr(task,"-")!=NULL || strstr(task,"*")!=NULL || strstr(task,"/")!=NULL)
		return 1;

		return 0;
}
void in_err(int n,char * message){
	if(n<0){
		perror(message);
		exit(1);
	}
}
void catch_ctrl_c_and_exit(){

	flag=1;
}
void  send_msg_handler(){
	char buffer[BUFFER_SZ]={};
	while(true){
		str_overwrite_stdout();

		//input buffer
		fgets(buffer,BUFFER_SZ,stdin);
		str_trim_lf(buffer,BUFFER_SZ);
		time_t currenttime;
		time(&currenttime);

		struct tm * mytime=localtime(&currenttime);

		struct json_object * jobj=json_object_new_object();
		struct json_object *jmessage=json_object_new_string(buffer);
		struct json_object *jname=json_object_new_string(name);

		struct json_object *jtask=json_object_new_string(last_task);
		struct json_object *jserv_sec=json_object_new_int(serv_time);
		struct json_object *jclient_sec=json_object_new_int(mytime->tm_hour*3600+mytime->tm_min*60+mytime->tm_sec);

		json_object_object_add(jobj,"Name",jname);
		json_object_object_add(jobj,"Message",jmessage);


		int falg_1=0;
		if(strstr(json_object_get_string(jmessage),"Wrong result!")!=NULL){
			falg_1=1;
		}
		if(is_task(buffer)==0){
			json_object_object_add(jobj,"Task",jtask);
			json_object_object_add(jobj,"ServerSec",jserv_sec);
			json_object_object_add(jobj,"ClientSec",jclient_sec);
			if(falg_1==0){
				last_message=1;
			}
		}
		const char * message=json_object_to_json_string(jobj);
		send(sockfd,message,strlen(message),0);
		if(strstr(buffer,"menu")!=NULL){
			for(int i=0;i<in;i++){
				printf("%s-->%d\n",names[i],i+1);
			}
			char p33[20];
			bzero(p33,20);
			printf(">");
			fgets(p33,20,stdin);
			str_trim_lf(p33,strlen(p33));
			send(sockfd,p33,strlen(p33),0);
		}
		if(strcmp(buffer,"exit")==0){
			bzero(buffer,BUFFER_SZ);
			break;
		}
		bzero(buffer,BUFFER_SZ);
	}
	catch_ctrl_c_and_exit();
}

void recv_msg_handler(){
	char message[BUFFER_SZ];
	char buffer[BUFFER_SZ];

	while(true){

		bzero(message,BUFFER_SZ);
		tt=0;
		int receive=recv(sockfd,message,BUFFER_SZ,0);

		struct json_object * json_parser;
		struct json_object * jname;
		struct json_object * jmessage;
		struct json_object * jserv_time;

		if(receive>0){
			last_message =-1;
			json_parser=json_tokener_parse(message);
			json_object_object_get_ex(json_parser,"Name",&jname);
			const char *temp22=json_object_get_string(jname);
			strcpy(recipientname,temp22);
			json_object_object_get_ex(json_parser,"Message",&jmessage);

			int flag_2=0;
			int flag_3=0;
			if(is_task(json_object_get_string(jmessage))==1){
				flag_2=1;
				printf("\n\nFor this task given 20 seconds!");
				strcpy(last_task,json_object_get_string(jmessage));
				json_object_object_get_ex(json_parser,"ServerSec",&jserv_time);
				serv_time=json_object_get_int(jserv_time);
			}else{
				flag_3=1;
			}
			int flag_4=0;
			int t_h;
			if(strstr(json_object_get_string(jmessage),"Wrong result!")!=NULL){
				last_message=0;
				flag_4=1;

				time_t currenttime2;
				time(&currenttime2);

				struct tm * mytime_2=localtime(&currenttime2);
			 	t_h=mytime_2->tm_hour*3600+mytime_2->tm_min*60+mytime_2->tm_sec;

			}
			printf("\n");
			bzero(buffer,BUFFER_SZ);
			sprintf(buffer,"%s: %s ",json_object_get_string(jname),json_object_get_string(jmessage));

			printf("%s\n",buffer);

			if(flag_4==1){
				if(t_h-serv_time<=20){
					printf("Left: %d:%d:%d\n\n",0,0,20-(t_h-serv_time));
				}
			}

			if(flag_4==0){
				if(flag_2==1){
					tt=1;
					last_message=0;
				}
				if(flag_3==1){
					last_message=1;
				}
			}
			str_overwrite_stdout();
		}else if(receive==0){
			break;
		}
		bzero(message,BUFFER_SZ);
	}
}
int main(int argc,char ** argv){

	struct sockaddr_in server_addr;

	//Socket Settings
	sockfd=socket(AF_INET,SOCK_STREAM,0);
	in_err(sockfd,"Error:socket!");

	setSockopts(&sockfd,2,SO_RCVTIMEO,SO_SNDTIMEO);

	server_addr.sin_family=AF_INET;
	server_addr.sin_port=htons(PORT);
	server_addr.sin_addr.s_addr=inet_addr(IP);

	//connect to server
    int err=connect(sockfd,(struct sockaddr *)&server_addr,sizeof(server_addr));
	in_err(err,"Error : connect!");

	char b[10];
	printf("%s\n","Login--1\nRegistration--2\n" );
	printf("%s",">" );
	fgets(b,10,stdin);
	str_trim_lf(b,strlen(b));

	printf("Enter  login: ");
	fgets(login,NAME_LEN_1,stdin);
	str_trim_lf(login,strlen(login));

	if(strcmp(b,"1")==0){
		struct json_object * job=json_object_new_object();
		struct json_object *jlog=json_object_new_string(login);
		struct json_object *jIn_Up=json_object_new_string("Sign In");

		json_object_object_add(job,"Login",jlog);
		json_object_object_add(job,"In_Up",jIn_Up);

		const char * ptr_1=json_object_to_json_string(job);
		send(sockfd,ptr_1,strlen(ptr_1),0);

		char b1[1024];
		recv(sockfd,b1,1024,0);
		if(strstr(b1,"Invalid login")!=NULL){
				return EXIT_FAILURE;
			}
		printf("%s","Enter  password:" );
		fgets(password,NAME_LEN_1,stdin);
		str_trim_lf(password,strlen(password));
		send(sockfd,password,strlen(password),0);

		bzero(b1,1024);
		recv(sockfd,b1,1024,0);
		if(strstr(b1,"Invalid password")!=NULL){
				return EXIT_FAILURE;
			}else{
				strcpy(name,b1);
			}

	}else{

		struct json_object * job=json_object_new_object();
		struct json_object *jlog=json_object_new_string(login);
		struct json_object *jIn_Up=json_object_new_string("Sign Up");

		json_object_object_add(job,"Login",jlog);
		json_object_object_add(job,"In_Up",jIn_Up);

		const char * ptr_2=json_object_to_json_string(job);
		send(sockfd,ptr_2,strlen(ptr_2),0);

		char b2[1024];
		recv(sockfd,b2,1024,0);
		if(strstr(b2,"Matching login!")!=NULL){
			printf("%s\n",b2 );
			return EXIT_SUCCESS;
		}else{
			printf("Enter your name: ");
			fgets(name,NAME_LEN_1,stdin);
			str_trim_lf(name,strlen(name));

			if(strlen(name)>NAME_LEN_1-1 || strlen(name)<2){
				printf("Enter name correctly\n");
				return EXIT_FAILURE;
			}
		  send(sockfd,name,NAME_LEN_1,0);


			printf("%s","Enter your password:" );
			fgets(password,NAME_LEN_1,stdin);
			str_trim_lf(password,strlen(password));
			send(sockfd,password,strlen(password),0);
		}
	}
	printf("============ WELCOME =============\n");


	char ttr[1000];
	char p1[20];
	recv(sockfd,p1,strlen(p1),0);
	bzero(ttr,1000);
	in=atoi(p1);
	for(int i=0;i<atoi(p1);i++){
		recv(sockfd,ttr,1000,0);
	  for(int j=0;j<strlen(ttr);j++)
			names[i]=malloc(strlen(ttr));
			strcpy(names[i],ttr);

		bzero(ttr,1000);
	}


	char p3[20];
	bzero(p3,20);
	recv(sockfd,p3,20,0);

	if(atoi(p3)>0){
		printf("You have %s unread Result!\n",p3 );
		printf("Read?\n");
		printf("Yes-->1\nNo-->2\n" );
		printf(">");
		int len=atoi(p3);
		bzero(p3,20);
		fgets(p3,20,stdin);
		str_trim_lf(p1,strlen(p3));
		send(sockfd,p3,strlen(p3),0);
		if(atoi(p3)==1){
			for(int i=0;i<len;i++){
				bzero(ttr,1000);
				sleep(1);
				recv(sockfd,ttr,1000,0);
				printf("%s\n",ttr );
			}
  	}
	}


	bzero(p1,20);
	recv(sockfd,p1,20,0);
	if(atoi(p1)>0){
		printf("You have %s unread Message!\n",p1 );
		printf("Read?\n");
		printf("Yes-->1\nNo-->2\n" );
		printf(">");
		bzero(p1,10);
		fgets(p1,10,stdin);
		str_trim_lf(p1,strlen(p1));
		send(sockfd,p1,strlen(p1),0);
	}
	pthread_t send_msg_thread;
	pthread_t recv_msg_thread;

        if(pthread_create(&send_msg_thread,NULL,(void*)send_msg_handler,NULL) != 0)
	{
		printf("Error:thread craete!");
		return EXIT_FAILURE;

	}

        if(pthread_create(&recv_msg_thread,NULL,(void*)recv_msg_handler,NULL) != 0)
	 {
		printf("Error:thread craete!");
		return EXIT_FAILURE;

	 }
	while(1){
		if(flag){
			printf("\nbye\n");
			break;
		}
	}
	close(sockfd);
	return EXIT_FAILURE;


}
