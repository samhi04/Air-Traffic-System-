#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <sys/msg.h>
#include <stdbool.h>

#define FILE_NAME "a_random_text_file.txt"
#define CHARACTER 'X'

typedef struct message_from_plane_struct{
	long msg_type;
	int plane_id;
	int plane_type;
	int arrival_airport;
	int departure_airport;
	int weight_of_plane;
	int passengers; //-1 for cargo planes
} msg_from_plane;

typedef struct message_to_plane_struct{
	long msg_type;
	char terminate;
	char ok_travel;
	//char ok_board;
	char ok_shutdown;
} msg_to_plane;
	
typedef struct message_to_airport_struct{
	long msg_type;
	int plane_id;
	//int departing_to;
	int weight_of_plane;
	char activity;	
	char arrived;
} msg_to_airport;

typedef struct message_from_airport_struct{
	long msg_type;
	int plane_id;
	char activity; //'A' for arrival, 'D' for departure
	char status;	//'G' for granted, 'R' for rejected
} msg_from_airport;


typedef struct terminate_struct{
	long msg_type;
	char terminate;
} msg_terminate;

typedef struct plane_info{
	//int plane_id;
	int departure_airport;
	int destination_airport;
} plane_info;

plane_info plane_array[10] = {0};

int main(){
	printf("Enter the number of airports to be handled/managed: ");
	int airport_count;
	scanf("%d",&airport_count);
	getchar(); //clear input buffer
	
	//set up message queue
	
	system("touch a_random_text_file.txt");
	key_t key = ftok(FILE_NAME, CHARACTER);
    	if (key == -1){
    	    printf("error in creating unique key\n");
    	    exit(1);
  	}

   	 int msgid = msgget(key, 0644|IPC_CREAT);    
   	 if (msgid == -1){
    	    printf("error in creating message queue\n");
   	     exit(1);
	 }	
	 
	 //data structure for storing details of planes
	 
	 int planes[] = {1,2,3,4,5,6,7,8,9,10};
	 int airports[] = {111,222,333,444,555,666,777,888,999,1000};
	 char terminate='N';
	 int planes_in_transit = 0; //used while terminating
	 while(terminate=='N' || planes_in_transit>0){                                                             /**/
	 	msg_terminate m_t;
	 	int r = msgrcv(msgid, &m_t, sizeof(msg_terminate)-sizeof(long), 6942069, IPC_NOWAIT);
	 	if(r>0){
	 		if(m_t.terminate=='Y'){
		 		terminate='Y';	
		 	}
	 	}

	 	for(int i=0;i<10;i++){
	 		msg_from_plane m1;
	 		int received = msgrcv(msgid, &m1, 24, planes[i], IPC_NOWAIT);
	 		if(received>0){
	 			printf("----------------------------------------------------------------------------\n");
	 			printf("Reveived %d bytes from plane of id %d\n", received, planes[i]);
	 			printf("plane_id: %d\n", m1.plane_id);
	 			printf("plane_type: %d\n", m1.plane_type);
	 			printf("arrival airport: %d\n", m1.arrival_airport);
	 			printf("departure airport: %d\n", m1.departure_airport);
	 			printf("plane weight : %d\n", m1.weight_of_plane);
	 			printf("passenger count: %d\n", m1.passengers);
	 			printf("\n");
	 			
	 			//send details to departure airport too begin loading/boarding
	 			
	 				if(m1.departure_airport>0){
	 					if(terminate!='Y'){
			 				printf("sending message to airport %d about plane %d departure\n", m1.departure_airport, m1.plane_id);
				 			msg_to_airport m2;
				 			if(m1.departure_airport==10) {
				 				m2.msg_type = 10000;
				 			}
				 			else {
				 				m2.msg_type = m1.departure_airport+m1.departure_airport*10+m1.departure_airport*100+m1.departure_airport*1000;
				 			}	
				 			//m2.departing_to = m1.arrival_airport;
				 			m2.plane_id = m1.plane_id;
				 			m2.weight_of_plane = m1.weight_of_plane;
				 			m2.activity = 'D';
				 			m2.arrived = 'X';
				 			plane_array[m1.plane_id-1].departure_airport = m1.departure_airport;
				 			plane_array[m1.plane_id-1].destination_airport = m1.arrival_airport;
				 			int sent = msgsnd(msgid, (void*)&m2, sizeof(msg_to_airport)-sizeof(long), 0);
				 			if(sent!=-1) {
				 				printf("Message sent\n");
				 				printf("----------------------------------------------------------------------------\n");
				 				sent=-1;
				 			}
				 			else {
				 				printf("Error while sending the message\n");
				 				printf("----------------------------------------------------------------------------\n");
				 			}
				 			
				 			
				 			
				 			//sending message to arrival airport about plane arrival
				 			
				 			printf("sending message to airport %d about plane %d arrival\n", m1.arrival_airport, m1.plane_id);
				 			//msg_to_airport m2;
				 			if(m1.arrival_airport==10) {
				 				m2.msg_type = 10000;
				 			}
				 			else {
				 				m2.msg_type = m1.arrival_airport+m1.arrival_airport*10+m1.arrival_airport*100+m1.arrival_airport*1000;
				 			}	
				 			//m2.departing_to = m1.arrival_airport;
				 			m2.plane_id = m1.plane_id;
				 			m2.weight_of_plane = m1.weight_of_plane;
				 			m2.activity = 'A';
				 			m2.arrived = 'N';
				 			sent = msgsnd(msgid, (void*)&m2, sizeof(msg_to_airport)-sizeof(long), 0);
				 			if(sent!=-1) {
				 				printf("Message sent\n");
				 				printf("----------------------------------------------------------------------------\n");
				 			}
				 			else {
				 				printf("Error while sending the message\n");
				 				printf("----------------------------------------------------------------------------\n");
				 			}
			 			
				 			//sending messsage to plane to start the journey
				 			
				 			//printf("sending message to plane %d \n", m1.plane_id);
				 			//msg_to_plane m3;
				 			//if(m1.plane_id==10) {
				 			//	m3.msg_type = 100;
				 			//}
				 			//else {
				 			//	m3.msg_type = m1.plane_id+m1.plane_id*10;
				 			//}
				 			//if(terminate==1) {
				 			//	m3.terminate = 'Y';
				 			//}
				 			//else {
				 			//	m3.terminate = 'N';
				 			//}
					 		//m3.ok_shutdown = 'N';
				 			//m3.ok_travel = 'Y';
				 			//sent = msgsnd(msgid, (void*)&m3, sizeof(msg_to_plane)-sizeof(long), 0);
				 			//if(sent!=-1) {
				 			//	printf("Message sent\n");
				 			//}
				 			//else {
				 			//	printf("Error while sending the message\n");
				 			//}
				 			//FILE *file = fopen("AirTrafficController.txt", "a");
		    					//if (file == NULL) {
		     				        //	perror("Error opening file");
							//	return 1;
		    					//}
		    					// Write data to the file
		   					//fprintf(file, "Plane %d has departed from Airport %d and will land at Airport %d.\n", m1.plane_id, m1.departure_airport, m1.arrival_airport);
		    					// Close the file
		    					//fclose(file);
		   					//printf("Data appended to file successfully.\n");
		   				}
		   				else{
			 				msg_to_plane mtp;
			 				mtp.terminate = 'Y';
			 				mtp.ok_travel = 'N';
			 				mtp.ok_shutdown = 'N';
			 				if(m1.plane_id !=10){
			 					mtp.msg_type = m1.plane_id + m1.plane_id*10;
			 				}
			 				else{
			 					mtp.msg_type = 100;
			 				}
			 				
			 				//send termination message to plane
			 				
			 				int sent = msgsnd(msgid, (void *)&mtp, sizeof(msg_to_plane)-sizeof(long), 0);
			 				if(sent!=0){
			 					printf("Error while sending message\n");
			 					printf("----------------------------------------------------------------------------\n");
			 				}
			 				else{
			 					printf("Message sent");
			 					printf("----------------------------------------------------------------------------\n");
			 				}
			 			}
   					}
	   				else{
	   					printf("plane %d has arrived at the airport %d\n", m1.plane_id,(m1.arrival_airport));
	   					//planes_in_transit--;
	   					//sending message to arrival airport to commence landing procedures
	   					msg_to_airport m5;
	   					m5.plane_id = m1.plane_id;
	   					m5.activity = 'A';
	   					m5.arrived = 'Y';
	   					m5.weight_of_plane = m1.weight_of_plane;
	   					if(m1.arrival_airport==10){
	   						m5.msg_type = 10000;
	   					}
	   					else{
	   						m5.msg_type = m1.arrival_airport+m1.arrival_airport*10 + m1.arrival_airport*100 + m1.arrival_airport*1000;
	   					}
	   					printf("sending message to airport %d to being landing process for plane %d\n",m1.arrival_airport, m1.plane_id);
	   					int sent = msgsnd(msgid, (void*)&m5, sizeof(msg_to_airport)-sizeof(long), 0);
			 			if(sent!=-1) {
			 				printf("Message sent\n");
			 				printf("----------------------------------------------------------------------------\n");
			 				sent = -1;
			 			}
			 			else {
			 				printf("Error while sending the message\n");
			 				printf("----------------------------------------------------------------------------\n");
			 			}
			 			
	   					//printf("Sending messsage to plane to begin shutdown procedures\n");
	   					//sending message to plane to ask it to shutdown
	   					//msg_to_plane m3;
			 			//if(m1.plane_id==10) {
			 			//	m3.msg_type = 100;
			 			//}
			 			//else {
			 			//	m3.msg_type = m1.plane_id+m1.plane_id*10;
			 			//}
			 			//if(terminate==1) {
			 			//	m3.terminate = 'Y';
			 			//}
			 			//else {
			 			//	m3.terminate = 'N';
			 			//}
				 		//m3.ok_shutdown = 'Y';
			 			//m3.ok_travel = 'N';
			 			//sent = msgsnd(msgid, (void*)&m3, 3*sizeof(char), 0);
			 			//if(sent!=-1) {
			 			//	printf("Message sent\n");
			 			//	sent=-1;
			 			//}
			 			//else {
			 			//	printf("Error while sending the message\n");
			 			//}
	   					
	   				}
			 }
			 else {
			 	//printf("No message received from plane %d\n",planes[i]);
			 }
		}
		
		//check for messages from airports
		
		for(int i=0;i<10;i++){
			msg_from_airport m8;
			//printf("%d\n", airports[i]);
			int received = msgrcv(msgid, &m8, sizeof(msg_from_airport)-sizeof(long), airports[i], IPC_NOWAIT);
			if(received>0){
				printf("----------------------------------------------------------------------------\n");
				printf("Message received from airport %d === %ld \n",i+1, m8.msg_type);
				printf("plane id: %d\n", m8.plane_id);
				printf("activity: %c\n", m8.activity);
				printf("status: %c\n", m8.status);
				printf("\n");
				
				//send messages to plane after message from airport
				int sent=-1;
				
				if(m8.status=='G'){
					if(m8.activity=='D'){
						printf("Boarding completed by airport %d for plane %d\n", i+1, m8.plane_id);
						printf("sending message to plane %d to begin journey\n", m8.plane_id);
			 			msg_to_plane m3;
			 			if(m8.plane_id==10) {
			 				m3.msg_type = 100;
			 			}
			 			else {
			 				m3.msg_type = m8.plane_id+m8.plane_id*10;
			 			}
			 			//if(terminate==1) {
			 			//	m3.terminate = 'Y';
			 			//}
			 			//else {
			 				m3.terminate = 'N';
			 			//}
				 		m3.ok_shutdown = 'N';
			 			m3.ok_travel = 'Y';
			 			sent = msgsnd(msgid, (void*)&m3, sizeof(msg_to_plane)-sizeof(long), 0);
			 			if(sent!=-1) {
			 				printf("Message sent\n");
			 				//increment to keep track of number of planes in transit
			 				planes_in_transit++;
			 				sent = -1;
			 			}
			 			else {
			 				printf("Error while sending the message\n");
			 			}
			 			
			 			
			 			FILE *file = fopen("AirTrafficController.txt", "a");
	    					if (file == NULL) {
	     				        	perror("Error opening file");
							return 1;
	    					}
	    					// Write data to the file
	   					fprintf(file, "Plane %d has departed from Airport %d and will land at Airport %d.\n", m8.plane_id, plane_array[m8.plane_id-1].departure_airport,plane_array[m8.plane_id-1].destination_airport);
	    					// Close the file
	    					fclose(file);
	   					printf("Data appended to file successfully.\n");
	   					printf("----------------------------------------------------------------------------\n");
					}
					if(m8.activity=='A'){
						printf("unboarding of plane %d received from airport %d\n", m8.plane_id,i+1);
						printf("Sending messsage to plane to begin shutdown procedures\n");
						//sending message to plane to ask it to shutdown
	   					msg_to_plane m3;
			 			if(m8.plane_id==10) {
			 				m3.msg_type = 100;
			 			}
			 			else {
			 				m3.msg_type = m8.plane_id+m8.plane_id*10;
			 			}
			 			//if(terminate==1) {
			 			//	m3.terminate = 'Y';
			 			//}
			 			//else {
			 				m3.terminate = 'N';
			 			//}
				 		m3.ok_shutdown = 'Y';
			 			m3.ok_travel = 'N';
			 			sent = msgsnd(msgid, (void*)&m3, 3*sizeof(char), 0);
			 			if(sent!=-1) {
			 				printf("Message sent\n");
			 				printf("----------------------------------------------------------------------------\n");
			 				//increment to keep track of number of planes in transit
			 				planes_in_transit--;
			 				sent=-1;
			 			}
			 			else {
			 				printf("Error while sending the message\n");
			 				printf("----------------------------------------------------------------------------\n");
			 			}
					}
				}
			}
			else{
				//printf("no message received from airport %d === %d \n",i+1, airports[i]);
			}
		}
	}
	
	//send termination messages to each airport
	
	int arp_term[] = {619,629,639,649,659,669,679,689,699,609}; //609 for address airport 10
	
	for(int j=0;j<10;j++){
		msg_terminate mt;
		mt.msg_type = arp_term[j];
		mt.terminate = 'Y';
		int sent = msgsnd(msgid, (void *)&mt, sizeof(msg_terminate)-sizeof(long), 0);
		if(sent!=0){
			printf("Error while sending message\n");
		}
		else{
			printf("Message sent\n");
		}
	}
	
	// close message queue before terminating the atc
	
	if (msgctl(msgid, IPC_RMID, NULL) == -1) {
       	 perror("msgctl");
       	 exit(EXIT_FAILURE);
    	}
    	system("rm a_random_text_file.txt");
	return 0;
}
