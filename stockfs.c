#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>	/* for memcpy */
#include <netinet/in.h>
#include <netdb.h>

int socket_set_up() {
	
	int sd;
	struct sockaddr_in server;

	sd = socket(AF_INET, SOCK_STREAM, 0);

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("76.13.114.90"); /* download.finance.yahoo.com */
	server.sin_port = htons(80);

	int result = connect(sd, (struct sockaddr *) &server, sizeof(server));
	
	return sd;
}
	 
	

int main(void) {

	int bytes, sd = socket_set_up();
	char request[1024], buffer[1024]; 
	char * symbol;
	
	symbol = "GOOG+AAPL";
	strcat(request, "GET /d/quotes.csv?s=");
	strcat(request, symbol);				
	strcat(request, "&f=snl1c1bab6a5 HTTP/1.0\nHOST: download.finance.yahoo.com\n\n");
	bytes = send(sd, request, strlen(request), 0);
	printf("Send %d bytes...\n", bytes);
	bytes = recv(sd, buffer, 1023, 0);
	printf("Read %d bytes...\n\n", bytes);
	printf("%s\n", buffer);
}
