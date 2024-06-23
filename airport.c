#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <string.h>
#include <pthread.h>
#include <sys/msg.h>

#define FILE_NAME "a_random_text_file.txt"
#define CHARACTER 'X'

typedef struct runway_struct{
	int runway_number;
	int max_load;
	//pthread_t id;
	pthread_mutex_t lock;
} runway_struct;

typedef struct message_from_atc_struct{
	long msg_type;
	int plane_id;
	//int departing_to;
	int weight_of_plane;
	char activity; //'A' for arrival, 'D' for departure	
	char arrived;
} msg_from_atc;

typedef struct message_to_atc_struct{
	long msg_type;
	int plane_id;
	char activity; //'A' for arrival, 'D' for departure
	char status;	//'G' for granted, 'R' for rejected
} msg_to_atc;

//typedef struct arriving_plane_info_struct{
//	int plane_id;
//	pthread_t tid;
//	char arrived;
//	int runway;  // store all mutexes in an array to simpily access using this value
//} arriving_plane_info;

typedef struct arriving_plane_info_struct {
    int msg_type;
    int plane_id;
    pthread_t tid;
    char arrived;
    runway_struct *runway; // store runway number chosen for the plane
    //long msg_type_to_atc;
    int msgid;
    int weight_of_plane;
    char serviced;
} arriving_plane_info;

typedef struct departing_plane_info_struct{
	int msg_type;
	int plane_id;
	runway_struct *runway_info_array;
	int weight_of_plane;
	int msgid;
	int airport_number; 
	int total_runways;
} departing_plane_info;

typedef struct terminate_struct{
	long msg_type;
	char terminate;
} msg_terminate;

void bubble_sort(runway_struct arr[], int n) {
    // Outer loop to iterate through all elements of the array
    for (int i = 0; i < n - 1; i++) {
	// Inner loop for the adjacent element comparisons
	for (int j = 0; j < n - i - 1; j++) {
	    // Compare adjacent elements and swap them if necessary
	    if (arr[j].max_load > arr[j + 1].max_load) {
	        // Swap arr[j] and arr[j + 1]
	        runway_struct temp = arr[j];
	        arr[j] = arr[j + 1];
	        arr[j + 1] = temp;
	    }
	}
    }
}

void *landing_ops(void *params){
	arriving_plane_info *arg = (arriving_plane_info *)params;
	while(arg->arrived=='N'){
		asm("nop");
	}
	pthread_mutex_lock(&((arg->runway)->lock));
	for(int i=0;i<2;i++){
		printf("Landing %d\n", i+1);
		sleep(1);
	}
	for(int i=0;i<3;i++){
		printf("deboarding/unloading %d\n", i+1);
		sleep(1);
	}
	arg->serviced = 'Y';
	pthread_mutex_unlock(&((arg->runway)->lock));
	msg_to_atc m2;
	m2.msg_type = arg->msg_type;
	m2.plane_id=arg->plane_id;
	m2.status = 'G';
	m2.activity = 'A';
	//printf("message type: %ld\n", m2.msg_type);
	//printf("plane id type: %d\n", m2.plane_id);
	//printf("status: %c\n", m2.status);
	//printf("activity: %c\n", m2.activity);
	//printf("msgid: %d\n", arg->msgid);
	int sent = msgsnd(arg->msgid, (void *)&m2, sizeof(msg_to_atc)-sizeof(long), 0);
	if(sent==0){
		printf("Message sent successfully\n");
		printf("Plane %d has landed\n",arg->plane_id);
		printf("----------------------------------------------------------------------------\n");
		sent = -1; //resetting the value
	}
	else{
		printf("Error while sending message\n");
		printf("----------------------------------------------------------------------------\n");
	}
	pthread_exit(NULL);
	
}

