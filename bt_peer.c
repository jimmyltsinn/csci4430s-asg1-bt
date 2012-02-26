#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

void show_help() {
    printf("The following commands are avalible: \n");
    printf("\tadd [filename]    \tAdd a new download job\n");
    printf("\tseed [filename]   \tAdd a new seed job\n");
    printf("\tsubseed [filename]\tAdd a subseed job\n");
    printf("\tstop              \tStop the current job\n");
    printf("\tresume            \tResume a stopped job\n");
    printf("\tprogress          \tShow the progress of current downloading job\n");
    printf("\tpeer              \tPrint the IP address and port of peers of current downloading job\n");
    printf("\thelp              \tShow this manual\n");
    printf("\texit              \tExit\n");
    return; 
}

char input_str[][9] = { "add", "seed", "subseed", 
                        "stop", "resume", "progress", 
                        "peer", "help", "exit"
                      };

int main(int argc, char **argv) {
    char status[20];
    int mode = 0;

    if (argc > 1) {
        printf("Usage: %s\n", argv[0]);
        exit(0);
    }
    
    printf(" == BT Peer client == \n");
    show_help();
    strcpy(status, "Idle");
    
    while (1) {
        char *cmd[2], input[127];
        int ipt = 0, i = 0;
        printf("[%s] >> ", status);
        fflush(stdout);

        i = read(fileno(stdin), input, 126);

        if (i <= 0) {
            printf("\n");
            goto out;
        }

        if (cmd[0] = strstr(input, "\n"))
            cmd[0][0] = '\0';

        cmd[0] = strtok(input, " ");
        printf("Input argument 1: %s\n", cmd[0]);
        cmd[1] = strtok(NULL, " ");
        printf("Input argument 2: %s\n", cmd[1]);
         
        if (!cmd[0]) {
            printf("Please enter an valid command. \n");
            continue;
        }
        
        i = 0;

        while (!ipt) {
            if (i > 9) break;
            printf("%s vs %s\n", cmd[0], input_str[i]);
            if (!strcmp(cmd[0], input_str[i])) {
                ipt = i + 1;
                break;
            }
            ++i;
        }

        if (!ipt) {
            printf("Please enter an valid command. \n");
            continue;
        }

        if ((ipt > 3) && (cmd[1])) {
            printf("This command do not require further argument. \n");
            continue;
        } else if ((ipt <= 3) && (!cmd[1])) {
            printf("This command require .torrent file name as argument. \n");
            continue;
        }
        
        switch (ipt) {
            case 1: 
                printf("Add ... \n");
                break;
            case 2:
                printf("Seed ... \n");
                break;
            case 3:
                printf("Subseed ... \n");
                break;
            case 4:
                printf("Stop ... \n");
                break;
            case 5:
                printf("Resume ... \n");
                break;
            case 6:
                printf("Progress ... \n");
                break;
            case 7: 
                printf("Peer ... \n");
                break;
            case 8: 
                printf("Help ... \n");
                show_help();
                break;
            case 9: 
                printf("Exit ... \n");
                goto out;
            default: 
                printf("Something goes wrong ... \n");
        }
    }

out:
    printf("Bye. \n");
    return 0;
}
