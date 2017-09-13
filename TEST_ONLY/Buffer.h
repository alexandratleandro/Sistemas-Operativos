#define SIZE_BUF    1024

//buffer
typedef struct List *buffer;
typedef struct List{
    int tipo;
    char pedido[SIZE_BUF];
    int n_socket;
    time_t inicio;
    time_t fim;
    buffer prox;
    
} List_buff;


//estrutura requests de threads
typedef struct{
    int tipo;
    char  pedido[SIZE_BUF];
    int n_socket;
    time_t inicio;
    time_t fim;
}infoThread;

typedef infoThread* request;

//*******************+BUFFER FUNCOES**************************+//

buffer criaBuffer(){
    buffer  aux;
    aux = (buffer)malloc(sizeof(List_buff)); 
    
    if (aux != NULL) { 
        aux->tipo=-1;
        memset(aux->pedido, 0, sizeof(aux->pedido));
        aux->n_socket = 0;
        aux->inicio=0;
        aux->fim=0;
        aux->prox = NULL;
    }

    #if DEBUG
        printf("Criado Buffer\n");
    #endif

    return aux;
}
//falta meter tempo 
void insereCliente(buffer lista, char pedido[], int n_socket,time_t inicio,time_t fim,int tipo){

    buffer node= (buffer)malloc(sizeof(List_buff)) ;
    buffer node2 = lista;

    if(node != NULL){ 
        node->tipo=tipo;
        strcpy(node->pedido, pedido);
        node->n_socket=n_socket;
        node->inicio=inicio;
        node->fim=fim;
        node->prox = NULL;
    }

    while(node2->prox!=NULL){ 
        node2 = node2->prox;
    }
    node2->prox = node;



    #if DEBUG
        printf("Inserido Cliente\n");
    #endif 

}

int lista_vazia( buffer  lista){
    return (lista->prox == NULL ? 1 : 0);
}
//List aux5=listaAmar->prox;
request eliminaCliente ( buffer lista){
    request r;
     if(lista_vazia(lista)){
        return NULL;
    } 
      buffer  aux = lista->prox;
    r= (request)malloc(sizeof(infoThread));
    if(r!=NULL){
        r->tipo=aux->tipo;
        strcpy(r->pedido,aux->pedido);
        r->n_socket=aux->n_socket;
        r->inicio=aux->inicio;
        r->fim=aux->fim;
    }
    lista->prox=aux->prox;

    free(aux);
    return r;

     #if DEBUG
        printf("Cliente eliminado\n");
    #endif

    return 0;
}

 buffer  proxCliente(buffer lista){

    if(lista_vazia(lista)){
        return NULL;
    }
    else
        return lista->prox;
}


  buffer  destroi_lista(buffer  lista){
   buffer temp_ptr;

    while (lista_vazia(lista) == 0) { 
        temp_ptr = lista;
        lista= lista->prox;
        free(temp_ptr);
    }
    free(lista);
     #if DEBUG
        printf("Lista destruida\n");
    #endif
    return NULL;
}



//*****************************************************************************//


