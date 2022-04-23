#include<stdio.h>
#include<stdlib.h>
#include<sys/types.h>
#include<sys/socket.h>
#include<string.h>
#include<unistd.h>
#include<arpa/inet.h>
#include<netinet/in.h>
#include<sys/wait.h>
#include<signal.h>
#include<sqlite3.h>
#define ERR_MSG(msg)do{\
	fprintf(stderr,"__%d__",__LINE__);\
	perror(msg);\
}while(0)
#define PORT 6666
#define A 1
#define B 2
#define C 3
#define D 4
#define E 5
#define F 6
//双方通信
typedef struct{
	int type;//操作码
	char id[128];//账号
	char password[128];//密码
	char chmod[10];//权限
}MSG;
//员工信息
typedef struct{
	int order; //操作码
	char name[128];
	char age[10];
	char sex[10];
	char id[128];
	char phone[128];
	char branch[128];
	char wage[10];
	char ins[128];//存查询到的结果用
	int com;//选择修改的字段
}INFO;
int do_sign(int newfd,MSG *msg,sqlite3 *db);//注册
void do_client(int newfd,sqlite3 *db);//子进程处理函数
int do_login(int newfd,MSG *msg,sqlite3 *db);//登录
int do_add(int newfd,INFO *info,sqlite3 *db);//添加
int do_user(int newfd,sqlite3 *db);//普通用户登录函数
int do_del(int newfd,INFO *info,sqlite3 *db);//按ID查找删除
int do_admin(int newfd,sqlite3 *db);//管理员登录函数
int do_search(int newfd,INFO *info,sqlite3 *db);//查询
int do_callback(void *arg,int column,char **column_text,char **column_name);//回调函数
int do_amend(int newfd,INFO *info,sqlite3 *db);//修改

void handler(int sig)//僵尸进程回收
{
	while(waitpid(-1,NULL,WNOHANG)>0);
	return ;
}
int main(int argc, const char *argv[])
{
	if(argc < 2)
	{
		fprintf(stderr,"请输入 IP \n");
		return -1;
	}
	//创建流式套接字
	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	
	if(sfd < 0)
	{
		ERR_MSG("socket");
		return -1;
	}

	if(sfd < 0)
	{
		ERR_MSG("socket");
		return -1;
	}

	struct sockaddr_in cin;
	socklen_t addrlen = sizeof(cin);
	pid_t pid;
	char* errmsg=NULL;
	//创建数据库 增加两张表 
	sqlite3* db=NULL;
	if(sqlite3_open("./my.db",&db)!=SQLITE_OK)
	{
		ERR_MSG("open failed\n");
		return -1;
	}

	char sql1[128] = "create table if not exists info(name char,age int,sex char,id char primary key,phone char,branch char,wage char);";
		
	if((sqlite3_exec(db,sql1,NULL,NULL,&errmsg))!=SQLITE_OK)
	{
		ERR_MSG("sqlite3_exec");
	}

	char sql[128] = "create table if not exists user(id char primary key,password char,chmod char);";
	
	if((sqlite3_exec(db,sql,NULL,NULL,&errmsg))!=SQLITE_OK)
	{
		ERR_MSG("sqlite3_exec");
	}

		//允许端口快速重用
	int reuse = 1;
	if(setsockopt(sfd, SOL_SOCKET, SO_REUSEADDR, &reuse, sizeof(reuse)) < 0)
	{
		ERR_MSG("setsockopt");
		return -1;
	}
	//填充地址信息结构体
	struct sockaddr_in sin; 			//man 7 ip
	sin.sin_family 		= AF_INET;
	sin.sin_port 		= htons(PORT); 	//端口号的网络字节序，1024~49151
	sin.sin_addr.s_addr = inet_addr(argv[1]); 	//本机IP地址 终端输入ifconfig查找

	//绑定服务器的IP地址和端口号 bind
	if(bind(sfd, (struct sockaddr*)&sin, sizeof(sin)) < 0)
	{
		ERR_MSG("bind");
		return -1;
	}
	printf("绑定成功\n");

	//将套接字设置为被动监听状态
	if(listen(sfd, 10) < 0)
	{
		ERR_MSG("listen");
		return -1;
	}
	printf("监听成功\n");

	signal(17,handler);//处理僵尸进程
	while(1)
	{

		//循环获取新的文件描述符
		int newfd = accept(sfd, (struct sockaddr*)&cin, &addrlen);
		if(newfd < 0)
		{
			ERR_MSG("accept");
			return -1;
		}
		printf("[%s | %d]newfd = %d 连接成功\n", \
				inet_ntoa(cin.sin_addr), ntohs(cin.sin_port), newfd);

		
		if((pid = fork()) < 0)
		{
			ERR_MSG("创建进程失败\n");
			return -1;
		}
		else if(pid==0)//子进程处理代码
		{
		
			do_client(newfd,db); 
			return 0;
		}
		else//父进程负责链接
		{
			close(newfd);
			
		}
	}


	close(sfd);

	return 0;
}

