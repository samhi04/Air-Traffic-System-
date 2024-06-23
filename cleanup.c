#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/msg.h>
#include <sys/types.h>

#define FILE_NAME "a_random_text_file.txt"
#define CHARACTER 'X'

typedef struct terminate_struct{
	long msg_type;
	char terminate;
} msg_terminate;

int main(){
	char terminate = 'N';
	while(terminate=='N'){
		printf("Do you want the Air Traffic Control system to terminate? Y for yes, N for no: ");
		scanf("%c",&terminate);
		getchar();	
	}
	//join message queue
	
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
	
	//send termination message
	
	msg_terminate m1;
	m1.msg_type=6942069;
	m1.terminate = 'Y';
	
	int sent = msgsnd(msgid, (void *)&m1,sizeof(msg_terminate)-sizeof(long), 0 );
	if(sent == 0){
		printf("Termination message sent. exiting...\n");
	}
	else{
		printf("Error while sending message.\n");
		exit(1);
	}
	return 0;	
}
