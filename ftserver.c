/*****************************************************************************************
Name: David Chen

Server side implementation of SSH File Transfer protocol. This program listens on
port_number for any incoming connections. Once a connection is received, ftserver.c
handles the requested command by either sending a list of its current directory or
a file 

USAGE: ./ftserver <port number>

Resources: http://beej.us/guide/bgnet
*****************************************************************************************/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>

#define MAX_ARG 2	// Number of arguments
#define PORT_LEN 6	// Max digit of a port number
#define HOSTMAME_LEN 50		// Max length of hostname
#define SLEEP_TIMER 100		// Time in between sending and receiving messages from server
#define MAX_FILENUM 100		// Max number of files
#define MAX_FILENAME 256	// Max length of filename

/*********************************************************************
** void *get_in_addr(struct sockaddr*)
** Checks if IPV4 or IPV6
** 
** Args: (sockaddr*) socket address
**********************************************************************/
void *get_in_addr(struct sockaddr *sa) {
	if(sa->sa_family == AF_INET) {
			return &(((struct sockaddr_in*)sa)->sin_addr);
		}

	return &(((struct sockaddr_in6*)sa)->sin6_addr);
}

/*********************************************************************
** int createDataConnection(char*, char* )
** Establish data connection 
** 
** Args: (int) data port number, (char*) host name
** Return: socket file descriptor of data connection
**********************************************************************/
int createDataConnection(char* dataPort, char* hostName) {
	static const bufferSize = 500;
    int sockfd;
    char buf[bufferSize];
    struct addrinfo hints, *servinfo, *p;
    int rv;
    char s[INET6_ADDRSTRLEN]; 
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    if ((rv = getaddrinfo(hostName, dataPort, &hints, &servinfo)) != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rv));
        return 1;
    }

    // loop through all the results and connect to the first we can
    for(p = servinfo; p != NULL; p = p->ai_next) {
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            continue;
        }
        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            continue;
        }
        break;
    }

    if (p == NULL) {
        fprintf(stderr, "Server data connect: failed to find address\n");
        exit(1);
    }

    inet_ntop(p->ai_family, get_in_addr((struct sockaddr*)p->ai_addr), s, sizeof s);

    freeaddrinfo(servinfo); // all done with this structure
    
    return sockfd;    
}

/********************************************************************
** struct sockaddr_in getServerAddress(int)
** Create the server address
** 
** Args: (int)port number
** Return: (struct sockaddr_in) server address
********************************************************************/
struct sockaddr_in getServerAddress(int portNumber) {
	struct sockaddr_in serverAddress;
	// Clear out address struct before use
	memset((char*)&serverAddress, '\0', sizeof(serverAddress));
	// IPv4
	serverAddress.sin_family = AF_INET;
	// Convert port number to network byte order
	serverAddress.sin_port = htons(portNumber);
	// Any address is allowed for connection to this process
	serverAddress.sin_addr.s_addr = INADDR_ANY;
	
	return serverAddress;
}

/*********************************************************************
** int createSocket()
** Create a socket
** 
** Args: none
** Return: (int) socket file descriptor
**********************************************************************/
int createSocket() {
	// TCP socket
	int socketFD = socket(AF_INET, SOCK_STREAM, 0);
	if (socketFD < 0) {
		fprintf(stderr, "SERVER: Error, cannot create socket\n");
		exit(1);
	}
	return socketFD;
}

