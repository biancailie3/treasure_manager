#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <errno.h>
#include <dirent.h>

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
}Treasure_t;

pid_t monitor_pid = -1;


// Handler pentru SIGCHLD - detecteaza terminarea procesului monitor
void sigchld_handler(int sig) 
{
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
//verificam daca copilul e procesul care se termina si e monitorul
    if (pid == -1) 
    {
        perror("Eroare la waitpid\n");
        return;
    }
    else if (pid == 0) 
    {
        // nuu s-a terminat niciun proces copil
        return;
    }
    if (pid == monitor_pid) 
    {
        printf("Monitorul s-a terminat:\n");
        monitor_pid = -1;
    }
}

// Funcție pentru afiarea unei comori
void view_treasure(const char *hunt_id, int treasure_id) 
{
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/treasures.bin", hunt_id);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) 
    {
        perror("Eroare la deschiderea fisierului de comori\n");
        return;
    }

    Treasure_t treasure;
    int found = 0;

    while (read(fd, &treasure, sizeof(Treasure_t)) == sizeof(Treasure_t)) 
    {
        if (treasure.treasure_id == treasure_id) 
        {
            printf("ID: %d\nUsername: %s\nLatitude: %.6lf\nLongitude: %.6lf\nClue: %s\nValue: %d\n",
                   treasure.treasure_id, treasure.username, treasure.latitude,
                   treasure.longitude, treasure.hint, treasure.value);
            found = 1;
            break;
        }
    }

    close(fd);

    if (!found) 
    {
        printf("Comoara cu ID-ul %d nu a fost gasita\n", treasure_id);
    }
}

//functie pentru afisarea comorilor dintr -un hunt
void list_treasures(const char *hunt_id) 
{
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/treasures.bin", hunt_id);

    struct stat file_info;
    if (stat(file_path, &file_info) == -1) 
    {
        perror("Eroare obtinere info din fisier\n");
        return;
    }

    printf("Treasure hunt: %s\n", hunt_id);
    printf("\nLista comori:\n");
    printf("---------------------------------------------------------------\n");

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) 
    {
        perror("Eroare la deschidere fisier de comori");
        return;
    }

    Treasure_t treasure;
    while (read(fd, &treasure, sizeof(Treasure_t)) == sizeof(Treasure_t)) {
        printf("ID: %d | Username: %s | Lat: %.6lf | Long: %.6lf | Hint: %s | Value: %d\n",
               treasure.treasure_id, treasure.username, treasure.latitude,
               treasure.longitude, treasure.hint, treasure.value);
    }

    close(fd);
}
//functie pt afisare hunturi si numar de comori din fiecare hunt
void list_hunts() {
    DIR *dir = opendir(".");
    if (dir == NULL) {
        perror("Eroare la deschiderea directorului curent\n");
        return;
    }

    printf("Lista hunt-uri disponibile:\n");
    printf("-------------------------\n");

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) 
    {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) 
        {
            struct stat st;
            // verificam daca e director
            if (stat(entry->d_name, &st) == 0 && S_ISDIR(st.st_mode)) 
            {
                // verificam daca are fisierul de comori
                char treasure_file[buffer_length];
                snprintf(treasure_file, buffer_length, "%s/treasures.bin", entry->d_name);
                
                // numara comorile din fisier
                int fd = open(treasure_file, O_RDONLY);
                if (fd != -1) 
                {
                    // obtinem dimensiunea fisierului si numaram cate comori sunt
                    struct stat file_stat;
                    fstat(fd, &file_stat);
                    int count = file_stat.st_size / sizeof(Treasure_t);
                    printf("- %s (%d comori)\n", entry->d_name, count);
                    close(fd);
                }
                 else 
                {
                    // daca nu exista fisierul de comori, afisam 0
                    printf("- %s (0 comori)\n", entry->d_name);
                }
            }
        }
    }
    closedir(dir);
}