void *takeoff_ops(void *params){
	departing_plane_info *arg = (departing_plane_info *)params;
	int runway_index;
	for(int i=0;i<arg->total_runways;i++){
		if(((arg->runway_info_array)[i]).max_load > arg->weight_of_plane){
			runway_index = i;
			break;
		}
	}
	printf("Runway %d has been alloted to plane %d : %d / %d\n", ((arg->runway_info_array)[runway_index]).runway_number, arg->plane_id, arg->weight_of_plane, ((arg->runway_info_array)[runway_index]).max_load);
	pthread_mutex_lock(&((arg->runway_info_array)[runway_index]).lock);
	for(int j=0;j<3;j++){
		printf("Boarding/loading %d\n", j+1);
		sleep(1);
	}
	for(int j=0;j<2;j++){
		printf("Takeoff %d\n", j+1);
		sleep(1);
	}
	
	//send message to atc about boarding and takeoff completion
	
	pthread_mutex_unlock(&((arg->runway_info_array)[runway_index]).lock);
	
	msg_to_atc m2;
	
	m2.msg_type = arg->msg_type;
	m2.plane_id=arg->plane_id;
	m2.status = 'G';
	m2.activity = 'D';
	int sent = msgsnd(arg->msgid, (void *)&m2, sizeof(msg_to_atc)-sizeof(long), 0);
	if(sent==0){
		printf("Message sent successfully\n");
		printf("Plane %d has taken off from airport %d\n",arg->plane_id, arg->airport_number);
		printf("----------------------------------------------------------------------------\n");
		sent = -1; //resetting the value
	}
	else{
	printf("Error while sending message\n");
		printf("----------------------------------------------------------------------------\n");
	}
	
	pthread_exit(NULL);
}

//declare array to store data of arriving planes
//use pointer to ensure data is not lost

arriving_plane_info *arrival_array[1000] = {NULL}; //make this large for easier access
int arrival_array_index_counter = 0; //index into the arrival array

//array for storing arrival thread values
	
pthread_t a_tid_array[1000] = {0};
int a_tid_counter =0; //used to keep track of the next free location in array

//array for storing departure thread values
	
pthread_t d_tid_array[1000] = {0};
int d_tid_counter =0; //used to keep track of the next free location in array
	
