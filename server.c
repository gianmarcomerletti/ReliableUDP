#include "util.h"

int main(int argc, char *argv[])
{
	int p_sockfd, c_sockfd; 				

	struct sockaddr_in p_clnt_addr; 		
	struct sockaddr_in c_clnt_addr; 
	struct sockaddr_in temp_addr;
	
	int p_addr_len, c_addr_len;				
	int p_recv_len, c_recv_len;
	
	int pid;

	char buffer[BUF_DIM];

	/* creo il socket del padre */
	p_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (p_sockfd < 0) {
		printf("Errore in socket()\n");
		exit(-1);
	}
	printf("\nSERVER: Socket (%d) creata correttamente.\n", p_sockfd);

	/* inizializzo indirizzo server */
	memset(&p_clnt_addr, 0, sizeof(p_clnt_addr));
	p_clnt_addr.sin_family = AF_INET;
	p_clnt_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	p_clnt_addr.sin_port = htons(SERV_PORT);
	printf("SERVER: Indirizzo IP settato correttamente\n");

	/* assegno indirizzo server al socket del padre */
	if (bind(p_sockfd, (struct sockaddr *)&p_clnt_addr, sizeof(p_clnt_addr)) < 0) {
		printf("Errore in bind()\n");
		exit(-1);
	}

	printf("SERVER: Il padre ha la porta %d\n", ntohs(p_clnt_addr.sin_port));

	p_addr_len = sizeof(p_clnt_addr);

	
	while (1) {
		printf("\nSERVER: Attendo connessione...\n");
		
		/* HANDSHAKE con il client */
		memset(buffer, 0, BUF_DIM);
		p_recv_len = recvfrom(p_sockfd, NULL, 0, 0, (struct sockaddr *)&p_clnt_addr, &p_addr_len);
		if (p_recv_len < 0) {
			printf("Errore in recvfrom()\n");
			exit(-1);
		}
		printf("SERVER: Client connesso\n");

		/* creo il socket per il figlio */
		c_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
		if (c_sockfd < 0) {
			printf("Errore in socket()\n");
			exit(-1);
		}
		printf("SERVER: Socket per il figlio (%d) creata correttamente.\n", c_sockfd);

		/* inizializzo indirizzo server per il figlio */
		memset(&c_clnt_addr, 0, sizeof(c_clnt_addr));
		c_clnt_addr.sin_family = AF_INET;
		c_clnt_addr.sin_addr.s_addr = htonl(INADDR_ANY);
		c_clnt_addr.sin_port = htons(0);
			
		/* assegno indirizzo server al socket del figlio */
		if (bind(c_sockfd, (struct sockaddr *)&c_clnt_addr, sizeof(c_clnt_addr)) < 0) {
			printf("Errore in bind()\n");
			exit(-1);
		}
		int dim = sizeof(temp_addr);
		getsockname(c_sockfd, (struct sockaddr *)&temp_addr, &dim);
		printf("SERVER: Il figlio ha la porta %d\n", ntohs(temp_addr.sin_port));

		/* invio numero porta del figlio al client */
		if (sendto(p_sockfd, (unsigned short *)&temp_addr.sin_port, sizeof(temp_addr.sin_port), 0, (struct sockaddr *)&p_clnt_addr, sizeof(p_clnt_addr)) < 0) {
			printf("Errore in sendto()\n");
			exit(-1);
		}
		printf("SERVER: Numero porta inviata al client\n");

		c_addr_len = sizeof(c_clnt_addr);

		pid = fork();
		
		/* caso FIGLIO */
		if (pid == 0) 
		{
			while(1) {
				printf("\nSERVER [%d]: Attendo istruzione dal client...\n", getpid());

				char cmd[10];
				char name[100];
				
				float loss_rate;
				char T_OUT;
				long timeout_ms;
				int winsize;
				int tries = 0;

				struct timeval reset = {0, 0}; 
				struct timeval SRTT = {0, 0}; 
				struct timeval RTO, RTT;
				

				/* reset timeout */
				setsockopt(c_sockfd, SOL_SOCKET, SO_RCVTIMEO,(char*)&reset, sizeof(reset));

				/* ricevo istruzione dal client */
				memset(buffer, 0, BUF_DIM);
				c_recv_len = recvfrom(c_sockfd, buffer, BUF_DIM, 0, (struct sockaddr*)&c_clnt_addr, &c_addr_len);
				if (c_recv_len < 0) {
					printf("Errore in recvfrom()\n");
					exit(-1);
				}

				memset(cmd, 0, sizeof(cmd));
				memset(name, 0, sizeof(name));
				sscanf(buffer, "%s %s %d %f %ld %c", cmd, name, &winsize, &loss_rate, &timeout_ms, &T_OUT);
				memset(buffer, 0, BUF_DIM);
				printf("SERVER [%d]: Comando ricevuto -> %s\n", getpid(), cmd);

				/**************** comando GET ***************/
				if ( (strcmp(cmd, "get") == 0) && (name != '\0') )  {

					printf("SERVER [%d]: Comando GET per il file -> %s (loss_rate = %.2f)\n", getpid(), name, loss_rate);

					FILE *file;
					long length;

					/* ottengo percorso file completo */
					struct stat st;
					char path[1000];
					getcwd(path, sizeof(path));
					strcat(path, "/files/");
					if (stat(path, &st) < 0) {
						mkdir(path, 0700);
					}
					strcat(path, name);

					/* apro il file */
					file = fopen(path, "rb");
					if (file != NULL) {
						fseek(file, 0, SEEK_END);
						length = ftell(file);
						fseek(file, 0, SEEK_SET);
					
					} else {
						printf("ERRORE: File not found!\n");
						continue;
					}

					/* calcolo numero di pacchetti */ 
					int num_packets = length / (BUF_DIM);
					if ((length % (BUF_DIM)) != 0) {
						num_packets++;
					}
					printf("SERVER [%d]: Numero di pacchetti da inviare = %d\n", getpid(), num_packets);

					struct timeval RTT_start[num_packets];
					struct timeval RTT_end[num_packets];

					/* imposto il timer iniziale */
					RTO.tv_sec = 0;
					RTO.tv_usec = timeout_ms*1000;
					setsockopt(c_sockfd, SOL_SOCKET, SO_RCVTIMEO,(char*)&RTO, sizeof(RTO));

  				 	int base = -1;          /* controllo ACK ricevuto */
    				int pkt_number = 0;     /* controllo pacchetto inviato */
					int noFinalACK = 1;     /* controllo ricezione ACK di terminazione */

					while (noFinalACK) {

						while (pkt_number <= num_packets && (pkt_number - base) <= winsize) {
							struct DATA_PKT dataPacket;

							if (pkt_number == num_packets) {
								/* raggiunta la fine, creo il pacchetto di terminazione */
								dataPacket = createTerminalPacket(pkt_number, 0);
								printf("SERVER [%d]: Invio pacchetto di terminazione...\n", getpid());
							} else {
								/* creo il pacchetto */
								char pkt_data[BUF_DIM];
								int pkt_len;
								fseek(file, pkt_number*(BUF_DIM), SEEK_SET);
								pkt_len = fread(pkt_data, 1, BUF_DIM, file);

								dataPacket = createDataPacket(pkt_number, pkt_len, pkt_data);
								printf("SERVER [%d]: Invio pacchetto %d...\n", getpid(), pkt_number);
							}

							/* invio il pacchetto */
							if (sendto(c_sockfd, &dataPacket, sizeof(dataPacket), 0,(struct sockaddr *)&c_clnt_addr, c_addr_len) < 0) {
								printf("Errore in sendto()\n");
								exit(-1);
							}
							gettimeofday(&RTT_start[pkt_number], NULL);
							pkt_number++;
						}

        				/* ricevo gli ACK per i pacchetti inviati */
        				struct ACK_PKT ack;
        				while ((c_recv_len = recvfrom(c_sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&c_clnt_addr, &c_addr_len)) < 0) {
        					
        					/* in caso di timeout, reinvio i pacchetti fino a esaurimento della finestra */
        					pkt_number = base + 1;
        					printf("\n!!! TIMEOUT per pacchetto %d !!!\n", pkt_number);

        					if (tries >= MAX_TRIES) {
        						printf("SERVER [%d]: Connessione con il client persa.\n", getpid());
        						exit(0);
        					}

        					while (pkt_number <= num_packets && (pkt_number - base) <= winsize) {
        						struct DATA_PKT dataPacket;

								if (pkt_number == num_packets) {
									/* raggiunta la fine, creo il pacchetto di terminazione */
									dataPacket = createTerminalPacket(pkt_number, 0);
									printf("SERVER [%d]: Invio pacchetto di terminazione...\n", getpid());
								} else {
									/* creo il pacchetto */
									char pkt_data[BUF_DIM];
									int pkt_len;
									fseek(file, pkt_number*(BUF_DIM), SEEK_SET);
									pkt_len = fread(pkt_data, 1, BUF_DIM, file);

									dataPacket = createDataPacket(pkt_number, pkt_len, pkt_data);
									printf("SERVER [%d]: Invio pacchetto %d...\n", getpid(), pkt_number);
								}

								/* invio il pacchetto */
								if (sendto(c_sockfd, &dataPacket, sizeof(dataPacket), 0,(struct sockaddr *)&c_clnt_addr, c_addr_len) < 0) {
									printf("Errore in sendto()\n");
									exit(-1);
								}
								gettimeofday(&RTT_start[pkt_number], NULL);
								pkt_number++;
							}
							tries++;

        				}
        				
        				if (ack.type != 4) {
        					printf("\nSERVER [%d]: ACK ricevuto per pacchetto %d\n", getpid(), ack.ack_no);

        					gettimeofday(&RTT_end[ack.ack_no], NULL);
        					timersub(&RTT_end[ack.ack_no], &RTT_start[ack.ack_no], &RTT);
        					SRTT = calculateSRTT(SRTT, RTT, (float)ALPHA);
        					RTO = calculateRTO(SRTT, (float)BETA);
        					/* set timer */
        					if (T_OUT == 'Y') {
        						setsockopt(c_sockfd, SOL_SOCKET, SO_RCVTIMEO,(char*)&RTO, sizeof(RTO));
								printf("SERVER [%d]: Nuovo valore di timeout = %ld ms\n", getpid(), (RTO.tv_sec)*1000 + (RTO.tv_usec)/1000);
        					}
							
        					if(ack.ack_no > base){
               					base = ack.ack_no;
            				}
        				} else {
        					printf("\nSERVER [%d]: ACK di terminazione ricevuto\n", getpid());
        					noFinalACK = 0;
        				}

        				tries = 0;
					}

					fclose(file);

					/* reset timer */
					setsockopt(c_sockfd, SOL_SOCKET, SO_RCVTIMEO,(char*)&reset, sizeof(reset));
					
					printf("\n\n*************************************\n");
					printf("*********   FILE INVIATO    *********\n");
					printf("*************************************\n\n"); 

				}

				/**************** comando PUT ***************/
				else if ( (strcmp(cmd, "put") == 0) && (name != '\0') ) {

					printf("SERVER [%d]: Comando PUT per il file -> %s (loss_rate = %.2f)\n", getpid(), name, loss_rate);

					/* ottengo percorso file completo */
					struct stat st;
					char path[1000];
					getcwd(path, sizeof(path));
					strcat(path, "/files/");
					if (stat(path, &st) < 0) {
						mkdir(path, 0700);
					}
					strcat(path, name);

					/* creo il file */	
					FILE* file;
					file = fopen(path, "wb");
					if (file == NULL) {
						printf("Errore in fopen()\n");
						exit(-1);
					}
					printf("SERVER [%d]: File creato -> %s\n", getpid(), name);

    				int base = -2;
    				int pkt_number = 0;

    				while (1) {
    					struct DATA_PKT dataPacket;
    					struct ACK_PKT ack;

    					c_recv_len = recvfrom(c_sockfd, &dataPacket, sizeof(dataPacket), 0, (struct sockaddr *)&c_clnt_addr, &c_addr_len);
						if (c_recv_len < 0) {
							printf("Errore in recvfrom()\n");
							fclose(file);
							remove(path);
							exit(-1);
						}
						pkt_number = dataPacket.seq_no;

						if (!is_lost(loss_rate)) {
							if (dataPacket.seq_no == 0 && dataPacket.type == 1) {
								printf("\nSERVER [%d]: Pacchetto iniziale ricevuto: %d: ACCETTATO\n", getpid(), dataPacket.seq_no);
								fwrite(dataPacket.data, 1, dataPacket.length, file);
								base = 0;
								ack = createACKPacket(3, base);
							}
							else if (dataPacket.seq_no == base + 1) {
								printf("\nSERVER [%d]: Pacchetto ricevuto: %d: ACCETTATO\n", getpid(), dataPacket.seq_no);
								fwrite(dataPacket.data, 1, dataPacket.length, file);
								base = dataPacket.seq_no;
								ack = createACKPacket(3, base);
							}
							else if (dataPacket.type == 1 && dataPacket.seq_no != base + 1) {
								printf("\nSERVER [%d]: Pacchetto ricevuto: %d: IGNORATO\n", getpid(), dataPacket.seq_no);
								ack = createACKPacket(3, base);
							}

							if (dataPacket.type == 2 && pkt_number == base) {
								base = -1;
								ack = createACKPacket(4, base);
							}

							/* invio ACK per pacchetto ricevuto */
							if (base >= 0) {
								if (sendto(c_sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&c_clnt_addr, c_addr_len) < 0) {
									printf("Errore in sendto()\n");
									exit(-1);
								}
								printf("SERVER [%d]: ACK %d inviato\n", getpid(), base);
							}
							else if (base == -1) {
								printf("\nSERVER [%d]: Pacchetto di terminazione ricevuto\n", getpid());
								if (sendto(c_sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&c_clnt_addr, c_addr_len) < 0) {
									printf("Errore in sendto()\n");
									exit(-1);
								}
								printf("SERVER [%d]: ACK di terminazione inviato\n", getpid());
							}

							if (dataPacket.type == 2 && base == -1) {

								fclose(file);

								/* reset timer */
								setsockopt(c_sockfd, SOL_SOCKET, SO_RCVTIMEO,(char*)&reset, sizeof(reset));
				
								printf("\n\n*************************************\n");
								printf("*********   FILE RICEVUTO   *********\n");
								printf("*************************************\n\n");

								break;
									
							}
						} 
						else {
							printf("\nSERVER [%d]: Pacchetto ricevuto: %d: SCARTATO\n", getpid(), dataPacket.seq_no);
						}		
    				}
    			}

				/**************** comando LIST **************/
				else if (strcmp(cmd, "list") == 0) {
					printf("SERVER [%d]: Comando LIST\n", getpid());
					FILE* proc = popen("ls files","r");
					int c = 0;
					int i = 0;
					memset(buffer, 0, BUF_DIM);
					while((c = fgetc(proc)) != EOF && i < (BUF_DIM-1)) {
						buffer[i++] = c;
					}
					buffer[i] = '\0';
					pclose(proc);
					if (sendto(c_sockfd, buffer, BUF_DIM, 0, (struct sockaddr*)&c_clnt_addr, c_addr_len) < 0) {
						perror("ERRORE: sendto()");
						exit(1);
					}
				}

				/**************** comando EXIT **************/		
				else if (strcmp(cmd, "exit") == 0) {
					printf("SERVER [%d]: Comando EXIT\n", getpid());
					exit(0);
				}
			}
		}

		else 
		{
			/* prevengo la creazione di zombie ignorando il segnale di SIGCHLD */
			signal(SIGCHLD,SIG_IGN); 
		}
	}
}

