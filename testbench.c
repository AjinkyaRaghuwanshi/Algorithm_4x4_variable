#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <time.h> //For srand()
//
// AHIR release utilities
//
#include <pthreadUtils.h>
#include <Pipes.h>
#include <pipeHandler.h>

#ifndef AA2C
	#include "vhdlCStubs.h"
#else
	#include "aa_c_model.h"
#endif

// includes the header.
#define PACKET_LENGTH_IN_WORDS  64


typedef struct _TbConfig {

	// if 1, input port 1 will be fed by data
	//   else input port 1 will be unused.
	int input_port_1_active;

	// if random dest is set, then
	// input port 1 can send data to either
	// output port 1 or output port 2.
	int input_port_1_random_dest_flag;

	// if random_dest_flag is 0, then
	// input port 1 writes only to
	// this destination port (provided
	// it is either 1 or 2).
	int input_port_1_destination_port;

	int input_port_2_active;
	int input_port_2_random_dest_flag;
	int input_port_2_destination_port;

	int input_port_3_active;
	int input_port_3_random_dest_flag;
	int input_port_3_destination_port;

	int input_port_4_active;
	int input_port_4_random_dest_flag;
	int input_port_4_destination_port;

} TbConfig;
TbConfig tb_config;

int __err_flag__ = 0;

void input_port_core(int port_id)
{
	//uint32_t send_buffer[PACKET_LENGTH_IN_WORDS];
	// variable to track the current word in the packet
	int i;

	// Length of the packet to be kept variable
	int PACKET_LENGTH_IN_WORDS_RANDOM_INPUT;

	srand(time(0));

	// Sequence Id based on the port they come from..
	// setting the seq_id from port_id
	uint8_t seq_id = port_id;

	// Creating a buffer for the variable packet length
	uint32_t send_buffer[256];

	while(1)
	{
		// Length of packet will be taken at random from 1 to 64
		PACKET_LENGTH_IN_WORDS_RANDOM_INPUT = ((rand() & 0x3F) + 1);



		// Sending data at the receiving input
		for(i = 0; i < PACKET_LENGTH_IN_WORDS_RANDOM_INPUT; i++)
		{
			send_buffer[i] = i;
		}


		// Now, further deciding the destination port here
		int dest_port =  -1;
		if(port_id == 1)
		{	// Taking the random value between 0 to 3
			// OR the destination as the current input port
			dest_port =
				(tb_config.input_port_1_random_dest_flag ? ((rand() & 0x3)+1) :
					tb_config.input_port_1_destination_port);
		}
		else if(port_id == 2)
		{
			dest_port =
				(tb_config.input_port_2_random_dest_flag ? ((rand() & 0x3)+1) :
					tb_config.input_port_2_destination_port);
		}
		else if(port_id == 3)
		{
			dest_port =
				(tb_config.input_port_3_random_dest_flag ? ((rand() & 0x3)+1) :
					tb_config.input_port_3_destination_port);
		}
		else if(port_id == 4)
		{
			dest_port =
				(tb_config.input_port_4_random_dest_flag ? ((rand() & 0x3)+1) :
					tb_config.input_port_4_destination_port);
		}


		// Now creating input data as packet
		if((dest_port == 1) || (dest_port == 2) || (dest_port == 3) || (dest_port == 4))
		{
			// creating the packet 0 from the send_buffer
			// made by giving the destination port, packet length and the input port where the data came from.
			send_buffer[0] = (dest_port << 24) | ( PACKET_LENGTH_IN_WORDS_RANDOM_INPUT << 8) | seq_id;
			// Now, writing the input data to the model
			if(port_id == 1)
				write_uint32_n ("in_data_1", send_buffer, PACKET_LENGTH_IN_WORDS_RANDOM_INPUT);
			else if (port_id == 2)
				write_uint32_n ("in_data_2", send_buffer, PACKET_LENGTH_IN_WORDS_RANDOM_INPUT);
			else if (port_id == 3)
				write_uint32_n ("in_data_3", send_buffer, PACKET_LENGTH_IN_WORDS_RANDOM_INPUT);
			else
				write_uint32_n ("in_data_4", send_buffer, PACKET_LENGTH_IN_WORDS_RANDOM_INPUT);

		}
	}
}