void do_client(int newfd,sqlite3 *db)//子进程处理函数
{
	MSG msg;
	while(1)
	{
		if(recv(newfd,&msg,sizeof(MSG),0)>0)
		{
			printf("type:%d\n",msg.type);
			switch(msg.type)
			{
			case A:
				do_sign(newfd,&msg,db);//注册
				break;
			case B:
				do_login(newfd,&msg,db);//登录
				break;
			case C:
				sqlite3_close(db);  //这里执行不到
				close(newfd);
				printf("客户端退出\n");
				break;
			default:
				printf("操作错误\n");
				break;
			}
		}

	}
}
int do_sign(int newfd,MSG *msg,sqlite3 *db)//注册
{
	//执行sql语句添加 注册成功或失败 send客户端
	char sql[512]={0};

	sprintf(sql,"insert into user values('%s','%s','%s');",msg->id,msg->password,msg->chmod);
	char *errmsg;
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=SQLITE_OK)
	{
		ERR_MSG("注册失败\n");	
		strcpy(msg->password,"exit");

	}
	else
	{
		printf("注册成功\n");
		strcpy(msg->password,"ok");

	}
	if(send(newfd,msg,sizeof(MSG),0)<0)
	{
		ERR_MSG("发送失败\n");
		return -1;
	}
	return 1;
}	

int do_login(int newfd,MSG *msg,sqlite3 *db)//登录
{
	//get_table获取表格执行查找sql语句
	char sql[512]={0};
	char *errmsg=NULL;
	char **pres=NULL;
	int row,column;
	//sql 按条件查找
	sprintf(sql,"select * from user where id = '%s' and password = '%s';",\
			msg->id,msg->password);
	if(sqlite3_get_table(db,sql,&pres,&row,&column,&errmsg)!=SQLITE_OK)
	{
		ERR_MSG("登录查询失败\n");
		return -1;
	}
	
	if(row!=0)
	{
		printf("%s用户登陆成功\n",msg->id);
		//根据查询结果 判断表内权限 调用用户、管理员函数
		//printf("%s\t",pres[5]);
			
		if(strcmp(pres[5],"admin")==0)
		{
	
			printf("11111111111\n");
			strcpy(msg->chmod,"admin");
			send(newfd,msg,sizeof(MSG),0);
			do_admin(newfd,db);
		}
		else
		{
			printf("22222222222\n");
			strcpy(msg->chmod,"user");
			send(newfd,msg,sizeof(MSG),0);
			do_user(newfd,db);
		}
	}
}
int do_admin(int newfd,sqlite3 *db)//管理员登录函数
{
	INFO info;
	while(1)
	{
		if(recv(newfd,&info,sizeof(INFO),0)<0)
		{
			ERR_MSG("管理员操作接收失败\n");
			return -1;
		}
		switch(info.order)
		{
		case A:
			do_add(newfd,&info,db);//增加
			break;
		case B:
			do_del(newfd,&info,db);//删除
			break;
		case C:
			do_amend(newfd,&info,db);//修改
			break;
		case D:
			do_search(newfd,&info,db);//查询
			break;
		case E:
			return 1;    //返回
			break;
		case F:
			close(newfd);   //退出
			return 0;
			break;
		default:
			printf("管理员输入错误\n");
		}
	}
	return 0;
   
}
int do_user(int newfd,sqlite3 *db)//普通用户登录函数
{
	INFO info;
	while(1)
	{
		if(recv(newfd,&info,sizeof(INFO),0)<0)
		{
			ERR_MSG("普通用户操作接收失败\n");
			return -1;
		}
		switch(info.order)
		{
			case 1:
				do_search(newfd,&info,db);
				break;
		}
	}
	return 0;
}
int do_add(int newfd,INFO *info,sqlite3 *db)//添加
{
	char sql[1024]={0};
	char *errmsg;
//	printf("12312312312313\n");
	sprintf(sql,"insert into info values('%s','%s','%s','%s','%s','%s','%s');",info->name,info->age,info->sex,info->id,info->phone,info->branch,info->wage);
	if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=SQLITE_OK)
	{
		ERR_MSG("添加到数据库失败\n");
	}

	printf("%s添加到数据库成功\n",info->name);

	return 0;
}

