#include "peer.h"

const char commands[][CMD_TYPE] = { "down", "add", "seed", "subseed", 
                                    "info", "stop", "resume", "progress", 
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

void init() {
    int i;

    mode = 0;
    fileid = 0;
    filesize = 0;
    filename = NULL;
    nchunk = 0;
    filebitmap = NULL;
    bitmap_size = 0;
    dling_peer = 0;
    peers_freq = NULL;
    filefd = -1;

    tracker_ip.s_addr = 0;
    tracker_port = 0;
    local_ip.s_addr = 0;
    local_port = 0;

    pthread_mutex_init(&mutex_filebm, NULL);
    pthread_mutex_init(&mutex_peer, NULL);
    pthread_mutex_init(&mutex_dling, NULL);
    pthread_mutex_init(&mutex_filefd, NULL);

    filename = NULL;

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
    
    pthread_create(&tmp, NULL, (void * (*) (void *)) thread_listen, NULL); 
    
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
            if (i > CMD_TYPE) break;
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

        if ((ipt > 4) && (!mode) && (ipt < 10)) {
            printf("Please first register a torrent file. \n");
            continue;
        }

        if ((ipt <= 4) && (mode)) {
            printf("You have already registered a torrent file and assign job. \n");
            continue;
        }

        if ((ipt > 4) && (cmd[1])) {
            printf("This command do not require further argument. \n");
            continue;
        } else if ((ipt <= 4) && (!cmd[1])) {
            printf("This command require .torrent file name as argument. \n");
            continue;
        }

        switch (ipt) {
            case 1: 
                printf("Down ...\n");
                if (reg_torrent(cmd[1])) 
                    continue;
                bitc_set(mode, 1);
                filefd_init();
                bitmap_init();
                memset(filebitmap, 0x00, bitmap_size);
                strcpy(status, "D");
                start();
                break;
            case 2: 
                printf("Add ... \n");
                if (reg_torrent(cmd[1]))
                    continue;
                bitc_set(mode, 1);
                bitc_set(mode, 2);
                filefd_init();
                bitmap_init();
                memset(filebitmap, 0x00, bitmap_size);
                strcpy(status, "D/U");
                start();
                break;
            case 3:
                printf("Seed ... \n");
                if (reg_torrent(cmd[1]))
                    continue;
                bitc_set(mode, 2);
                filefd_init();
                bitmap_init();
                memset(filebitmap, 0xff, bitmap_size);
                strcpy(status, "U");
                start();
                break;
            case 4: 
                printf("Subseed ... \n");
                if (reg_torrent(cmd[1]))
                    continue;
                bitc_set(mode, 2);
                filefd_init();
                bitmap_init();
                subseed_promt(cmd[1]);
                strcpy(status, "sU");
                start();
            case 5:
                printf("Info ...\n");
                info();
                break; 
            case 6:
                printf("Stop ... \n");
                strcpy(status_backup, status);
                strcpy(status, "P");
                stop();
                break;
            case 7:
                printf("Resume ... \n");
                strcpy(status, status_backup);
                start();
                break;
            case 8:
                printf("Progress ... \n");
                progress();
                break;
            case 9: 
                printf("Peer ... \n");
                list();
                break;
            case 10:
                printf("Help ... \n");
                help();
                break;
            case 11:
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
