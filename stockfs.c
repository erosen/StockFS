#define FUSE_USE_VERSION  26
#include <fuse.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <stdio.h>
#include <string.h>	/* for memcpy */
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>

/* Take in the socket and requested symbol, then output the data the server responds with */
void getStockInfo(char* symbol, char *buffer) {
	
	int bytes, sd;
	char request[1024];
	
	struct sockaddr_in server;

	sd = socket(AF_INET, SOCK_STREAM, 0);

	memset(&server, 0, sizeof(server));
	server.sin_family = AF_INET;
	server.sin_addr.s_addr = inet_addr("76.13.114.90"); /* download.finance.yahoo.com */
	server.sin_port = htons(80);

	connect(sd, (struct sockaddr *) &server, sizeof(server));
	
	strcat(request, "GET /d/quotes.csv?s=");
	strcat(request, symbol);				
	strcat(request, "&f=snl1c1bab6a5 HTTP/1.0\nHOST: download.finance.yahoo.com\n\n");
	bytes = send(sd, request, strlen(request), 0);
	printf("Send %d bytes...\n", bytes);
	bytes = recv(sd, buffer, 1023, 0);
	printf("Read %d bytes...\n\n", bytes);
	printf("%s\n", buffer);
	
	shutdown(sd, 0); /* shutdown the socket */
	
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

static const char stockfs_buffer[1024];


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
        stbuf->st_size = 4096; /* 4kb file size */
    }
    
    return res;
}

static int stockfs_readdir(const char *path, void *buf, 
	fuse_fill_dir_t filler, off_t offset, struct fuse_file_info *fi) {
		
    (void) offset;
    (void) fi;

    if(strcmp(path, "/") != 0)
        return -ENOENT;

    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    //filler(buf, stockfs_path + 1, NULL, 0);

    return 0;
}

static int stockfs_open(const char *path, struct fuse_file_info *fi) {
    
    
    if((fi->flags & 3) != O_RDONLY)
        return -EACCES;

    return 0;
}

static int stockfs_read(const char *path, char *buf, 
	size_t size, off_t offset, struct fuse_file_info *fi) {
    size_t len;
    (void) fi;
    
    len = strlen(stockfs_buffer);
    if (offset < len) {
        if (offset + size > len)
            size = len - offset;
        memcpy(buf, stockfs_buffer + offset, size);
    } else
        size = 0;

    return size;
}

static int stockfs_release(const char *path, struct fuse_file_info *fi) {
	return 0;
}

static int stockfs_mknod(const char *path, mode_t mode, dev_t dev) { 
	
	
	
	return 0;
}

static struct fuse_operations stockfs_oper = {
    .getattr	= stockfs_getattr,
    .readdir	= stockfs_readdir,
    .open	= stockfs_open,
    .read	= stockfs_read,
    .release	= stockfs_release,
    .mknod	= stockfs_mknod,
};

int main(int argc, char *argv[]) {

	char *symbol; 
		
	symbol = "msft";
	
	//getStockInfo(symbol, stockfs_buffer);
	
	//parseStockInfo(buffer);
	
	return fuse_main(argc, argv, &stockfs_oper, NULL);
	
}
