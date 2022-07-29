#include "util.h"

int main(int argc, char *argv[])
{
	int p_sockfd, c_sockfd;

	struct sockaddr_in p_serv_addr;
	struct sockaddr_in c_serv_addr;

	int p_addr_len, c_addr_len;
	int p_recv_len, c_recv_len;

	int pid, port;

	char buffer[BUF_DIM];

	srand48(2345);
	
	if (argc < 2) {
		printf("Usage: client <indirizzo IP del server>\n");
		exit(-1);
	}

	/* creo il socket del padre*/
	p_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (p_sockfd < 0) {
		printf("Errore in socket()\n");
		exit(-1);
	}
	printf("\nCLIENT: Socket (%d) creata correttamente.\n", p_sockfd);
	

	/* inizializzo indirizzo server (padre) */
	memset((void *)&p_serv_addr, 0, sizeof(p_serv_addr));
	p_serv_addr.sin_family = AF_INET;
	p_serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	p_serv_addr.sin_port = htons(SERV_PORT);
	printf("CLIENT: Indirizzo IP settato correttamente\n");
	printf("CLIENT: Il server ha la porta %d\n", SERV_PORT);
	p_addr_len = sizeof(p_serv_addr);

	/* HANDSHAKE con il server */
	printf("CLIENT: Connessione con il server...\n");
		
	if ((sendto(p_sockfd, NULL, 0, 0, (struct sockaddr *)&p_serv_addr, sizeof(p_serv_addr))) < 0) {
		printf("Errore in sendto()\n");
		exit(-1);
	}

	/* leggo dal socket il pacchetto di risposta contenente il numero di porta */
	p_recv_len = recvfrom(p_sockfd, &port, sizeof(port), 0, (struct sockaddr *)&p_serv_addr, &p_addr_len);
	if (p_recv_len < 0) {
		printf("Errore in recvfrom()\n");
		exit(-1);
	}
	printf("CLIENT: Numero porta figlio server: %d\n", ntohs(port));

	/* creo il socket per il figlio */
	c_sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if (c_sockfd < 0) {
		printf("Errore in socket()\n");
		exit(-1);
	}
	printf("CLIENT: Socket per il figlio (%d) creata correttamente.\n", c_sockfd);

	/* inizializzo indirizzo server (figlio) */
	c_addr_len = sizeof(c_serv_addr);
	memset((void *)&c_serv_addr, 0, sizeof(c_serv_addr));
	c_serv_addr.sin_family = AF_INET;
	c_serv_addr.sin_addr.s_addr = inet_addr(argv[1]);
	c_serv_addr.sin_port = port;
	printf("CLIENT: Il numero di porta ricevuto è:  %d\n", htons(c_serv_addr.sin_port));


	while(1) {
		
		printf("\n------------------------------------------------------\n");
		printf("***** MENU ***** \n Seleziona uno dei seguenti comandi: \n 1.) get <file_name> \n 2.) put <file_name> \n 3.) list \n 4.) exit");		
		printf("\n------------------------------------------------------\n");
		printf("\nCLIENT: Attendo istruzione...\n");
		
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
		
		char temp[BUF_DIM];

		memset(buffer, 0, BUF_DIM);
		memset(temp, 0, BUF_DIM);
		memset(cmd, 0, sizeof(cmd));
		memset(name, 0, sizeof(name));
		
		scanf(" %[^\n]", temp);
		sscanf(temp, "%s %s", cmd, name);
		if ((strcmp(cmd, "get") == 0) || (strcmp(cmd, "put") == 0)) {
			printf("\nCLIENT: Inserisci dimensione della finestra:\t\t\t");
			scanf(" %d", &winsize);
			if (winsize < 1) {
				printf("ERRORE: valore non valido\n");
				continue;
			}
			printf("CLIENT: Inserisci probabilità perdita pacchetti [0.0 -> 1.0]:\t");
			scanf(" %f", &loss_rate);
			if (loss_rate < 0 || loss_rate >= 1) {
				printf("ERRORE: valore non valido\n");
				continue;
			}
			printf("CLIENT: Inserisci valore del timeout [msec]:\t\t\t");
			scanf(" %ld", &timeout_ms);
			if (timeout_ms <= 0) {
				printf("ERRORE: valore non valido\n");
				continue;
			}
			printf("CLIENT: Richiedi valore timeout adattativo: [Y/N]:\t\t");
			scanf(" %c", &T_OUT);
			printf("%c\n", T_OUT);
			if ((T_OUT != 'Y') && (T_OUT != 'N')) {
				printf("ERRORE: valore non valido\n");
				continue;
			}
		}

		sprintf(buffer, "%s %d %f %ld %c", temp, winsize, loss_rate, timeout_ms, T_OUT);
		


		pid = fork();

		/* caso FIGLIO */
		if (pid == 0) 
		{
			if ((sendto(c_sockfd, buffer, BUF_DIM, 0, (struct sockaddr *)&c_serv_addr, sizeof(c_serv_addr))) < 0) {
				printf("Errore in sendto()\n");
				exit(-1);
			}
			memset(buffer, 0, BUF_DIM);

			/**************** comando GET ***************/
			if ( (strcmp(cmd, "get") == 0) && (name != '\0') ) {

				/* creo il file */	
				FILE* file;
				file = fopen(name, "wb");
				if (file == NULL) {
					printf("Errore in fopen()\n");
					exit(-1);
				}
				printf("CLIENT [%d]: File creato -> %s\n", getpid(), name);
				int flength;

				struct timeval start, end;
				gettimeofday(&start, NULL);

    			int base = -2;
    			int pkt_number = 0;

    			while (1) {
    				struct DATA_PKT dataPacket;
    				struct ACK_PKT ack;

    				c_recv_len = recvfrom(c_sockfd, &dataPacket, sizeof(dataPacket), 0, (struct sockaddr *)&c_serv_addr, &c_addr_len);
					if (c_recv_len < 0) {
						printf("Errore in recvfrom()\n");
						fclose(file);
						remove(name);
						exit(-1);
					}
					pkt_number = dataPacket.seq_no;

					if (!is_lost(loss_rate)) {
						if (dataPacket.seq_no == 0 && dataPacket.type == 1) {
							printf("\nCLIENT [%d]: Pacchetto iniziale ricevuto: %d: ACCETTATO\n", getpid(), dataPacket.seq_no);
							flength += fwrite(dataPacket.data, 1, dataPacket.length, file);
							base = 0;
							ack = createACKPacket(3, base);
						}
						else if (dataPacket.seq_no == base + 1) {
							printf("\nCLIENT [%d]: Pacchetto ricevuto: %d: ACCETTATO\n", getpid(), dataPacket.seq_no);
							flength += fwrite(dataPacket.data, 1, dataPacket.length, file);
							base = dataPacket.seq_no;
							ack = createACKPacket(3, base);
						}
						else if (dataPacket.type == 1 && dataPacket.seq_no != base + 1) {
							printf("\nCLIENT [%d]: Pacchetto ricevuto: %d: IGNORATO\n", getpid(), dataPacket.seq_no);
							ack = createACKPacket(3, base);
						}

						if (dataPacket.type == 2 && pkt_number == base) {
							base = -1;
							ack = createACKPacket(4, base);
						}

						/* invio ACK per pacchetto ricevuto */
						if (base >= 0) {
							if (sendto(c_sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&c_serv_addr, c_addr_len) < 0) {
								printf("Errore in sendto()\n");
								exit(-1);
							}
							printf("CLIENT [%d]: ACK %d inviato\n", getpid(), base);
						}
						else if (base == -1) {
							printf("\nCLIENT [%d]: Pacchetto di terminazione ricevuto\n", getpid());
							if (sendto(c_sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&c_serv_addr, c_addr_len) < 0) {
								printf("Errore in sendto()\n");
								exit(-1);
							}
							printf("CLIENT [%d]: ACK di terminazione inviato\n", getpid());
						}

						if (dataPacket.type == 2 && base == -1) {
							gettimeofday(&end, NULL);
							fclose(file);
							printf("\007");
							struct timeval time;
							timersub(&end, &start, &time);
							double througput_Kbps = 7.8125*(flength)/((time.tv_sec)*1000 + (time.tv_usec)/1000);
				
							printf("\n\n*************************************\n");
							printf("*********   FILE RICEVUTO   *********\n");
							printf("*          ---------------          *\n");
							printf("*  %d bytes in %ld ms\t    *\n", flength, (time.tv_sec)*1000 + (time.tv_usec)/1000);
							printf("*  THROUGPUT = %.2lf Kbps\t    *\n", througput_Kbps);
							printf("*                                   *\n");
							printf("*************************************\n\n");
							exit(0);	
						}
					} 
					else {
						printf("\nCLIENT [%d]: Pacchetto ricevuto: %d: SCARTATO\n", getpid(), dataPacket.seq_no);
					}
    			}
    		}

    		/**************** comando PUT ***************/
			if ( (strcmp(cmd, "put") == 0) && (name != '\0') )  {

				printf("CLIENT [%d]: Comando PUT per il file -> %s\n", getpid(), name);

				FILE *file;
				long length;

				/* apro il file */
				file = fopen(name, "rb");
				if (file != NULL) {
					fseek(file, 0, SEEK_END);
					length = ftell(file);
					fseek(file, 0, SEEK_SET);
				} else {
					printf("ERRORE: File not found!\n");
					exit(-1);
				}

				/* calcolo numero di pacchetti */ 
				int num_packets = length / (BUF_DIM);
				if ((length % (BUF_DIM)) != 0) {
					num_packets++;
				}
				printf("CLIENT [%d]: Numero di pacchetti da inviare = %d\n", getpid(), num_packets);

				struct timeval RTT_start[num_packets];
				struct timeval RTT_end[num_packets];

				/* imposto il timer iniziale */
				RTO.tv_sec = 0;
				RTO.tv_usec = timeout_ms*1000;
				setsockopt(c_sockfd, SOL_SOCKET, SO_RCVTIMEO,(char*)&RTO, sizeof(RTO));

				struct timeval start, end;
				gettimeofday(&start, NULL);

  				int base = -1;          /* controllo ACK ricevuto */
    			int pkt_number = 0;     /* controllo pacchetto inviato */
				int noFinalACK = 1;     /* controllo ricezione ACK di terminazione */

				while (noFinalACK) {

					while (pkt_number <= num_packets && (pkt_number - base) <= winsize) {
						struct DATA_PKT dataPacket;

						if (pkt_number == num_packets) {
							/* raggiunta la fine, creo il pacchetto di terminazione */
							dataPacket = createTerminalPacket(pkt_number, 0);
							printf("CLIENT [%d]: Invio pacchetto di terminazione...\n", getpid());
						} else {
							/* creo il pacchetto */
							char pkt_data[BUF_DIM];
							int pkt_len;
							fseek(file, pkt_number*(BUF_DIM), SEEK_SET);
							pkt_len = fread(pkt_data, 1, BUF_DIM, file);

							dataPacket = createDataPacket(pkt_number, pkt_len, pkt_data);
							printf("CLIENT [%d]: Invio pacchetto %d...\n", getpid(), pkt_number);
						}

						/* invio il pacchetto */
						if (sendto(c_sockfd, &dataPacket, sizeof(dataPacket), 0,(struct sockaddr *)&c_serv_addr, c_addr_len) < 0) {
							printf("Errore in sendto()\n");
							exit(-1);
						}
						gettimeofday(&RTT_start[pkt_number], NULL);
						pkt_number++;
					}

        			/* ricevo gli ACK per i pacchetti inviati */
        			struct ACK_PKT ack;
        			while ((c_recv_len = recvfrom(c_sockfd, &ack, sizeof(ack), 0, (struct sockaddr *)&c_serv_addr, &c_addr_len)) < 0) {
        					
        				/* in caso di timeout, reinvio i pacchetti fino a esaurimento della finestra */
        				pkt_number = base + 1;
        				printf("\n!!! TIMEOUT per pacchetto %d !!!\n", pkt_number);

        				if (tries >= MAX_TRIES) {
        					printf("CLIENT [%d]: Connessione con il server persa.\n", getpid());
        					kill(pid, SIGINT);
        					exit(0);
        				}

        				while (pkt_number <= num_packets && (pkt_number - base) <= winsize) {
        					struct DATA_PKT dataPacket;

							if (pkt_number == num_packets) {
								/* raggiunta la fine, creo il pacchetto di terminazione */
								dataPacket = createTerminalPacket(pkt_number, 0);
								printf("CLIENT [%d]: Invio pacchetto di terminazione...\n", getpid());
							} else {
								/* creo il pacchetto */
								char pkt_data[BUF_DIM];
								int pkt_len;
								fseek(file, pkt_number*(BUF_DIM), SEEK_SET);
								pkt_len = fread(pkt_data, 1, BUF_DIM, file);

								dataPacket = createDataPacket(pkt_number, pkt_len, pkt_data);
								printf("CLIENT [%d]: Invio pacchetto %d...\n", getpid(), pkt_number);
							}

							/* invio il pacchetto */
							if (sendto(c_sockfd, &dataPacket, sizeof(dataPacket), 0,(struct sockaddr *)&c_serv_addr, c_addr_len) < 0) {
								printf("Errore in sendto()\n");
								exit(-1);
							}
							gettimeofday(&RTT_start[pkt_number], NULL);
							pkt_number++;
						}
						tries++;
					}
        				
        			if (ack.type != 4) {
        				printf("\nCLIENT [%d]: ACK ricevuto per pacchetto %d\n", getpid(), ack.ack_no);

        				gettimeofday(&RTT_end[ack.ack_no], NULL);
        				timersub(&RTT_end[ack.ack_no], &RTT_start[ack.ack_no], &RTT);
        				SRTT = calculateSRTT(SRTT, RTT, (float)ALPHA);
        				RTO = calculateRTO(SRTT, (float)BETA);
        				/* set timer */
        				if (T_OUT == 'Y') {
							setsockopt(c_sockfd, SOL_SOCKET, SO_RCVTIMEO,(char*)&RTO, sizeof(RTO));
							printf("CLIENT [%d]: Nuovo valore di timeout = %.3lf ms\n", getpid(), RTO.tv_usec/1000.0);
						}

        				if(ack.ack_no > base){
               				base = ack.ack_no;
            			}
        			} else {
        				printf("\nCLIENT [%d]: ACK di terminazione ricevuto\n", getpid());
        				noFinalACK = 0;
        			}

        			tries = 0;
				}

				gettimeofday(&end, NULL);
				fclose(file);
				printf("\007");
				struct timeval time;
				timersub(&end, &start, &time);
				double througput_Kbps = 7.8125*(length)/((time.tv_sec)*1000 + (time.tv_usec)/1000);

				/* reset timer */
				setsockopt(c_sockfd, SOL_SOCKET, SO_RCVTIMEO,(char*)&reset, sizeof(reset));
					
				printf("\n\n*************************************\n");
				printf("*********   FILE INVIATO    *********\n");
				printf("*          ---------------          *\n");
				printf("*  %ld bytes in %ld ms\t    *\n", length, (time.tv_sec)*1000 + (time.tv_usec)/1000);
				printf("*  THROUGPUT = %.2lf Kbps\t    *\n", througput_Kbps);
				printf("*                                   *\n");
				printf("*************************************\n\n");

				exit(0); 

			}

    		/**************** comando LIST **************/
			else if (strcmp(cmd, "list") == 0) {
				printf("\n------------------------\n");
				printf("     LISTA DEI FILE     \n\n");
				char fileList[100];
				if (recvfrom(c_sockfd, fileList, sizeof(fileList), 0, (struct sockaddr *)&c_serv_addr, &c_addr_len) < 0) {
					perror("ERRORE: recvfrom()");
					exit(1);
				}
				printf("%s\n", fileList);
				printf("------------------------\n");
				exit(0);
			}

			/**************** comando EXIT **************/		
			else if (strcmp(cmd, "exit") == 0) {
				printf("CLIENT [%d]: Comando EXIT\n", getpid());
				exit(0);
			}

			printf("CLIENT [%d]: Comando non valido\n", getpid());
			
		}

		else 
		{
			/* elimino gli zombie */
			waitpid(-1, NULL,  WUNTRACED | WCONTINUED);
			if (strcmp(cmd, "exit") == 0) {
				exit(0);
			}
			
		}
	}
}


