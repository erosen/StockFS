#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>	/* for memcpy */
#include <netinet/in.h>
#include <netdb.h>

int main(void)
{
	int sd;
	struct sockaddr_in server;

	sd = socket(AF_INET, SOCK_STREAM, 0);

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("76.13.114.90");
	server.sin_port = htons(80);

	int result = connect(sd, (struct sockaddr *) &server, sizeof(server));
	printf("result: %i\n", result);
	char buffer[1024]= "0";
	int bytes;

	char request[1024] = "GET ";
	strcat(request, "/d/quotes.csv?s=aapl&f=l1 HTTP/1.0\n");
	strcat(request, "HOST: download.finance.yahoo.com\n\n");
	bytes = send(sd, request, strlen(request), 0);
	printf("Send %d bytes...\n", bytes);
	bytes = recv(sd, buffer, 1023, 0);
	printf("Read %d bytes...\n\n", bytes);
	printf("%s\n", buffer);
}
