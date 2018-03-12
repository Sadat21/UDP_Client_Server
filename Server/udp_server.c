#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdio.h>
#include <string.h>

#include <pthread.h>
#include <signal.h>

#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#define PORT 8001

#define LOSE_PACKET 1					// 0 for false, 1 for true
#define PACKETLEG_ID 5				// LOSE_PACKET must be 1, 0 <= PACKET_ID <= 7
#define LOST_ALREADY_VALUE 0 	// Each time a packet is lost, when it becomes 1, packets will not be lost
int lost_already;


int global_ack_value;
int sock;
struct sockaddr_in client_address;
int client_address_len;
char client_name[100];

int ack_recieved = 0;
int timeout_exceeded = 0;
int timeout_exceeded_count = 0;
int file_size;

void * timer_thread(){
	//printf("Point timer , ack:%d timeout:%d\n", ack_recieved, timeout_exceeded);
	sleep(3);
	timeout_exceeded = 1;
	timeout_exceeded_count++;
	pthread_exit(NULL);

	//printf("Point timer_end , ack:%d timeout:%d\n", ack_recieved, timeout_exceeded);
}

void * ack_thread(){
	//printf("Point ack , ack:%d timeout:%d\n", ack_recieved, timeout_exceeded);
	char ack_buffer[100];
	int len = recvfrom(sock, ack_buffer, sizeof(ack_buffer), 0,
										 (struct sockaddr *)&client_address,
										 &client_address_len);

	if (len > 0) {
		ack_recieved = 1;
		timeout_exceeded_count = 0;
	}
	//printf("Point ack_end , ack:%d timeout:%d\n", ack_recieved, timeout_exceeded);
	pthread_exit(NULL);
}

int wait_for_ack(int correct_ack){
	//Create 2 threads and run
	pthread_t threads[2];
	pthread_create(&threads[0], NULL, ack_thread, NULL);
	pthread_create(&threads[1], NULL, timer_thread, NULL);

	//printf("Point 1 , ack:%d timeout:%d\n", ack_recieved, timeout_exceeded);


	//Listen for input
	while (true) {
		if (ack_recieved == 1) {
			//Kill both threads
			pthread_cancel(threads[1]);
			ack_recieved = 0;
			timeout_exceeded = 0;
			return 1;
		}
		if (timeout_exceeded == 1) {
			//Kill both threads
			pthread_cancel(threads[0]);
			ack_recieved = 0;
			timeout_exceeded = 0;
			//If the client timed out for a long time, just end the transfer
			if (timeout_exceeded_count >= 4){
				file_size = 0;
			}
			return 0;
		}
	}
}

// int wait_for_ack(int correct_ack){
// 	int fd[2];
// 	pipe(fd);
// 	int pid = fork();
// 	if (pid == 0) {
// 		/* Child 1 Process AKA ack receiver*/
// 		int c1 = 0;
// 		close(fd[0]);
// 		//write(fd[1], &c1, sizeof(int));
//
//
// 		char ack_buffer[100];
// 		int len = recvfrom(sock, ack_buffer, sizeof(ack_buffer), 0,
// 		                   (struct sockaddr *)&client_address,
// 		                   &client_address_len);
// 		if (len > 0) {
// 			printf("Function: wait_for_ack() found the ack\n");
// 			c1 = 100;
// 			write(fd[1], &c1, sizeof(int));
// 			close(fd[1]);
// 			printf("Function: wait_for_ack() Sent the ack to parent\n");
//
// 		}
// 		exit(1);
//
// 	} else if (pid < 0) {
// 		printf("Error.\n");
// 		exit(-1);
// 	} else {
// 		/* Parent Process */
// 		int fd2[2];
// 		pipe(fd2);
// 		int pid2 = fork();
// 		if (pid2 == 0) {
// 			/* Child 2 Process AKA Timer*/
// 			int c2 = 100;
// 			close(fd2[0]);
// 			sleep(5);
// 			write(fd2[1], &c2, sizeof(int));
// 			close(fd2[1]);
// 			exit(1);
// 		} else if (pid2 > 0) {
// 			/* Parent Process */
// 			int c1 = 0;
// 			int c2 = 0;
//
//
// 			while(true){
// 				//Read if Ack is Recieved (true)
// 				//printf("Got Here0\n");
//
// 				printf("Got Here1\n");
// 				read(fd[0], &c1, sizeof(int));
// 				printf("Got Here2\n");
// 				//printf("Got Here3\n");
//
// 				if (c1 != 0) {
// 					return 1;
// 				}
//
// 				//Read if timer times out
// 				/*
// 				printf("Came to the timer section\n");
// 				close(fd2[1]);
// 				read(fd2[0], &c2, sizeof(int));
//
// 				if (c2 != 0) {
// 					return 0;
// 					//Kill Process 1 -------------------------------------------------------------------------------------------
// 				}
// 				*/
// 			}
//
// 		}
// 	}
//
// }