/*************************************************************************
** int bindSocket()
** Binds socket with address
** 
** Args: (struct sockaddr_in) server address, (int) socket file desciptor
** Return: (int) socket file descriptor
**************************************************************************/
void bindSocket(struct sockaddr_in serverAddress, int socketFD) {
	if (bind(socketFD, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
		fprintf(stderr, "SERVER: ERROR on binding. Try another port number.\n");
		exit(1);
	}
}

/*********************************************************************
** int isValidFile(char*, int, char **)
** Checks if given file name is in current directory
** 
** Args: (char*) file name, (int)number of files, (char**) file list
** Return: 1 if true, 0 otherwise
**********************************************************************/
int isValidFile(char* fileName, int fileNum, char fileList[][MAX_FILENAME]) {
	int i;
	for (i = 0; i < fileNum; i++) {
		if (strcmp(fileName, fileList[i]) == 0)
			return 1;
	}
	return 0;	
}

/*****************************************************************
** int getFileSize(char* )
** Get the size of a file
** 
** Args: char* file name
** Return: (int)size of file in bytes
*****************************************************************/
int getFileSize(char* fileName) {
	struct stat st;
	stat(fileName, &st);
	return st.st_size - 1;
}

/*****************************************************************
** int getFilesInDir(char **)
** Retrieve all file names in current directory
** 
** Args: char** file list
** Return: (int)number of files 
*****************************************************************/
int getFilesInDir(char fileList[][MAX_FILENAME] ) {
 	// Open current directory
    DIR* folder;
    folder = opendir(".");
    // Pointer to each file in current directory
	struct dirent* fileInDir;
    // Copy the name of each file into fileList
    int fileNumber = 0;
    while (fileInDir = readdir(folder)) {
        // Skip file names with . and ..
         if (strcmp(fileInDir->d_name, ".") != 0 && strcmp(fileInDir->d_name, "..") != 0) {
            strcpy(fileList[fileNumber], fileInDir->d_name);
            fileNumber++;
         }
    }
    // Close directory
    closedir(folder);

	return fileNumber;
}

/*********************************************************************************************
** void getClientCommand(int, char*, char*, char*)
** Recevies command arguments from client
** 
** Args: (int)connection socket, (char*)host name, (char*)client command, (char*) data port
** Return: none
*********************************************************************************************/
void getClientCommand(int establishedConnectionFD, char* hostName, char* clientCommand, char* dataPort) {
	recv(establishedConnectionFD, hostName, HOSTMAME_LEN, 0);
	//printf("SERVER: Client hostname: %s\n", hostName);
	recv(establishedConnectionFD, dataPort, PORT_LEN, 0);
	//printf("SERVER: Client port %s\n", dataPort);
	recv(establishedConnectionFD, clientCommand, MAX_FILENAME, 0);
	//printf("SERVER: Client command %s\n", clientCommand);
}

/********************************************************************
** void getFilesInDir(int, int, char**)
** Send directory list to client
** 
** Args: (int)data socket, (int)number of files, (char**) file list
** Return: (int)number of files in current directory
********************************************************************/
void sendDirectory(int dataSocketFD, int fileNum, char fileList[][MAX_FILENAME]) {
	printf("SERVER: sending directory list...\n");

	char fileNumString[12];
	memset(fileNumString, '\0', sizeof(fileNumString));
	// Convert fileNum from int to stirng
	sprintf(fileNumString, "%d", fileNum);
	//Send number of files to client
	send(dataSocketFD, fileNumString, strlen(fileNumString), 0);
	usleep(SLEEP_TIMER);

	// Send all file names to client
	int i;
	for (i = 0; i < fileNum; i++) {
		send(dataSocketFD, fileList[i], strlen(fileList[i]), 0);
		//printf("filename: %s filelen: %d\n", fileList[i], strlen(fileList[i]));
		usleep(SLEEP_TIMER);
	}
	printf("SERVER: directory list send\n");
}

/*******************************************************************************************************************************
** void sendFile(int, char*)
** Sends a file to client

** Args: (int)data socket, (char*) file name,(int)number of files, (char**) file list
** Return: none
*******************************************************************************************************************************/
void sendFile(int dataSocketFD, char* fileName) {
	printf("SERVER: sending file %s\n", fileName);

	static const bufferSize = 4096;
	// Send the size of the file to client
	char fileSize[12];
	memset(fileSize, '\0', sizeof(fileSize));
	sprintf(fileSize, "%d", getFileSize(fileName));
	send(dataSocketFD, fileSize, strlen(fileSize), 0);
	usleep(SLEEP_TIMER);

	char buffer[bufferSize];
	memset(buffer, '\0', sizeof(buffer));
	int input_file = open(fileName, O_RDONLY);

	while(1) {
		// Read the file
		int bytesRead = read(input_file, buffer, sizeof(buffer));
		if (bytesRead == 0) // Done reading
			break;
		if (bytesRead < 0) {
			fprintf(stderr, "SERVER: error reading from file %s", fileName);
			return;
		}

		void *p = buffer;
		while (bytesRead > 0) {
			// Send buffer to client
			int bytesSent = send(dataSocketFD, p, bytesRead, 0);
			usleep(SLEEP_TIMER);
			bytesRead -= bytesSent;
			p += bytesSent;
		}
	}
	close(input_file);

	printf("File sent\n");
}

/***********************************************************************************************
** void performCommand(char*, int, int, char**)
** Perform command based on client argument
** 
** Args: (char*) command -l or -g, (int)data socket, (int) number of files, (char**) file list
** Return: (int)number of files in current directory
************************************************************************************************/
void performCommand(char* command, int dataSocketFD, int fileNum, char fileList[][MAX_FILENAME]) {
	// Get first command (-l or -g)
	char* token = strtok(command, " \n");
    if (strcmp(token, "-l") == 0)	// Send dir list
        sendDirectory(dataSocketFD, fileNum, fileList);
    else if (strcmp(token, "-g") == 0) {
        token = strtok(NULL, " \n"); // Retrieve file name
        char* status;
        if (isValidFile(token, fileNum, fileList)) {	// Valid file name
			status = "VALID";
			send(dataSocketFD, status, strlen(status), 0);
			usleep(SLEEP_TIMER);
			sendFile(dataSocketFD, token);	// Send the file content
        }
		else {
			printf("SERVER: Invalid file: does not exist\n");	// Invalid file name
			status = "INVALID";
			send(dataSocketFD, status, strlen(status), 0);	
		}
	}
	else 	// Invalid command
		printf("SERVER: Invalid command %s", token);	
}

/********************************************************************
** int establishConnection(int)
** Establish connection with client
** 
** Args: (int) socket file descriptor
** Return: (int)file descriptor of established connection
********************************************************************/
int establishConnection(int socketFD) {
	struct sockaddr_in clientAddress;
	socklen_t sizeOfClientInfo;
    int establishedConnectionFD;
    char ipAddr[INET_ADDRSTRLEN];

	sizeOfClientInfo = sizeof(clientAddress);
	// Accept connection		
	establishedConnectionFD = accept(socketFD, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);

	// Print out the IP address of connected client
	inet_ntop(AF_INET, &(clientAddress.sin_addr), ipAddr, INET_ADDRSTRLEN);
	printf("Connection established with %s\n", ipAddr);

	return establishedConnectionFD;
}

int main(int argc, char* argv[]) {

	// Validate arguments
	if (argc < MAX_ARG) {
		fprintf(stderr, "USAGE: %s port\n", argv[0]);
		exit(1);	
	}
	
	char fileList[MAX_FILENUM][MAX_FILENAME];	// String of file names in dir
	int fileNum = getFilesInDir(fileList);	// Number of files in dir
	int establishedConnectionFD, dataSocketFD;
	char clientCommand[256], dataPort[PORT_LEN], hostName[HOSTMAME_LEN];

	// Prepare address
	struct sockaddr_in serverAddress = getServerAddress(atoi(argv[1]));

	// Create the socket
	int socketFD = createSocket();

	// Bind socket to port
	bindSocket(serverAddress, socketFD);

	// Listen for incoming connections
    listen(socketFD, 5);	

	while (1) {
		// Clear out arrays
		memset(hostName, '\0', sizeof(hostName));
		memset(clientCommand, '\0', sizeof(clientCommand));
		memset(dataPort, '\0', sizeof(dataPort));

		printf("Waiting for a connection...\n");	

		// Accept the connection
		establishedConnectionFD = establishConnection(socketFD);

		// Receive the client command arguments
		getClientCommand(establishedConnectionFD, hostName, clientCommand, dataPort);

		// Establish the data connection
		dataSocketFD = createDataConnection(dataPort, hostName);
		
		// Perform client request	
		performCommand(clientCommand, dataSocketFD, fileNum, fileList);

		// Close sockets
		close(dataSocketFD);
		close(establishedConnectionFD);
	}
	close(socketFD);	
	
	return 0;
}

