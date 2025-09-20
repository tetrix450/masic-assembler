#include <stdio.h>
#include <string.h>

unsigned long hash(unsigned char *str) {
    unsigned long hash = 5381;
    int c;

    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;  // hash * 33 + c
    }
    
    return hash;
}

int has_duplicate(unsigned long list[], int length){
    for(int i = 0; i < length; i++){
        unsigned long element1 = list[i];
        for(int j = 0; j < length; j++){
            unsigned long element2 = list[j];
            if(element1 == element2 && i != j){
                return i*1000 + j;
            }
        }
    }

    return 0;
}

char keyword[] = "JMP CLC STC CLI STI HLT INC DEC LOAD STORE "
"ADD ADCSUB AND OR NOT NEG CMP NOP JO JNO JZ JE JNZ JNE JNAE JB JAE JNB JBE JNA "
"JC JNC JA JNBE JS JNS SHL SHR ROL ROR RCL RCR PUSH POP CALL RET "
"INT IRET RETI LDSP LOAZ SIGNED UNSIGNED";

int main(){

    FILE* file = fopen("out.txt","w");
    if (file == NULL) {
        printf("Error opening the file.\n");
        return 1;
    }

    char* token;
    // Get the first token
    token = strtok(keyword, " \n");
    int hash_list_length = 53;
    unsigned long hash_list[hash_list_length];

    // Continue getting tokens while strtok returns non-NULL
    int i = 0;
    while (token != NULL) {
        hash_list[i] = hash(token);

        fprintf(file,"#define HASH_%s %u\n",token,hash_list[i]);

        printf("%d: Token: %s, Hash: %u\n", i, token, hash_list[i]);
        token = strtok(NULL, " \n");  // Get the next token
        i++;
    }

    printf("Collision detected? %d\n", has_duplicate(hash_list, hash_list_length));
    fclose(file);

    return 0;
}