int do_del(int newfd,INFO *info,sqlite3 *db)//按ID查找删除
{
	char sql[512]={0};
	char *errmsg=NULL;
	char **pres=NULL;
	int row,cloumn;
	
	sprintf(sql,"select * from info where id = '%s';",info->id);
	if(sqlite3_get_table(db,sql,&pres,&row,&cloumn,&errmsg)!=SQLITE_OK)
	{
		ERR_MSG("查找错误\n");
		return -1;
	}
	//sql按字段名 删除行
	if(row==1)
	{
		sprintf(sql,"delete from info where id = '%s';",info->id);	
		if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=SQLITE_OK)
		{
			ERR_MSG("删除失败\n");
		}
		printf("%s删除成功\n",info->id);
	}

	if(row!=1)
	{
		printf("%s不存在\n",info->id);
		strcpy(info->id,"no");
		if(send(newfd,info,sizeof(INFO),0)<0)
		{
			ERR_MSG("删除失败\n");
		}
	}
	return 0;
}

int do_search(int newfd,INFO *info,sqlite3 *db)//查询
{
	char sql[256]={0};
	char *errmsg=NULL;
	//查找到后 用回调函数返回打印 
	sprintf(sql,"select * from info where id = '%s';",info->id);
	if(sqlite3_exec(db,sql,do_callback,(void*)&newfd,&errmsg)!=SQLITE_OK)
	{
		ERR_MSG("查询失败\n");
	}
	printf("查询成功\n");

	return 0;
}

int do_callback(void *arg,int column,char **column_text,char **column_name)//回调函数
{
	INFO info;
	int newfd=*((int*)arg);
	//将结果打印到结构体中返回
	sprintf(info.ins,"%s,%s,%s,%s,%s,%s,%s",\
			column_text[0],column_text[1],column_text[2],column_text[3],column_text[4],column_text[5],column_text[6]);
	if(send(newfd,&info,sizeof(INFO),0)<0)
	{
		ERR_MSG("回调失败\n");
	}
	return 0;
}

int do_amend(int newfd,INFO *info,sqlite3 *db)//修改
{
	char sql[512]={0};
	char *errmsg=NULL;
	char **pres=NULL;
	int row,cloumn;
	//查询要修改的那一行
	sprintf(sql,"select * from info where id = '%s';",info->id);
	if(sqlite3_get_table(db,sql,&pres,&row,&cloumn,&errmsg)!=SQLITE_OK)
	{
		ERR_MSG("查询无此用户\n");
		return 0;
	}
	if(row!=0)
	{
		printf("查询成功\n");
		//按条件查找到行 更新行的指定字段
		switch(info->com)
		{
		case 1:
			sprintf(sql,"update info set name = '%s' where id = '%s';",info->name,info->id);
			break;
		case 2:
			sprintf(sql,"update info set age = '%s' where id = '%s';",info->age,info->id);
			break;
		case 3:
			sprintf(sql,"update info set sex = '%s' where id = '%s';",info->sex,info->id);
			break;
		case 4:
			sprintf(sql,"update info set id = '%s' where id = '%s';",info->id,info->id);
			break;
		case 5:
			sprintf(sql,"update info set phone = '%s' where id = '%s';",info->phone,info->id);
			break;
		case 6:
			sprintf(sql,"update info set branch = '%s' where id = '%s';",info->branch,info->id);
			break;
		case 7:
			sprintf(sql,"update info set wage = '%s' where id = '%s';",info->wage,info->id);
			break;
		default:
			printf("客户端输入错误\n");
			return 0;
		}
		if(sqlite3_exec(db,sql,NULL,NULL,&errmsg)!=SQLITE_OK)
		{
			ERR_MSG("更新失败\n");
			return 0;
		}
		if(send(newfd,info,sizeof(INFO),0)<0)
		{
			ERR_MSG("更新失败");
		}
	}

	printf("%s用户更新成功\n",info->id);
	return 0;
}
