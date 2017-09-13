
//Alexandra Tomé Leandro 2013146082
//Esforço: 80h

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

 //meus ****//

 #include "Buffer.h"
 #include "Estatisticas.h"
 //   //

// Produce debug information
#define DEBUG	  	1	

// Header of HTTP reply to client 
#define	SERVER_STRING 	"Server: simpleserver/0.1.0\r\n"
#define HEADER_1	"HTTP/1.0 200 OK\r\n"
#define HEADER_2	"Content-Type: text/html\r\n\r\n"

#define GET_EXPR	"GET /"
#define CGI_EXPR	"cgi-bin/"
#define SIZE_BUF	1024

 #define f_config "config.txt"



int  fireup(int port);
void identify(int socket);
void get_request(int socket);
int  read_line(int socket, int n);
void send_header(request req);
int send_page(request req);
void execute_script(int socket);
void not_found(int socket);
void catch_ctrlc(int);
void cannot_execute(int socket);


int descomprimir (request req);
//configurar inicial
void config();
//buffer
int iscomprimido(char req[]);
buffer  criaBuffer();
void insereCliente(buffer  lista, char pedido[], int n_socket,time_t inicio,time_t fim,int tipo);
 buffer proxCliente(buffer  lista);
request eliminaCliente (buffer  lista);
int lista_vazia( buffer lista);
 buffer destroi_lista( buffer lista);
//threads
 void threads();
 void* servir(void* n);
void* mudarConfs(void* args);
void destroiPool();
void terminarThreads();
//escanolamento- threads
void* escalonar(void *args);
request fif();
request normal();
request comprimido();
int threadDisponivel();

//mapear memoria e processo estatisticas
void mapear_shared();
void process_Esta();

//semaforos
sem_t * semMemParti;
sem_t *semFila;
sem_t * semCons;
sem_t *semThreads;
pthread_mutex_t * mutexes;

void Semaforos();

//limpar
void cleanup();



char buf[SIZE_BUF]; 
char req_buf[SIZE_BUF];
char buf_tmp[SIZE_BUF];
int socket_conn,new_conn;


//config inicial
int port=0;
int num_threads;
int scheduling=0;
char allowed[SIZE_BUF];
//threads
pthread_t tConfs;
char * myfifo = "/tmp/myfifo";
pthread_t escalonamento;
pthread_t * worker; 
int *num_thread;
int* ocupadas;//threads cupadas
int * fechar;
request * requests;
//Shared memory 
int shmid;
struct sharedMem *mem;
//processo estatisticas    		
pid_t pidEst;
//buffers
 buffer normais;
 buffer comp ;