int main(int argc, char *argv[]) {
	// port to start the server on
	int SERVER_PORT = PORT;

	// socket address used for the server
	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;

	// htons: host to network short: transforms a value in host byte
	// ordering format to a short value in network byte ordering format
	server_address.sin_port = htons(SERVER_PORT);

	// htons: host to network long: same as htons but to long
	server_address.sin_addr.s_addr = htonl(INADDR_ANY);

	// create a UDP socket, creation returns -1 on failure

	if ((sock = socket(PF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("could not create socket\n");
		return 1;
	}
	printf("server socket created\n");
	// bind it to listen to the incoming connections on the created server
	// address, will return -1 on error
	if ((bind(sock, (struct sockaddr *)&server_address,
	          sizeof(server_address))) < 0) {
		printf("could not bind socket\n");
		return 1;
	}
	printf("binding was successful\n");

	// socket address used to store client address
	client_address_len = sizeof(client_address);


	// run indefinitely
	while (true) {


		char buffer[9000];
		printf("Waiting for New Client\n");
		timeout_exceeded_count = 0;

		// read content into buffer from an incoming client
		int len = recvfrom(sock, buffer, sizeof(buffer), 0,
		                   (struct sockaddr *)&client_address,
		                   &client_address_len);


		// inet_ntoa prints user friendly representation of the
		// ip address
		buffer[len] = '\0';
		printf("received: '%s' from client %s on port55 %d\n", buffer,
		       inet_ntoa(client_address.sin_addr),ntohs(client_address.sin_port));

		// send same content back to the client ("echo")
		int sent_len = sendto(sock, buffer, len, 0, (struct sockaddr *)&client_address,
		      client_address_len);
		printf("server sent back message:%d\n",sent_len);




		//Read File Name
		char filename[500];
		len = recvfrom(sock, filename, sizeof(filename), 0,
		                   (struct sockaddr *)&client_address,
		                   &client_address_len);
		filename[len] = '\0';

		if(strncmp(filename, "quit", 4) == 0){
			printf("Server is shutting down\n");
			break;
		}
		printf("Filename is: %s\n", filename);

		/* Read from a txt file */
		FILE *fp;
		char file_buffer[2000000];
		char c;
		fp = fopen(filename, "r");

		int index = 0;
		while (fscanf(fp, "%c", &c) != EOF) {
			file_buffer[index] = c;
			index++;
		}
		file_buffer[index] = '\0';
		file_size = index;
		fclose(fp);



		//Let Client know full size of file
		int length = snprintf( NULL, 0, "%d", file_size );
		char* str = malloc( length + 1 );
		snprintf( str, length + 1, "%d", file_size );
		printf("Size of File: NEEWW : %s\n", str);


		sent_len = sendto(sock, str, strlen(str), 0, (struct sockaddr *)&client_address,
					client_address_len);
		free(str);



		//Calculate OctoLegs / OctoBlocks
		int octa_leg = 0;
		int head = 0;
		int start = 0, end = 0;
		int old_head, old_start, old_end, old_ack_value;
		int ack_value = 0;
		int lost_already = LOST_ALREADY_VALUE;
		char ack[100];


		while (file_size != 0) {
			old_start = start;
			old_head = head;
			old_end = end;
			old_ack_value = ack_value;
			if (file_size >= 8888) {
				//Send an OctoBlock
				octa_leg = 1111;
				//Send All 8 octa_leg
				for (int i = 0; i < 8; i++) {
					start = head;
					end = start + octa_leg;

					//Simulate Packet being lost
#if LOSE_PACKET
					if (i == PACKETLEG_ID && lost_already < 1) {
						ack_value++;
						i++;
						head = end;
						start = head;
						end = start + octa_leg;
						lost_already++;
					}
#endif


					//Send Segment
					char temp[octa_leg + 1];
					strncpy(temp, file_buffer + start, octa_leg);
					temp[octa_leg] = ack_value;
					ack_value++;
					sent_len = sendto(sock, temp, sizeof(temp), 0, (struct sockaddr *)&client_address,
								client_address_len);
					printf("server sent back message:%d		octa_leg#:%d \n",sent_len - 1, ack_value - 1);

					head = end;
				}
				//Wait For Ack
				int result = wait_for_ack(ack_value);
				//If Ack comes then change file size
				if (result == 1) {
					file_size -= 8888;
					printf("Ack Recieved\n");
				}
				else{
					start =old_start;
					end = old_end;
					head = old_head;
					ack_value = old_ack_value;
					printf("Timed Out\n");
				}



			}
			else if(file_size >= 8){
				int partial_octa_block = file_size - (file_size%8);
				octa_leg = partial_octa_block / 8;
				//Send 8 octalegs
				for (int i = 0; i < 8; i++) {
					start = head;
					end = start + octa_leg;

					//Simulate Packet being lost
#if LOSE_PACKET
					if (i == PACKETLEG_ID && lost_already < 1) {
						ack_value++;
						i++;
						head = end;
						start = head;
						end = start + octa_leg;
						lost_already++;
					}
#endif

					//Send Segment
					char temp[octa_leg + 1];
					strncpy(temp, file_buffer + start, octa_leg);
					temp[octa_leg] = ack_value;
					ack_value++;
					sent_len = sendto(sock, temp, sizeof(temp), 0, (struct sockaddr *)&client_address,
								client_address_len);
					printf("server sent back message:%d		octa_leg#:%d \n",sent_len - 1, ack_value - 1);
					head = end;

				}

				//Wait For Ack
				int result = wait_for_ack(ack_value);
				//If Ack Comes Calculate new size
				if (result == 1) {
					file_size -= partial_octa_block;
					printf("Ack Recieved\n");
				}
				else{
					start =old_start;
					end = old_end;
					head = old_head;
					ack_value = old_ack_value;
					printf("Timed Out\n");
				}

			}
			else if(file_size > 0){
				//Send teh remaining bytes and pad the remainder to make 8
				octa_leg = 1;
				for (int i = 0; i < 8; i++) {
					start = head;
					end = start + octa_leg;

					//Simulate Packet being lost
#if LOSE_PACKET
					if (i == PACKETLEG_ID && lost_already < 1) {
						ack_value++;
						i++;
						head = end;
						start = head;
						end = start + octa_leg;
						lost_already++;
					}
#endif

					//Send Segment
					char temp[octa_leg + 1];
					strncpy(temp, file_buffer + start, octa_leg);
					temp[octa_leg] = ack_value;
					ack_value++;
					sent_len = sendto(sock, temp, sizeof(temp), 0, (struct sockaddr *)&client_address,
								client_address_len);
					printf("server sent back message:%d		octa_leg#:%d \n",sent_len - 1, ack_value - 1);

					head = end;

				}
				//Wait For Ack
				int result = wait_for_ack(ack_value);
				//If Ack Comes Calculate new size
				if (result == 1) {
					file_size = 0;
					printf("Ack Recieved\n");
				}
				else{
					start =old_start;
					end = old_end;
					head = old_head;
					ack_value = old_ack_value;
					printf("Timed Out\n");
				}
			}
		}
		printf("Client Finished.\n");



	}
	close(sock);
	return 0;
}
