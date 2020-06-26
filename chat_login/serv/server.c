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
#include <time.h>
#include <mysql/mysql.h>
#include "task.c"
#include <stdarg.h>

#define PORT 1155
#define IP "127.5.8.49"
#define true 1

#define BUFFER_SZ 2048
#define NAME_LEN_1 32

#define MAX_CLIENTS 20
static _Atomic unsigned int cli_count=0;
static int uid=100;

// MYSQL Settings
static char * host="localhost";
static char * user="root";
static char * password="Vladimir @ 1997";
static char * dbname="chatroom_db";


unsigned int port=3306;
static char * unix_socket = NULL;
unsigned int flag = 0;

MYSQL * conn;
MYSQL_RES * res;
MYSQL_ROW row;

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

void mysql_connect(){
	conn = mysql_init(NULL);

  if(!mysql_real_connect(conn,host,user,password,dbname,port,unix_socket,flag)){
    fprintf(stderr,"Error: %s [%d]\n",mysql_error(conn),mysql_errno(conn) );
    exit(1);
  }
}

//client struct
typedef struct{
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char log[NAME_LEN_1];
	char name[NAME_LEN_1];
	char recipientname[NAME_LEN_1];
}client_t;

client_t *clients[MAX_CLIENTS];

pthread_mutex_t client_mutex = PTHREAD_MUTEX_INITIALIZER;

void finish_with_error(MYSQL *con){
  fprintf(stderr, "%s\n", mysql_error(con));
  mysql_close(con);
  exit(1);
}
void str_overwrite_stdout(){
	printf("\r%s",">");
	fflush(stdout);
}
int is_task(const char * task){


	if(strstr(task,"+")!= NULL || strstr(task,"-")!=NULL || strstr(task,"*")!=NULL || strstr(task,"/")!=NULL)
		return 1;

	return 0;
}
void str_trim_lf(char * arr,int len){
	for(int i=0;i<len;i++){
		if(arr[i]=='\n'){
			arr[i]='\0';
			break;
		}
	}
}

void in_err(int n,char * message){
	if(n<0){
		perror(message);
		exit(1);
	}
}

void queue_remove(int uid){

	pthread_mutex_lock(&client_mutex);

  char buff[2048];
	sprintf(buff,"Update Users set Online='%s' where Id=%d ","No",uid);
	if( mysql_query(conn,buff)){
		finish_with_error(conn);
	}

	pthread_mutex_unlock(&client_mutex);
}

void ptint_ip_addr(struct sockaddr_in addr){

	printf("%s",inet_ntoa(addr.sin_addr));

}

void send_message(char *s,client_t *cli){

	pthread_mutex_lock(&client_mutex);

  char buff[2048];

	sprintf(buff,"Select Sockfd from Users Where  Name='%s'",cli->recipientname);;
  if(mysql_query(conn,buff)){
		finish_with_error(conn);
	}
  res=mysql_store_result(conn);

	int d;
	while (row=mysql_fetch_row(res)) {
		 d=atoi(row[0]);
		 int send_res=send(d,s,strlen(s),0);
		 if(send_res<0){
			 printf("Error :send!");
			 break;
   	}
  }

	pthread_mutex_unlock(&client_mutex);

}

