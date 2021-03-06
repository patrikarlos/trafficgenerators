/* fpont 1/00 */
/* pont.net    */
/* tcpServer.c */

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <stdio.h>
#include <unistd.h> /* close */
#include <string.h> /* memset() */

#include <sys/time.h> /* select() */
#include <stdlib.h>
#include <iostream>

#define SUCCESS 0
#define ERROR   1

#define END_LINE 0x00
#define SERVER_PORT 1500
#define MAX_MSG 65536
#define MAX_PDU	100000

#include "udpgen.h"


struct pdudata{
  u_int32_t seq_no;
  u_int64_t send_start;
  u_int64_t send_stop;
  u_int64_t recv_start;
  u_int64_t recv_stop;
  timeval send_dept_time;
  timeval recv_arrival_time;
//  u_int32_t length;
};

struct pdudata logdat[MAX_PDU];
int pducount=0;

static inline u_int64_t realcc(void){
  u_int64_t cc;
  asm volatile("rdtsc":"=&A"(cc));
  return cc;
}


int main (int argc, char *argv[]) {
  
  int sd, newSd, cliLen, recvBytes, recvPkts;
  int segsize=MAX_MSG;
	
  u_int32_t exp_id,run_id,key_id;
  exp_id=run_id=key_id=0;
	
for(int i=0;i<MAX_PDU;i++){
    logdat[i].send_start=0;
    logdat[i].send_stop=0;
    logdat[i].recv_start=0;
    logdat[i].recv_stop=0;
  }	
	
  int myPort=SERVER_PORT;
  struct sockaddr_in cliAddr, servAddr;
  char msg[MAX_MSG];

  struct timeval PktArr,tidf;

  myPort=atoi(argv[1]);
  segsize=1500;


  /* create socket */
  sd = socket(AF_INET, SOCK_STREAM, 0);
   if(sd<0) {
    perror("cannot open socket ");
    return ERROR;
  }
  
  /* bind server port */
  servAddr.sin_family = AF_INET;
  servAddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servAddr.sin_port = htons(myPort);
  
  if(bind(sd, (struct sockaddr *) &servAddr, sizeof(servAddr))<0) {
    perror("cannot bind port ");
    return ERROR;
  }

  listen(sd,5);

  
    struct tm *file_stime;
    char file_name[20];
    file_name[19]='\0';
    gettimeofday(&tidf,NULL);
    file_stime=localtime(&tidf.tv_sec);
    strftime(file_name,20,"%Y-%m-%d %H.%M.%S",file_stime);
    //    int cond=1;
    //    struct timeval accept_timeout;
    //fd_set rset;
    
    char filename[200];
    bzero(&filename,200);
    sprintf(filename,"tcpserver.log");
      
    FILE *pFile;
    pFile=fopen(filename,"a+");
    fprintf(pFile, "\n%s **Server Starting**\n",file_name);
    fclose(pFile);
    

  
newconn:  while(sd!=0) {
    printf("%s: waiting for data on port TCP %u, will read %d segments \n",argv[0],myPort, segsize);
    cliLen = sizeof(cliAddr);
    newSd = accept(sd, (struct sockaddr *) &cliAddr, (socklen_t*)&cliLen);
    if(newSd<0) {
      perror("cannot accept connection ");
      return ERROR;
    }
    printf("Connected to : %s : %d \n", inet_ntoa(cliAddr.sin_addr) ,ntohs(cliAddr.sin_port) );

    pFile=fopen(filename,"a+");
    gettimeofday(&tidf,NULL);
    file_stime=localtime(&tidf.tv_sec);
    strftime(file_name,20,"%Y-%m-%d %H.%M.%S",file_stime);
    fprintf(pFile, "%s: ",file_name);
    fprintf(pFile, "%s %d Connected \n",inet_ntoa(cliAddr.sin_addr),ntohs(cliAddr.sin_port));
    fclose(pFile);

    recvBytes=0;
    recvPkts=0;
    /* init line */
    memset(msg,0x0,MAX_MSG);
    /* receive segments */
    
    int n,n2;
    double byteCount,pktCount;
    pktCount=0;
    byteCount=0;
    transfer_data *message; 
    //    unsigned int Acounter=0;
    int charErr=0;
    int counter=-1;
    
    unsigned int msgcounter=0;
    segsize=1500;
    while(newSd!=0) {
      n2=0;
      n=0;
      memset(msg,0x0,MAX_MSG);
      
      while(n<segsize && n2!=-1){
	//	printf("Preread, %d: newSd = %d, msg = %p, msg[n2] = %p, msg+MAX = %p, segsize=%d, segsize-n2=%d", n2,newSd,&msg, &msg[n2], &msg+MAX_MSG, segsize, segsize-n2);
	n2 = read(newSd, &msg[n2], (segsize-n2));
	if(n2<0){
	  //	  printf("n2<0, %d: newSd = %d, msg = %p, msg[n2] = %p, msg+MAX = %p, segsize=%d, segsize-n2=%d", n2,newSd,&msg, &msg[n2], &msg+MAX_MSG, segsize, segsize-n2);
	  perror("problem");
	  n2=-1;
	  n=-1;
	  close(newSd);
	  close(sd);
	  newSd=0;
	  sd=0;
	  exit(0);
	} 
	if (n2==0) {
	  perror("EoF");
	  n2=-1;
	  n=-1;
	  close(newSd);
	  newSd=0;
	  //	  goto newconn;
	  close(sd);
	  sd=0;
	  exit(0);
	  
	} 
	
	//	printf("Read %d of %d bytes\n",n2,n);
	n+=n2;	
	if(counter==-1){
	  n2=-1;
	}
      }
      
      //		rstop=realcc();
      //      printf("Processing message, its %d bytes. counter = %d\n",n,counter);
      gettimeofday(&PktArr,NULL);
      if(n<=0){
	printf("%s: cannot receive data, got %d bytes \n",argv[0], n);
	close(newSd);
	newSd=0;
	//			 cond=0;
	continue;
      } else {
	if(n<40){
	  //	  Printf("1.\n");
	  byteCount+=n;
	  pktCount++;
	  printf("[%g]: Got small packet, %g \n", pktCount,byteCount);
	} else {
	  //	  printf("2.\n");
	  /* print received message */
	  message=(transfer_data*)msg;
	  byteCount+=n;
	  pktCount++;
	  /*
	    printf("NORD:%d:%d:%d\n",message->exp_id,message->run_id,message->key_id);
	    printf("HORD:%d:%d:%d\n",ntohl(message->exp_id),ntohl(message->run_id),ntohl(message->key_id));
	  */
	  charErr=0;
	  //	printf("Payload is %d bytes.\n", n);
	  //	  for(Acounter=0;Acounter<(n-(sizeof(transfer_data)-1500));Acounter++){
	  //	if(message->junk[Acounter]!='x'){
	  //    printf("Err: %c (%d) ", (message->junk[Acounter]),Acounter);
	  //    charErr++;
	  //  } else {
	  //    //				printf("Received: %c ", (message->junk[Acounter]));
	  //   }
	  //}	
	  if(charErr>0){
	    printf("CharError is %d\n",charErr);
	  }
	  
	  if( (counter==-1)){
	    exp_id=ntohl(message->exp_id);
	    run_id=ntohl(message->run_id);
	    key_id=ntohl(message->key_id);
	  }
	  //	  printf("3.\n");
	  if( (ntohl(message->exp_id)!=exp_id) || (ntohl(message->run_id)!=run_id) || (ntohl(message->key_id)!=key_id) ){ 
	    printf("Missmatch of exp/run/key_id %lu:%lu:%lu expected %u:%u:%u .\n", (long unsigned int)ntohl(message->exp_id),(long unsigned int)ntohl(message->run_id),(long unsigned int)ntohl(message->key_id), exp_id,run_id,key_id);
	  }
	  if( (counter==-1) ){
	    msgcounter=ntohl(message->counter); /* Init the counter */
	    printf("Initial message;%lu:%lu:%lu;%u:%u:%u;(Got;expected)\n", (long unsigned int)ntohl(message->exp_id),(long unsigned int)ntohl(message->run_id),(long unsigned int)ntohl(message->key_id), exp_id,run_id,key_id);
	    segsize=n;
	    printf("initializing segsize to %d bytes.\n",segsize);
	    if(msgcounter!=0) {
	      printf("First packet did not hold 0 as it should, it contained the value %d.\n", msgcounter);
	    }
	  } else {
	    msgcounter++;
	    if(msgcounter!=ntohl(message->counter)){
	      if(ntohl(message->counter)==0) {
		/* Probably a new client. Make no fuss about it.*/
		msgcounter=ntohl(message->counter);	
	      } else {
		//	      printf("Packet missmatch, expected %d got %d, a loss of %d packets.\n",msgcounter, message->counter,message->counter-msgcounter);
		msgcounter=ntohl(message->counter);	
	      }
	    }
	  }		
	  //	  printf("5.\n");
	  counter++;
	}
      }//while(newsd)
    }
  }
  //printf("Received from %s:TCP%d <%s>\n",inet_ntoa(cliAddr.sin_addr),ntohs(cliAddr.sin_port), line);
  /* init line */
  
}

