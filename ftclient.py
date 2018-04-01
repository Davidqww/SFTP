## Name: David Chen
## 
## Client side implementation of an SSH File Transfer Protocol. This program connects to ftserver.c 
## through port_number, and opens a dataport for content transferring 
## USAGE: python3 ftclient.py <hostname> <port_number> <command> [filename] <dataport>
## command:
##	-l: Displays current directory of ftserver.c onto terminal
##	-g: Saves [filename] into current directory from ftserver.c 
## Resources: http://beej.us/guide/bgnet

import socket
import sys
import time
import os

#############################################################
## validateArgs()
## Make sure user enters correct arguments in cmd line
## servername server_port command filename data_port
## Args: none
## Return: none
#############################################################
def validateArgs():
	if len(sys.argv) < 5 or len(sys.argv) > 6:
		print("USAGE ", sys.argv[0], " servername server_port command filename data_port")
		exit(1)
	elif sys.argv[3] != "-l" and sys.argv[3] != "-g":
		print("Invalid command")
		exit(1)
	elif sys.argv[3] == "-g" and len(sys.argv) != 6:
		print("USAGE ", sys.argv[0], " servername server_port command filename data_port")
		exit(1)
	elif sys.argv[3] == "-l" and len(sys.argv) != 5:
		print("USAGE ", sys.argv[0], " servername server_port command filename data_port")
		exit(1)

#############################################################
## sendRequest()
## Send requests to server
## 
## Args: socket connection, request command
## Return: none
#############################################################
def sendRequest(sock, request):
	# Send host name
	sock.send(sys.argv[1].encode("utf-8"))
	time.sleep(0.05)
	# Send data port
	sock.send(sys.argv[len(sys.argv) - 1].encode("utf-8"))
	time.sleep(0.05)
	# Send command (-l or -g) 
	sock.send(request.encode("utf-8"))

#############################################################
## receiveDirectory()
## Retrieve file names from server directory
## 
## Args: connection socket
## Return: none
#############################################################
def receiveDirectory(connection):
	# Get the number of files 
	fileNumber = connection.recv(4).decode("utf-8")
	# Loop until all file names are received
	for x in range (0, int(fileNumber)):
		filename = connection.recv(50).decode("utf-8");
		print(filename)

####################################################################################################
## writeFile()
## Create file and write in it
## 
## Args: String containing bytes to write
## Return: none
## Resource: https://stackoverflow.com/questions/11968976/list-files-in-only-the-current-directory
####################################################################################################
def writeFile(fileToWrite):
	# Check if file name already exists
	files = [f for f in os.listdir('.') if os.path.isfile(f)]
	for f in files:
		if f == sys.argv[4]:
			print("Duplicate file name")
			return

	# Create the file
	fh = open(sys.argv[4], 'w')
	fh.write(fileToWrite)
	fh.close()
	print("Transfer complete:", sys.argv[4])

#############################################################
## receiveFile()
## Retrieve file from server
## 
## Args: connection socket
## Return: none
#############################################################
def receiveFile(connection):
	# Check if file names are valid
	status = connection.recv(10).decode("utf-8")

	# If invalid, display error and exit
	if status == "INVALID":
		print("Invalid file name:", sys.argv[4])
		return

	# Get the file size
	totalFileSize = int(connection.recv(12).decode("utf-8"))

	currentFile = ''
	# Loop until we get bytes that are equal to file size
	while len(currentFile) < totalFileSize:
		bytesReceived = connection.recv(4096).decode("utf-8")
		currentFile += bytesReceived 

	writeFile(currentFile)

def main():
	validateArgs()

	request = sys.argv[3]
	if request == "-g":
		request += " "
		request += sys.argv[4]

	# Create IPv4 TCP socket
	sock = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

	# Connect to server
	serverName = sys.argv[1]
	serverPort = int(sys.argv[2])
	sock.connect((serverName, serverPort))

	sendRequest(sock, request)	
	
	# Set-up data port
	dataPort = int(sys.argv[len(sys.argv) - 1])
	clientSocket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	clientSocket.bind((serverName, dataPort))
	clientSocket.listen(5)

	# Accept data connection
	connection, address = clientSocket.accept()

	# Determine which request to handle
	if request == "-l":
		receiveDirectory(connection) 
	else: 
		receiveFile(connection)

main()