void * handle_client(void * arg){
	char  *names[32];
	int in=0;
    char buffer[BUFFER_SZ];
    char name[NAME_LEN_1];
	char login[NAME_LEN_1];
	char pass[NAME_LEN_1];
    char recipientname[NAME_LEN_1];
    char buff[1024];
    int leave_flag=0;
    cli_count++;
    client_t *cli = (client_t *)arg;
		char temp_1[2048];

		recv(cli->sockfd,temp_1,2048,0);

		struct json_object * json_parser_1;
		struct json_object * jlog;
		struct json_object * jIn_Up;

		json_parser_1=json_tokener_parse(temp_1);
		json_object_object_get_ex(json_parser_1,"Login",&jlog);
		json_object_object_get_ex(json_parser_1,"In_Up",&jIn_Up);

		strcpy(login,json_object_get_string(jlog));
		strcpy(cli->log,login);

    const char *InUp=json_object_get_string(jIn_Up);
		if(strcmp(InUp,"Sign In")==0){
			sprintf(buff,"Select count(*) from Users  where Login='%s'",login);
			if(mysql_query(conn,buff)){
					finish_with_error(conn);
			}
			res=mysql_store_result(conn);

			int con1;
			while(row=mysql_fetch_row(res)){
				con1=atoi(row[0]);
			}
			if(con1==1){
				sprintf(buff,"Enter your password:");
				send(cli->sockfd,buff,strlen(buff),0);
			}else{
				sprintf(buff,"Invalid login");
				send(cli->sockfd,buff,strlen(buff),0);
			}

			recv(cli->sockfd,pass,NAME_LEN_1,0);

			sprintf(buff,"Select MD5('%s')",pass);
			if(mysql_query(conn,buff)){
					finish_with_error(conn);
			}
			res=mysql_store_result(conn);

			char  hashpass[256];
			while(row=mysql_fetch_row(res)){
			strcpy(hashpass, row[0]);
			}

			sprintf(buff,"Select Password from Users where Login='%s'",login);
			if(mysql_query(conn,buff)){
					finish_with_error(conn);
			}
			res=mysql_store_result(conn);

			char  basepass[256];
			while(row=mysql_fetch_row(res)){
			strcpy(basepass, row[0]);
			}
			if(strcmp(basepass,hashpass)==0){
				sprintf(buff,"Select Name,Id from Users where Login='%s'",login);
				if(mysql_query(conn,buff)){
						finish_with_error(conn);
				}
				res=mysql_store_result(conn);

				char  baseName[32];
				while(row=mysql_fetch_row(res)){
				strcpy(baseName, row[0]);
				strcpy(cli->name,row[0]);
				cli->uid=atoi(row[1]);
				}
				printf("\n" );
				sprintf(buff,"%s has joined\n",baseName);
				send(cli->sockfd,baseName,strlen(baseName),0);
				printf("%s",buff);

					sprintf(buff,"Update Users set Online='%s',Sockfd=%d where Login='%s'","Yes",cli->sockfd,login);
					if(mysql_query(conn,buff)){
							 finish_with_error(conn);
					 }

			}else{
				sprintf(buff,"Invalid password");
				send(cli->sockfd,buff,strlen(buff),0);
			}

		}else{

			sprintf(buff,"Select count(*) from Users  where Login='%s'",login);
			if(mysql_query(conn,buff)){
					finish_with_error(conn);
			}
			res=mysql_store_result(conn);

			int con2;
			while(row=mysql_fetch_row(res)){
				con2=atoi(row[0]);
			}
			printf("%d\n",con2 );
			if(con2>0){
				sprintf(buff,"Matching login!");
				send(cli->sockfd,buff,strlen(buff),0);
			}else{
				sprintf(buff,"Insert Into Users(Login) Values('%s')",login);
				if(mysql_query(conn,buff)){
						finish_with_error(conn);
				}
				sprintf(buff,"Enter your name:");
				send(cli->sockfd,buff,strlen(buff),0);
			}

    	if(recv(cli->sockfd,name,NAME_LEN_1,0)<=0 || strlen(name)>32){
        	printf("Enter the name correctly\n");
        	leave_flag=1;
    	}else{
        	strcpy(cli->name,name);
        	sprintf(buff,"Update Users set Name='%s' where Login='%s'",cli->name,login);
        	if(mysql_query(conn,buff)){
            	finish_with_error(conn);
        	}
    	}
			recv(cli->sockfd,pass,NAME_LEN_1,0);

			sprintf(buff,"Select MD5('%s')",pass);
			if(mysql_query(conn,buff)){
					finish_with_error(conn);
			}
			res=mysql_store_result(conn);

			char  hashpass[200];
			while(row=mysql_fetch_row(res)){
			strcpy(hashpass, row[0]);
			}
			strcpy(cli->log,login);
			sprintf(buff,"Update Users set Sockfd=%d,Password='%s',Online='%s' where Login='%s'",cli->sockfd,hashpass,"Yes",login);
			if(mysql_query(conn,buff)){
					finish_with_error(conn);
			}
						sprintf(buff,"Select Name,Id from Users where Login='%s'",login);
						if(mysql_query(conn,buff)){
								finish_with_error(conn);
						}
						res=mysql_store_result(conn);

						char  baseName[32];
						while(row=mysql_fetch_row(res)){
						strcpy(baseName, row[0]);
						cli->uid=atoi(row[1]);
						}
						printf("\n" );
						sprintf(buffer,"%s has joined\n",baseName);
						printf("%s",buffer);
        }
				bzero(recipientname,NAME_LEN_1);
				bzero(buffer,BUFFER_SZ);
		sprintf(buff,"Select Name from Users where Login<>'%s'",login);
		if(mysql_query(conn,buff)){
				finish_with_error(conn);
		}
		res=mysql_store_result(conn);

		while(row=mysql_fetch_row(res)){
		  names[in++]=row[0];
		}
		char p1[10];
		sprintf(p1,"%d",in);
		send(cli->sockfd,p1,strlen(p1),0);
		for(int i=0;i<in;i++){
			sleep(1);
			send(cli->sockfd,names[i],strlen(names[i]),0);
		}
		bzero(p1,10);


			sprintf(buff,"Select count(*) from Expressions where User='%s' and Transmitted ='%s'",cli->name,"No");
			if(mysql_query(conn,buff)){
							finish_with_error(conn);
			}
			res=mysql_store_result(conn);
			int h1;
			while(row=mysql_fetch_row(res)){
							h1=atoi(row[0]);
							printf("h1=%d\n",h1);
							send(cli->sockfd,row[0],strlen(row[0]),0);
			}
			if(h1>0){
				recv(cli->sockfd,p1,10,0);
				if(atoi(p1)==1){
					sprintf(buff,"Select Task,Result,RecipientName from Expressions where  User='%s' and Transmitted ='%s'",cli->name,"No");
					if(mysql_query(conn,buff)){
									finish_with_error(conn);
					}
					res=mysql_store_result(conn);
					char *b1[50];
					int ind=0;
					char  mm2[1024];
					while(row=mysql_fetch_row(res)){
								  bzero(mm2,1024);
									sprintf(mm2,"Task:%s,%s:%s",row[0],row[2],row[1]);
									sprintf(buff,"Update Expressions  set Readed ='%s' where task='%s' and User='%s'","Yes",row[0],cli->name);
									if(mysql_query(conn,buff)){
											finish_with_error(conn);
										}

									sleep(1);
									send(cli->sockfd,mm2,strlen(mm2),0);
					}
				}
			}





			sprintf(buff,"Select count(*) from Expressions where RecipientName='%s' and Transmitted ='%s'",cli->name,"No");
			if(mysql_query(conn,buff)){
							finish_with_error(conn);
			}
			res=mysql_store_result(conn);
			int h2;
			while(row=mysql_fetch_row(res)){
							h2=atoi(row[0]);
							send(cli->sockfd,row[0],strlen(row[0]),0);
			}
			if(h2>0){
				recv(cli->sockfd,p1,10,0);
				if(atoi(p1)==1){
					  bzero(buffer,BUFFER_SZ);
						strcpy(buffer,"Server");
				}
			}

    char tasks[2048];
    while(true){
        if(leave_flag){
            break;
        }

        struct json_object * parser_json;
        struct json_object * jname;
				struct json_object * jrname;
        struct json_object *jmessage;
        struct json_object *jtask;
        struct json_object *jserv_sec;
        struct json_object *jcli_sec;
        struct json_object *jobj;
        printf("buffer:%s\n",buffer);
			if(strstr(buffer,"Server")!=NULL){
													sprintf(buff,"Select Task, RecipientName from Expressions where RecipientName='%s' and Transmitted ='%s'",cli->name,"No");
													if(mysql_query(conn,buff)){
																	finish_with_error(conn);
													}
													res=mysql_store_result(conn);
													 char *rpp[10];
													 int index=0;
													while(row=mysql_fetch_row(res)){

																	rpp[index++]=row[0];
													}
													for(int i=0;i<index;i++){

															time_t currenttime;
															time(&currenttime);
															struct tm * mytime=localtime(&currenttime);
															jserv_sec=json_object_new_int(mytime->tm_hour*3600+mytime->tm_min*60+mytime->tm_sec);
															jobj=json_object_new_object();
															jmessage=json_object_new_string(rpp[i]);
															jname=json_object_new_string(cli->name);
															json_object_object_add(jobj,"Name",jname);
															json_object_object_add(jobj,"Message",jmessage);
															json_object_object_add(jobj,"ServerSec",jserv_sec);
															const char * t_1=json_object_get_string(jobj);
															char  s_buffer[strlen(t_1)];
															for(int i=0;i<strlen(t_1);i++){
																			s_buffer[i]=t_1[i];
															}
															printf("%s",s_buffer);
															send(cli->sockfd,s_buffer,strlen(s_buffer),0);
															sprintf(buff,"Update Expressions Set Transmitted  ='%s' Where RecipientName ='%s' and Task='%s'","No",cli->recipientname,rpp[i]);
															if(mysql_query(conn,buff)){
																			finish_with_error(conn);
															}
															sleep(5);
													}

							bzero(buffer,BUFFER_SZ);
						continue;
					}
					int receive=recv(cli->sockfd,buffer,BUFFER_SZ,0);
	        		parser_json=json_tokener_parse(buffer);

					if(strstr(buffer,"menu") != NULL){
						char p33[20];
						bzero(p33,20);
						recv(cli->sockfd,p33,20,0);
						strcpy(cli->recipientname,names[atoi(p33)-1]);
						sprintf(buff,"Update Users set RecipientName='%s' where Login='%s'",cli->recipientname,login);
						if(mysql_query(conn,buff)){
								finish_with_error(conn);
							}
						printf("%s\n",names[atoi(p33)-1] );
						bzero(buffer,BUFFER_SZ);
						continue;
					}
        if(strstr(buffer,"exit") !=NULL){
            jobj=json_object_new_object();
            jmessage=json_object_new_string("has left");
            json_object_object_get_ex(parser_json,"Name",&jname);
            json_object_object_add(jobj,"Name",jname);
            json_object_object_add(jobj,"Message",jmessage);
            bzero(buffer,BUFFER_SZ);
            const char * ptr=json_object_get_string(jname);
            printf("\n%s has left\n",ptr );
            sprintf(buffer,"%s\n",json_object_get_string(jobj));
            send_message(buffer,cli);
            leave_flag = 1;
        }else if(receive > 0){
            if(strlen(buffer)>0){
                json_object_object_get_ex(parser_json,"Message",&jmessage);
                const char * ptr=json_object_get_string(jmessage);
                if(is_task(ptr)==0){
                    json_object_object_get_ex(parser_json,"ServerSec",&jserv_sec);
                    json_object_object_get_ex(parser_json,"ClientSec",&jcli_sec);
                    json_object_object_get_ex(parser_json,"Task",&jtask);
                    const char * tas=json_object_get_string(jtask);
                    int s_sec=json_object_get_int(jserv_sec);
                    int c_sec=json_object_get_int(jcli_sec);
                    if(c_sec-s_sec>19){
                        char time_mess[1024];
                        sprintf(time_mess,"Time is up! %s did not have time.",cli->name);
                        jmessage=json_object_new_string(time_mess);
                        jname=json_object_new_string("Server");
                        jobj=json_object_new_object();
                        json_object_object_add(jobj,"Name",jname);
                        json_object_object_add(jobj,"Message",jmessage);
                        sprintf(buffer,"%s\n",json_object_get_string(jobj));
                        send_message(buffer,cli);
                        continue;
                    }
                    int cli_res=json_object_get_int(jmessage);
                    sprintf(buff,"Select Result from Expressions Where Task='%s'",tas);
                    if(mysql_query(conn,buff)){
                        finish_with_error(conn);
                    }
                    res=mysql_store_result(conn);
                    int result;
                    while (row=mysql_fetch_row(res)) {
                            result=atoi(row[0]);
                    }
                        if(result==cli_res){
                        send_message(buffer,cli);
												sprintf(buff,"Select Online from Users Where Name='%s'",cli->recipientname);
														if(mysql_query(conn,buff)){
																		finish_with_error(conn);
														}
														res=mysql_store_result(conn);
														 char rp2[32];
														while(row=mysql_fetch_row(res)){
																		strcpy(rp2,row[0]);
														}
														if(strcmp(rp2,"No")==0){
															sprintf(buff,"Update Expressions Set Readed='%s' Where User='%s' and Task='%s'","No",cli->recipientname,tas);
															if(mysql_query(conn,buff)){
																			finish_with_error(conn);
															}
														}else{
															sprintf(buff,"Update Expressions Set Readed='%s' Where User='%s' and Task='%s'","Yes",cli->recipientname,tas);
															if(mysql_query(conn,buff)){
																			finish_with_error(conn);
															}
														}
                    }else{
                        jmessage=json_object_new_string("Wrong result!");
                        jname=json_object_new_string("Server");
                        jobj=json_object_new_object();
                        json_object_object_add(jobj,"Name",jname);
                        json_object_object_add(jobj,"Message",jmessage);
                        json_object_object_add(jobj,"ServerSec",jserv_sec);
                        sprintf(buffer,"%s\n",json_object_get_string(jobj));
                        send(cli->sockfd,buffer,sizeof(buffer),0);
                    	}
                	}else{
                  		char buff[2048];
                    	char res_1[1024];
						int r=stringConvertToArithmeticOperations(ptr);
						sprintf(res_1,"%d",r);

                      	sprintf(buff,"Select Online from Users Where Name='%s'",cli->recipientname);
                        if(mysql_query(conn,buff)){
                            finish_with_error(conn);
                        }
                        res=mysql_store_result(conn);
                         char rp[32];
                        while(row=mysql_fetch_row(res)){
                            strcpy(rp,row[0]);
                        }
                        if(strcmp(rp,"Yes")==0){
														time_t currenttime;
                            time(&currenttime);
                            struct tm * mytime=localtime(&currenttime);
                            jserv_sec=json_object_new_int(mytime->tm_hour*3600+mytime->tm_min*60+mytime->tm_sec);
                            jobj=json_object_new_object();
                            json_object_object_get_ex(parser_json,"Message",&jmessage);
                            json_object_object_get_ex(parser_json,"Task",&jtask);
							jname=json_object_new_string(cli->name);
                            json_object_object_add(jobj,"Name",jname);
                            json_object_object_add(jobj,"Message",jmessage);
                            json_object_object_add(jobj,"ServerSec",jserv_sec);
                            const char * t_1=json_object_get_string(jobj);
                            char  s_buffer[strlen(t_1)];
                            for(int i=0;i<strlen(t_1);i++){
                                s_buffer[i]=t_1[i];
                            }
                            send_message(s_buffer,cli);
				                    sprintf(buff,"Insert Into Expressions(Task,User,UID,Result,Transmitted,RecipientName) Values('%s','%s',%d,'%s','%s','%s')",ptr,cli->name,cli->uid,res_1,"Yes",cli->recipientname);
				                    if(mysql_query(conn,buff)){
				                        finish_with_error(conn);
				                    }
                        }else{
													sprintf(buff,"Insert Into Expressions(Task,User,UID,Result,Transmitted,RecipientName) Values('%s','%s',%d,'%s','%s','%s')",ptr,cli->name,cli->uid,res_1,"No",cli->recipientname);
													if(mysql_query(conn,buff)){
															finish_with_error(conn);
													}
												}

            }
                str_trim_lf(buffer,strlen(buffer));
            }
        }else{
            printf("Error:-1\n");
            leave_flag = 1;
        }
        bzero(buffer,BUFFER_SZ);
    }
    close(cli->sockfd);
    queue_remove(cli->uid);
    free(cli);
    cli_count--;
    pthread_detach(pthread_self());
    return NULL;
}