int main(int argc, char ** argv)
{
	struct sockaddr_in client_name;
	socklen_t client_name_len = sizeof(client_name);
	
     int i;
	signal(SIGINT,catch_ctrlc);

	//com ficheiro config.txt deixa de ser necessario

	// Verify number of arguments
	/*if (argc!=2) {
		printf("Usage: %s <port>\n",argv[0]);
		exit(1);
	}
	port=atoi(argv[1]);*/

	//LER CONFIGURAÇOES INICIAIS de config.txt
	
	config();

	//criar memoria partilhada
	
	mapear_shared();
	//criar semaforos 
    Semaforos();

	//criar buffers de pedidos- um para pedidos de paginas normais e outro para comprimidas 
     normais=criaBuffer();

     comp= criaBuffer();

	 //Criar pool de threads
	 fechar= malloc(sizeof(int)*num_threads);
	 for(i=0;i<num_threads;i++){
	 	fechar[i]=0;
	 }

	 ocupadas=(int*)malloc(num_threads*sizeof(int)); //inicializar array que vai guardar threads ocupadas

    if(ocupadas<0){
    	printf("Erro no shmat da memoria partilhada\n");
    	exit(1);
    }

	 requests=malloc(num_threads*sizeof(request)); //inicializar array pedidos de threads
	 

      pthread_create(&escalonamento,NULL,&escalonar,NULL); //iniciar thread de escalonar
	 
    
    pthread_create(&tConfs,NULL,&mudarConfs,NULL);//thread a espera de mudança de configuraçoes
	
 
	threads(); //iniciar pool threads
	
	//processo filho--Estatisticas
	if((pidEst=fork()) == 0){
		process_Esta(semMemParti,mem,num_threads);
		exit(0);
	}

	printf("Listening for HTTP requests on port %d\n",port);

	// Configure listening port
	if ((socket_conn=fireup(port))==-1){
		//deu erro
		cleanup();
		exit(1);
	}

	// Serve requests 
	while (1)
	{
		// Accept connection on socket
		printf("Waiting for connection.\n");
		if ( (new_conn = accept(socket_conn,(struct sockaddr *)&client_name,&client_name_len)) == -1 ) {
			printf("Error accepting connection\n");
			cleanup();
			exit(1);
		}
		printf("Accepted connection\n");

		// Identify new client
		identify(new_conn);

		// Process request
		get_request(new_conn);

		//----------meter nos buffers
        //um de cada vez a mexer nos buffers
		sem_wait(semFila);
       
		//pagina comprimida
		
			//tipo 1--comprimido
		if(iscomprimido(req_buf)){
			printf("\nInserir cliente buffer comprimidos\n");
			insereCliente(comp,req_buf,new_conn,time(NULL),0,1); //ver tempo inal nas duas
			printf("Clinte inserido nos comprimidos\n");
			sem_post(semCons);
			

		}
		//pagina normal-tipo 0
		else{
			printf("Inserir cliente buffer normais\n");
			insereCliente(normais,req_buf,new_conn,time(NULL),0,0);
			printf("Inserir cliente inserido nos buffer normais\n");
			sem_post(semCons);
			

			
		}

		sem_post(semFila);

	}

}

//***********************FUNCAO QUE LE CONFIGURAÇOES DE FICHEIRO config.txt***********************+//
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
				strncpy(allowed,token,strlen(token));
				//allowed=token;
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

//**************************THREADS***************************************//
//inicializar pool de threads-- manda para funçao servir o index delas
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
//funçao das threads que le os requests do array
void* servir(void* n){

    int i = *((int *)n);
    int servido=0;
    #if DEBUG
		printf("Thread %d criada\n",i);
	#endif
	while(1){

		if(fechar[i]==0){
			sleep(30);
        
	        pthread_mutex_lock(&(mutexes[i]));

	        printf("\nPedido %s vai ser servido pela thread:%d\n\n",requests[i]->pedido,i);
	       
	        if(requests[i]->tipo==0) {//normais
	           servido=send_page(requests[i]);
	           printf("\nMandou pedido normal\n");
	       }
	        else if(requests[i]->tipo==1){ //comprimidos
	        	

	        	servido=descomprimir (requests[i]);
	        	printf("\nMandou pedido comprimido\n");
	           
	        }
	        close(requests[i]->n_socket);
	        if(servido==1){
		
		        requests[i]->fim=time(NULL);
		        sem_wait(semMemParti);
		        mem[i].lido=0;
		        mem[i].tipo=requests[i]->tipo; //estatico ou comprimido
		        strcpy( mem[i]. fichHtml, requests[i]->pedido );
		    
		        mem[i].inicio=requests[i]->inicio;
		        mem[i].fim=requests[i]->fim;
		        
		        sem_post(semMemParti);
		     }
		     servido=0;
		     ocupadas[i]=0;
	        sem_post(semThreads);
        }
        else{
        	printf("\nAcabar thread %d\n",i);
        	pthread_exit(NULL);
        }
    }
}
 //funçao para verificar prox thread disponivel
int threadDisponivel(){
    int i;
    for(i=0;i<num_threads;i++){
        if(ocupadas[i]==0)
            return i;
    }
    return -1;
}

