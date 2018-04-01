# SFTP

Final Project for Computer Networks (CS_372_400_W2018)

2-connection client-server network application

### ftclient.py:

Client side implementation of SSH File Transfer Protocol. This program connects to ftserver.c through port_number, and opens a dataport for content transferring. 


##### USAGE:  python3 ftclient.py <hostname> <port_number> <command> [filename] <dataport>
command:
	
	-l: Displays current directory of ftserver.c onto terminal
	-g: Saves [filename] into current directory from ftserver.c 


### ftserver.c:

Server side implementation of SSH File Transfer protocol. This program listens on port_number for any incoming connections. Once a connection is received, ftserver.c handles the requested command by either sending a list of its current directory or a file. 

#### USAGE:  ./ftserver <port_number>
