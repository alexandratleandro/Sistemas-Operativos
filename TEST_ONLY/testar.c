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
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>



#define SIZE_BUF	1024
 //MINHAS//
 #define f_config "config.txt"

 //END MINHAS//

char * myfifo = "/tmp/myfifo";

//***********ESTRUTURAS*********//


void config();
//threads
//threads
void* servir(void* n);
 void threads();
 
void* mudarConfs(void* args);
void destroiPool();
void terminarThreads();

pthread_t * worker; 
int *num_thread;
int * fechar;



//semaforos
sem_t* semConfs;


//pipes
int pipee;
//config inicial
int port=0;
int num_threads;
int scheduling=0;
char *allowed;

pthread_t tConfs;



//END MINHAS//



int main(int argc, char ** argv)
{

	
     int i;
	config();
     fechar= malloc(sizeof(int)*num_threads);
	 for(i=0;i<num_threads;i++){
	 	fechar[i]=0;
	 }
	
    //thread a espera de mudança de configuraçoes
    pthread_create(&tConfs,NULL,&mudarConfs,NULL);
	//iniciar thread de escalonar

	 //esperar que se escreva para mudar conf noutro programa 
    sem_unlink("CONFS");

    semConfs=sem_open("CONFS",O_CREAT|O_EXCL,0700,0);

   
    threads();

    return 0;
 }

 //funçao das threads que le os requests do array
void* servir(void* n){

    int i = *((int *)n);
    printf("\nThread %d criada\n",i);
    while(1){
    	printf("\n[THREAD%d]-OLA\n",i);
    }
	
}
   
//ler configuraçoes
void config(){
	
	char *token;
	const char s[2]="=";
	char linha[50];

	FILE *f = fopen(f_config,"r");
	if(f){
		#if DEBUG
			printf("Dentro do ficheiro de configuracoes\n");
		#endif

		while(fgets(linha, 50, f)!=NULL){
			token=strtok(linha,s);

			if (strcmp(token,"SERVERPORT") == 0){
				token=strtok(NULL,s);
				port=atoi(token);
				#if DEBUG
					printf("Porto:%d\n",port);
				#endif
			}

			else if (strcmp(token,"SCHEDULING") == 0){
				token=strtok(NULL,s);
				scheduling=atoi(token);
				#if DEBUG
					printf("Scheduling:%d\n",scheduling);
				#endif

			}
			else if (strcmp(token,"THREADPOOL") == 0){
				token=strtok(NULL,s);
				num_threads=atoi(token);
				#if DEBUG
					printf("Numero threads:%d\n",num_threads);
				#endif
				
			}
			else if (strcmp(token,"ALLOWED") == 0){
				token=strtok(NULL,s);
				allowed=token;
				#if DEBUG
					printf("Allowed:%s\n",allowed);
				#endif
				
			}
		}
		fclose(f);
		
		#if DEBUG
			printf("Ficheiros de configuraçoes fechado\n");
		#endif
	}
	else{
		printf("Configuration file not found\n");
		exit(1);
	}
}

void threads(){

	#if DEBUG
		printf("\nVou começar a criar a pool de Threads\n");
	#endif
    
	worker = malloc(sizeof(pthread_t)*num_threads);
    num_thread = malloc(sizeof(int)*num_threads);
    int i;
    
    for(i=0;i<num_threads;i++){

        num_thread[i] = i;
        pthread_create(&worker[i], NULL, &servir,&num_thread[i]); 
    }
   
}

//funaço da thread escalonar
void* mudarConfs(void *args) {

	int fd;
    
    char str[SIZE_BUF];
    char * token;

    
    unlink(myfifo);//antes pq nao cheguei a fazer quando acabou--> vai para cleanup do projeto
    if((mkfifo(myfifo,O_CREAT|O_EXCL|0666))<0) {
		 perror("Nao conseguiu criar unnamed pipe");
 		exit(0);
 	}

    
    while(1){
	    
	    int i;
	    if((fd=open(myfifo,O_RDONLY))<0){
			perror("Nao consegue abrir pipe para ler");
			exit(0);
		}
	    read(fd, str, SIZE_BUF);
	    printf("\n[PIPE]Received: %s\n", str);

	    for(i=0;i<num_threads;i++){
	    	fechar[i]=1;
	    }
        
        terminarThreads();
        destroiPool();
        
		
		for (token = strtok(str,";"); token!=NULL; token = strtok(NULL, ";")){

	        //printf("%s\n",token);
	      
			char* aux=malloc(sizeof(token));
			memcpy ( aux, token, strlen(token) );
			char c2[1000];
			strcpy(c2, aux);
	
    		char str[3];
    		strncpy(str, c2, 3);
			//printf("%s\n",str);
	        if (strcmp(str,"NUM") == 0){
	        	char *t = strrchr(token, ':');
	   
				num_threads=atoi(t+1);
				
				printf("\nNumero threads:%d\n",num_threads);
				
				

	    	}
	    	else if (strcmp(str,"ESC") == 0){
				char *t  = strrchr(token, ':');
				scheduling=atoi(t+1);
				
				printf("\nEscalonamento:%d\n",scheduling);
				
		    }

		    else if (strcmp(str,"ALL") == 0){
				char *t  = strrchr(token, ':');
				strncpy(allowed,t+1,strlen(t+1));
				//allowed=t+1;
				
				printf("\nAllowed:%s\n",allowed);
				


		    }
            
		    fechar=malloc(sizeof(int)*num_threads);
		    for(i=0;i<num_threads;i++){
		    	fechar[i]=0;
		    }
        	threads();

        }

    	
    }


	   close(fd);
	

}

void terminarThreads(){
	int i;
	for(i=0;i<num_threads;i++){
        
            pthread_join(worker[i],NULL);
	}
}

void destroiPool(){
	free(worker);

	free(fechar);
	
}