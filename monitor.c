#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>

#define USER_LEN 32
#define INDICIU_LENGTH 128
#define CMD_FILE "cmd.txt"
#define buffer_length 256

typedef struct {
    int treasure_id;
    char username[USER_LEN];
    double latitude;
    double longitude;
    char hint[INDICIU_LENGTH];
    int value;
} Treasure_t;


//functie pentru a afisa o comoara
void view_treasure(const char *hunt_id, int treasure_id) 
{
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/treasures.bin", hunt_id);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) 
    {
        printf("Eroare la deschiderea fisierului de comori\n");
        return;
    }

    Treasure_t treasure;
    int gasit = 0;
    char result[1024]={0};

    while (read(fd, &treasure, sizeof(Treasure_t)) == sizeof(Treasure_t)) {
        if (treasure.treasure_id == treasure_id) {
            char temp[512];
            snprintf(temp, sizeof(temp), "ID: %d\nUsername: %s\nLat: %.6lf\nLongit: %.6lf\nHint: %s\nValoare: %d\n",
                   treasure.treasure_id, treasure.username, treasure.latitude,
                   treasure.longitude, treasure.hint, treasure.value);
            strcat(result, temp);
            gasit = 1;
            break;
        }
    }

    close(fd);
    // Verificam daca am gasit comoara
    if (!gasit) 
    {
        printf("Comoara cu ID-ul %d nu a fost gasita\n", treasure_id);
    } else 
    {
        printf("%s", result);
    }
}

//functie pentru afisarea comorilor dintr-un hunt
void list_treasures(const char *hunt_id) 
{
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/treasures.bin", hunt_id);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) 
    {
        printf("Eroare la deschidere fisier de comori\n");
        return;
    }

    char result[4096] = {0};
    char temp[512];
    
    snprintf(temp, sizeof(temp), "Treasure hunt: %s\n\nLista comori:\n", hunt_id);
    strcat(result, temp);
    strcat(result, "---------------------------------------------------------------\n");

    Treasure_t treasure;
    while (read(fd, &treasure, sizeof(Treasure_t)) == sizeof(Treasure_t)) 
    {
        snprintf(temp, sizeof(temp), "ID: %d | Username: %s | Lat: %.6lf | Long: %.6lf | Hint: %s | Value: %d\n",
               treasure.treasure_id, treasure.username, treasure.latitude,
               treasure.longitude, treasure.hint, treasure.value);
        strcat(result, temp);
    }

    close(fd);
    printf("%s", result);
}

//functie pentru afisarea hunt-urilor si numarului de comori din fiecare hunt
void list_hunts() 
{
    // deschidem directorul curent si verificam daca s-a deschis
    DIR *dir = opendir(".");
    if (dir == NULL) 
    {
        printf("Eroare la deschiderea directorului curent\n");
        return;
    }

    char result[4096] = {0};
    strcat(result, "Lista hunt-uri disponibile:\n");
    strcat(result, "--------------------------------\n");

    int gasit = 0; //cu variabila gasit verificam daca am gasit minim un hunt 
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) 
    {
        // verificam daca e director
        if (strcmp(entry->d_name,".") && strcmp(entry->d_name,"..")) 
        {
            struct stat st;
            if (stat(entry->d_name, &st) == 0 && S_ISDIR(st.st_mode)) 
            {
                char treasure_file[buffer_length];
                snprintf(treasure_file, buffer_length, "%s/treasures.bin", entry->d_name);
                // verificam daca are fisierul de comori
                int fd = open(treasure_file, O_RDONLY);
                if (fd != -1) 
                {
                    struct stat file_stat;
                    fstat(fd, &file_stat);
                    int count = file_stat.st_size/sizeof(Treasure_t);
                    char temp[256];
                    snprintf(temp, sizeof(temp), "- %s (%d comori)\n", entry->d_name, count);
                    strcat(result, temp);
                    close(fd);
                } 
                else 
                {
                    char temp[256];
                    snprintf(temp, sizeof(temp), "- %s (0 comori)\n", entry->d_name);
                    strcat(result, temp);
                }
                gasit = 1; // am gasit minim un hunt
            }
        }
    }
    closedir(dir);

    //daca nu sunt hunt-uri, afisam mesaj
    if (!gasit) 
    {
        strcat(result, "Nu exista hunt-uri disponibile\nIntrodu alta comanda:\n");
    }
    
    printf("%s", result);
}

void monitor_semnal(int semnal) 
{
    char buffer[buffer_length] = {0};
    int fd = open(CMD_FILE, O_RDONLY);
    if (fd == -1) 
    {
        printf("Eroare la deschiderea fisierului de comenzi\n");
        return;
    }
    // citim comanda din fisier
    if (fd >= 0) 
    {
        read(fd, buffer, buffer_length - 1);
        close(fd);
    }

    char cmd_copy[buffer_length];
    strncpy(cmd_copy, buffer, buffer_length);
    //spargem comanda in cuvinte
    char *cmd = strtok(buffer, " \n");
    char *arg1 = strtok(NULL, " \n");
    char *arg2 = strtok(NULL, " \n");

    if (cmd == NULL) return;

    if (strcmp(cmd, "list_hunts") == 0) 
    {
        list_hunts();
    } 
    else if (strcmp(cmd, "list_treasures") == 0 && arg1) 
    {
        list_treasures(arg1);
    } 
    else if (strcmp(cmd, "view_treasure") == 0 && arg1 && arg2) 
    {
        view_treasure(arg1, atoi(arg2));
    } 
    else if (strcmp(cmd, "stop") == 0) 
    {
        printf("Monitorul se opreste...\n");
        fflush(stdout); 
        usleep(1000000);
        exit(0);
    } 
    else 
    {
        printf("Comanda necunoscuta: %s\n", cmd_copy);
    }
    fflush(stdout);
}

int main(int argc, char *argv[]) 
{

    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = monitor_semnal;
    sigaction(SIGUSR1, &sa, NULL);

    printf("\n----- Comenzi disponibile -----\n");
    printf("1) list_hunts\n");
    printf("2) list_treasures <hunt_id>\n");
    printf("3) view_treasure <hunt_id> <treasure_id>\n");
    printf("4) calculate_score\n");    
    printf("5) stop_monitor\n");
    printf("------------------------------------\n");
    fflush(stdout);
  

    while (1) 
    {
        pause();
    }

    return 0;
}
