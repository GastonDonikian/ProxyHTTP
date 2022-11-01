#include "include/mcpUtils.h"

struct mcpargs *mcp_args;

int main(int argc, char **argv){
    
    mcp_args = malloc(sizeof(mcp_args));
    parse_args(argc, argv, mcp_args);

    
    char command[COMMAND_BUFFER_SIZE];
    char *arg;
    int command_found;

    while(true){
        //preparo los args
        fgets(command, COMMAND_BUFFER_SIZE, stdin);
        strtok(command," \n");
        arg = strtok(NULL, "\n");

        if((command_found = executeCommand(command, arg)) == -2){
            puts("Comando inválido: usar help para ver más opciones");
        }
        else if(command_found < 0){
            puts("Algo salió mal"); //TODO: mensaje de error
        }

    }
    

}