int main(){
	printf("Enter airport number: ");
	int airport_number;
	scanf("%d", &airport_number);
	int message_type_from_atc;
	int message_type_to_atc;
	int terminate_msg_type;
	if(airport_number==10){
		message_type_from_atc = 10000;
		message_type_to_atc = 1000;
		terminate_msg_type = 609;
	}
	else{
		message_type_from_atc = airport_number + airport_number*10 + airport_number*100 + airport_number*1000;
		message_type_to_atc = airport_number + airport_number*10 + airport_number*100;
		terminate_msg_type = 600+airport_number*10+9;
	}	
	getchar();
	printf("Enter the number of runways: ");
	int runway_count;
	scanf("%d",&runway_count);
	int total_runways = runway_count+1;
	getchar();
	runway_struct *runway_info = (runway_struct *)malloc(sizeof(runway_struct)*(runway_count+1));
	char input_string[100];
	printf("Enter the load capacity of runways as a space seperated list: ");
	fgets(input_string, sizeof(input_string), stdin);
	char *char_int = strtok(input_string, " \n");
	int count = 0;
	while(char_int !=NULL && count <runway_count){
		runway_info[count].runway_number=count+1;
		runway_info[count].max_load=atoi(char_int);
		pthread_mutex_init(&runway_info[count].lock, NULL);
		count++;
		char_int = strtok(NULL," \n");	
	}
	
	//setting up default runway
	
	runway_info[runway_count].max_load = 15000;
	runway_info[runway_count].runway_number = -1;
	pthread_mutex_init(&runway_info[runway_count].lock, NULL);
	
	printf("Loads for each runways are: ");
	for(int i =0; i<total_runways; i++){
		printf("%d ", runway_info[i].max_load);
	}
	printf("\n");
	
	//sort the array to enable easy best fit access
	
	bubble_sort(runway_info, total_runways);
	
	for(int i=0;i<total_runways;i++){
		printf("Runway number: %d, load: %d\n", runway_info[i].runway_number, runway_info[i].max_load);
	}
	
	//initialise thread attributes for later use
	
	pthread_attr_t attr;
	pthread_attr_init(&attr);
	
	//creating thread for handling the termination of threads
	
	
	
	//check for messages from atc
	
	 system("touch a_random_text_file.txt");
	 key_t key = ftok(FILE_NAME, CHARACTER);
	 if (key == -1){
	 	printf("error in creating unique key\n");
		exit(1);
	 }

        int msgid = msgget(key, 0644);    
	 while (msgid == -1){
	       printf("no message queue with the given key\n");
		sleep(1);
		msgid = msgget(key, 0644); 
	}
	//printf("message queue msgid: %d",msgid);
	msg_from_atc m1;
	msg_to_atc m2;
	int shutdown=0;
	int received=0;
	int sent = -1;
	int incoming = 0; //keeps track of number of incoming planes
	
	while(shutdown==0){
		//check for termination
		msg_terminate mt;
		received = msgrcv(msgid, &mt, sizeof(msg_terminate)-sizeof(long), terminate_msg_type, IPC_NOWAIT);
		if(received>0){
			if(mt.terminate == 'Y'){
				shutdown=1;
			}
			received = 0;
		}
		
		received = msgrcv(msgid, &m1, sizeof(msg_from_atc)-sizeof(long), message_type_from_atc, IPC_NOWAIT);
		if(received>0){
			if(m1.activity=='A'){
				if(m1.arrived=='N'){
					//means a plane is going to arrive
					printf("Message about future arrival of plane %d has been received\n",m1.plane_id);
					for(int i=0;i<total_runways;i++){
						if(m1.weight_of_plane<runway_info[i].max_load){
							printf("Runway %d has been alloted with a max load of %d for a plane %d of weight %d\n", i+1, runway_info[i].max_load, m1.plane_id, m1.weight_of_plane);
							printf("waiting\n");
							printf("storing data into arrival array\n");
							arriving_plane_info *a1 = (arriving_plane_info *)malloc(sizeof(arriving_plane_info));
							a1->plane_id = m1.plane_id;
							a1->runway = &runway_info[i]; //0 for default runway. check for this expliicitly
							a1->arrived = 'N';
							a1->serviced = 'N';
							a1->msgid = msgid;
							a1->weight_of_plane = m1.weight_of_plane;
							a1->msg_type = airport_number + airport_number*10 + airport_number*100;
							arrival_array[arrival_array_index_counter] = a1;
							//create thread for handling this plane and pass the array index address so that the thread can access this data;\
							//note that the mutex lock is stored in an array odf size equal to the size of the number of runways and can thus be indexed into 
							//by using the runway field of the arriving_plane_info.
							pthread_create(&a_tid_array[a_tid_counter],&attr, landing_ops, arrival_array[arrival_array_index_counter++]);
							printf("Thread %ld  created for handling arrival of plane %d\n", a_tid_array[a_tid_counter++], m1.plane_id);
							break;
						}
					}
					//create thread here
					printf("----------------------------------------------------------------------------\n");
					incoming++;
					
				}
				else{
					printf("Received landing request for plane %d\n",m1.plane_id);
					//for(int i=0;i<total_runways;i++){
					//	if(m1.weight_of_plane<runway_info[i].max_load){
					//		printf("Runway %d has been alloted with a max load of %d for a plane %d of weight %d\n", i+1, runway_info[i].max_load, m1.plane_id, m1.weight_of_plane);
					//		break;
					//	}
					//}
					
					//modify data structure here
					
					//in the mutex, put a busy wait loop before the critical section. in this way, the thread doesn't blockk access to the 
					//runway foor any departing planes
					
					//pthread_create(&tid_array[tid_counter++])
					//m2.msg_type = message_type_to_atc;
					//m2.plane_id=m1.plane_id;
					//m2.status = 'G';
					//m2.activity = 'A';
					//sent = msgsnd(msgid, (void *)&m2, sizeof(msg_to_atc)-sizeof(long), 0);
					//if(sent==0){
					//	printf("Message sent successfully\n");
					//	printf("Plane %d has been granted landing at airport %d\n",m1.plane_id, airport_number);
					//	printf("----------------------------------------------------------------------------\n");
					//	sent = -1; //resetting the value
					//}
					//else{
					//	printf("Error while sending message\n");
					//	printf("----------------------------------------------------------------------------\n");
					//}
					for(int i=0;i<arrival_array_index_counter;i++){
						if( (arrival_array[i])->plane_id==m1.plane_id && (arrival_array[i])->serviced=='N'){
							(arrival_array[i])->arrived='Y';
						}
					}
					
				}
			}
			else if(m1.activity=='D'){
				printf("Received departure request for plane %d\n",m1.plane_id);
				//m2.msg_type = message_type_to_atc;
				//m2.plane_id=m1.plane_id;
				//m2.status = 'G';
				//m2.activity = 'D';
				//sent = msgsnd(msgid, (void *)&m2, sizeof(msg_to_atc)-sizeof(long), 0);
				//if(sent==0){
				//	printf("Message sent successfully\n");
				//	printf("Plane %d has taken off from airport %d\n",m1.plane_id, airport_number);
				//	printf("----------------------------------------------------------------------------\n");
				//	sent = -1; //resetting the value
				//}
				//else{
				//	printf("Error while sending message\n");
				//	printf("----------------------------------------------------------------------------\n");
				//}
				//for(int i=0;i<total_runways;i++){
				//	if(m1.weight_of_plane < runway_info[i].max_load){
					
						//create the data structure to send to the thread
						
						departing_plane_info *dep_info = (departing_plane_info*)malloc(sizeof(departing_plane_info));
						dep_info->airport_number = airport_number;
						dep_info->msgid = msgid;
						dep_info->plane_id = m1.plane_id;
						dep_info->msg_type = message_type_to_atc;
						dep_info->weight_of_plane = m1.weight_of_plane;
						dep_info->runway_info_array = runway_info;	
						dep_info->total_runways = total_runways;
						
						//create thread
						
						int creation_status = pthread_create(&d_tid_array[d_tid_counter], &attr, takeoff_ops, dep_info);
						if(creation_status!=0){
							printf("Error in creating thread... terminanting\n");
							printf("----------------------------------------------------------------------------\n");
							exit(1);
						}
						else{
							printf("Thread %ld  created for handling departure of plane %d\n", d_tid_array[d_tid_counter++], m1.plane_id);
							printf("----------------------------------------------------------------------------\n");
						}
				//		break;
					//}
				//	else{
				//		printf("No runway can provide safe landing... please try to alternative airports\n");
				//	}
				}
			}
		}
		//printf("Do you want to close the airport")
	
	
	//for(int i=0;i<total_runways;i++){
	//	pthread_create(&(runway_info[i].id),&attr, activity, NULL);
	//	printf("thread for runway %d has been created\n", runway_info[i].runway_number);
	//}
	
	//wait for threads to join
	
	//for(int i=0;i<total_runways;i++){
	//	pthread_join(runway_info[i].id,NULL); //does not take a pointer
	//	printf("thread for runway %d has been shutdown\n", runway_info[i].runway_number);
	//}
	
	//free up heap
	
	//free(runway_info);
	
	//delete mutexes
	
	for(int k=0;k<total_runways;k++){
		pthread_mutex_destroy(&((runway_info[k]).lock));
	}
	
	//join all threads
	
	for(int k=0; k<a_tid_counter;k++){
		pthread_join(a_tid_array[k], NULL);
	}
	
	for(int k=0; k<d_tid_counter;k++){
		pthread_join(d_tid_array[k], NULL);
	}
	
	//system("rm a_random_text_file.txt");
	
	return 0;
}
