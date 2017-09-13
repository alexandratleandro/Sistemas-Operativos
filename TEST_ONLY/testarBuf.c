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
request normal();
request comprimido();
request fif();
buffer comp;
buffer normais;

int escalonamento=1; // 1-- fif; 2-normal; 3-comprimido

main(){
	int i;
	 normais=criaBuffer();
      comp=criaBuffer();
	

	if(escalonamento==1){
		insereCliente(comp,"esteC1",1,time(NULL),0,1);
	insereCliente(normais,"este1",1,time(NULL),0,0);
	insereCliente(comp,"esteC2",1,time(NULL),0,1);
	insereCliente(normais,"este2",1,time(NULL),0,0);
	insereCliente(comp,"esteC3",1,time(NULL),0,1);
	insereCliente(normais,"este3",1,time(NULL),0,0);


	request r=fif();
	printf("%s\n",r->pedido);

	 r=fif();
	printf("%s\n",r->pedido);

	r=fif();
	printf("%s\n",r->pedido);

	r=fif();
	printf("%s\n",r->pedido);

	r=fif();
	printf("%s\n",r->pedido);

	r=fif();
	printf("%s\n",r->pedido);

	}

	else if (escalonamento==2){

	//normais 
	insereCliente(comp,"esteC1",1,time(NULL),0,1);
	insereCliente(normais,"este1",1,time(NULL),0,0);
	insereCliente(comp,"esteC2",1,time(NULL),0,1);
	insereCliente(normais,"este2",1,time(NULL),0,0);
	insereCliente(comp,"esteC3",1,time(NULL),0,1);
	insereCliente(normais,"este3",1,time(NULL),0,0);

	request r=normal();
	printf("%s\n",r->pedido);

	 r=normal();
	printf("%s\n",r->pedido);

	r=normal();
	printf("%s\n",r->pedido);

	r=normal();
	printf("%s\n",r->pedido);

	r=normal();
	printf("%s\n",r->pedido);

	r=normal();
	printf("%s\n",r->pedido);

	}

     
    else if(escalonamento==3){

	insereCliente(normais,"este1",1,time(NULL),0,0);
	sleep(1);
	insereCliente(comp,"esteC1",1,time(NULL),0,1);
	sleep(1);
	insereCliente(normais,"este2",1,time(NULL),0,0);
	sleep(1);
	insereCliente(comp,"esteC2",1,time(NULL),0,1);
	sleep(1);
	insereCliente(normais,"este3",1,time(NULL),0,0);
	sleep(1);
	insereCliente(comp,"esteC3",1,time(NULL),0,1);

	request r=comprimido();
	printf("%s\n",r->pedido);

	 r=comprimido();
	printf("%s\n",r->pedido);

	r=comprimido();
	printf("%s\n",r->pedido);

	r=comprimido();
	printf("%s\n",r->pedido);

	r=comprimido();
	printf("%s\n",r->pedido);

	r=comprimido();
	printf("%s\n",r->pedido);

    }



	/*request r= eliminaCliente(comp);
     buffer node2=normais;
		while(node2->prox!=NULL){ 
			node2=node2->prox;
        	printf("%s\n",node2->pedido);
    }

    buffer aux = proxCliente(normais);
    printf("Prox cliente:%s\n",aux->pedido);
    request r1= eliminaCliente(normais);
    node2=normais;
		while(node2->prox!=NULL){ 
			node2=node2->prox;
        	printf("%s\n",node2->pedido);
    }
     node2=comp;
		while(node2->prox!=NULL){ 
			node2=node2->prox;
        	printf("%s\n",node2->pedido);
    }*/
}

request fif(){
   
    request r;
     
    buffer  proxD=proxCliente(comp);
    buffer  proxN=proxCliente(normais);
   
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



request normal(){
  
    request r;
    if(!lista_vazia(normais)){
    	
    	
    	r=eliminaCliente(normais);
    	
    }
    else{
    	
    	
		r=eliminaCliente(comp);
	}

    

   return r;

    
}


request comprimido(){
    
    request r;
    if(!lista_vazia(comp)){
    	 
    	r=eliminaCliente(comp);

   }
    else{
    	
		r=eliminaCliente(normais);
	}

    

   return r;
}
