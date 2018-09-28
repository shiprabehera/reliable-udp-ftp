This program implements reliable file transfer protocol over UDP. It uses the Stop-And-Wait protocol to achieve this - i.e it waits till it receives ACK for one packet before sending the next. Both server and client are in a while loop and keep on sending packets until they receive ACK from one another.

It contains the following files:
README.txt 
client/uftp_client.c 
client/makefile 
client/foo1.jpg
client/foo2.txt
client/foo3.jpg
server/uftp_server.c 
server/makefile

Compile and Start the Server:
1. Put the server folder in appropriate location, either on your local machine or remote server.
2. Type "make clean" followed by "make server". You will see an executable called server in that folder.
3. If you are on a remote linux machine, get the IP address of the server by entering ifconfig | grep "inet addr". 
4. Start the server by entering ./server 8000
5. If you get the error "unable to bind socket", please use a different port or kill the process using that port.


Compile and Start the Client:
1. Put the client folder in appropriate location, either on your local machine or remote server.
2. Type "make clean" followed by "make client". You will see an executable called client in that folder.
3. Start the client by entering ./client <ip_address_of_server> <port_number> 

Commands:
GET:
	Usage: get [FILENAME.ext]
	Purpose: Client downloads a file from the server.
	Implementation: After sending the command to the server, the client waits to get the total file size it is requesting. It sends an ACK if it receives it successfully and NACK otherwise. Once it gets the file size, it decides how many packets it should be reading by calculating total_file_size/MAXBUFSIZE where MAXBUFSIZE is 100. It then keeps listening on the socket till it receives those n no.of packets. If the server cannot send any packet, client will send NACK and server will resend the packet again. Client also maintains the total size it has received till this point. Server encrypts the packets by simple xor and client decrypts them using the same key. It also keeps on writing the data to a file. After receiving the packets, client checks to see if there are any remaining packets left by total_file_size - packet_size. If there are still some packets left, it again starts listening until it has received the total file size. Then, it fetches the md5sum from the server and sends an ACK if it matches. If not, server will keep on transmitting till client receives the correct packets.

PUT:
	Usage: put [FILENAME.ext]
	Purpose: Client uploads a file to the server.
	Implementation: After sending the command to the server, the client sends the total file size it is sending. It recieves an ACK if the server receives it successfully and NACK otherwise. Once the server gets the file size, it decides how many packets it should be reading by calculating total_file_size/MAXBUFSIZE where MAXBUFSIZE is 100. It then keeps listening on the socket till it receives those n no.of packets. If the client cannot send any packet, server will send NACK and client will resend the packet again. Server also maintains the total size it has received till this point. Client encrypts the packets by simple xor and server decrypts them using the same key. It also keeps on writing the data to a file. After receiving the packets, server checks to see if there are any remaining packets left by total_file_size - packet_size. If there are still some packets left, it again starts listening until it has received the total file size. Then, it fetches the md5sum from the client and sends an ACK if it matches. 	

LS:
	Usage: ls 
	Purpose: Server sends a list of its files to the client.

DELETE:
	Usage: delete [FILENAME.ext]
	Purpose: Client asks the server to delete a file from its local directory. The server replies with appropriate message.

EXIT
DELETE:
	Usage: exit
	Purpose: Client asks the server to exit gracefully.	Server breaks out of its loop and closes the socket.


Few things to keep in mind:
Reliabiltity is achieved for files>5MB, but it takes a while. The program prints out a progress bar though.
There are 3 files inside the client subdirectory, which you can send to the server.
