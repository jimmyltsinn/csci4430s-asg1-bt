#include "peer.h"

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

static char commands[][9] = { "add", "seed", "subseed", 
                              "stop", "resume", "progress", 
                              "peer", "help", "exit"
                            };

int init() {
    fileid = 0;
    filesize = 0;
    nchunk = 0;
    mode = 0;
    filebitmap = NULL;
    filefd = 0;

    tracker_ip.s_addr = 0;
    tracker_port = 0;
    local_ip.s_addr = 0;
    local_port = 0;

    pthread_mutex_init(&mutex_finished, NULL);
    pthread_mutex_init(&mutex_downloading, NULL);
    pthread_mutex_init(&mutex_peers, NULL);

    filename = NULL;

    return 0;
}

void getlocalsetting() {
    char input[18];

    printf("Please input your IP address: ");
    fgets(input, 18, stdin);
    fflush(stdin);
    inet_aton(input, &local_ip);

    printf("Please input the listening port: ");
    fgets(input, 18, stdin);
    fflush(stdin);
    local_port = htons(atoi(input));

    return;
}

void info() {
    printf("Local IP: %s:%d\n", inet_ntoa(local_ip), ntohs(local_port));
    printf("Tracker IP: %s:%d\n", inet_ntoa(tracker_ip), ntohs(tracker_port));

    printf("\n");

    printf("File ID: 0x%x\n", fileid);
    printf("File name: %s\n", filename ? filename : "(Unknown)");
    printf("File size: %d\n", filesize);
    printf("Number of chunk: %d\n", nchunk);

    return;
}

int main(int argc, char **argv) {
    char status[20];
    char ip[16];

    if (argc != 1 && argc != 3) {
        printf("Usage: %s [localIP listenPort]\n", argv[0]);
        exit(0);
    }
    
    printf(" == BT Peer client == \n");
    fflush(stdin);
   
    init();
    
    if (argc < 3) {
        getlocalsetting();
    } else {
        inet_aton(argv[1], &local_ip);
        local_port = htons(atoi(argv[2]));
    }

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
        cmd[1] = strtok(NULL, " ");
         
        if (!cmd[0]) {
            printf("Please enter an valid command. \n");
            continue;
        }
        
        i = 0;

        while (!ipt) {
            if (i > 9) break;
            if (!strcmp(cmd[0], commands[i])) {
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
                mode = 1;
                strcpy(status, "Download");
                read_torrent(cmd[1]);
                break;
            case 2:
                printf("Seed ... \n");
                mode = 2;
                strcpy(status, "Seed");
                read_torrent(cmd[1]);
                break;
            case 3:
                printf("Subseed ... \n");
                mode = 3;
                strcpy(status, "Subseed");
                read_torrent(cmd[1]);
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
        info();
    }

out:
    printf("Bye. \n");
    return 0;
}
