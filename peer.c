#include "peer.h"

void help() {
    printf("The following commands are avalible: \n");
    if (!mode) {
        printf("\tadd [filename]    \tAdd a new download job\n");
        printf("\tseed [filename]   \tAdd a new seed job\n");
        printf("\tsubseed [filename]\tAdd a subseed job\n");
    }
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

void progress() {
    int i, j;
    double progress;

    for (i = 0; i < nchunk; ++i) 
        if (bit_get(filebitmap, i))
            ++j;
    
    progress = j / (nchunk + 1);

    printf("[Progress] %f completed. ", progress);

    return;
}

int main(int argc, char **argv) {
    char status[20], status_backup[20];
    char ip[16];
    pthread_t tmp;

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

    help();
    strcpy(status, "Idle");
    
    pthread_create(&tmp, NULL, thread_listen, NULL); 
    
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

        if ((ipt <= 3) && (mode)) {
            printf("You have already registered a torrent file and assign job. \n");
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
                if (add_job(cmd[1])) continue;
                bitc_set(mode, 1);
                bitc_set(mode, 2);
                if (add_job(cmd[1])) {
                    mode = 0;
                    continue;
                }
                strcpy(status, "Download");
                memset(filebitmap, 0, (nchunk + 8) >> 3);
                start();
                break;
            case 2:
                printf("Seed ... \n");
                if (add_job(cmd[1])) continue;
                bitc_set(mode, 2);
                if (add_job(cmd[1])) {
                    mode = 0;
                    continue;
                }
                strcpy(status, "Seed");
                memset(filebitmap, 0, (nchunk + 8) >> 3);
                start();
                break;
            case 3:
                printf("Subseed ... \n");
                if (add_job(cmd[1])) continue;
                bitc_set(mode, 2);
                bitc_set(mode, 3);
                if (add_job(cmd[1])) {
                    mode = 0;
                    continue;
                }
                subseed_promt(cmd[1]); 
                strcpy(status, "Subseed");
                start();
                break;
            case 4:
                printf("Stop ... \n");
                strcpy(status_backup, status);
                strcpy(status, "Pause");
                stop();
                break;
            case 5:
                printf("Resume ... \n");
                strcpy(status, status_backup);
                start();
                break;
            case 6:
                printf("Progress ... \n");
                progress();
                break;
            case 7: 
                printf("Peer ... \n");
                list();
                break;
            case 8: 
                printf("Help ... \n");
                help();
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
