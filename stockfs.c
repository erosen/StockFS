/* Fuse based file system that displays stock ticker information */
/* and keeps a list of favorited stocks                          */
/* Created by Elie Rosen										 */
/* April 14, 2012 												 */

#define FUSE_USE_VERSION  26
#include <fuse.h> /* FUSE library */
#include <sys/socket.h> /* connect to the internet */
#include <sys/types.h>
#include <stdio.h>
#include <string.h>	/* for memcpy */
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h> /* error codes */
#include <fcntl.h>
#include <time.h> 
#include <unistd.h>
#include <arpa/inet.h>

typedef struct {
	int flag;
	char *symbol;
} stock_files; 

stock_files use_table[128], favorite_table[128]; /* support up to 128 stocks */

/* Find the next slot available in the use table */
int getUseIndex(const char *buf) {
	
	int i, index = -1;
	for(i = 0; i < 128; i++) { /* try to find existing entry */
		if(strcmp(buf, use_table[i].symbol) == 0) {
			index = i;
			break;
		}
	}
	
	return index;
}

/* Find the next slot available in the favorite table */
int getFavoriteIndex(const char *buf) {
	
	int i, index = -1;
	for(i = 0; i < 128; i++) { /* try to find existing entry */
		if(strcmp(buf, favorite_table[i].symbol) == 0) {
			index = i;
			break;
		}
	}
	
	return index;
}

/* Find the next slot available in the use table */
int getNextUse() {
	
	int i, index;
	for (i = 0; i < 128; i++) {
		if(use_table[i].flag == 0) {
			index = i;
			break;
		}
	}
	
	return index;
}	

/* Find the next slot available in the favorite table */
int getNextFavorite() {
	
	int i, index;
	for (i = 0; i < 128; i++) {
		if(favorite_table[i].flag == 0) {
			index = i;
			break;
		}
	}
	
	return index;
}

/* Take in the socket and requested symbol, then output the data the server responds with */
static char *getStockInfo(char *symbol) {
	
	int sd;
	char request[1024] = "\0", temp[1024], stockfs_buffer[1024] = "\0";
	
	struct sockaddr_in server;

	sd = socket(AF_INET, SOCK_STREAM, 0);

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("76.13.114.90"); /* download.finance.yahoo.com */
	server.sin_port = htons(80);

	connect(sd, (struct sockaddr *) &server, sizeof(server));

	strcat(request, "GET /d/quotes.csv?s=");
	strcat(request, symbol);				
	strcat(request, "&f=snl1c1bab6a5 HTTP/1.0\nHOST: download.finance.yahoo.com\n\n\0");
	send(sd, request, strlen(request), 0);
	recv(sd, temp, 1023, 0);
	
	shutdown(sd, 0); /* shutdown the socket */
	
	return 	strcat(stockfs_buffer, temp);
}

/* Parse the server data into tokens that translate to the required data and respond with stock information*/
static char *parseStockInfo(char *buffer) {
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
	
	strcpy(buffer, "\0"); /* Clear out the buffer for return message */
	
	/* Check to see if there is a valid Bid if not stock is invalid */
	if(strcmp(data[2], "0.00") != 0) {
		strcat(buffer, "\nSymbol: ");
		strcat(buffer, data[0]);
		strcat(buffer, "\nCompany: ");
		strcat(buffer, data[1]);
		strcat(buffer, "\nLast Trade: ");
		strcat(buffer, data[2]);
		strcat(buffer, "\nChange: ");
		strcat(buffer, data[3]);
		strcat(buffer, "\nBid: ");
		strcat(buffer, data[4]);
		strcat(buffer, "\nAsk: ");
		strcat(buffer, data[5]);
		strcat(buffer, "\nBid Size: ");
		strcat(buffer, data[6]);
		strcat(buffer, "\nAsk Size: ");
		strcat(buffer, data[7]);
	}
	else {
		strcat(buffer, "\nstockfs: ");
		strcat(buffer, data[0]);
		strcat(buffer, " is not a valid stock");
	}
		
	return strcat(buffer, "\n\0");
}	

/* Parse only the stock symbol for storage */
static char *parseStockSymbol(char *buffer) {
	
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
	
	strcpy(buffer, "\0"); /* Clear out the buffer for return message */
	strcat(buffer, data[0]);
	return buffer;
}	

/* If the path is the parent directory, report that it is a directory, 
 * all other files are reported with a 4kb size */
