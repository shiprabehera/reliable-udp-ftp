#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <string.h>
#include <dirent.h>
#include<ctype.h>
/* You will have to modify the program below */

#define MAXBUFSIZE 4000

int send_to_client(int sock, char buffer[], int buffer_len, struct sockaddr_in remote, int remote_length) {
	int nbytes;
	nbytes=sendto(sock, buffer, buffer_len, 0, (struct sockaddr *)&remote, remote_length);
	if(nbytes < 0){
		perror("send to client function failed\n");
		return -1;
	} else{
		return nbytes;
	}
}

int recv_from_client(int sock, char buffer[], int buffer_len, struct sockaddr_in remote, unsigned int remote_length) {
	int nbytes;
	nbytes = recvfrom(sock, buffer, buffer_len, 0, (struct sockaddr *)&remote, &remote_length);
	if(nbytes < 0) {
		perror("\nreceive from client function failed\n");
		return -1;
	} else{
		return nbytes;
	}
}

void send_packets(int sock, char *buffer, int bytes_read, struct sockaddr_in remote, unsigned int remote_length) {
	while(1) {
		int nbytes = sendto(sock, buffer, bytes_read, 0,(struct sockaddr *)&remote, sizeof(remote));
	    if (nbytes < 0) {
	    	perror("failed to send packets to server\n");
	    }
	              
	    memset(buffer, 0, MAXBUFSIZE);
	    nbytes = recvfrom(sock, (char *)buffer, MAXBUFSIZE, 0, (struct sockaddr *)&remote, &remote_length);
	    if(nbytes < 0) {
	    	perror("Failed to ACK");
	    } else if(strcmp(buffer, "ACK") == 0) {
	    	//confirmed that ACK has been received and break out of this loop
	    	break;
	    }
	}
}
void calc_checksum(char *checksum, char *filename) {
	char cmd[100];
	//calculating md5 hash of the file		        
	strcpy(cmd, "md5sum ");
	strcat(cmd, filename);
	printf("%s\n", cmd);
	//this will execute md5sum filename.ext
	FILE* md5fp = popen(cmd, "r");
	if (md5fp == NULL) {
		printf("Error opening file\n");
	}  
	//reads from md5fp stream and stores in checkum
	if (fgets (checksum, 100, md5fp) != NULL ) {
		puts(checksum);
	}
	//get rid of the filename at the end
	strtok(checksum, " ");
	//printf("md5sum of the file is %s\n", checksum);
	//fclose(md5fp);
	//return checksum;
}

//encypting the message with key -- TODO: use a better function
void xor_encrypt(char *message, int len) {
	unsigned char key = 0XFF;
    for(int i = 0; i < len; i++) {
    	message[i] ^= key;
    }
}

//decrypting the message with key
void xor_decrypt(char *message, int len){
	unsigned char key = 0XFF;
    for(int i = 0; i <len; i++)
    	message[i] ^= key;
}

/* Function to execute ls command */
void ls(char returnMessage[500]) {
	struct dirent *dp;
	int length = 0;
	DIR *currentdir;

	//Open the current directory
	currentdir = opendir(".");
	if(currentdir != NULL) {
		//readdir will return NULL when reaching end of the directory
		while((dp = readdir(currentdir)) != NULL) {
			//Append the list of directory entries to the buffer
			if ( strcmp(dp->d_name, ".")!=0 && strcmp(dp->d_name, "..")!=0 ) {
				sprintf(returnMessage + strlen(returnMessage), "%s\n", dp->d_name);
			}			
		}
		//Close the directory and clean up
		closedir(currentdir); 
	} else {
		printf("Unable to open directory.\n");
	}

}


/* function to trim the trailing space */
char * trim(char *c) {
    char * e = c + strlen(c) - 1;
    while(*c && isspace(*c)) c++;
    while(e > c && isspace(*e)) *e-- = '\0';
    return e;
}

    