// Creating the input data sender thread to continuously send data to the input
void input_port_1_sender ()
{
	input_port_core(1);
}
DEFINE_THREAD(input_port_1_sender);

void input_port_2_sender ()
{
	input_port_core(2);
}
DEFINE_THREAD(input_port_2_sender);

void input_port_3_sender ()
{
	input_port_core(3);
}
DEFINE_THREAD(input_port_3_sender);

void input_port_4_sender ()
{
	input_port_core(4);
}
DEFINE_THREAD(input_port_4_sender);

// Now creating the output dat handler to read the data and verify it
void output_port_core(int port_id)
{
	// Here creating the packet counter to catch any potential packet mismatch
	int PCOUNT = 0;
	// Now here initializing any error detector
	int err = 0;
	// now creating the current word number holder
	int j;

	// Now creating a new variable as packet to store the current packet
	uint32_t packet[256];

	// Used to take the remaining (Packet_Length -1)
	uint32_t dummy_packet[256];

	while(1)
	{
		// now, creating a variable to read the header of the packets
		uint32_t header[1];

		// Reading the first header
		if(port_id == 1)
			read_uint32_n ("out_data_1", header, 1);
		else if(port_id == 2)
			read_uint32_n ("out_data_2", header, 1);
		else if(port_id == 3)
			read_uint32_n ("out_data_3", header, 1);
		else
			read_uint32_n ("out_data_4", header, 1);

		// Now removing the length of the packet from the 0th word
		// for further use in the reading of the packet at the output
		int PACKET_LENGTH_IN_WORDS_RANDOM_OUTPUT = (((1 << 8) - 1) & ( header[0] >> 8 ));


		// Now creating a new variable as packet to store the current packet
		uint32_t packet[PACKET_LENGTH_IN_WORDS_RANDOM_OUTPUT];
		uint32_t dummy_packet[PACKET_LENGTH_IN_WORDS_RANDOM_OUTPUT - 1];

		// Now reading the incoming packets and increamentingthe current word count
		if(PACKET_LENGTH_IN_WORDS_RANDOM_OUTPUT > 1)
		{
			if(port_id == 1)
					read_uint32_n ("out_data_1", dummy_packet, PACKET_LENGTH_IN_WORDS_RANDOM_OUTPUT - 1);
				else if(port_id == 2)
					read_uint32_n ("out_data_2", dummy_packet, PACKET_LENGTH_IN_WORDS_RANDOM_OUTPUT - 1);
				else if(port_id == 3)
					read_uint32_n ("out_data_3", dummy_packet, PACKET_LENGTH_IN_WORDS_RANDOM_OUTPUT - 1);
				else
					read_uint32_n ("out_data_4", dummy_packet, PACKET_LENGTH_IN_WORDS_RANDOM_OUTPUT - 1);
		}
		PCOUNT++;

		// Now writing the packet0 array taht is the header of the incoming packet
		packet[0] = header[0];

		// Now taking the data from the current packet holder and moving to the final output packet
		// Remaining words are saved in the packet
		for( j = 0; j < PACKET_LENGTH_IN_WORDS_RANDOM_OUTPUT - 1; j++)
		{
			packet[j+1] = dummy_packet[j];
		}

		// Now finding the destination ID so as to check the correctness of the packet
		int dest = (packet[0] >> 24);

		// Now finding the input port of the recieved packet
		int input_port = (packet[0] & 0x7);


		//
		// check the destination?
		//

		// Now checking if the destination ID of the sent packet at the sender and the reciever are
		// same so as to check the legitimacy of the packet
		// if the port are not same then their is a error the is to be reported out
		// at the console
		if(dest != port_id)
		{
			fprintf(stderr,"Error: at port %d, packet number %d from input port %d,"
					" destination mismatch!\n", port_id, PCOUNT, input_port);
			err = 1;
		}
		else
		{
			// if there is no error then we print the recieved data about the packet
			fprintf(stderr,"\nRx[%d] at output port %d from input port %d. Packet Length %d\n",
					PCOUNT, port_id, input_port, PACKET_LENGTH_IN_WORDS_RANDOM_OUTPUT);
		}


		// check integrity of the packet.

		// Now to check the integrity of the packet we compare it with the packet sent at the input
		int I;
		for(I=1; I < PACKET_LENGTH_IN_WORDS_RANDOM_OUTPUT; I++)
		{
			// if there is a mismatch then
			// it is setting the error flag
			if (packet[I] != I)
			{
				fprintf(stderr,"\nError: packet[%d]=%d, expected %d.\n",
					I, packet[I], I);
				err = 1;
			}
		}

		// If the error flag is set to 1 then the loop is broken
		if(err)
		{
			__err_flag__ = 1;
			break;
		}
	}

}

