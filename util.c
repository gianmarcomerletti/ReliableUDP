#include "util.h"

struct DATA_PKT createDataPacket (int seq_no, int length, char* data){

    struct DATA_PKT pkt;

    pkt.type = 1;
    pkt.seq_no = seq_no;
    pkt.length = length;
    memset(pkt.data, 0, sizeof(pkt.data));
    memcpy(pkt.data, data, length);

    return pkt;
}

struct DATA_PKT createTerminalPacket (int seq_no, int length){

    struct DATA_PKT pkt;

    pkt.type = 2;
    pkt.seq_no = seq_no;
    pkt.length = 0;
    memset(pkt.data, 0, sizeof(pkt.data));

    return pkt;
}

struct ACK_PKT createACKPacket(int ack_type, int base) {
    
    struct ACK_PKT ack;
    
    ack.type = ack_type;
    ack.ack_no = base;
    return ack;
}

struct timeval timermul(struct timeval t, float factor) {
    struct timeval result = {0,0};
    long usec;

    usec = (t.tv_sec*1000000)+t.tv_usec;
    usec *= factor;

    while (usec > 1000000) {
        result.tv_sec++;
        usec -= 1000000;
    }
    result.tv_usec = usec;
    
    return result;
}

struct timeval calculateSRTT(struct timeval SRTT_old, struct timeval RTT, float alpha) {
    struct timeval SRTT = {0,0};
    struct timeval temp;
    
    temp = timermul(RTT, 1-alpha);
    SRTT = timermul(SRTT_old, alpha);
    timeradd(&SRTT, &temp, &SRTT);
    printf("SRTT = %ld us\n", (SRTT.tv_sec)*1000000 + SRTT.tv_usec);

    return SRTT;
}

struct timeval calculateRTO(struct timeval SRTT, float beta) {
    struct timeval RTO = {0,0};

    RTO = timermul(SRTT, beta);
    printf("RTO = %ld us\n", (RTO.tv_sec)*1000000 + RTO.tv_usec);

    return RTO;
}

/* funzione per simulare perdita pacchetti */
int is_lost(float loss_rate) {
    double random_value;
    random_value = drand48();
    if (random_value < loss_rate)
        return(1);
    return(0);
}
