#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <pthread.h>

#define MAXLINE 512
#define MAXMEM 5
#define NAMELEN 20
#define SERV_PORT 8080
#define LISTENQ 5

int listenfd, connfd[MAXMEM];
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
char user[MAXMEM][NAMELEN];
void Quit();
void rcv_snd(int n);
void play_game(int a, int b);
int main()
{
	pthread_t thread;
	struct sockaddr_in serv_addr, cli_addr;
	socklen_t length;
	char buff[MAXLINE];

	//用socket建server的fd
	listenfd = socket(AF_INET, SOCK_STREAM, 0);

	if (listenfd < 0)
	{
		printf("Socket created failed.\n");
		return -1;
	}
	//網路連線設定
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_port = htons(SERV_PORT); //port80
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	//用bind開監聽器
	if (bind(listenfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
	{
		printf("Bind failed.\n");
		return -1;
	}
	//用listen開始監聽
	printf("listening...\n");
	listen(listenfd, LISTENQ);

	//建立thread管理server
	pthread_create(&thread, NULL, (void *)(&Quit), NULL);

	//紀錄閒置的client(-1)
	//initialize
	int i = 0;
	for (i = 0; i < MAXMEM; i++)
		connfd[i] = -1;
	memset(user, '\0', sizeof(user));
	printf("initialize...\n");

	while (1)
	{
		length = sizeof(cli_addr);
		for (i = 0; i < MAXMEM; i++)
			if (connfd[i] == -1)
				break;
		//等待client端連線
		printf("receiving...\n");
		connfd[i] = accept(listenfd, (struct sockaddr *)&cli_addr, &length);

		//對新client建thread，以開啟訊息處理
		pthread_create(malloc(sizeof(pthread_t)), NULL, (void *)(&rcv_snd), (void *)i);
	}
	return 0;
}
//關閉server
void Quit()
{
	char msg[10];
	while (1)
	{
		scanf("%s", msg);
		if (strcmp("/quit", msg) == 0)
		{
			printf("Bye~\n");
			close(listenfd);
			exit(0);
		}
	}
}

void rcv_snd(int n)
{
	char msg_notify[MAXLINE];
	char msg_recv[MAXLINE];
	char msg_send[MAXLINE];
	char who[MAXLINE];
	char name[NAMELEN];
	char message[MAXLINE];

	char msg1[] = "<SERVER> Who do you want to play? ";
	char msg2[] = "<SERVER> Complete.\n";
	char msg3[] = "<SERVER> Rrefuse to receive.";
	char msg4[] = "<SERVER> Download...\n";
	char msg5[] = "<SERVER> Confirm?";
	char msg6[] = "ok";
	char check[MAXLINE];
	char ok[3];

	int i = 0;
	int retval;

	//獲得client的名字
	int length;
	length = recv(connfd[n], name, MAXLINE, 0);
	if (length > 0)
	{
		name[length] = '\0';
		strcpy(user[n], name);
	}
	//告知所有人有新client加入
	memset(msg_notify, '\0', sizeof(msg_notify));
	strcpy(msg_notify, name);
	strcat(msg_notify, " join\n");
	for (i = 0; i < MAXMEM; i++)
		if (connfd[i] != -1)
			send(connfd[i], msg_notify, strlen(msg_notify), 0);
	//接收某client的訊息並轉發
	while (1)
	{
		memset(msg_recv, '\0', sizeof(msg_recv));
		memset(msg_send, '\0', sizeof(msg_send));
		memset(message, '\0', sizeof(message));
		memset(check, '\0', sizeof(check));

		if ((length = recv(connfd[n], msg_recv, MAXLINE, 0)) > 0)
		{
			msg_recv[length] = 0;
			//輸入quit離開
			if (strcmp("/quit", msg_recv) == 0)
			{
				close(connfd[n]);
				connfd[n] = -1;
				pthread_exit(&retval);
			}
			//輸入chat傳給特定人
			else if (strncmp("/play", msg_recv, 5) == 0)
			{
				int player;
				printf("game...\n");
				send(connfd[n], msg1, strlen(msg1), 0);
				length = recv(connfd[n], who, MAXLINE, 0);
				who[length] = 0;
				for (i = 0; i < MAXMEM; i++)
					if (connfd[i] != -1 && strncmp(who, user[i], strlen(who) - 1) == 0)
					{
						player = i;
						break;
					}
				strcpy(msg_send, "<SERVER> ");
				strcat(msg_send, name);
				strcat(msg_send, " want to play with you. Receive? (y/n)\n");

				send(connfd[player], msg_send, strlen(msg_send), 0);
				// memset(msg_send, 0, sizeof(msg_send));
				length = recv(connfd[player], check, MAXLINE, 0);
				check[length] = 0;
				printf("Ans=");

				//Yes傳送檔案
				if (strncmp(check, "Y", 1) == 0 || strncmp(check, "y", 1) == 0)
				{
					printf("yes\n");
					play_game(n, player);
				}
				//No取消傳送
				else if (strncmp(check, "N", 1) == 0 || strncmp(check, "n", 1) == 0)
				{
					printf("no\n");

					send(connfd[n], msg3, strlen(msg3), 0);
					memset(message, '\0', sizeof(message));
				}
			}
			//傳檔案
			else if (strncmp("/send", msg_recv, 5) == 0)
			{
				printf("file needs to be send...\n");
				pthread_mutex_lock(&mutex);
				send(connfd[n], msg1, strlen(msg1), 0);
				pthread_mutex_unlock(&mutex);
				length = recv(connfd[n], who, MAXLINE, 0);
				who[length] = 0;
				printf("send to %s", who);
				//server傳送確認
				//				pthread_mutex_lock(&mutex);
				send(connfd[n], msg5, strlen(msg5), 0);
				printf("confirm\n");
				//				pthread_mutex_unlock(&mutex);

				//				pthread_mutex_lock(&mutex);
				recv(connfd[n], message, MAXLINE, 0);
				//				message[length]=0;
				printf("receive\n");
				//				pthread_mutex_unlock(&mutex);
				//詢問是否要接收
				strcpy(msg_send, "<SERVER> ");
				strcat(msg_send, name);
				strcat(msg_send, " want to send you a file. Receive? (y/n)");

				for (i = 0; i < MAXMEM; i++)
				{
					if (connfd[i] != -1)
					{
						if (strncmp(who, user[i], strlen(who) - 1) == 0)
						{
							pthread_mutex_lock(&mutex);
							send(connfd[i], msg_send, strlen(msg_send), 0);
							pthread_mutex_unlock(&mutex);

							length = recv(connfd[i], check, MAXLINE, 0);
							check[length] = 0;
							printf("Ans=");

							//Yes傳送檔案
							if (strncmp(check, "Y", 1) == 0 || strncmp(check, "y", 1) == 0)
							{
								printf("yes\n");

								pthread_mutex_lock(&mutex);
								send(connfd[i], msg4, strlen(msg4), 0);
								pthread_mutex_unlock(&mutex);

								length = recv(connfd[i], ok, strlen(ok), 0);
								ok[length] = 0;
								if (strcmp(ok, msg6) == 0)
								{
									pthread_mutex_lock(&mutex);
									send(connfd[i], message, strlen(message), 0);
									pthread_mutex_unlock(&mutex);
								}

								send(connfd[n], msg2, strlen(msg2), 0);
								printf("complete\n");
							}
							//No取消傳送
							else if (strncmp(check, "N", 1) == 0 || strncmp(check, "n", 1) == 0)
							{
								printf("no\n");

								send(connfd[n], msg3, strlen(msg3), 0);
								memset(message, '\0', sizeof(message));
							}
						}
					}
				}
			}
			//顯示目前在線
			else if (strncmp("/list", msg_recv, 5) == 0)
			{
				strcpy(msg_send, "<SERVER> Online:");
				for (i = 0; i < MAXMEM; i++)
					if (connfd[i] != -1)
					{

						strcat(msg_send, user[i]);
						strcat(msg_send, " ");
					}
				strcat(msg_send, "\n");
				send(connfd[n], msg_send, strlen(msg_send), 0);
			}
			//直接傳給每個人
			else
			{
				strcpy(msg_send, name);
				strcat(msg_send, ": ");
				strcat(msg_send, msg_recv);

				for (i = 0; i < MAXMEM; i++)
					if (connfd[i] != -1)
					{
						if (strcmp(name, user[i]) == 0)
							continue;
						else
							send(connfd[i], msg_send, strlen(msg_send), 0);
					}
			}
		}
	}
}

void play_game(int p1, int p2)
{
	char msg_comfirm[MAXLINE] = {0}, msg_win[MAXLINE] = {0}, msg_lose[MAXLINE] = {0};
	char position[MAXLINE];
	int player[2] = {p1, p2}, endgame = 0, x, y;
	int arr[3][3] = {{0}};
	memset(arr, 0, sizeof(arr));
	sprintf(msg_comfirm, "#play begin \n<SERVER> %s is playing with %s...\nGame Begin...\n%s first...\n", user[p1], user[p2], user[p2]);
	send(connfd[p1], msg_comfirm, strlen(msg_comfirm), 0);
	send(connfd[p2], msg_comfirm, strlen(msg_comfirm), 0);

	printf("%s is playing with %s...\n", user[p1], user[p2]);
	memset(msg_comfirm, 0, sizeof(msg_comfirm));
	for (int i = 1; i <= 9; i++)
	{
		char msg_round[MAXLINE] = {0};
		sprintf(msg_round, "#play round %d %d %d %d %d %d %d %d %d\n", arr[0][0], arr[0][1], arr[0][2], arr[1][0], arr[1][1], arr[1][2], arr[2][0], arr[2][1], arr[2][2]);
		send(connfd[player[i % 2]], msg_round, strlen(msg_round), 0);
		send(connfd[player[(i - 1) % 2]], msg_round, strlen(msg_round), 0);
		memset(msg_round, 0, sizeof(msg_round));
		//endgame = game(arr);
		if (endgame)
		{
			printf("the game is the end...\n");
			sprintf(msg_win, "#play end \n<SERVER> You are the Winner!!!\n");
			sprintf(msg_lose, "#play end \n<SERVER> You are the loser...\n");
			send(connfd[player[i % 2]], msg_win, strlen(msg_win), 0);
			send(connfd[player[(i - 1) % 2]], msg_lose, strlen(msg_lose), 0);
			break;
		}
		char turn1[] = "Your turn... (x y)>>\n";
		send(connfd[player[i % 2]], turn1, strlen(turn1), 0);
		char turn2[] = "the opponent turn...\n";
		send(connfd[player[(i - 1) % 2]], turn2, strlen(turn2), 0);
		memset(turn1, 0, sizeof(turn1));
		memset(turn2, 0, sizeof(turn2));
		int length = recv(connfd[player[i % 2]], position, MAXLINE, 0);
		if (strcmp(position, "/quitgame") == 0)
		{
			endgame = 1;
			break;
		}
		sscanf(position, "%d %d", &x, &y);
		printf("x=%d, y=%d\n", x, y);
		arr[x - 1][y - 1] = i % 2 ? 1 : 2;
		char msg_choose[MAXLINE];
		sprintf(msg_choose, "%s choose %d-%d...\n", user[player[i % 2]], x, y);
		send(connfd[player[i % 2]], msg_choose, strlen(msg_choose), 0);
		send(connfd[player[(i - 1) % 2]], msg_choose, strlen(msg_choose), 0);
		memset(msg_choose, 0, sizeof(msg_choose));
	}
	if (!endgame)
	{
		// peace
	}
}