// Now creating the output reciever thread to continuously recieve the data from the output of the switch
void output_port_1_receiver ()
{
	output_port_core(1);
}
DEFINE_THREAD(output_port_1_receiver);

void output_port_2_receiver ()
{
	output_port_core(2);
}
DEFINE_THREAD(output_port_2_receiver);

void output_port_3_receiver ()
{
	output_port_core(3);
}
DEFINE_THREAD(output_port_3_receiver);

void output_port_4_receiver ()
{
	output_port_core(4);
}
DEFINE_THREAD(output_port_4_receiver);


int main(int argc, char* argv[])
{

	if(argc < 3)
	{
		// Asking for which type of packet transfer
		fprintf(stderr,"Usage: %s [trace-file] [test_type] \n trace-file=null for no trace, stdout for stdout\n" "test_type = 1to1/1to2/1to3/1to4/1toall/2to1/2to2/2to3/2to4/2toall/3to1/3to2/3to3/3to4/3toall/4to1/4to2/4to3/4to4/4toall/alltoall\n",
				argv[0]);
		return(1);
	}

	// creating the file pointer
	FILE* fp = NULL;
	// creating the trace file writer
	if(strcmp(argv[1],"stdout") == 0)
	{
		fp = stdout;
	}
	else if(strcmp(argv[1], "null") != 0)
	{
		fp = fopen(argv[1],"w");
		if(fp == NULL)
		{
			fprintf(stderr,"Error: could not open trace file %s\n", argv[1]);
			return(1);
		}
	}

	// Now creating the variables to store the input recived from the user to
	// which type of mode of transfer does the user wants to use.
	int __1to1 = (strcmp(argv[2],"1to1") == 0);
	int __1to2 = (strcmp(argv[2],"1to2") == 0);
	int __1to3 = (strcmp(argv[2],"1to3") == 0);
	int __1to4 = (strcmp(argv[2],"1to4") == 0);
	int __1toall = (strcmp(argv[2],"1toall") == 0);

	int __2to1 = (strcmp(argv[2],"2to1") == 0);
	int __2to2 = (strcmp(argv[2],"2to2") == 0);
	int __2to3 = (strcmp(argv[2],"2to3") == 0);
	int __2to4 = (strcmp(argv[2],"2to4") == 0);
	int __2toall = (strcmp(argv[2],"2toall") == 0);

	int __3to1 = (strcmp(argv[2],"3to1") == 0);
	int __3to2 = (strcmp(argv[2],"3to2") == 0);
	int __3to3 = (strcmp(argv[2],"3to3") == 0);
	int __3to4 = (strcmp(argv[2],"3to4") == 0);
	int __3toall = (strcmp(argv[2],"3toall") == 0);

	int __4to1 = (strcmp(argv[2],"4to1") == 0);
	int __4to2 = (strcmp(argv[2],"4to2") == 0);
	int __4to3 = (strcmp(argv[2],"4to3") == 0);
	int __4to4 = (strcmp(argv[2],"4to4") == 0);
	int __4toall = (strcmp(argv[2],"4toall") == 0);

	int __alltoall = (strcmp(argv[2],"alltoall") == 0);

#ifdef AA2C
	init_pipe_handler();
	start_daemons (fp,0);
#endif
	// test configuration setup.
	//  both input ports active, send
	//  randomly to output ports.


	// Now taking the input from the user for the kind of data transfer is required by it.
	// If the setting the input port of the below varaible to active if the condition below is fulfilled
	tb_config.input_port_1_active = (__1to1 || __1to2  || __1to3 || __1to4 || __1toall || __alltoall);
	// Now setting the output port if the user requires it to be random
	tb_config.input_port_1_random_dest_flag = (__1toall || __alltoall);
	// Now setting the output port if the user defines it to be from the below given choices
	tb_config.input_port_1_destination_port = (__1to1 ? 1 : ( __1to2 ? 2 : (__1to3 ? 3 : (__1to4 ? 4 : -1))));

	tb_config.input_port_2_active = (__2to1 || __2to2 ||__2to3 ||__2to4|| __2toall || __alltoall);
	tb_config.input_port_2_random_dest_flag = (__2toall || __alltoall);
	tb_config.input_port_2_destination_port = (__2to1 ? 1 : ( __2to2 ? 2 : (__2to3 ? 3 : (__2to4 ? 4 : -1))));


	tb_config.input_port_3_active = (__3to1 || __3to2 ||__3to3 ||__3to4|| __3toall || __alltoall);
	tb_config.input_port_3_random_dest_flag = (__3toall || __alltoall);
	tb_config.input_port_3_destination_port = (__3to1 ? 1 : ( __3to2 ? 2 : (__3to3 ? 3 : (__3to4 ? 4 : -1))));


	tb_config.input_port_4_active = (__4to1 || __4to2 ||__4to3 ||__4to4|| __4toall || __alltoall);
	tb_config.input_port_4_random_dest_flag = (__4toall || __alltoall);
	tb_config.input_port_4_destination_port = (__4to1 ? 1 : ( __4to2 ? 2 : (__4to3 ? 3 : (__4to4 ? 4 : -1))));



	//
	// start the receivers
	//

	// Now starting the reciever threads
	PTHREAD_DECL(output_port_1_receiver);
	PTHREAD_CREATE(output_port_1_receiver);

	PTHREAD_DECL(output_port_2_receiver);
	PTHREAD_CREATE(output_port_2_receiver);

	PTHREAD_DECL(output_port_3_receiver);
	PTHREAD_CREATE(output_port_3_receiver);

	PTHREAD_DECL(output_port_4_receiver);
	PTHREAD_CREATE(output_port_4_receiver);

	// start the senders.

	// Now starting the sender threads
	PTHREAD_DECL(input_port_1_sender);
	PTHREAD_CREATE(input_port_1_sender);

	PTHREAD_DECL(input_port_2_sender);
	PTHREAD_CREATE(input_port_2_sender);

	PTHREAD_DECL(input_port_3_sender);
	PTHREAD_CREATE(input_port_3_sender);

	PTHREAD_DECL(input_port_4_sender);
	PTHREAD_CREATE(input_port_4_sender);


	// Wait on the four output threads
	PTHREAD_JOIN(output_port_1_receiver);
	PTHREAD_JOIN(output_port_2_receiver);
	PTHREAD_JOIN(output_port_3_receiver);
	PTHREAD_JOIN(output_port_4_receiver);

	// setting the error flag if any error is recieved
	if(__err_flag__)
	{
		fprintf(stderr,"\nFAILURE.. there were errors\n");
	}
	return(0);
}
