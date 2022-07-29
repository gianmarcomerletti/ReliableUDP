#include <sys/types.h>
#include <sys/time.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <time.h>

#define SERV_PORT 	5555
#define BUF_DIM		1024
#define WINSIZE 	5
#define ALPHA		0.9
#define BETA		2
#define MAX_TRIES	10

/* struttura per DATA PACKET */
struct DATA_PKT {
	int type;
	int seq_no;
	int length;
	char data[BUF_DIM];
};

/* struttura per ACK PACKET */
struct ACK_PKT {
	int type;
	int ack_no;
};

/* funzioni ausiliare */
struct timeval calculateSRTT(struct timeval SRTT_old, struct timeval RTT, float alpha);
struct timeval calculateRTO(struct timeval SRTT, float beta);
int is_lost(float loss_rate);

/* funzione per la creazione di pacchetti */
struct DATA_PKT createDataPacket (int seq_no, int length, char* data);
struct DATA_PKT createTerminalPacket (int seq_no, int length);
struct ACK_PKT createACKPacket (int ack_type, int base);
