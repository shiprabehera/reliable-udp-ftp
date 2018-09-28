#include <stdio.h>
#include <sys/types.h> /*sys/socket.h uses sys/types.h*/
#include <sys/socket.h> /*defines constants such as SOCK DGRAM used in socket based I/O */ 
#include <netinet/in.h> /*defines structures such as sockaddr in */
#include <arpa/inet.h>
#include <netdb.h> /* defines host ip addresses */
#include <unistd.h>
#include <signal.h>
#include <stdio.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>
#include <stdlib.h>
#include <memory.h>
#include <errno.h>
#include <ctype.h>

#define MAXBUFSIZE 4000

int send_to_server(int sock, char buffer[], int buffer_len, struct sockaddr_in remote, unsigned int remote_length) {
	int nbytes;
	nbytes = sendto(sock, buffer, buffer_len, 0, (struct sockaddr *)&remote, sizeof(remote));
	if(nbytes < 0){
		perror("send to server function failed\n");
		return -1;
	} else{
		return nbytes;
	}
}

int recv_from_server(int sock, char buffer[], int buffer_len, struct sockaddr_in remote, unsigned int remote_length) {
	int nbytes;
	nbytes = recvfrom(sock, buffer, buffer_len, 0, (struct sockaddr *)&remote, &remote_length);
	if(nbytes < 0) {
		perror("Failed to receive packet from server");
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
	//printf("md5sum of the file being sent is %s\n", checksum);
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

/* function to trim the trailing space from input*/
char * trim(char *c) {
    char * e = c + strlen(c) - 1;
    while(*c && isspace(*c)) c++;
    while(e > c && isspace(*e)) *e-- = '\0';
    return e;
}

/* You will have to modify the program below */

int main(int argc , char *argv[])
{
	int nbytes;                             // number of bytes send by sendto()
	int sock;                               //this will be our socket
	char buffer[MAXBUFSIZE];

	struct sockaddr_in remote;              //"Internet socket address structure"
	FILE *infp; 							// input file pointer
	FILE *outfp;							// output file pointer
	FILE *md5fp;							//md5 file pointer

	//int packet_size = 0, bytes_left, bytes_read, bytes_written, total_file_size, packets;
	char checksum[100], cmd[100];

	if (argc < 3)
	{
		printf("USAGE:  <server_ip> <server_port>\n");
		exit(1);
	}

	/******************
	 Here we populate a sockaddr_in struct with
	 information regarding where we'd like to send our packet 
	 i.e the Server.
	 ******************/
	bzero(&remote, sizeof(remote));               //zero the struct
	remote.sin_family = AF_INET;                 //address family
	remote.sin_port = htons(atoi(argv[2]));      //sets port to network byte order
	remote.sin_addr.s_addr = inet_addr(argv[1]); //sets remote IP address; converts IP address to long format

    //Causes the system to create a generic socket of type UDP (datagram)
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
	{
		printf("unable to create socket");
	}

	/******************
	 sendto() sends immediately.  
	 it will report an error if the message fails to leave the computer
	 however, with UDP, there is no error if the message is lost in the network once it leaves the computer.
	 ******************/
	char command[] = "apple";	
	//nbytes = sendto(sock, (const char *)command, strlen(command), 0, (const struct sockaddr *) &remote, sizeof(remote)); //add flags here
    printf("Hello message sent.\n");

	// Blocks till bytes are received
	struct sockaddr_in from_addr;
	unsigned int addr_length = sizeof(struct sockaddr);
	bzero(buffer, sizeof(buffer));
	int remote_length;
	//nbytes = recvfrom(sock, (char *)buffer, MAXBUFSIZE, 0, (struct sockaddr *) &from_addr, &addr_length); // add flags here  

	

	char *message_for_user = "-------------------\nUSAGE:\nget [FILENAME.ext]\nput [FILENAME.ext]\ndelete [FILENAME.ext]\nls\nexit\nEnter command to execute on the server: ";
	printf("%s", message_for_user);
	
	char userinput[MAXBUFSIZE];
	/* read input commands from user */
	while(1) {
		memset(buffer, 0, sizeof(buffer));
		fgets (userinput, sizeof(userinput), stdin); /* read in a line */
		trim(userinput);
		nbytes = sendto(sock, (const char *)userinput, strlen(userinput), 0, (const struct sockaddr *) &remote, sizeof(remote)); 
		if (nbytes < 0) {
			perror("Unable to send instructions to the server. Please try again.\n");
			printf("%s", message_for_user);
		}
		char *command = strtok(userinput, " ");
		char *filename;
		//send command to server
		
		if (strcmp(command, "get") == 0) {
			int packet_size = 0, bytes_left, bytes_read, bytes_written, packets, total_file_size;
			filename = strtok (NULL, " ");
			outfp = fopen(filename, "wb");
  			printf("Getting this file--------->%s\n", filename);
 			//TODO: figure out why filename is set to null later
	  		char fn[100];
	  		strcpy(fn, filename); //jugaad for now

  			if(outfp != NULL) {
  				//receive the file size  before fetching the whole file
				while(1) {
					memset(buffer, 0, sizeof(buffer));
					recv_from_server(sock, (char *)buffer, MAXBUFSIZE, remote, remote_length);
					total_file_size = atoi(buffer);
					if(total_file_size > 0) {
						printf("Total file size to be received is %d\n", total_file_size);
						send_to_server(sock, "ACK", 3, remote, remote_length);
						break;
					} else {
						printf("Failed to receive the file size and request again\n");
						send_to_server(sock, "NACK",4, remote, remote_length);
					}
				}
				//need to divide into packets if the file won't fit the buffer
				if(total_file_size > MAXBUFSIZE) {
					packets = total_file_size/MAXBUFSIZE;
				}

				fseek(outfp, 0, SEEK_SET);
				printf("Trying to get %d packets from server\n", packets);
				//read packets from server
				int orig_packets = packets;
				int curr = 0;
				while(packets > 0) {
					memset(buffer, 0, MAXBUFSIZE);
					//receive the packets from the server
					while(1){
						memset(buffer, 0, MAXBUFSIZE);
						nbytes = recv_from_server(sock, (char *)buffer, MAXBUFSIZE, remote, remote_length);
						if(nbytes == MAXBUFSIZE){
							send_to_server(sock, "ACK", 3, remote, remote_length);
							break;
						} else {
							printf("Failed to receive the packet and request again\n");
							send_to_server(sock, "NACK",4, remote, remote_length);
						}
					}
					xor_decrypt(buffer, nbytes);
					bytes_written = fwrite(buffer, 1, nbytes, outfp);
					if(bytes_written < 0) {
						printf("Write to file from buffer failed\n");
					}
					
					packet_size += nbytes;
					//shift file pointer to the location till last chunk was written
					fseek(outfp, packet_size, SEEK_SET);
					int percent = packets*100/orig_packets;
					if (percent % 10 == 0 && percent!= curr) printf("Remaining %d percent to be received---->\n", percent);
					curr = percent;
					packets --;
				}
				// also if (total_file_size < MAXBUFSIZE) then it won't be divided into chunks as before, and we can read the entire file in bytes_left      	
				bytes_left = total_file_size - packet_size;
				memset(buffer, 0, MAXBUFSIZE);

				printf("Trying to get the last few bytes\n");
				while(1){
					memset(buffer, 0, MAXBUFSIZE);
					nbytes = recv_from_server(sock, (char *)buffer, bytes_left, remote, remote_length);
					if(nbytes == bytes_left) {
	                	send_to_server(sock, "ACK", 3, remote, remote_length);
	                    break;
	                } else {
	                	send_to_server(sock, "NACK",4, remote, remote_length);
	                }
				}
				xor_decrypt(buffer, bytes_left);
				bytes_written = fwrite(buffer, 1, bytes_left, outfp);
				//flush contents to disk
				fflush(outfp);
				if(bytes_written < 0) {
					perror("Writing the last few bytes failed\n");
				}
				size_t sizeee = ftell(outfp);
		        printf("Total bytes received till now %zu\n",sizeee);
				fclose(outfp);

				printf("Trying to get the md5sum\n");
				//receive the md5sum
				memset(buffer, 0, MAXBUFSIZE);
				nbytes = recv_from_server(sock, (char *)buffer, 100, remote, remote_length);
                  	
				//calculating md5sum of the file
				calc_checksum(checksum, fn);

		        printf("Computed MD5sum is %s\nExpected MD5sum is %s\n", checksum, buffer);

	            if(strcmp(checksum, buffer) == 0) {
					printf("Files are identical\n");
					send_to_server(sock, "ACK", 3, remote, remote_length);
				} else {
					send_to_server(sock, "NACK",4, remote, remote_length);
				}				

  			} else {
				perror("Error  ");
				fprintf(stderr, "Unable to open file: %s\n", filename);
			}	

			printf("File received successfully\n");
			printf("%s", message_for_user);

		} else if (strcmp(command, "put") == 0) {
			int packet_size = 0, bytes_left, bytes_read, bytes_written, packets, total_file_size;
			filename = strtok (NULL, " ");
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
		        int iter = 0;
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

	          	nbytes = recvfrom(sock, (char *)buffer, MAXBUFSIZE, 0, (struct sockaddr *)&from_addr, &addr_length);
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

  			printf("\n%s", message_for_user);

		} else if (strcmp(command, "delete") == 0) {
			nbytes = recvfrom(sock, (char *)buffer, MAXBUFSIZE, 0, (struct sockaddr *) &from_addr, &addr_length);
			printf("Server replies \n");
			printf("%s\n", buffer);
			printf("%s", message_for_user);
			
		} else if (strcmp(command, "ls") == 0) {
			nbytes = recvfrom(sock, (char *)buffer, MAXBUFSIZE, 0, (struct sockaddr *) &from_addr, &addr_length);
			printf("Server replies \n");
			printf("%s\n", buffer);
			printf("%s", message_for_user);
			
		} else if (strcmp(command, "exit") == 0) {
			nbytes = recvfrom(sock, (char *)buffer, MAXBUFSIZE, 0, (struct sockaddr *) &from_addr, &addr_length);
			printf("%s\n", buffer);
			printf("\n%s", message_for_user);
			//printf("Exiting! Bye!");
			//break;	
		} else {
			nbytes = recvfrom(sock, (char *)buffer, MAXBUFSIZE, 0, (struct sockaddr *) &from_addr, &addr_length);
			printf("%s\n", buffer);
			printf("\n%s", message_for_user);
		}
	
	}

	close(sock);
 
}