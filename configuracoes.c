

#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <ctype.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <stdlib.h>
#include <string.h>


#include <fcntl.h>
#include <error.h>
#include <errno.h>
#include <signal.h>
#include <pthread.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/ipc.h>
#include <semaphore.h>



#define SIZE_BUF	1024



int main(int argc, char ** argv){
	int escolha=0;
	int port;
	
    char str[SIZE_BUF];
    char str2[SIZE_BUF];
    int fd;
    char * myfifo = "/tmp/myfifo";
    char pedido[SIZE_BUF];

  
    while(escolha!=4){
    	printf("[ESCOLHER MUDANÇA]1.Numero Threads;2.Scheduling;3.Allowed;4.quit\n");
	
		scanf("%d",&escolha);
		if(escolha==1){
			scanf("%s",pedido);
			strncat (str,"NUM:",4);
			strncat (str,pedido,strlen(pedido));
			strncat (str,";",1);
			
		}
		else if(escolha==2){
			scanf("%s",pedido);
			strncat (str,"ESC:",4);
			strncat (str,pedido,strlen(pedido));
			strncat (str,";",1);

		}
		else if(escolha==3){
			scanf("%s",pedido);
			strncat (str,"ALL:",4);
			strncat (str,pedido,strlen(pedido));
			strncat (str,";",1);

		}
	
    }
   strncpy(str2, str, strlen(str)-1);
   //printf("%s\n",str2);
	
	 

	 if((fd=open(myfifo,O_WRONLY))<0){
		perror("Cannot open pipe for writing");
		exit(0);
	}
   
    write(fd, str2, sizeof(str2));
    close(fd);


}

