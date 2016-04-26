#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>

pthread_mutex_t lock;
char tem_str[30];
double tempature = 0.0;
int PORT_NUMBER;
double tempatureList[3600]; 
double temSum = 0.0;
int count = 0;
double average = 0.0;
double low = 200.0;
double high = -100.0;
int infoIgnore = 1;
int requestType = 0;   /* This is the request sign received from Pebble watch */
int prevReq = 5;    /* The original degree is Celsius. */
int standby = 0;    /* 0 for not standby, 1 for standby mode. */
int chooseCF = 0;   /* 0 means C, 1 means F */
int disconnectFlag = 0;   /*  0 means disconnected, 1 means connected. */
int end_flag = 1;     /* The end sign of the whole program */
int motionValue = 0;  /* the motion value received from motion sensor, 0 means nothing detected , 1 means motion detected */
char* outdoor_str;   /* the value for outdoor temperature */

double toFahrenheit(double tem) {
   return tem * 1.8 + 32;
}

double toCelsius(double tem) {
   return ( tem - 32 ) / 1.8;
}

void* fun1(void* p){
	/*
		1. Open the file
	*/
	int fd = open("/dev/cu.usbmodem1411", O_RDWR);

   pthread_mutex_lock(&lock);
	if (fd == -1)  disconnectFlag = 0;  //oops, the arduino is disconnected.
   else disconnectFlag = 1;    // connected 
   pthread_mutex_unlock(&lock);
	/*
		2. Configure fd for USB
	*/
	struct termios options; // struct to hold options
	tcgetattr(fd, &options); // associate with this fd
	cfsetispeed(&options, 9600); // set input baud rate
	cfsetospeed(&options, 9600); // set output baud rate
	tcsetattr(fd, TCSANOW, &options); // set options 

	/*
		3. Read & Print
	*/
	char buf[100];
	int bytes_read = read(fd, buf, 100);
	buf[bytes_read] = '\0';
	char tem[100];
	strcpy(tem, buf);
	//bytes_read = read(fd, buf, 100);

   pthread_mutex_lock(&lock);

   int end = end_flag;

   pthread_mutex_unlock(&lock);  
	
   int readInLast1 = 0;   /* These two variables used to judge the disconnection of Arduino Board. */
   int readInLast2 = 0;

   while(end){
		if(tem[strlen(tem) - 1] == '\n') {

         readInLast2 = 0;
         readInLast1++;
         if(readInLast1 >= 50) {
            pthread_mutex_lock(&lock);
            disconnectFlag = 0;        /* Disconnection detected. */
            pthread_mutex_unlock(&lock);
         }

		  char *token = " ";
		  char *pp;
      char *motionPP;
		  //printf("%s\n", tem);
		  if(tem[0] != '\n'){

		    motionPP = strtok(tem, token);
//            printf("%s\n", motionPP);
		    strtok(NULL, token);
		    strtok(NULL, token);
        strtok(NULL, token);
		    pp = strtok(NULL, token);

          if ( infoIgnore == 1 ) {
            infoIgnore = 0;
            continue; // the first data should be deserted.
          }  

		    pthread_mutex_lock(&lock);

        motionValue = atoi(motionPP);
		    strcpy(tem_str, pp);
		    tempature = atof(pp);
          if(count < 3600){
            tempatureList[count] = tempature;
            count++;
            temSum += tempature;
            if( tempature < low ) low = tempature;
            if( tempature > high ) high = tempature;
            average = temSum / (double)count;

          } else {
            double temDeQueue = tempatureList[count % 3600];
            tempatureList[count % 3600] = tempature;
            count++;
            temSum = temSum - temDeQueue + tempature;
            average = temSum / 3600.0;
            if ( temDeQueue == low || temDeQueue == high ){
               low = tempature ;
               high = tempature;
               for ( int i = 0; i < 3600 ; i++ ){
                  if ( tempatureList[i] < low ) low = tempatureList[i];
                  if ( tempatureList[i] > high ) high = tempatureList[i];  
               }
            }

          }
		    //printf("The most recent tempature is: %s\n", tem_str);
          //printf("The average tempature is: %.4f\n", average);
          //printf("The lowest tempature is: %.4f\n", low);
          //printf("The highest tempature is: %.4f\n", high);
		    pthread_mutex_unlock(&lock);

	  //	    printf("%.4f\n", tempature);
		  }
		 
		  bytes_read = read(fd, buf, 100);
		  buf[bytes_read] = '\0';
        //printf("New Arduino data received.\n");
		  strcpy(tem, buf);
		}
		else{
         readInLast1 = 0;
         readInLast2++;
         if(readInLast2 >= 50) {
            pthread_mutex_lock(&lock);
            disconnectFlag = 0;        /* Disconnection detected. */
            pthread_mutex_unlock(&lock);
         }

		  bytes_read = read(fd, buf, 100);
		  buf[bytes_read] = '\0';
        //printf("Arduino data received.\n");
		  strcat(tem, buf);
		}

      pthread_mutex_lock(&lock);
      int req = requestType;
      int previousRequest = prevReq;
      int standbyLocal = standby;
      pthread_mutex_unlock(&lock);


      if( ( req == 5 || req == 6 )&& !standbyLocal){      /* Send request to Arduino board */  /* 5: C   6: F */
         char request[10];
         if(req == 5) request[0] = '0';
         if(req == 6) request[0] = '1';

         pthread_mutex_lock(&lock);
         prevReq = req;
         pthread_mutex_unlock(&lock);

         request[1] = '\0';

         write(fd, request, strlen(request));   
      }

      if( req == 7 ){
         char request[10];
         request[0] = '2';
         request[1] = '\0';
         write(fd, request, strlen(request));  
      }

      if( req == 8 ){
         char request[10];
         if(previousRequest == 5) request[0] = '0';
         if(previousRequest == 6) request[0] = '1';
         request[1] = '\0';
         write(fd, request, strlen(request));  
      }

      if( req == 9 ){
         char request[10];
         request[0] = '#';
         request[1] = '\0';

         pthread_mutex_lock(&lock);
         strcat(request, outdoor_str);
         pthread_mutex_unlock(&lock);

         request[6] = '*';
         request[7] = '3';
         request[8] = '\0';
         write(fd, request, strlen(request));  
      }
      
      pthread_mutex_lock(&lock);
      requestType = 0;
      if( req == 7 ) standby = 1;
      if( req == 8 ) standby = 0;
      end = end_flag;
      pthread_mutex_unlock(&lock);
	}

	/*
		4. Last step
	*/
	close(fd);
}