int main(){

  mysql_connect();

	int listenfd=0,connfd=0;
	struct sockaddr_in serv_addr;
	struct sockaddr_in cli_addr;
	pthread_t tid;

	//Socket Settings
	listenfd=socket(AF_INET,SOCK_STREAM,0);
	in_err(listenfd,"Error:socket!");

	setSockopts(&listenfd,3,SO_REUSEPORT,SO_REUSEADDR,SO_KEEPALIVE);

	serv_addr.sin_family=AF_INET;
	serv_addr.sin_port=htons(PORT);
	serv_addr.sin_addr.s_addr=inet_addr(IP);

	int bind_res;
	bind_res = bind(listenfd,(struct sockaddr *)&serv_addr,sizeof(serv_addr));
	in_err(bind_res,"Error:bind! ");

	in_err(listen(listenfd,10),"Error:listen");

	printf("============ WELCOME =============\n");

	while(true){

	  int cli_size;
	  connfd=accept(listenfd,(struct sockaddr *)&cli_addr,&cli_size);
	  in_err(connfd,"Error:accept!");
	  if(cli_count+1==MAX_CLIENTS){

	      printf("Maximum clients connected.Connection Rejected\n");
	      ptint_ip_addr(cli_addr);
	      close(connfd);
	      continue;
	  }
	  //clients setings
	  client_t *cli=(client_t *)malloc(sizeof(client_t));
	  cli->address=cli_addr;
	  cli->sockfd=connfd;
	  cli->uid=uid++;

	//  queue_add(cli);
	  pthread_create(&tid,NULL,&handle_client,(void *) cli);
	  sleep(1);
	}
	 mysql_close(conn);
	return EXIT_SUCCESS;
}