static int stockfs_getattr(const char *path, struct stat *stbuf) {
    int res = 0;

    memset(stbuf, 0, sizeof(struct stat));
    if(strcmp(path, "/") == 0) {
        stbuf->st_mode = S_IFDIR | 0755;
        stbuf->st_nlink = 2;
    }
    else {
        stbuf->st_mode = S_IFREG | 0444;
        stbuf->st_nlink = 1;
        stbuf->st_uid = getuid();
        stbuf->st_gid = getgid();
        stbuf->st_size = 4096; /* 4kb file size */
        stbuf->st_atime = time(NULL);
        stbuf->st_mtime = stbuf->st_atime;
        stbuf->st_ctime = stbuf->st_atime;    
    }
    
    return res;
}

/* List all of the favorite stocks */
static int stockfs_readdir(const char *path, void *buf, 
	fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
		
    (void) offset;
    (void) fi;
    int i;
        
    filler(buf, ".", NULL, 0); /* default */
    filler(buf, "..", NULL, 0); /* default */
    
    /* find any stock that is favorited and list it as an added file */
	for(i = 0; i < 128; i++) {
		if(favorite_table[i].flag == 1) {
			filler(buf, favorite_table[i].symbol, NULL, 0);
		}
	}
	
    return 0;
}

/* Mark a stock as in use and search to see if it is being used already */
static int stockfs_open(const char *path, struct fuse_file_info *fi) {
    
	int index;
	char *symbol = getStockInfo((char *)path + 1);
	
	symbol = parseStockSymbol(symbol);
	
	index = getUseIndex(symbol);
	
	if(index == -1)
		index = getNextUse();
	
	use_table[index].flag = 1;
	use_table[index].symbol = symbol;
	
	
    return 0;
}

/* Display parsed information about the stock, also show error if it is not a stock */
static int stockfs_read(const char *path, char *buf, 
	size_t size, off_t offset, struct fuse_file_info *fi) {
    
    size_t len;
    (void) fi;
    
	strcpy(buf, path + 1); /* copy the path over, remove the first character */
	
    strcpy(buf, getStockInfo(buf));
    strcpy(buf, parseStockInfo(buf));
    
	len = strlen(buf);
    if (offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buf, buf + offset, size);
    } else 
        size = 0;

    return size;
}

/* Mark the stock as unused when finished and clear the symbol value */
static int stockfs_release(const char *path, struct fuse_file_info *fi) {
	
	int index;
	char *symbol = getStockInfo((char *)path + 1);
	
	symbol = parseStockSymbol(symbol);
	
	index = getUseIndex(symbol);
	
	use_table[index].flag = 0;
	use_table[index].symbol = "\0";

	return 0;
}

/* Normally used to set the time but instead this is used to add a stock to the favorites list */
static int stockfs_utimens(const char *path, const struct timespec ts[2]) {

	int index;
	char *symbol = "\0";
	
	symbol = getStockInfo((char *)path + 1);
	strcpy(symbol, parseStockSymbol(symbol));
	symbol = strdup(symbol);
	
	index = getNextFavorite(symbol);
	favorite_table[index].flag = 1;
	favorite_table[index].symbol = symbol;	
		
	return 0;
}

/* No files are written but a write request can be handled */
static int stockfs_write(const char * path, const char *fill, size_t size, off_t offset, struct fuse_file_info *fi) {
	(void) fi;
	(void) offset;
	(void) fill;
	(void) size;
	
	return 0;
}
	
/* When file system initilizes, initialize the tables for search ability */
void *stockfs_init() {
	
	int i;
	for (i = 0; i < 128; i++) {
		use_table[i].flag = 0;
		use_table[i].symbol = "\0";
		favorite_table[i].flag = 0;
		favorite_table[i].symbol = "\0";
	}
	
	return NULL;
}

/* List of valid operations that fuse can interpret and handle */
static struct fuse_operations stockfs_oper = {
    .getattr	= stockfs_getattr,
    .readdir	= stockfs_readdir,
    .open	= stockfs_open,
    .read	= stockfs_read,
    .release	= stockfs_release,
    .utimens	= stockfs_utimens,
    .write	= stockfs_write,
    .init	= stockfs_init,
};

int main(int argc, char *argv[]) {
		
	return fuse_main(argc, argv, &stockfs_oper, NULL);
	
}
