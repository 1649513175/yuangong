#include <stdio.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/wait.h>
#define ERR_MSG(msg) do{\
	fprintf(stderr, "__%d__ ", __LINE__);\
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
    int order;//操作码
    char name[128];
    char age[10];
    char sex[10];
    char id[128];
    char phone[128];
    char branch[128];                                   
    char wage[10];
	char ins[128];//存查询到的结果
	int com;//选择修改的字段
}INFO;

int do_login(int sfd,MSG *msg);//登录
int do_sign(int sfd,MSG *msg);//注册
int do_add(int sfd,INFO *info);//添加
int do_admin(int sfd,INFO* info);//管理员登录函数
int do_user(int sfd,INFO *info);//普通用户登录函数
int do_del(int sfd,INFO *info);//按id删除
int do_search(int sfd,INFO *info);//按id查找
int do_amend(int sfd,INFO *info);//修改

int main(int argc, const char *argv[])
{
	if(argc < 2)
	{
		fprintf(stderr, "请输入 IP \n");
		return -1;
	}

	//创建流式套接字
	int sfd = socket(AF_INET, SOCK_STREAM, 0);
	if(sfd < 0)
	{
		ERR_MSG("socket");
		return -1;
	}

	//绑定客户端的IP和端口---->非必须
	
	//填充要连接的服务器的地址信息结构体
	struct sockaddr_in sin;
	sin.sin_family 		= AF_INET;
	sin.sin_port 		= htons(PORT); 	//服务器的端口
	sin.sin_addr.s_addr = inet_addr(argv[1]); 	//服务器绑定的IP

	//连接服务器
	if(connect(sfd, (struct sockaddr*)&sin, sizeof(sin)) < 0)
	{
		ERR_MSG("connect");
		return -1;

	}
	printf("connect success\n");

	MSG msg;
	int i;

	while(1)
	{
		printf("****************************\n");
		printf("****1.注册 2.登录 3.退出****\n");
		printf("****************************\n");
		printf("请选择：\n");
		scanf("%d",&i);
		switch(i)
		{
		case 1:
			do_sign(sfd,&msg);//注册

			break;
		case 2:
			do_login(sfd,&msg);//登录
			break;
		case 3:			
			exit(0);
			//退出
		default:
			printf("输入错误\n");
		}
	}
	//关闭套接字
	close(sfd);
	return 0;
}

int do_sign(int sfd,MSG *msg)//注册
{
	msg->type=A;
	printf("输入账号ID：\n");
	scanf("%s",msg->id);
	printf("输入密码：\n");
	scanf("%s",msg->password);
	printf("输入用户权限-user或admin:\n");
	scanf("%s",msg->chmod);

	if(send(sfd,msg,sizeof(MSG),0)<0)
	{
		ERR_MSG("发送失败\n");
		return -1;
	}

	if(recv(sfd,msg,sizeof(MSG),0)<0)
	{
		ERR_MSG("接收失败\n");
		return -1;
	}
	printf("%s用户注册成功！\n",msg->id);
	return 1;
}
int do_login(int sfd,MSG *msg)//登录
{
	msg->type=B;
	INFO info;
	printf("请输入账号ID：\n");
	scanf("%s",msg->id);
	printf("请输入密码password: \n");
	scanf("%s",msg->password);
	
	if(send(sfd,msg,sizeof(MSG),0)<0)
	{
		ERR_MSG("发送失败\n");
		return -1;
	}

	if(recv(sfd,msg,sizeof(MSG),0)<0)
	{
		ERR_MSG("接收失败\n");
		return -1;
	}
	if(strcmp(msg->chmod,"admin")==0)
	{
		printf("%s管理员登录\n",msg->id);
		do_admin(sfd,&info);//管理员界面；
		return 1;
	}
	else if(strcmp(msg->chmod,"user")==0)
	{
		printf("%s用户登录\n",msg->id);
		do_user(sfd,&info);//普通用户登录界面
		return 1;
	}
	else
	{
		ERR_MSG("登陆错误\n");
		return -1;
	}
	return 0;
}
int do_admin(int sfd,INFO* info)//管理员登录函数
{
	int i;
	while(1)
	{
		printf("*******************************\n");
		printf("**********hello admin!*********\n");
		printf("***1.添加***2.删除***3.修改****\n");
		printf("***4.查找***5.返回***6.退出****\n");
		printf("*******************************\n");
		printf("请选择:");
		scanf("%d",&i);
		switch(i)
		{
		case 1:
			info->order=A;
			do_add(sfd,info);//增加
			break;
		case 2:
			info->order=B;
			do_del(sfd,info);//删除
			break;
		case 3:
			info->order=C;
			do_amend(sfd,info);//修改
			break;
		case 4:
			info->order=D;
			do_search(sfd,info);//查询
			break;
		case 5:
			info->order=E;//返回
			return 1;
			break;
		case 6:
			info->order=F;//退出
		//	close(sfd);
			exit(0);
		default:
			printf("输入错误\n");
		}
	}
}
int do_user(int sfd,INFO *info)//普通用户登录函数
{
	int i;
	while(1)
	{
		printf("*********hello*user*********\n");
		printf("*******1.查询***2.退出******\n");
		printf("****************************\n");
		printf("请选择:");
		scanf("%d",&i);
		switch(i)
		{
		case 1:
			info->order=1;
			do_search(sfd,info);//查询
			break;
		case 2:
			return 0;
		}
	}
	return 0;
}