//funçao da thread escalonar
void* escalonar(void *args) {

    #if DEBUG
    	printf("\nThread escalonar operacional\n");
    #endif

    while(1){
  
        sem_wait(semCons); //esperar enquanto nao tem pedidos nos buffers
        sleep(10);
         #if DEBUG
    	    printf("\nEscalonar ja tem pedidos nos buffers-- vai escalonar\n");
   		 #endif	
        
        sem_wait(semFila); //uma de cada vez a mexer nos buffers
	   
        if(scheduling == 1){
        	printf("\nEscolhido-fifo \n");
           fif();
        }
        else if(scheduling ==2){
        	printf("\nEscolhido-fifo normais \n");
        	normal();
        	
        }
        else if(scheduling== 3){
        	printf("\nEscolhido-fifo comprimidos \n");
        	comprimido();
        }

    

        else{
        	printf("\nErro no scheduling especificado\n");
        }


        sem_post(semFila);

    }

	pthread_exit(NULL);
}
//escolher request mais velho entre filas dos dois tipos
request auxiliarFif(buffer proxD,buffer proxN){
	request r;
     
   
    //mais antigo dos dois
    if((proxD==NULL)&&(proxN==NULL)){
        r= NULL;
         return r;
    }
    else if(proxD==NULL){
       
        r=eliminaCliente(normais);
        return r;
    }
    else if(proxN==NULL){
         
        r=eliminaCliente(comp);
        return r;
    }

    if((proxN->inicio)<=(proxD->inicio)){
         
    	r=eliminaCliente(normais);}
    else{
         
    	r=eliminaCliente(comp);}


   return r;

}
request fif(){
   
    request r;
    int t;
     sem_wait(semThreads);
    buffer  proxD=proxCliente(comp);
    buffer  proxN=proxCliente(normais);
    r=auxiliarFif(proxD,proxN);

    t=threadDisponivel();

    ocupadas[t]=1;
   

    requests[t]=r;
    printf("\n Meteu na thread %d o request %s\n",t,requests[t]->pedido);

   pthread_mutex_unlock(&(mutexes[t]));

   return r;

}

request normal(){
  
    request r;
   
    sem_wait(semThreads);
    if(!lista_vazia(normais)){
    	
    	r=eliminaCliente(normais);
    	
    }
    else{
    	
		r=eliminaCliente(comp);
	}

     int t=threadDisponivel();

    ocupadas[t]=1;
   

    requests[t]=r;

    printf("\n Meteu na thread %d o request %s\n",t,requests[t]->pedido);

   pthread_mutex_unlock(&(mutexes[t]));

   return r;

    
}


request comprimido(){
     request r;
    //esperar por thread livre
    sem_wait(semThreads);
    
    if(!lista_vazia(comp)){
    	
    	 
    	r=eliminaCliente(comp);

   }
    else{
    	
		r=eliminaCliente(normais);
	}

    int t=threadDisponivel();

    ocupadas[t]=1;
   

    requests[t]=r;

    printf("\n Meteu na thread %d o request %s\n",t,requests[t]->pedido);

   pthread_mutex_unlock(&(mutexes[t]));

   return r;
}




//::::::::::::::::::::THREAD QUE ESCALO PEDIDOS QUE VAO CHEGANDO:::::::::::::::::::::::.//



