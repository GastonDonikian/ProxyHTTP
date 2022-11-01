#include "include/mcpUtils.h"

#include "include/proxyClient.h"

#include "include/shaUtils.h"

#include "include/metrics.h"

int setupMCPSocket(struct httpargs * http_args){
    int opt = true, ipSocket;

    if((ipSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0){
        logError("Error creando el socket MCP");
        return -1;
    }

    if (setsockopt(ipSocket, SOL_SOCKET, SO_REUSEADDR, (char *) &opt, sizeof(opt)) < 0) {
        logError("Error al ponerle opciones al socket MCP");
        return -1;
    }

    struct sockaddr_in ipAddress;

    //limpio mi dirección ip
    memset(&ipAddress, 0, sizeof(ipAddress));

    ipAddress.sin_family = AF_INET;
    ipAddress.sin_addr.s_addr = inet_addr(http_args->mng_addr);
    ipAddress.sin_port = htons(http_args->mng_port);

    //conecto mi socket con la dirección
    if (bind(ipSocket, (struct sockaddr *) &ipAddress, sizeof(ipAddress)) < 0) {
        logError("Error en el bind de MCP");
        close(ipSocket);
        return -1;
    }

    fcntl(ipSocket, F_SETFL, O_RDWR|O_NONBLOCK);

    return ipSocket;
}

static int simpleMCPRead(int fromSocket , uint8_t *buffer ,  struct sockaddr * addr , socklen_t * addr_len ){

    ssize_t len = recvfrom(fromSocket, buffer, sizeof(mcp_package), MSG_DONTWAIT, addr, addr_len);

    if(len < 0 ){
        logError("Error leyendo el mensaje MCP\n");
        return -1;
    }else if (sizeof(mcp_package) != len){
        logError("No me llego entero");
        return -2;
    }else{
        return len;
    }
}

/* static void print_mcp(mcp_package package){
	printf("Passphrase:");
    
	for(int i = 0 ; i < 32 ; i++){
		printf("%d", (uint8_t)package.passphrase[i]);
	}
	printf("\n");

	printf("Is_response: %d\n", package.is_response);
	printf("Type: %d\n", package.type);
	printf("Body: %ld\n", package.body);

} */

static int parseAndExecuteRequest(mcp_package *package){

    if(package->is_response == true){
        logError("Error: esperaba un pedido");
        return -1;
    }

    package->is_response = true;   
    
    bool authCheck = checkAuth(package->passphrase);

    if(authCheck == false){
        puts("Error: credenciales incorrectas");
        getPassphrase((char *)package->passphrase);
        package->type = AUTH_ERROR;
        package->body = 0;
        return -1;
    }   

    getPassphrase((char *)package->passphrase); //Quiero mi contra
 
    uint8_t type = package->type;
    
    switch(type){
        case GET_HISTORICAL_CONNECTIONS:
        package->type = OK;
        package->body = getMetric(HISTORICAL_CONNECTIONS);
        break;
        case GET_CONCURRENT_CONNECTIONS:
        
        package->type = OK;
        package->body = getMetric(CONCURRENT_CONNECTIONS);


        break;
        case GET_TOTAL_BYTES_SENT:

        package->type = OK;
        package->body = getMetric(SENT_BYTES);
        
        break;
        case SET_READ_BUFFER_SIZE:

        package->type = OK;        
        if(setReadBufferSize(package->body) < 0)
            package->type = SERVER_ERROR;
        package->body = 0;
        
        break;
        default:

        package->type = INVALID_REQUEST_ERROR;
        package->body = 0;

        logError("Error: pedido MCP inválido");
        return -1;

        break;
    }
    
    return 1;
}


int handleMCPRequest(int mcpSocket ){
    struct sockaddr_storage addr;
    socklen_t len = sizeof(addr);

    mcp_package package;
    memset(&package , 0 , sizeof(mcp_package));

    if (( simpleMCPRead(mcpSocket , (uint8_t *)&package ,(struct sockaddr *)&addr, &len) )< 0 ){
        printf("paquete droppeado see you in hell\n");
        return -1;
    }
    if (parseAndExecuteRequest(&package) >= 0 ){
        sendto(mcpSocket, &package, sizeof(mcp_package), MSG_DONTWAIT,(struct sockaddr *)&addr , len);
        return 1;        
    }
    
    return -1;
}

