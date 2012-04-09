#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>	/* for memcpy */
#include <netinet/in.h>
#include <netdb.h>

/* This function sets up the socket connection and returns the socket for later use */
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
/* Take in the socket and requested symbol, then output the data the server responds with */
void getStockInfo(int sd, char* symbol, char *buffer) {
	
	int bytes;
	char request[1024];
	
	strcat(request, "GET /d/quotes.csv?s=");
	strcat(request, symbol);				
	strcat(request, "&f=snl1c1bab6a5 HTTP/1.0\nHOST: download.finance.yahoo.com\n\n");
	bytes = send(sd, request, strlen(request), 0);
	printf("Send %d bytes...\n", bytes);
	bytes = recv(sd, buffer, 1023, 0);
	printf("Read %d bytes...\n\n", bytes);
	printf("%s\n", buffer);
	
}

/* Parse the server data into tokens that translate to the required data */
void parseStockInfo(char *buffer) {
	int i, j;
	char data[8][25];
	
	for (i = 0; i < 1023; i++) { /* find start of string */
		if ( (buffer[i] == '\r') && (buffer[i+1] == '\n') && (buffer[i+2] == '\r') && (buffer[i+3] == '\n') ) {
			i += 5;
			break;
		}
	}

	for (j = 0; j < 8; j++) { /* Stock symbol */
		data[0][j] = buffer[i];
		i++;
		
		if (buffer[i] == '"') {
			data[0][j+1] = '\0';
			i+=3;
			break;
		}
	}
	
	for (j = 0; j < 40; j++) { /* Company */
		data[1][j] = buffer[i];
		i++;
		
		if(buffer[i] == '"') {
			data[1][j+1] = '\0';
			i+=2;
			break;
		}
	}

	for (j = 0; j < 10; j++) { /* last trade */
		data[2][j] = buffer[i];
		i++;
		
		if(buffer[i] == ',') {
			data[2][j+1] = '\0';
			i++;
			break;
		}
	}
	
	for (j = 0; j < 10; j++) { /* change */
		data[3][j] = buffer[i];
		i++;
		
		if(buffer[i] == ',') {
			data[3][j+1] = '\0';
			i++;
			break;
		}
	}
	
	for (j = 0; j < 10; j++) { /* bid */
		data[4][j] = buffer[i];
		i++;
		
		if(buffer[i] == ',') {
			data[4][j+1] = '\0';
			i++;
			break;
		}
	}
	
	for (j = 0; j < 10; j++) { /* ask */
		data[5][j] = buffer[i];
		i++;
		
		if(buffer[i] == ',') {
			data[5][j+1] = '\0';
			i++;
			break;
		}
	}
		
	for (j = 0; j < 10; j++) { /* bid size */
		data[6][j] = buffer[i];
		i++;
		
		if(buffer[i] == ',') {
			data[6][j+1] = '\0';
			i++;
			break;
		}
	}
	
	for (j = 0; j < 10; j++) { /* ask size */
		data[7][j] = buffer[i];
		i++;
		
		if(buffer[i] == '\r') {
			data[7][j+1] = '\0';
			break;
		}
	}
	
	printf("Symbol: %s\n", data[0]);
	printf("Company: %s\n", data[1]);
	printf("Last Trade: %s\n", data[2]);
	printf("Change: %s\n", data[3]);
	printf("Bid: %s\n", data[4]);
	printf("Ask: %s\n", data[5]);
	printf("Bid Size:%s\n", data[6]);
	printf("Ask Size: %s\n", data[7]);
}

int main(void) {

	int sd = socket_set_up();
	char *symbol; 
	static char buffer[1024];
	//static char data[8][40]; /* parsed stock info */
		
	symbol = "msft";
	
	getStockInfo(sd, symbol, buffer);
	
	parseStockInfo(buffer);
	
	return 0;
	
}