// Handler pentru SIGUSR1 - proceseaza comenzile trimise monitorului
void monitor_signal_handler(int sig) 
{
    // citim comanda din fisier
    char buffer[buffer_length] = {0};
    int fd = open(CMD_FILE, O_RDONLY);
    if (fd >= 0) 
    {
        read(fd, buffer, buffer_length-1);
        close(fd);
    }
    // verificam daca am citit ceva
    char cmd_copy[buffer_length];
    strncpy(cmd_copy, buffer, buffer_length);
    //spargem comanda in cuvinte
    char *cmd = strtok(buffer, " \n");
    char *arg1 = strtok(NULL, " \n");
    char *arg2 = strtok(NULL, " \n");

    if (cmd == NULL) return;
    // verificam ce comanda a fost primita
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
        usleep(3000000);  // 3 secunde întârziere
        exit(0);
    }
    else 
    {
        printf("Comanda necunoscuta: %s\n", cmd_copy);
    }
}

// functie pentru scrierea comenzilor in fisier
void write_command_to_file(const char *cmd) {
    int fd = open(CMD_FILE, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd >= 0) 
    {
        write(fd, cmd, strlen(cmd));
        close(fd);
    } 
    else 
    {
        perror("Eroare la scrierea in fisierul de comenzi\n");
    }
}
// functie pentru a rula monitorul
void run_monitor() 
{
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = monitor_signal_handler;
    sigaction(SIGUSR1, &sa, NULL);

    printf("Monitorul a pornit.\n");
    while (1) 
    {
        pause();  // asteapta comenzi
    }
}

int main() 
{
    char input[buffer_length];

    // Configurarea handler-ului pentru SIGCHLD
    struct sigaction sa_chld;
    memset(&sa_chld, 0, sizeof(sa_chld));
    sa_chld.sa_handler = sigchld_handler;
    sigaction(SIGCHLD, &sa_chld, NULL);

    while (1) 
    {
        printf("hub> ");
        fflush(stdout);
        int len = read(0, input, buffer_length - 1);
        if (len <= 0) continue;
        input[len - 1] = '\0';  // eliminam \n

        if (strcmp(input, "start_monitor") == 0) 
        {
            if (monitor_pid > 0) 
            {
                printf("Monitor deja pornit\n");
                fflush(stdout);
                continue;
            }

            pid_t pid = fork();
            if (pid == 0) 
            {
                run_monitor();  // copilul devine monitor
                exit(0); // in caz ca run_monitor() se termina
            }
            else if (pid > 0) 
            {
                monitor_pid = pid;
                printf("Monitor pornit\n");
                fflush(stdout);
            } 
            else 
            {
                perror("Eroare la fork\n");
            }
        }
        else if (strcmp(input, "list_hunts") == 0) 
        {
            if (monitor_pid <= 0) 
            {
                printf("Monitorul nu este pornit\n");
                fflush(stdout);
                continue;
            }
            write_command_to_file("list_hunts");
            kill(monitor_pid, SIGUSR1);
        } 
        else if (strncmp(input, "list_treasures ", 15) == 0) 
        {
            if (monitor_pid <= 0) 
            {
                printf("Monitorul nu este pornit\n");
                continue;
            }
            write_command_to_file(input);
            kill(monitor_pid, SIGUSR1);
        } 
        else if (strncmp(input, "view_treasure ", 14) == 0) 
        {
            if (monitor_pid <= 0) 
            {
                printf("Monitorul nu este pornit\n");
                continue;
            }
            write_command_to_file(input);
            kill(monitor_pid, SIGUSR1);
        } 
        else if (strcmp(input, "stop_monitor") == 0) 
        {
            if (monitor_pid <= 0) 
            {
                printf("Monitorul nu este pornit\n");
                continue;
            }
            write_command_to_file("stop");
            kill(monitor_pid, SIGUSR1);
            // nu face waitpid aici, pentru ca monitorul se va opri singur
        }
        else if (strcmp(input, "exit") == 0) 
        {
            if (monitor_pid > 0) 
            {
                printf("Opreste mai intai monitorul (stop_monitor)\n");
                continue;
            }
            break;
        } 
        else 
        {
            printf("Comanda necunoscuta\n");
        }
    }

    return 0;
}
