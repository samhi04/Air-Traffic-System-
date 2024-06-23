#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/ipc.h>
#include <string.h>
#include <sys/msg.h>
#include <sys/sem.h>

#define FILE_NAME "a_random_text_file.txt"
#define CHARACTER 'X'
#define MAX_CHILD_PROCESSES 100

typedef struct {
    pid_t child_pid;
    int pipe_fd[2];
} pcs;

typedef struct message_from_atc_struct{
    long msg_type;
    char terminate;
    char ok_travel;
    char ok_shutdown;
} msg_from_atc;

typedef struct message_struct_1{
    long msg_type;
    int p_id;
    int p_type;
    int arrival_airport;
    int departure_airport;
    int weight_of_plane;
    int passengers;
} msg_1;

int main() {
    printf("Enter Plane ID: ");
    int plane_id;
    int incoming_msg_type;
    scanf("%d", &plane_id);
    if(plane_id==10) incoming_msg_type = 100;
    else incoming_msg_type = plane_id+plane_id*10;

    printf("Enter the Type of plane: ");
    int plane_type;
    scanf("%d", &plane_type);
    
    int total_plane_weight = 150;
    pcs *pcs_array=NULL;
    int passenger_count = 0;
    
    // Semaphore initialization
    key_t key = ftok(FILE_NAME, CHARACTER);
    int semaphore_id = semget(key, 1, IPC_CREAT | 0666);
    union semun {
        int val;
        struct semid_ds *buf;
        ushort *array;
    } arg;
    arg.val = 1; // Initialize the semaphore value to 1
    semctl(semaphore_id, 0, SETVAL, arg);

    if (plane_type == 1) {
        printf("Enter the number of occupied seats: ");
        scanf("%d", &passenger_count);
        total_plane_weight+=75*5;

        pcs_array = (pcs *)malloc(sizeof(pcs) * passenger_count);

        for (int i = 0; i < passenger_count; i++) {
            // Acquire the semaphore
            struct sembuf sem_op;
            sem_op.sem_num = 0;
            sem_op.sem_op = -1;
            sem_op.sem_flg = 0;
            semop(semaphore_id, &sem_op, 1);

            if (pipe(pcs_array[i].pipe_fd) == -1) {
                perror("pipe");
                return -1;
            }

            pcs_array[i].child_pid = fork();
            if (pcs_array[i].child_pid == -1) {
                perror("fork");
                return -1;
            }

            if (pcs_array[i].child_pid == 0) {
                // Child process
                close(pcs_array[i].pipe_fd[0]);

                printf("Enter the weight of the luggage for passenger %d: ", i + 1);
                int weight_of_luggage;
                scanf("%d", &weight_of_luggage);

                printf("Enter your weight for passenger %d: ", i + 1);
                int weight_of_passenger;
                scanf("%d", &weight_of_passenger);

                int total_weight = weight_of_luggage + weight_of_passenger;
                char weight[10];
                sprintf(weight, "%d", total_weight);

                write(pcs_array[i].pipe_fd[1], weight, strlen(weight) + 1);

                close(pcs_array[i].pipe_fd[1]);
                exit(0);
            } else {
                // Parent process
                close(pcs_array[i].pipe_fd[1]);

                // Wait for the child process to finish
                wait(NULL);

                // Release the semaphore
                sem_op.sem_op = 1;
                semop(semaphore_id, &sem_op, 1);
            }
        }

        // Parent process
        for (int i = 0; i < passenger_count; i++) {
            char weight[10];
            ssize_t bytes_read = read(pcs_array[i].pipe_fd[0], weight, sizeof(weight));
            if (bytes_read == -1) {
                perror("read");
            } else {
                int int_weight = atoi(weight);
                printf("Weight of passenger %d is %d\n", i + 1, int_weight);
                total_plane_weight+=int_weight;
            }

            close(pcs_array[i].pipe_fd[0]);
        }
    }
    else if(plane_type==0){
        printf("Enter the number of Cargo items: ");
        passenger_count=-1;
        int cargo_count;
        scanf("%d", &cargo_count);
        printf("Enter the average weight of the cargo items: ");
        int avg_weight;
        scanf("%d", &avg_weight);
        total_plane_weight += cargo_count*avg_weight;
    }
    else{
        printf("Invalid plane type, exiting...");
        return -1;
    }
    printf("Total weight of the plane is: %d\n", total_plane_weight);
    
    printf("Enter Airport Number for Departure: ");
    int dep_airport;
    scanf("%d", &dep_airport);
    printf("Enter Airport number for Arrival: ");
    int arr_airport;
    scanf("%d", &arr_airport);
    
    //communicating data to the air traffic controller using the message queue
    
    system("touch a_random_text_file.txt");
    key_t key1 = ftok(FILE_NAME, CHARACTER);
    if (key1 == -1){
  	printf("error in creating unique key\n");
        exit(1);
    }

    int msgid = msgget(key1, 0644);    
    while (msgid == -1){
       printf("no message queue with the given key\n");
        sleep(1);
        msgid = msgget(key1, 0644); 
    }
    
    //setup message
    
    msg_1 request;
    //request.mtype = plane_id;
    //request.mtext[0] = plane_id;
    //request.mtext[1] = plane_type;
    //request.mtext[2] = arr_airport;
    //request.mtext[3] = dept_airport;
    //request.mtext[4] = total_plane_weight;
    //request.mtext[5] = passenger_count;
    
    request.msg_type = plane_id;
    request.p_id = plane_id;
    request.p_type = plane_type;
    request.arrival_airport = arr_airport;
    request.departure_airport = dep_airport;
    request.weight_of_plane = total_plane_weight;
    request.passengers = passenger_count;
    
    //send message
    
    int sent = msgsnd(msgid, (void*)&request, 24, 0);
    if(sent==-1){
    	printf("error in sending message\n");
    }
    else{
    	printf("Message sent\n");
    }
    //system("rm a_random_text_file.txt");
    
    //waitng for instructions for atc
    printf("Waiting for message from atc...\n");
    msg_from_atc new_msg;
    int received = msgrcv(msgid, &new_msg, sizeof(msg_from_atc)-sizeof(long), incoming_msg_type, 0);
    if(received >0){
    printf("----------------------------------------------------------------------------\n");
    printf("reeived messages from atc: \n");
    printf("terminate status: %c\n", new_msg.terminate);
    printf("travel status: %c\n", new_msg.ok_travel);
    printf("shutdown status: %c\n", new_msg.ok_shutdown);
    	if(new_msg.terminate=='Y'){
    		printf("Termination request received from atc. shutting down...\n");
    		free(pcs_array);
    		return 0;
    	}
    	else if(new_msg.ok_travel=='Y'){
    		for(int k=0; k<=30; k++){
    			printf("Travelling %d\n", k);
    			sleep(1);
    		}
    		printf("Requesting landing...\n");
    		printf("----------------------------------------------------------------------------\n");
    		request.departure_airport = -dep_airport; //indicates that plane is ready for landing. this message will be forwarded to airport by atc.
    		msgsnd(msgid, (void*)&request, 24, 0);
    	} 
    }
    
    //waitng for next message from atc...
    
    received = msgrcv(msgid, &new_msg, sizeof(msg_from_atc)-sizeof(long), incoming_msg_type, 0);
    
    if(received>0){
    printf("reeived messages from atc: \n");
    printf("terminate status: %c\n", new_msg.terminate);
    printf("travel status: %c\n", new_msg.ok_travel);
    printf("shutdown status: %c\n", new_msg.ok_shutdown);
    	if(new_msg.ok_shutdown=='Y'){
    		//printf("successfully landed at airport and finished uboarding... shutting down\n");
    		printf("Plane %d has travelled from airport %d to airport %d\n", plane_id, dep_airport, arr_airport);
    	}
    }

    free(pcs_array);
    return 0;
}