int main (int argc, char * argv[] )
{

	int sock;                           //This will be our socket
	struct sockaddr_in sin, remote;     //"Internet socket address structure"
	unsigned int remote_length;         //length of the sockaddr_in structure
	int nbytes;                        //number of bytes we receive in our message
	char buffer[MAXBUFSIZE];             //a buffer to store our received message
	FILE *infp; 							// input file pointer
	FILE *outfp;							// output file pointer
	FILE *md5fp;							//md5 file pointer
  	
	char checksum[100], cmd[100];

	if (argc != 2)
	{
		printf ("USAGE:  <port>\n");
		exit(1);
	}

	/******************
	  This code populates the sockaddr_in struct with
	  the information about our socket
	 ******************/
	bzero(&sin,sizeof(sin));                    //zero the struct
	sin.sin_family = AF_INET;                   //address family
	sin.sin_port = htons(atoi(argv[1]));        //htons() sets the port # to network byte order
	sin.sin_addr.s_addr = INADDR_ANY;           //supplies the IP address of the local machine


	//Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("unable to create socket");
	}


	/******************
	 Once we've created a socket, we must bind that socket to the 
	 local address and port we've supplied in the sockaddr_in struct
	 ******************/
	if (bind(sock, (struct sockaddr *)&sin, sizeof(sin)) < 0)
	{
		printf("unable to bind socket\n");
	}

	remote_length = sizeof(remote);

	while(1) {
		//waits for an incoming message
		memset(buffer, 0, MAXBUFSIZE);
		nbytes = recvfrom(sock, (char *)buffer, MAXBUFSIZE, 0, ( struct sockaddr *) &remote, &remote_length); // add flags here
		trim(buffer);
		char *command = strtok(buffer, " ");
		char *filename;
		
		/* TODO: Make ls reliable */
		if (strcmp(command, "ls") == 0) {
			printf("Sending output of ls to client---->\n");
			char returnMessage[500];
			memset(returnMessage, 0, strlen(returnMessage));
			ls(returnMessage);
			nbytes = sendto(sock, returnMessage, strlen(returnMessage), 0, (struct sockaddr*)&remote, sizeof(remote));
			if(nbytes == -1) {
			printf("Error sending ls: %s\n", strerror(errno));
			}
			
		} else if (strcmp(command, "get") == 0) {
			int packet_size = 0, bytes_left, bytes_read, bytes_written, packets, total_file_size;
			filename = strtok (NULL, " ");
			char *file_contents = NULL;
			char buffer[MAXBUFSIZE];
			//getFile(returnMessage, filename);
			infp = fopen(filename, "rb");
			if (infp != NULL) {
				calc_checksum(checksum, filename);

				//get file size
		    	fseek(infp, 0, SEEK_END); //move to end
	        	size_t total_file_size = ftell(infp);
	        	rewind(infp); //move to beginning again

	        	memset(buffer, 0, sizeof(buffer));
	        	//need to divide into packets if the file won't fit the buffer
	        	if (total_file_size > MAXBUFSIZE) {
	          		packets = total_file_size/MAXBUFSIZE;
	        	}

		        //send the file size
		        sprintf(buffer,"%zu",total_file_size);

		        //keep on sending until it receives ACK
		        send_packets(sock, buffer, MAXBUFSIZE, remote, remote_length);
		        memset(buffer, 0, sizeof(buffer));
				
				//continue till 0 packets are left
		        printf("Sending %d packets\n", packets);
		        int orig_packets = packets;
		        int curr = 0;
		        while (packets > 0) {
	          		memset(buffer, 0, MAXBUFSIZE);
	          		bytes_read = fread(buffer, 1, MAXBUFSIZE, infp);
	          		if (bytes_read < 0) {
	          			perror("Failed to read file into packet\n");
	          		}
	          		//encrypt packet before sending
	          		xor_encrypt(buffer, bytes_read);
	          		//start sending
	          		send_packets(sock, buffer, bytes_read, remote, remote_length);

	          		packet_size += bytes_read;
	          		//shift file pointer to the location till last chunk was read
	          		fseek(infp, packet_size, SEEK_SET);
	          		int percent = packets*100/orig_packets;
	          		if (percent % 10 == 0 && percent!= curr) printf("Remaining %d percent to be sent---->\n", percent);
	          		curr = percent;
	          		packets --;          
	        	}

	        	// also if (total_file_size < MAXBUFSIZE) then it won't be divided into packets as before, and we can read the entire file in bytes_left
	        	bytes_left = total_file_size - packet_size; 
	        	bytes_read = fread(buffer, 1, MAXBUFSIZE, infp);
	        	if (bytes_read < 0) {
	          		perror("Failed to read last bytes\n");
	          	}
	        	
	        	xor_encrypt(buffer, bytes_read);
	        	printf("Sending rest of %d packets\n", bytes_left);
	        	//send rest of the packets
	        	send_packets(sock, buffer, bytes_read, remote, remote_length);

	        	fclose(infp);
	      		//send md5sum to server
	      		printf("Sending md5sum\n");
	      		nbytes = sendto(sock, checksum, MAXBUFSIZE, 0, (struct sockaddr *)&remote, sizeof(remote));
	            if (nbytes < 0) {
	          		perror("Failed to send md5\n");
	          	}
	        	memset(buffer, 0, sizeof(buffer));

	          	nbytes = recv_from_client(sock, (char *)buffer, MAXBUFSIZE, remote, remote_length);
	            if(nbytes < 0) {
	            	perror("Failed to receive the ACK about md5sum\n");
	            }
	        	if(strcmp(buffer, "ACK")== 0) {
	          		printf("\nFile sent successfully\n");
	          	} else {
	          		printf("\nFile was not sent successfully\n");
	          	}

			} else {
				perror("Error  ");
				fprintf(stderr, "Unable to open file: %s\n", filename);
			}
		} else if (strcmp(command, "put") == 0) {
			int packet_size = 0, bytes_left, bytes_read, bytes_written, packets, total_file_size;
			filename = strtok (NULL, " ");
			outfp = fopen(filename, "wb");
	  		printf("Receiving this file from client ---------> %s\n", filename);
	  		//TODO: figure out why filename is set to null later
	  		char fn[100];
	  		strcpy(fn, filename); //jugaad

		   	if(outfp != NULL) {
	        	//receive the file size  before fetching the whole file
				while(1) {
					memset(buffer, 0, sizeof(buffer));
					recv_from_client(sock, (char *)buffer, MAXBUFSIZE, remote, remote_length);
					total_file_size = atoi(buffer);
					if(total_file_size > 0) {
						printf("Total file size to be received is %d\n", total_file_size);
						send_to_client(sock, "ACK", 3, remote, remote_length);
						break;
					} else {
						printf("Failed to receive the file size and request again\n");
						send_to_client(sock, "NACK",4, remote, remote_length);
					}
				}
				//need to divide into packets if the file won't fit the buffer

				if(total_file_size > MAXBUFSIZE) {
					packets = total_file_size/MAXBUFSIZE;
				
				fseek(outfp, 0, SEEK_SET);
				printf("Trying to get %d packets from client\n", packets);
				//read packets from client
				int orig_packets = packets;
				int curr = 0;
				while(packets > 0) {
					memset(buffer, 0, MAXBUFSIZE);
					//receive the packets from the client
					while(1){
						memset(buffer, 0, MAXBUFSIZE);
						nbytes = recv_from_client(sock, (char *)buffer, MAXBUFSIZE, remote, remote_length);
						if(nbytes == MAXBUFSIZE) {
							send_to_client(sock, "ACK", 3, remote, remote_length);
							break;
						} else {
							printf("Failed to receive the packet and request again\n");
							send_to_client(sock, "NACK",4, remote, remote_length);
						}
					}
					xor_decrypt(buffer, nbytes);
					bytes_written = fwrite(buffer, 1, nbytes, outfp);
					if(bytes_written < 0) {
						printf("Write to file from buffer failed\n");
					}
					
					packet_size += nbytes;
					//shift file pointer to the location till last packet was written
					fseek(outfp, packet_size, SEEK_SET);
					int percent = packets*100/orig_packets;
					if (percent % 10 == 0 && percent!= curr) printf("Remaining %d percent to be received---->\n", percent);
					curr = percent;
					packets --;
				}}

	        	size_t sizee = ftell(outfp);
		        printf("Total bytes received till now %zu\n",sizee);

				// also if (total_file_size < MAXBUFSIZE) then it won't be divided into chunks as before, and we can read the entire file in bytes_left      	
				bytes_left = total_file_size - packet_size;
				memset(buffer, 0, MAXBUFSIZE);
				printf("Trying to get the last %d bytes\n", bytes_left);

				while(1){
					memset(buffer, 0, MAXBUFSIZE);
					nbytes = recv_from_client(sock, (char *)buffer, bytes_left, remote, remote_length);
					printf("Received the last few bytes %d\n", nbytes);
					if(nbytes == bytes_left) {
						printf("Received the last few bytes correctly %d\n", nbytes);
	                	send_to_client(sock, "ACK", 3, remote, remote_length);
	                    break;
	                } else {
	                	send_to_client(sock, "NACK",4, remote, remote_length);
	                }
				}
				xor_decrypt(buffer, bytes_left);
				bytes_written = fwrite(buffer, 1, bytes_left, outfp);
				//flush contents to disk
				fflush(outfp);
				if(bytes_written < 0) {
					perror("Writing the last few bytes failed\n");
				}
				printf("bytes written to file %d\n", bytes_written);
				size_t sizeee = ftell(outfp);
		        printf("Total bytes received till now %zu\n",sizeee);
		        fclose(outfp);
				printf("Trying to get the md5sum\n");
				//receive the md5sum
				memset(buffer, 0, MAXBUFSIZE);
				nbytes = recv_from_client(sock, (char *)buffer, 100, remote, remote_length);
	                 	
				//calculating md5sum of the file
				calc_checksum(checksum, fn);

		        printf("Computed MD5sum is %s\nExpected MD5sum is %s\n", checksum, buffer);
	            if(strcmp(checksum, buffer) == 0) {
					printf("Files are identical\n");
					send_to_client(sock, "ACK", 3, remote, remote_length);
				} else {
					send_to_client(sock, "NACK",4, remote, remote_length);
				}
					
	        } else {
					perror("Error  ");
					fprintf(stderr, "Unable to open file: %s\n", filename);
			}
			printf("File received successfully\n");

		} else if (strcmp(command, "delete") == 0) {
			filename = strtok (NULL, " ");
			fopen(filename, "wb");
			int delete_result = remove(filename);
			char msg[] = "File deleted successfully";
			if (delete_result != 0) {
				strcpy(msg, "Could not delete file, try again!");
				
			} 
			nbytes = sendto(sock, (const char *)msg, strlen(msg), 0, (struct sockaddr *)&remote, sizeof(remote));
			printf("Deleting this file%s\n", filename);
		} else if (strcmp(command, "exit") == 0) {
			char msg1[] = "Server shutting off...";
			nbytes = sendto(sock, (const char *)msg1, strlen(msg1), 0, (struct sockaddr *)&remote, sizeof(remote));
			printf("Shutting off ...\n");
			//close(sock);
			exit(1);
			//break;
		} else {
			char msg11[] = "\nThis command was not understood by server.\n";
			nbytes = sendto(sock, (const char *)msg11, strlen(msg11), 0, (struct sockaddr *)&remote, sizeof(remote));
		}
	    printf("\nWaiting for further instructions...\n");

	}

	close(sock);
}