int do_add(int sfd,INFO *info)//添加
{
	printf("请输入姓名:");
	scanf("%s",info->name);
	printf("请输入年龄:");
	scanf("%s",info->age);
	printf("请输入性别:");
	scanf("%s",info->sex);
	printf("请输入个人ID:");
	scanf("%s",info->id);
	printf("请输入手机号:");
	scanf("%s",info->phone);
	printf("请输入部门:");
	scanf("%s",info->branch);
	printf("请输入工资:");
	scanf("%s",info->wage);
	
	if(send(sfd,info,sizeof(INFO),0)<0)
	{
		ERR_MSG("员工信息添加失败\n");
	}
	
	printf("%s添加成功\n",info->name);
	return 0;
}

int do_del(int sfd,INFO *info)//按id删除
{
	printf("请输入要删除对象的ID：");
	scanf("%s",info->id);
	
	if(send(sfd,info,sizeof(INFO),0)<0)
	{
		ERR_MSG("员工信息删除失败\n");
		return 0;
	}
	
	if(recv(sfd,info,sizeof(INFO),0)<0)
	{
		ERR_MSG("员工信息删除失败\n");
		return 0;
	}
	//判断返回的ID ，正确是原ID 错误no
	if(strcmp(info->id,"no")==0)
	{
		printf("删除失败\n");
		return 0;
	}
	printf("%s删除成功\n",info->id);
	
	return 0;
}

int do_search(int sfd,INFO *info)//按id查找
{
	printf("请输入要查找的ID：");
	scanf("%s",info->id);
	if(send(sfd,info,sizeof(INFO),0)<0)
	{
		ERR_MSG("员工信息查找失败\n");
		return 0;
	}
	//接收回调函数的结果
	if(recv(sfd,info,sizeof(INFO),0)<0)
	{
		ERR_MSG("员工信息查找失败\n");
		return 0;
	}
	printf("该员工信息为:\n");
	printf("%s\n",info->ins);
 	return 0;
}
int do_amend(int sfd,INFO *info)//修改
{
	printf("请输入要修改对象的ID：");
	scanf("%s",info->id);

	printf("请输入要修改哪一项:");
	printf("***1.姓名*2.年龄*3.性别*4.ID*5.手机号*6.部门*7.薪资***\n");
	scanf("%d",&(info->com));
	//将操作码和修改后的信息发送
	switch(info->com)
	{
	case 1:
		printf("请输入新的姓名\n");
		scanf("%s",info->name);
		break;
	case 2:
		printf("请输入新的年龄\n");
		scanf("%s",info->age);
		break;

	case 3:
		printf("请输入新的性别\n");
		scanf("%s",info->sex);
		break;

	case 4:
		printf("请输入新的ID\n");
		scanf("%s",info->id);
		break;

	case 5:
		printf("请输入新的手机号\n");
		scanf("%s",info->phone);
		break;

	case 6:
		printf("请输入新的部门\n");
		scanf("%s",info->branch);
		break;

	case 7:
		printf("请输入新的薪资\n");
		scanf("%s",info->wage);
		break;

	}
	if(send(sfd,info,sizeof(INFO),0)<0)
	{
		ERR_MSG("员工信息修改失败\n");
		return 0;
	}
	printf("%s用户修改成功\n",info->id);	
	return 0;
}