int start_server(int PORT_NUMBER)
{

      // structs to represent the server and client
      struct sockaddr_in server_addr,client_addr;    
      
      int sock; // socket descriptor

      // 1. socket: creates a socket descriptor that you later use to make other system calls
      if ((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
			perror("Socket");
			exit(1);
      }
      int temp;
      if (setsockopt(sock,SOL_SOCKET,SO_REUSEADDR,&temp,sizeof(int)) == -1) {
			perror("Setsockopt");
			exit(1);
      }

      // configure the server
      server_addr.sin_port = htons(PORT_NUMBER); // specify port number
      server_addr.sin_family = AF_INET;         
      server_addr.sin_addr.s_addr = INADDR_ANY; 
      bzero(&(server_addr.sin_zero),8); 
      
      // 2. bind: use the socket and associate it with the port number
      if (bind(sock, (struct sockaddr *)&server_addr, sizeof(struct sockaddr)) == -1) {
			perror("Unable to bind");
			exit(1);
      }

      // 3. listen: indicates that we want to listn to the port to which we bound; second arg is number of allowed connections
      if (listen(sock, 5) == -1) {
			perror("Listen");
			exit(1);
      }
          
      // once you get here, the server is set up and about to start listening
      printf("\nServer configured to listen on port %d\n", PORT_NUMBER);
      fflush(stdout);
     
      char prevSendBack[20];
      strcpy ( prevSendBack, " ");


      pthread_mutex_lock(&lock);

      int end = end_flag;

      pthread_mutex_unlock(&lock);  
      
   while(end){
     // 4. accept: wait here until we get a connection on that port
      int sin_size = sizeof(struct sockaddr_in);
      int fd = accept(sock, (struct sockaddr *)&client_addr,(socklen_t *)&sin_size);
      printf("Server got a connection from (%s, %d)\n", inet_ntoa(client_addr.sin_addr),ntohs(client_addr.sin_port));
      
      // buffer to read data into
      char request[1024];
      
      // 5. recv: read incoming message into buffer
      int bytes_received = recv(fd,request,1024,0);
      // null-terminate the string
      request[bytes_received] = '\0';
      printf("Here comes the message:\n");
      printf("%s\n", request);

      char *token = " ";
      char *pp;

      strtok(request, token);
      pp = strtok(NULL, token);     /* The exact request received from Pebble Watch */

      pthread_mutex_lock(&lock);

      if( strcmp(pp, "/CurTemp") == 0)  requestType = 1;
      if( strcmp(pp, "/AvgTemp") == 0)  requestType = 2;
      if( strcmp(pp, "/MaxTemp") == 0)  requestType = 3;
      if( strcmp(pp, "/MinTemp") == 0)  requestType = 4;
      if( strcmp(pp, "/celsius") == 0)  requestType = 5;
      if( strcmp(pp, "/fahrenheit") == 0)  requestType = 6;
      if( strcmp(pp, "/off") == 0)  requestType = 7;
      if( strcmp(pp, "/on") == 0)  requestType = 8;
      if( strlen(pp) == 14 ) {    // the request for displaying outdoor temperature on Arduino.
          requestType = 9;
          outdoor_str = pp + 9;
      }
      if( strcmp(pp, "/motion") == 0 ) {    // the request for displaying the result of motion sensor.
          requestType = 10;
      }

      pthread_mutex_unlock(&lock);

      // this is the message that we'll send back
      /* it actually looks like this:
        {
           "name": "cit595"
        }
      */
      char reply[128];
      reply[0] = '\0';
      char * str = "{\n\"name\": \"";
      strcat(reply, str);

      //strcat(reply, "The tempature now is\n");
      // Store the  Most Recent Temp into reply string
      
      pthread_mutex_lock(&lock);    
      
      if(disconnectFlag){
      
         char sendBackTem[20];
      	if ( requestType == 1 ) {
            if (standby == 1) strcat(reply, "Stand-by Mode");
            else {
               if ( chooseCF == 1 ) {       
                  snprintf(sendBackTem , 20, "%f" , toFahrenheit(tempature));
                  strcat(reply, sendBackTem);
                  strcat(reply, " F");
               }
               else {
                  snprintf(sendBackTem , 20, "%f" , tempature);
                  strcat(reply, sendBackTem);
                  strcat(reply, " C");
               }
            }
         } 

         if ( requestType == 2 ) {
            if (standby == 1) strcat(reply, "Stand-by Mode");
            else {
               if ( chooseCF == 1 ) {       
                  snprintf(sendBackTem , 20, "%f" , toFahrenheit(average));
                  strcat(reply, sendBackTem);
                  strcat(reply, " F");
               }
               else {
                  snprintf(sendBackTem , 20, "%f" , average);
                  strcat(reply, sendBackTem);
                  strcat(reply, " C");
               }
            }
         }
         if ( requestType == 3 ) {
            if (standby == 1) strcat(reply, "Stand-by Mode");
            else {
               if ( chooseCF == 1 ) {       
                  snprintf(sendBackTem , 20, "%f" , toFahrenheit(high));
                  strcat(reply, sendBackTem);
                  strcat(reply, " F");
               }
               else {
                  snprintf(sendBackTem , 20, "%f" , high);
                  strcat(reply, sendBackTem);
                  strcat(reply, " C");
               }
            }
         }
         if ( requestType == 4 ) {
            if (standby == 1) strcat(reply, "Stand-by Mode");
            else {
               if ( chooseCF ==  1 ) {       
                  snprintf(sendBackTem , 20, "%f" , toFahrenheit(low));
                  strcat(reply, sendBackTem);
                  strcat(reply, " F");
               }
               else {
                  snprintf(sendBackTem , 20, "%f" , low);
                  strcat(reply, sendBackTem);
                  strcat(reply, " C");
               }
            }
         }
         if ( requestType == 5 ) {
            if (standby == 1) strcat(reply, "Stand-by Mode");
            else {
               chooseCF = 0;
               strcat(reply, "Now in Celsius Mode");
            }
         }       
         if ( requestType == 6 ) {
            if (standby == 1) strcat(reply, "Stand-by Mode");
            else {
               chooseCF = 1;
               strcat(reply, "Now in Fahrenheit Mode");
           }
         }
         if ( requestType == 7 ) {
            standby = 1;
            strcat(reply, "Stand-by Mode");
         }
         if ( requestType == 8 ) {
            standby = 0;
            strcat(reply, "Now Active");
         }
         if ( requestType == 10 ) {
            if (standby == 1) strcat(reply, "Stand-by Mode");
            else {
              if(motionValue == 0) strcat(reply, "No motion detected.");
              else strcat(reply, "Motion Detected!!"); 
            }
         }
         /* This part used to record the previous data, which could be showed whenever chooseCF request received. */
         if(requestType == 1 || requestType == 2 || requestType == 3 || requestType == 4){
            strcpy( prevSendBack, sendBackTem );
         }
      } else {      /* disconnected with Arduino. */
         strcat(reply, "Arduino Disconnected");
      }
      pthread_mutex_unlock(&lock);
      
      strcat(reply, "\"\n}\n");
      printf("The returned mes is: %s\n", reply);  
      // printf("requestType is %d\n", requestType);
      // printf("prevReq is %d\n", prevReq);


      // 6. send: send the message over the socket
      // note that the second argument is a char*, and the third is the number of chars
      send(fd, reply, strlen(reply), 0);
      //printf("Server sent message: %s\n", reply);

      // 7. close: close the socket connection
      close(fd);
      
      pthread_mutex_lock(&lock);
      end = end_flag;
      pthread_mutex_unlock(&lock); 
  	}	

      close(sock);
      printf("Server closed connection\n");
  
      return 0;
} 

void* fun2(void* p){
	pthread_mutex_lock(&lock);
	int port = PORT_NUMBER;
	pthread_mutex_unlock(&lock);
	
	start_server(port);

}

void* fun3(void* p){
  pthread_mutex_lock(&lock);

  int end = end_flag;

  pthread_mutex_unlock(&lock);

  while(end){
    char end_char[10];
    scanf("%s", end_char);    //  no '\n' should be here ..
    char* end_cmp = "q";
    printf("the word:%s was accepted\n", end_char);
    int result = strcmp(end_cmp, end_char);
    printf("%d\n", result);
    if (strcmp(end_cmp, end_char) == 0) end = 0;
  }

  pthread_mutex_lock(&lock);
  end_flag = 0;
  printf("end_flag = %d\n", end_flag);

  pthread_mutex_unlock(&lock);      /* Why the end_char doesn't work??? */

}


int main(int argc, char *argv[]){
	// check the number of arguments
  if (argc != 2)
    {
      printf("\nUsage: server [port_number]\n");
      exit(0);
    }

   PORT_NUMBER = atoi(argv[1]);

   pthread_t t1, t2, t3;
   pthread_create(&t1, NULL, &fun1, NULL);
   pthread_create(&t2, NULL, &fun2, NULL);
   pthread_create(&t3, NULL, &fun3, NULL);
   pthread_join(t1, NULL);
   pthread_join(t2, NULL);
   pthread_join(t3, NULL);

   return 0;
}