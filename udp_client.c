#include <arpa/inet.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <sys/types.h>
#define PORT 8001

int main() {
	const char* server_name = "localhost";//loopback
	const int server_port = PORT;

	struct sockaddr_in server_address;
	memset(&server_address, 0, sizeof(server_address));
	server_address.sin_family = AF_INET;

	// creates binary representation of server name
	// and stores it as sin_addr
	//inet_pton: convert IPv4 and IPv6 addresses from text to binary form

	inet_pton(AF_INET, server_name, &server_address.sin_addr);


	server_address.sin_port = htons(server_port);

	// open socket
	int sock;
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("could not create socket\n");
		return 1;
	}
	printf("client socket created\n");






	int quit = 0;
	char filename[100];
	//Run indefinitely
	while (!quit) {

		// data that will be sent to the server
		const char* data_to_send = "Hi Server!!!";

		// send data
		int len =
				sendto(sock, data_to_send, strlen(data_to_send), 0,
							 (struct sockaddr*)&server_address, sizeof(server_address));
		printf("message has been sent to server\n");
		// received echoed data back
		char buffer[200000];																												//CHANGEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEEE
		int recv_bytes=recvfrom(sock, buffer, len, 0, NULL, NULL);
		printf("received bytes = %d\n",recv_bytes);
		buffer[len] = '\0';
		printf("recieved: '%s'\n", buffer);

		printf("Your choices are: 736.txt, 739.txt, 1KB.txt, 2KB.txt, 4KB.txt, 8KB.txt, 8888.txt, 32KB.txt, 'quit'\n");
		printf("Enter a file you want to recieve: ");

		int run = 0;
		while (run == 0) {
			scanf("%s", filename);
			if (strncmp(filename, "736.txt", 7) == 0 || strncmp(filename, "739.txt", 7) == 0
					|| strncmp(filename, "1KB.txt", 7) == 0
					|| strncmp(filename, "2KB.txt", 7) == 0 || strncmp(filename, "4KB.txt", 7) == 0
					|| strncmp(filename, "8KB.txt", 7) == 0 || strncmp(filename, "8888.txt", 8) == 0
					|| strncmp(filename, "32KB.txt", 8) == 0 || strncmp(filename, "quit", 4) == 0 )
			{
				run = 1;
			}
			else{
				printf("Incorrect option, please try again: ");
			}
		}



		//Send File name to Server to recieve
		if (sendto(sock, filename, strlen(filename), 0, (struct sockaddr*)&server_address, sizeof(server_address)) == -1 )
	  {
	    printf("sendto failed\n");
	    return 1;
	  }

		if(strncmp(filename, "quit", 4) == 0){
			break;
		}

		//Recieve File Size and Calculate how many loops of data will arrive
		int expected_size = recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);
		buffer[expected_size] = '\0';
		expected_size = atoi(buffer);


		printf("Size of file is: %d\n", expected_size);

		int count = 0;
		if(expected_size >= 8888){
			int temp = expected_size / 8888;
			expected_size = expected_size % 8888;
			count += temp;
		}
		if (expected_size == 0) {
			/* Do Nothing */
		}
		else if (expected_size % 8 == 0 || expected_size < 8) {
			count += 1;
		}
		else{
			count += 2;
		}
		printf("Expect %d OctoLegs.\n", count * 8 );

		//Number of extra padding bits to expect
		//If extras = 8, then no padding, else that is the number of extras
		int extras = 8 - expected_size % 8;


		/* Write the content on a different file */
		const char* ack;
		int leg_order_count = 0;
		FILE* fp = fopen("new_file.txt", "w");
		for(int c = 0; c < count; c++){
			for(int k = 0; k < 8; k++){
				recv_bytes=recvfrom(sock, buffer, sizeof(buffer), 0, NULL, NULL);
				int leg_order = buffer[recv_bytes - 1];
				printf("received bytes = %d		Leg order = %d   leg_order_count = %d ",recv_bytes - 1, leg_order, leg_order_count);
				//If it's the next octa_leg needed, put it in the buffer, else discard it and await the next octa_leg
				if (leg_order == leg_order_count) {
					for (int i = 0; i < recv_bytes - 1; i++) {
						fputc(buffer[i], fp);
					}
					leg_order_count++;
					printf("Status: printed\n");


				}
				else{
					k--;
					printf("Status: ignored\n");
					//Send Ack (if leg_order < leg_order_count - 8) --- indicating that the server is sending the last octa_block
					if (leg_order <= leg_order_count - 8) {
						ack = "You're behind one octa_block";
						int len = sendto(sock, ack, strlen(ack), 0, (struct sockaddr*)&server_address, sizeof(server_address));
						printf("Sent Ack: %s \n", ack);
					}
				}

			}
			//Send Ack Stating OctoBlock Recieved, c indicates that the octoblock is recieved, send c + 1. FIXXXXXXXXXXXXXXXXXXXXXXXXXXXXXX LOGIC
			ack = "Recieved Block";
			//sleep();
			int len = sendto(sock, ack, strlen(ack), 0, (struct sockaddr*)&server_address, sizeof(server_address));
			printf("Sent Ack: %s \n", ack);


		}

		//Clean up padding
		off_t position;
		if (extras != 8) {
			printf("Cleaning up extra bits\n");
			fseeko(fp,-extras,SEEK_END);
	    position = ftello(fp);
	    ftruncate(fileno(fp), position);

		}


		fclose(fp);
		printf("Wrote the data to 'new_file.txt'\n");


		//Testing Purposes Only runs once
		quit = 1;
	}

	// close the socket
	close(sock);
	return 0;
}
