
//gcc client.c -o client

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <arpa/inet.h> 
#include <time.h> 
#include <unistd.h>
#include <fcntl.h>

void replace(char *psz, char c, char w)
{
	psz = strchr(psz, c);
		
	if (psz != NULL)
	{
		*psz = w;
	}
}

int main(int argc , char *argv[])
{
	struct sockaddr_in addrServer;
	char   szServerReply[128];
	int    nSocketDesc, nRecv, nOldFL;
	char   szCommand[64];
	
	// Create socket
	nSocketDesc = socket(AF_INET, SOCK_STREAM, 0);
	
	if (nSocketDesc == -1)
	{
		printf("Could not create socket");
		return 1;
	}
		
	addrServer.sin_addr.s_addr = inet_addr("127.0.0.1");
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(5000);

	// Connect to remote server
	if (connect(nSocketDesc, (struct sockaddr *)&addrServer, sizeof(addrServer)) < 0)
	{
		puts("connect error");
		return 2;
	}
	
	puts("Connected\n");

	nOldFL = fcntl(0, F_GETFL, 0);			// save orignal stdin status
	fcntl(0, F_SETFL, (nOldFL | O_NDELAY));	// set stdin for non blocking input
	
	puts("Enter server command (beat, time, exit)");
	
	while (1)
	{
		// get server command from user
		szCommand[0] = 0;
		fgets(szCommand, sizeof(szCommand)-1, stdin);
		
		if (szCommand[0] != 0)
		{
			replace(szCommand, '\n', 0); // strip '\n' terminator
			printf("Sending %s request\n", szCommand);
			
			if (send(nSocketDesc, szCommand, strlen(szCommand), 0) < 0)
			{
				puts("Send failed");
				return 1;
			}
		}

		nRecv = recv(nSocketDesc, szServerReply, sizeof(szServerReply), MSG_DONTWAIT);
		
		if (nRecv > 0)
		{
			szServerReply[nRecv] = 0;
			printf("Response: %s\n", szServerReply);
		}
		else if (nRecv == 0)
		{
			puts("socket connection lost");
			return 0;
		}
	}

	close(nSocketDesc);

	fcntl(0, F_SETFL, nOldFL); // restore orignal state of stdin
	
	return 0;
}


