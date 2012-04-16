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
	int favorite;
	const char *symbol;
} stock_files; 

static stock_files table[128]; /* support up to 128 stocks */

int getIndex(const char *path) {
	
	int i, index = -1;
	for(i = 0; i < 128; i++) { /* try to find existing entry */
		if(strcmp(path, table[i].symbol) == 0) {
			index = i;
			break;
		}
	}
	
	return index;
}

int getNextPlace() {
	
	int i, index;
	for (i = 0; i < 128; i++) {
		if(table[i].favorite == 0) {
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
        stbuf->st_mode = S_IFREG | 0666;
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

static int stockfs_readdir(const char *path, void *buf, 
	fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
		
    (void) offset;
    (void) fi;
    int i;
        
    filler(buf, ".", NULL, 0); /* default */
    filler(buf, "..", NULL, 0); /* default */
    
    /* find any stock that is favorited and list it as an added file */
	for(i = 0; i < 128; i++) {
		if(table[i].favorite == 1) {
			filler(buf, table[i].symbol, NULL, 0);
		}
	}
	
    return 0;
}
	
static int stockfs_open(const char *path, struct fuse_file_info *fi) {
    
	//int index;

	//index = getNextPlace();
	
	//table[index].symbol = (path + 1);
	
    return 0;
}

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

static int stockfs_release(const char *path, struct fuse_file_info *fi) {
	
	//int index;
	
	//index = getIndex(path);
	
	//table[index].used = 0;

	return 0;
}

static int stockfs_mknod(const char *path, mode_t mode, dev_t dev) { 
	
	//int index;
	//index = getIndex(path);
	
	//table[index].favorite = 1;	
	
	return 0;
}

/* When file system initilizes, create an empty table for later use */
void *stockfs_init() {
	
	int i;
	for (i = 0; i < 128; i++) {
		//table[i].used = 0;
		table[i].favorite = 0;
		//table[i].symbol = "\0";
	}
	
	return NULL;
}

static int stockfs_utimens(const char *path, const struct timespec ts[2]) {

	int index;
	
   // index = getIndex(path + 1);
	
	//if(index == -1)   /* if the symbol is not found, find the next open place */
	index = getNextPlace();
	
	//table[index].used = 1;		
	//table[index].favorite = 1;	
	table[index].symbol = (path + 1);
	
	return 0;
}
	
static struct fuse_operations stockfs_oper = {
    .getattr	= stockfs_getattr,
    .readdir	= stockfs_readdir,
    .open	= stockfs_open,
    .read	= stockfs_read,
    .release	= stockfs_release,
    .mknod	= stockfs_mknod,
    .init	= stockfs_init,
    .utimens	= stockfs_utimens,
};

int main(int argc, char *argv[]) {
		
	return fuse_main(argc, argv, &stockfs_oper, NULL);
	
}
