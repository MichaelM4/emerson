
//gcc server.c -o server -lpthread

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>
#include <fcntl.h>
#include <termios.h>

/////////////////////////////////////////////////////////////////////

int g_nExitServer;

/////////////////////////////////////////////////////////////////////
// listening thread handling connection for each client
//
// processes the following received requests:
//  "beat" - keeps the connection alive for another 30 seconds.
//  "time" - returns the current server time.
//           after 10 time requests the connection is closed.
//  "exit" - signals the shut down of the server.
//
void *connection_handler(void *pClient)
{
	time_t tTimeOut = time(NULL) + 30;
	int    nSock = *(int*)pClient;
	int    nReadSize;
	char   szClientMessage[100];
	int    nMessageCount = 0;
	
	puts("New connection accepted");

	// Receive a message from client
	while (tTimeOut > time(NULL))
	{
		nReadSize = recv(nSock, szClientMessage, sizeof(szClientMessage), MSG_DONTWAIT);

		if (nReadSize > 0)
		{
			szClientMessage[nReadSize] = 0;

			printf("Received: %s\n", szClientMessage);

			tTimeOut = time(NULL) + 30;

			if (strcmp(szClientMessage, "beat") == 0)
			{
				strcpy(szClientMessage, "heartbeat received");
			}
			else if (strcmp(szClientMessage, "exit") == 0)
			{
				puts("exit received");
				strcpy(szClientMessage, "exit received");
				g_nExitServer = 1;
				tTimeOut = 0;
			}
			else
			{
				++nMessageCount;

				if (nMessageCount >= 10)
				{
					g_nExitServer = 1;
					tTimeOut = 0;
				}

				if (strcmp(szClientMessage, "time") == 0)
				{
					time_t ticks = time(NULL);
					snprintf(szClientMessage, sizeof(szClientMessage), "%s", ctime(&ticks));
					puts("Sending time response");
				}
			}

			// Send the message back to client
			write(nSock , szClientMessage , strlen(szClientMessage));
		}
		else if (nReadSize == 0)
		{
			puts("Client side socket has closed");
			tTimeOut = 0;
		}
		
		sleep(1); // lowers the threads burden on the CPU
	}

	puts("Closing server side socket");
	close(nSock);
	
	// free the socket pointer
	free(pClient);
	
	return 0;
}

/////////////////////////////////////////////////////////////////////
int main(int argc, char *argv[])
{
	int    *pnTemp;
	int    nListener = 0, nNewSocket = 0, nOldFL;
	fd_set fdCurrentSockets, fdWorkingSockets;
	struct sockaddr_in addrServer;
	struct timeval tVal;
	char   szBuff[1025];
	char   szCommand[128];
	time_t tTime;

	nListener = socket(AF_INET, SOCK_STREAM, 0);
	memset(&addrServer, '0', sizeof(addrServer));

	addrServer.sin_family      = AF_INET;
	addrServer.sin_addr.s_addr = htonl(INADDR_ANY);
	addrServer.sin_port        = htons(5000);

	bind(nListener, (struct sockaddr*)&addrServer, sizeof(addrServer));
	listen(nListener, 10);

	FD_ZERO(&fdCurrentSockets);
	FD_SET(nListener, &fdCurrentSockets);

	tVal.tv_sec  = 1;
	tVal.tv_usec = 0;

	// save current FL and set input to non blocking
	nOldFL = fcntl(0, F_GETFL, 0);
	fcntl(0, F_SETFL, (nOldFL | O_NDELAY));

	puts("Type quit and ENTER to exit");

	g_nExitServer = 0;

	while (g_nExitServer == 0)
	{
		// make backup copy because select will modify it
		fdWorkingSockets = fdCurrentSockets;

		if (select(FD_SETSIZE, &fdWorkingSockets, NULL, NULL, &tVal) < 0)
		{
			perror("select() error");
			return 1;
		}

		if (FD_ISSET(nListener, &fdWorkingSockets))
		{
			nNewSocket = accept(nListener, (struct sockaddr*)NULL, NULL);

			tTime = time(NULL);
			snprintf(szBuff, sizeof(szBuff), "%.24s\r\n", ctime(&tTime));
			write(nNewSocket, szBuff, strlen(szBuff));

			pthread_t threadSniffer;
			pnTemp  = malloc(1);
			*pnTemp = nNewSocket;

			if (pthread_create(&threadSniffer, NULL, connection_handler, (void*)pnTemp) < 0)
			{
				perror("could not create thread");
				return 2;
			}

			puts("Handler assigned");
		}

		szCommand[0] = 0;
		fgets(szCommand, sizeof(szCommand)-1, stdin);

		if (strcmp(szCommand, "quit\n") == 0)
		{
			g_nExitServer = 1;
		}
	}

	fcntl(0, F_SETFL, nOldFL); // restore original flags
	close(nListener);
	puts("Shutting down server");

	return 0;
}

