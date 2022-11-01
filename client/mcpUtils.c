#include "include/mcpUtils.h"

//Arma un socket IPv4 para comunicarse con el proxy
int setupRemoteSocket(struct mcpargs *mcp_args , struct sockaddr_in * servaddr){

    int sockfd;
    if((sockfd = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        puts("Error creando el socket\n");
        return -1;
    }

    // Limpio la dirección
    bzero(servaddr, sizeof(struct sockaddr_in));

    //Ajusto opciones
    servaddr->sin_addr.s_addr = inet_addr(mcp_args->addr);
    servaddr->sin_port = htons(atoi(mcp_args->port));
    servaddr->sin_family = AF_INET;


	//conecto mi socket con la dirección
    if (connect(sockfd, (struct sockaddr *)servaddr, sizeof(*servaddr)) < 0) {
        puts("Error en el bind de MCP");
        close(sockfd);
        return -1;
    } 
	return sockfd;
	
}


static int simpleMCPRead(int fromSocket , uint8_t *buffer ,  struct sockaddr * addr , socklen_t * addr_len ){
    
    ssize_t len = recvfrom(fromSocket, buffer, sizeof(mcp_package), MSG_WAITALL, addr, addr_len);

    if(len < 0 ){
		printf("size: %ld\n", sizeof(mcp_package));
        puts("Error leyendo el mensaje MCP");
        return -1;
    }else if (sizeof(mcp_package) != len){
        puts("No me llego entero");
        return -2;
    }else{
        return len;
    }
}

extern struct mcpargs *mcp_args;

get_command get_commands[] = {
        [0]={"history", "Muestra el número de conexiones históricas al proxy", getHistoricalConnections}, 
        [1]={"concurrent", "Muestra el número de conexiones concurrentes al proxy", getConcurrentConnections}, 
        [2]={"bytes", "Muestra el número total de bytes que pasaron por el proxy", getTotalBytesSent}, 
        [3]={"", "", NULL},
};

set_command set_commands[] = {
    [0]={"readbuffer", "Cambia el tamaño del buffer de lectura del proxy", setReadBufferSize},
    [1]={"","",NULL},
};

static void print_mcp(mcp_package package){
	printf("Passphrase:");
	for(int i = 0 ; i < 32 ; i++){
		printf("%d", (uint8_t)package.passphrase[i]);
	}
	printf("\n");

	printf("Is_response: %d\n", package.is_response);
	printf("Type: %d\n", package.type);
	printf("Body: %ld\n", package.body);

}

static int parseResponse(mcp_package *response){

    if(response->is_response == false){
        puts("Error: esperaba una respuesta");
        return -1;
    }

    uint8_t code = response->type;

    switch(code){
        case OK:
            return checkAuth(response->passphrase);
        break;
        case AUTH_ERROR:
            puts("Error: El servidor no reconoció la clave");
        break;
        case INVALID_REQUEST_ERROR:
            puts("Error: El pedido no tiene un formato válido");
        break;
        case SERVER_ERROR:
            puts("Error: Hubo un error en el proxy");
        break;
    }
    return -1;
}

static int sendAndReceiveQuery(mcp_package * package){

    struct sockaddr_in addr;
    socklen_t len = sizeof(addr);

    int serverSocket = setupRemoteSocket(mcp_args , &addr);   

	int bytesSent = sendto(serverSocket, (uint8_t * )package, sizeof(mcp_package), 0, (struct sockaddr *)&addr, len);

	if(bytesSent < 0){
		puts("Error enviando al proxy");
		return -1;
	}


	struct sockaddr_in addr_read;
    socklen_t len_read = sizeof(addr_read);
    
    int readBytes; 
    if (( readBytes = simpleMCPRead(serverSocket , (uint8_t * ) package ,(struct sockaddr *)&addr_read, &len_read) ) < 0 ){
        puts("paquete droppeado see you in hell");
        return -1;
    }

	return readBytes;
}

//Informa sobre las funciones del cliente
void help(){
    
    int i = 0;
    get_command get = get_commands[i];
    set_command set = set_commands[i];

    puts("Monitoreo:");        
        

    do
    {
        get = get_commands[i];
        printf("\t%s: %s\n", get.name, get.desc);
        i++;
    } while (get_commands[i].command != NULL);
      
    i = 0;
    puts("Configuración: ");
    while(set.command != NULL){
        set = set_commands[i];
        printf("\t%s <ARG>: %s\n", set.name, set.desc);
        i++;
    }
}

//De existir, ejecuta el comando
int executeCommand(char *command_name, char *arg){

    if(!strcmp(command_name, "help")){
        help();
        return 1;
    }

    get_command get;
    for(int i = 0 ; get_commands[i].command != NULL ; i++){
        get = get_commands[i];

        if(!strcmp(get.name, command_name)){
            return (*get.command)(); //Ejecuto el comando
        }
    }

    set_command set;
    for(int i = 0 ; set_commands[i].command != NULL ; i++){
        set = set_commands[i];

        if(!strcmp(set.name, command_name)){
            return (*set.command)(arg); //Ejecuto el comando
        }
    }

    return -2;
}

uint64_t getHistoricalConnections(){

    mcp_package *package;
    package = malloc(sizeof(mcp_package));

    getPassphrase(package->passphrase);
    package->is_response=0;
    package->type=GET_HISTORICAL_CONNECTIONS;
    package->body=(uint64_t)0;

	int readBytes;

    //Guarda la respuesta en el mismo paquete
	if((readBytes = sendAndReceiveQuery(package)) < 0){
        puts("Error enviando paquete");
		return -1;
	}

    int result = parseResponse(package);

    if(result > 0){
        printf("Hubieron %ld conexiones en la historia del proxy\n", package->body);
    }

    return result;
}

uint64_t getConcurrentConnections(){

	mcp_package *package;
    package = malloc(sizeof(mcp_package));

    
    getPassphrase(package->passphrase);
    package->is_response=0;
    package->type=GET_CONCURRENT_CONNECTIONS;
    package->body=(uint64_t)0;

	int readBytes;

	if((readBytes = sendAndReceiveQuery(package)) < 0){
        puts("Error enviando paquete");
		return -1;
	}

   int result = parseResponse(package);

    if(result > 0){
        printf("Hay %ld conexiones concurrentes\n", package->body);
    }

    return result;
}

uint64_t getTotalBytesSent(){

	mcp_package *package;
    package = malloc(sizeof(mcp_package));

    getPassphrase(package->passphrase);
    package->is_response=0;
    package->type=GET_TOTAL_BYTES_SENT;
    package->body=(uint64_t)0;

	int readBytes;

	if((readBytes = sendAndReceiveQuery(package)) < 0){
        puts("Error enviando paquete");
		return -1;
	}

    int result = parseResponse(package);

    if(result > 0){
        printf("Se enviaron %ld bytes\n", package->body);
    }

    return result;
}

uint8_t setReadBufferSize(char *buffSize){
	mcp_package *package;
    package = malloc(sizeof(mcp_package));

	char *endptr;

    getPassphrase(package->passphrase);
    package->is_response=0;
    package->type=SET_READ_BUFFER_SIZE;
    package->body=(uint64_t)strtol(buffSize, &endptr, 10);

	int readBytes;

	if(endptr != NULL && (readBytes = sendAndReceiveQuery(package)) < 0){
        puts("Error enviando paquete");
		return -1;
	}

    int result = parseResponse(package);

    if(result > 0){
        printf("Éxito modificando el buffer de lectura\n");
    }

    return result;
}

