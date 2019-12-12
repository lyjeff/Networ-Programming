#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netdb.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define MESSAGE_BUFF 500
#define USERNAME_BUFF 10

typedef struct
{
	char *prompt;
	int socket;
} thread_data;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;

//與server連線
void *connect_to_server(int socket_fd, struct sockaddr_in *address)
{
	int response = connect(socket_fd, (struct sockaddr *)address, sizeof *address);
	if (response < 0)
	{
		fprintf(stderr, "connect() failed: %s\n", strerror(errno));
		exit(1);
	}
	else
		printf("Connected\n");
}

//打字、傳送訊息
void *send_message(char prompt[USERNAME_BUFF + 4], int socket_fd, struct sockaddr_in *address, char username[])
{
	char message[MESSAGE_BUFF];
	char buff[MESSAGE_BUFF];
	char notice[] = "/send";
	FILE *fp;
	memset(message, '\0', sizeof(message));
	memset(buff, '\0', sizeof(buff));

	send(socket_fd, username, strlen(username), 0);

	while (fgets(message, MESSAGE_BUFF, stdin) != NULL)
	{
		printf("%s> ", username);
		if (strncmp(message, "/quit", 5) == 0)
		{
			printf("Close connection...\n");
			exit(0);
		}
		/*
		   else if(strncmp(message, "/send", 5) == 0)
		   {

		//notice server thar client want to send a file
		send(socket_fd,message,strlen(message),0);
		fp=fopen("./send/send.txt","r");
		fscanf(fp,"%s",buff);
		fclose(fp);
		send(socket_fd,buff,strlen(buff),0);
		}
		 */
		send(socket_fd, message, strlen(message), 0);
		memset(message, '\0', sizeof(message));
	}
}

void *receive(void *threadData)
{
	int socket_fd, response;
	char message[MESSAGE_BUFF];
	thread_data *pData = (thread_data *)threadData;
	socket_fd = pData->socket;
	char *prompt = pData->prompt;

	char buff[MESSAGE_BUFF];
	char buff2[2];
	FILE *fp;

	char catch[] = "<SERVER> Download...\n";
	char confirm[] = "<SERVER> Confirm?";
	char login_fail[] = "<SERVER> Login Failed...\n";
	char play_begin[] = "#play begin ";
	char play_round[] = "#play round ";
	char play_end[] = "#play end ";
	char ok[] = "ok";

	//接收訊息
	while (1)
	{
		memset(message, 0, sizeof(message));
		memset(buff, 0, sizeof(buff));
		fflush(stdout);
		response = recv(socket_fd, message, MESSAGE_BUFF, 0);
		// printf("message=%s\n", message);
		if (response == -1)
		{
			fprintf(stderr, "recv() failed: %s\n", strerror(errno));
			break;
		}
		else if (response == 0)
		{
			printf("\nPeer disconnected\n");
			break;
		}
		message[response] = '\0';
		//傳檔案
		if (strcmp(message, confirm) == 0)
		{
			fp = fopen("./send/owo.txt", "r");
			fgets(buff, MESSAGE_BUFF, fp);
			fclose(fp);
			send(socket_fd, buff, strlen(buff), 0);
		}
		else if (strncmp(message, catch, 20) == 0)
		{
			send(socket_fd, ok, 2, 0);
			fp = fopen("./receive/owo.txt", "w");
			pthread_mutex_lock(&mutex);
			recv(socket_fd, buff, MESSAGE_BUFF, 0);
			pthread_mutex_unlock(&mutex);
			fprintf(fp, "%s", buff);
			fclose(fp);
		}
		else if (strcmp(message, login_fail) == 0)
		{
			printf("Login Failed...\n");
			printf("Stop the program and login again!\n");
			break;
		}
		else if (strstr(message, "want to play with you. Receive? (y/n)") != NULL)
		{
			printf("%s", message);
			fgets(buff, MESSAGE_BUFF, stdin);
			send(socket_fd, buff, strlen(buff), 0);
			// memset(buff, 0, sizeof(buff));
			// memset(message, 0, sizeof(message));
			// fflush(stdout);
			// continue;
		}
		else if (strstr(message, play_begin) != NULL)
		{
			printf("%s\n", message);
			// printf("%s", message + strlen(play_begin));
			// memset(message, '\0', MESSAGE_BUFF);
			// memset(buff, '\0', sizeof(buff));
			// fflush(stdout);
		}
		else if (strstr(message, play_round) != NULL)
		{
			char *str = message + strlen(play_round);
			int arr[3][3];
			sscanf(str, "%d %d %d %d %d %d %d %d %d", &arr[0][0], &arr[0][1], &arr[0][2], &arr[1][0], &arr[1][1], &arr[1][2], &arr[2][0], &arr[2][1], &arr[2][2]);
			printf("\n=======================\n");
			for (int i = 0; i < 3; i++)
			{
				for (int j = 0; j < 3; j++)
					if (arr[i][j] == 0)
						printf("-");
					else if (arr[i][j] == 1)
						printf("O");
					else
						printf("X");
				printf("\n");
			}
			// memset(message, '\0', MESSAGE_BUFF);
			// memset(buff, '\0', sizeof(buff));
			// fflush(stdout);
		}
		else if (strncmp(message, play_end, strlen(play_end)) == 0)
		{
			printf("%s", message + strlen(play_end));
			// fflush(stdout);
			continue;
		}
		else if (strstr(message, "turn") != NULL)
		{
			printf("%s", message);
			if (strncmp(message, "Your", strlen("Your")) == 0)
			{
				fgets(buff, MESSAGE_BUFF, stdin);
				send(socket_fd, buff, strlen(buff), 0);
				// memset(buff, 0, sizeof(buff));
				if (strcmp(buff, "/quitgame") == 0)
					continue;
			}
			// fflush(stdout);
		}
		else
		{
			printf("%s", message);
			printf("%s", prompt);
			// fflush(stdout);
		}
		memset(message, 0, sizeof(message));
		memset(buff, 0, sizeof(buff));
		fflush(stdout);
	}
}

int main(int argc, char **argv)
{
	long port = strtol(argv[2], NULL, 10);
	struct sockaddr_in address, cl_addr;
	char *server_address;
	int socket_fd, response;
	char prompt[USERNAME_BUFF + 4];
	char username[USERNAME_BUFF];
	pthread_t thread;

	//檢查arguments
	if (argc < 3)
	{
		printf("Usage: ./cc_thread [IP] [PORT]\n");
		exit(1);
	}

	//進入大廳
	printf("Enter your name: ");
	fgets(username, USERNAME_BUFF, stdin);
	username[strlen(username) - 1] = 0;
	strcpy(prompt, username);
	strcat(prompt, "> ");
	//連線設定
	server_address = argv[1];
	address.sin_family = AF_INET;
	address.sin_addr.s_addr = inet_addr(server_address);
	address.sin_port = htons(port);
	socket_fd = socket(AF_INET, SOCK_STREAM, 0);

	connect_to_server(socket_fd, &address);

	//建thread data
	thread_data data;
	data.prompt = prompt;
	data.socket = socket_fd;

	//建thread接收訊息
	pthread_create(&thread, NULL, (void *)receive, (void *)&data);

	//傳送訊息
	send_message(prompt, socket_fd, &address, username);

	//關閉
	close(socket_fd);
	pthread_exit(NULL);
	return 0;
}
