//Alexandra Tomé Leandro 2013146082
//Esforço: 80h

#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <unistd.h>

#define NAME   "/tmp/server.log"
#define TAM  (100)
#define FILESIZE (TAM * sizeof(struct logs))

 struct sharedMem {
	int lido;
    int tipo; //estatico ou comprimido
    char fichHtml[100] ;
    time_t inicio;
    time_t fim;
};
 
struct logs
{
    int tipo; //estatico ou comprimido
    char fichHtml[100];
    char* inicio;
    char* fim;
};
struct sharedMem *mem;

sem_t* semMemParti;

int i;
int fd;
int result;
struct logs *map;  
int indice=0;

int pnormais=0;
int pcomprimidos=0;
double diferenceNormais=0;
double diferenceComprimidos=0;

void iniciarMmap(){
    
    fd = open(NAME, O_RDWR|O_CREAT|O_TRUNC,(mode_t)0600);
    if (fd == -1) {
        perror("Erro a abrir log para ler");
        exit(EXIT_FAILURE);
    }

    result=lseek(fd,FILESIZE-1,SEEK_SET);

     if (result == -1)
    {
        close(fd);
        perror("Error  lseek() ");
        exit(EXIT_FAILURE);
    }

    result = write(fd, "", 1);
    if (result != 1)
    {
        close(fd);
        perror("Error writing last byte of the file");
        exit(EXIT_FAILURE);
    }

    /* Now the file is ready to be mmapped.  */
    map = (struct logs *)mmap(0, FILESIZE, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (map == MAP_FAILED)
    {
        close(fd);
        perror("Error mapear file");
        exit(EXIT_FAILURE);
    }

}    
 void printar( const struct logs *med)
{
    printf("%d, %s, %s,%s\n", med->tipo,med->fichHtml, med->inicio, med->fim);
}

 void imprimirMmmap(){
    for(i=0;i<indice;i++){
        printar(&map[i]);
       // printf("%d, %s %d,%d\n", map[i].tipo,map[i]->fichHtml, map[i].inicio, map[i].fim);
    }
}

void fecharMmmap(){
	if (munmap(map, FILESIZE) == -1) {
        perror("Error un-mmapping the file");
    }
    close(fd);
    

}
double tempoMedio(int tipoPedido){
	if(tipoPedido==0){//pedidos normais
		return diferenceNormais/pnormais;
	}

	else{
		return diferenceComprimidos/pcomprimidos;
	}

}
   
void imprime(int sig)
{
	printf("Pedidos estaticos servidos:%d\n",pnormais);
	printf("Pedidos comprimidos servidos:%d\n",pcomprimidos);

	if(pnormais!=0){
	    
	     printf("Média estaticos servidos:%f\n",tempoMedio(0));
	}
	else{
		 printf("Média estaticos servidos:%f\n",0.0);
	}
     if(pcomprimidos!=0){
	   
	    printf("Média comprimidos servidos:%f\n",tempoMedio(1));
    }
    else{
    	printf("Média comprimidos servidos:%f\n",0.0);
    }
} 

void reset(int sig)
{
    pnormais=0;
    pcomprimidos=0;
    diferenceNormais=0;
    diferenceComprimidos=0; 
   
} 


void catch_ctrlcProcEst(int sig)
{
    printf("/n Terminar processo de estatisticas\n");
    imprimirMmmap();

    fecharMmmap();

	exit(0);
}


char *getHoras(time_t t){
    char *aux;

      struct tm *tmp = gmtime(&t);
    aux=(char*)malloc(9*sizeof(char));
   
    sprintf(aux,"%d:%d:%d", tmp->tm_hour, tmp->tm_min, tmp->tm_sec);
    return aux;
}

void process_Esta(sem_t* MemParti,struct sharedMem *m,  int num_threads){

    printf("Processo filho estatisticas: %d\n Filho do processo principal:%d\n", getpid(),getppid());
     signal(SIGINT,catch_ctrlcProcEst);
    signal(SIGUSR1,imprime);
    signal(SIGUSR2,reset);
    iniciarMmap();
    semMemParti=MemParti;
    mem=m;
    int i;
    int index=-1;
    


    while(1){
     
    	for(i=0;i<num_threads;i++){
    		if(mem[i].lido==0){
               index=i;

               break;
    		}
    	}
    	
    	if(index!=-1){
    		
    		if(indice<TAM){
	    		struct logs m;
	    		sem_wait(semMemParti);
	    		

	    		memset(&m, '\0', sizeof(m));
	    		
	    		m.tipo=mem[index].tipo; //estatico ou comprimido
                 
	    		strcpy(m.fichHtml, mem[index].fichHtml );
	    		
		        m.inicio=getHoras(mem[index].inicio);
		        m.fim=getHoras(mem[index].fim);

		        double diference= difftime(mem[index].fim,mem[index].inicio); // returns the difference of seconds between time1 and time2 i.e. (time1 - time2)
		       
		        if(mem[index].tipo==0){
		        	pnormais++;
		        	diferenceNormais+=diference;
		        }

		        else{
		        	pcomprimidos++;
		        	diferenceComprimidos+=diference;
		        }

	       		map[indice] = m;
	       		indice ++;
	       		mem[index].lido=1;
	       		
	       		index=-1;
	        	sem_post(semMemParti);
	        }
	        else{
	        	printf("MMAP cheia\n");
	        	index=-1;
	        }

    	}
        
        

    }
    
}