//funaço da thread mudar configuraçoes 
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
				//memcpy(allowed,t+1,strlen(t+1));
				//allowed=t+1;
				sprintf(allowed,"%s",t+1);
				printf("\nAllowed:%s\n",allowed);
				


		    }
             ocupadas=malloc(sizeof(int)*num_threads);
		    for(i=0;i<num_threads;i++){
		    	ocupadas[i]=0;
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



//***************************************************************//
//********************PROCESSO ESTATISTICAS E MEMORIA PARTILHADA PARA COMUNICAR COM ELE***************//
void mapear_shared(){ //Cria segmento de memoria partilhada para comunicao entre process principal e processo de configuracoes
    int i;
    

     #ifdef DEBUG
    	printf("Começar memoria partilhada...\n");
    #endif

    if((shmid=shmget(IPC_PRIVATE,num_threads*sizeof(struct sharedMem), IPC_CREAT|0777)) <0){
		printf("Error creating shared memory\n");
		exit(1);
	}

	mem =(struct sharedMem*)malloc(num_threads*sizeof(struct sharedMem));

    mem = (struct sharedMem*) shmat(shmid, NULL, 0);
    if(mem<0){
    	printf("Erro no shmat da memoria partilhada\n");
    	exit(1);
    }
   
    for(i=0;i<num_threads;i++){
    	struct sharedMem m;
    	memset(&m, '\0', sizeof(m));
  
         mem[i]=m;
    	mem[i].lido=1;
    }
    

    #if DEBUG
		printf("Memoria partilhada criada e mapeada\n");
	#endif
    
    printf("Shared memory created successfully.\n");
}



//::::::::::::::::: SEMAFOROS:::::::::::::::::::::::::::::://
//inicializa os semaforos a usar no programa
void Semaforos(){

	//semaforo memoria partilhada
	sem_unlink("MP");

    semMemParti=sem_open("MP",O_CREAT|O_EXCL,0700,1);

    
    //quando escreve pedidos nao le e quando le nao escreve e so um de cada vez-- sera que posso meter a ler varios a mesmo tempo?
    sem_unlink("FILA");

    semFila=sem_open("FILA",O_CREAT|O_EXCL,0700,1);
    //tinha espera ativa na thread que escalona--> assim fica bloqueado ate escreverem algo e fazerem post 
    sem_unlink("CONSUMIDOR");

    semCons=sem_open("CONSUMIDOR",O_CREAT|O_EXCL,0700,0);

    sem_unlink("SEMTHREADS");
    semThreads=sem_open("SEMTHREADS",O_CREAT|O_EXCL,0700,num_threads);

   int i;


   mutexes=  malloc(num_threads * sizeof (pthread_mutex_t )); //inicializar smaforos de ler em threads

   for(i=0;i<num_threads;i++){
   		pthread_mutex_init(&(mutexes[i]), NULL);
   		pthread_mutex_lock(&(mutexes[i]));
   }

   


}



//*****************************************************************************************************//
//*********************LIMPAR TUDO*******************************************************+//
//espera que todas as threads da pool acabem a execucao.deve ser usada antes de destruir a pool
void terminarThreads(){
	int i;
	for(i=0;i<num_threads;i++){
        if(ocupadas[i]==1)
            pthread_join(worker[i],NULL);
	}
}

void destroiPool(){
	free(worker);
	free(ocupadas);
	free(fechar);
	
}
void cleanup(){
	int i;
     
	terminarThreads();
 	for(i=0; i<num_threads;i++){
		pthread_join(worker[i],NULL);
		//remove(nome_thread[i]);--faxer
	}

	destroiPool();

	//memoria partilhada
	shmdt(mem);
	shmctl(shmid, IPC_RMID, NULL);
    //esperar por processos filho
    kill(pidEst,SIGINT);
	while(wait(NULL)!=-1);

	//bufers
	free(normais);
	free(comp);
	
     //pipe
     unlink(myfifo);
     //semaforos
     sem_close(semMemParti);
     sem_close(semFila);
     sem_close(semCons);
    
	 
	 #if DEBUG
		printf("Limpos memorias parti,pipe,semaforos,buffers,processos filho e threads\n");
	#endif


}

//*******************************************************************+//

//***************************AUXILIARES**************************************+//
//saber se pedido é comprimido
int iscomprimido(char req []){
	
	const char *d = strrchr(req, '.');
	if(d==NULL){
		return 0;
	}
    if(strcmp(d+1,"gz")==0)
    	return 1;
    else
    	return 0;
}

//nao da mas acho que é assim
int descomprimir (request req){
	FILE *codigo;
	char buf_tmp [SIZE_BUF];
	char comando [1000];
	sprintf(buf_tmp,"htdocs/%s",req->pedido);
	int validar=0;
	char * token;
	
    for (token = strtok(allowed,";"); token!=NULL; token = strtok(NULL, ";")){
    	if(strcmp(token,req->pedido)==0){
    		validar=1;
    		break;
    	}

    }
    if(validar==0){

      send(req->n_socket,"Pagina comprimida nao autorizada",32,0);
      return -1;
    }
    else{
		sprintf(comando,"gunzip -c %s",buf_tmp);
		if((codigo=popen(comando,"r"))==NULL){
			printf("Page not found, alerting client\n");
			not_found(req->n_socket);

		}
		else{
			
			send_header( req);

			while(fgets(buf_tmp,SIZE_BUF,codigo))
				send(req->n_socket,buf_tmp,strlen(buf_tmp),0);


		}
		pclose(codigo);
		return 1;
	}
	//close(req->n_socket);
}


//*************************************************************************+//


//::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::::://
// Processes request from client
void get_request(int socket)
{
	int i,j;
	int found_get;

	found_get=0;
	while ( read_line(socket,SIZE_BUF) > 0 ) {
		if(!strncmp(buf,GET_EXPR,strlen(GET_EXPR))) {
			// GET received, extract the requested page/script
			found_get=1;
			i=strlen(GET_EXPR);
			j=0;
			while( (buf[i]!=' ') && (buf[i]!='\0') )
				req_buf[j++]=buf[i++];
			req_buf[j]='\0';
		}
	}	

	// Currently only supports GET 
	if(!found_get) {
		printf("Request from client without a GET\n");
		exit(1);
	}
	// If no particular page is requested then we consider htdocs/index.html
	if(!strlen(req_buf))
		sprintf(req_buf,"index.html");

	#if DEBUG
	printf("get_request: client requested the following page: %s\n",req_buf);
	#endif

	return;
}


// Send message header (before html page) to client
void send_header(request req)
{
	#if DEBUG
	printf("send_header: sending HTTP header to client\n");
	#endif
	sprintf(buf,HEADER_1);
	send(req->n_socket,buf,strlen(HEADER_1),0);
	sprintf(buf,SERVER_STRING);
	send(req->n_socket,buf,strlen(SERVER_STRING),0);
	sprintf(buf,HEADER_2);
	send(req->n_socket,buf,strlen(HEADER_2),0);

	return;
}


// Execute script in /cgi-bin
void execute_script(int socket)
{
	// Currently unsupported, return error code to client
	cannot_execute(socket);
	
	return;
}


// Send html page to client
int send_page(request req)
{
	FILE * fp;

	// Searchs for page in directory htdocs
	sprintf(buf_tmp,"htdocs/%s",req->pedido);

	#if DEBUG
	printf("send_page: searching for %s\n",buf_tmp);
	#endif

	// Verifies if file exists
	if((fp=fopen(buf_tmp,"rt"))==NULL) {
		// Page not found, send error to client
		printf("send_page: page %s not found, alerting client\n",buf_tmp);
		not_found(req->n_socket);
		return -1;
	}
	else {
		// Page found, send to client 
	
		// First send HTTP header back to client
		send_header(req);

		printf("send_page: sending page %s to client\n",buf_tmp);
		while(fgets(buf_tmp,SIZE_BUF,fp))
			send(req->n_socket,buf_tmp,strlen(buf_tmp),0);
		
		// Close file
		fclose(fp);
		return 1;
	}

	

}


// Identifies client (address and port) from socket
void identify(int socket)
{
	char ipstr[INET6_ADDRSTRLEN];
	socklen_t len;
	struct sockaddr_in *s;
	int port;
	struct sockaddr_storage addr;

	len = sizeof addr;
	getpeername(socket, (struct sockaddr*)&addr, &len);

	// Assuming only IPv4
	s = (struct sockaddr_in *)&addr;
	port = ntohs(s->sin_port);
	inet_ntop(AF_INET, &s->sin_addr, ipstr, sizeof ipstr);

	printf("identify: received new request from %s port %d\n",ipstr,port);

	return;
}


// Reads a line (of at most 'n' bytes) from socket
int read_line(int socket,int n) 
{ 
	int n_read;
	int not_eol; 
	int ret;
	char new_char;

	n_read=0;
	not_eol=1;

	while (n_read<n && not_eol) {
		ret = read(socket,&new_char,sizeof(char));
		if (ret == -1) {
			printf("Error reading from socket (read_line)");
			return -1;
		}
		else if (ret == 0) {
			return 0;
		}
		else if (new_char=='\r') {
			not_eol = 0;
			// consumes next byte on buffer (LF)
			read(socket,&new_char,sizeof(char));
			continue;
		}		
		else {
			buf[n_read]=new_char;
			n_read++;
		}
	}

	buf[n_read]='\0';
	#if DEBUG
	printf("read_line: new line read from client socket: %s\n",buf);
	#endif
	
	return n_read;
}


// Creates, prepares and returns new socket
int fireup(int port)
{
	int new_sock;
	struct sockaddr_in name;

	// Creates socket
	if ((new_sock = socket(PF_INET, SOCK_STREAM, 0))==-1) {
		printf("Error creating socket\n");
		return -1;
	}

	// Binds new socket to listening port 
 	name.sin_family = AF_INET;
 	name.sin_port = htons(port);
 	name.sin_addr.s_addr = htonl(INADDR_ANY);
	if (bind(new_sock, (struct sockaddr *)&name, sizeof(name)) < 0) {
		printf("Error binding to socket\n");
		return -1;
	}

	// Starts listening on socket
 	if (listen(new_sock, 5) < 0) {
		printf("Error listening to socket\n");
		return -1;
	}
 
	return(new_sock);
}


// Sends a 404 not found status message to client (page not found)
void not_found(int socket)
{
 	sprintf(buf,"HTTP/1.0 404 NOT FOUND\r\n");
	send(socket,buf, strlen(buf), 0);
	sprintf(buf,SERVER_STRING);
	send(socket,buf, strlen(buf), 0);
	sprintf(buf,"Content-Type: text/html\r\n");
	send(socket,buf, strlen(buf), 0);
	sprintf(buf,"\r\n");
	send(socket,buf, strlen(buf), 0);
	sprintf(buf,"<HTML><TITLE>Not Found</TITLE>\r\n");
	send(socket,buf, strlen(buf), 0);
	sprintf(buf,"<BODY><P>Resource unavailable or nonexistent.\r\n");
	send(socket,buf, strlen(buf), 0);
	sprintf(buf,"</BODY></HTML>\r\n");
	send(socket,buf, strlen(buf), 0);

	return;
}


// Send a 5000 internal server error (script not configured for execution)
void cannot_execute(int socket)
{
	sprintf(buf,"HTTP/1.0 500 Internal Server Error\r\n");
	send(socket,buf, strlen(buf), 0);
	sprintf(buf,"Content-type: text/html\r\n");
	send(socket,buf, strlen(buf), 0);
	sprintf(buf,"\r\n");
	send(socket,buf, strlen(buf), 0);
	sprintf(buf,"<P>Error prohibited CGI execution.\r\n");
	send(socket,buf, strlen(buf), 0);

	return;
}


// Closes socket before closing
void catch_ctrlc(int sig)
{
	printf("Server terminating\n");
	cleanup();
	close(socket_conn);
	

	exit(0);
}

