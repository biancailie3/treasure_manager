#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <dirent.h>
#include <sys/stat.h>

#define CMD_FILE "cmd.txt"
#define buffer_length 256

//monitor pid este -1 pentru a verifica daca monitorul este pornit sau nu
pid_t monitor_pid = -1;
int pipe_fd[2]; // Pipe pentru comunicare între monitor si hub

// Buffer pentru a citi date din pipe
char pipe_buffer[4096];

// handler pentru SIGCHLD, detecteaza terminarea procesului monitor
void sigchld_handler(int sig) 
{
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (pid == monitor_pid) 
    {
        printf("Monitorul s-a oprit.\n");
        monitor_pid = -1;
        close(pipe_fd[0]); // inchidem capatul de citire al pipe-ului
    }
}

//functie pentru scrierea comenzilor in fisier
void write_command_to_file(const char *cmd) 
{
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

// Functie pentru a executa comanda calculate_score pentru un hunt
void calculate_score_for_hunt(const char *hunt_id) 
{
    int score_pipe[2];
    if (pipe(score_pipe) == -1) 
    {
        perror("Eroare la crearea pipe-ului pentru scoruri");
        return;
    }

    pid_t pid = fork();
    if (pid == -1) 
    {
        perror("Eroare la fork pentru calculare scor\n");
        close(score_pipe[0]);
        close(score_pipe[1]);
        return;
    }

    if (pid == 0) 
    {
        // Proces copil
        close(score_pipe[0]); // inchidem capatul de citire
        dup2(score_pipe[1], STDOUT_FILENO);// redirectionam stdout catre pipe
        close(score_pipe[1]);

        // Executam programul calculate_score cu hunt_id ca argument
        execl("./calculate_score", "calculate_score", hunt_id, NULL);

        perror("Eroare la execl pentru calculate_score\n");
        exit(1);
    }
    else 
    {
        // Proces parinte: citim din pipe
        close(score_pipe[1]); // inchidem capatul de scriere

        char buffer[4096];
        ssize_t bytes_read;

        printf("Rezultate pentru %s:\n", hunt_id);
        while ((bytes_read = read(score_pipe[0], buffer, sizeof(buffer) - 1)) > 0) 
        {
            buffer[bytes_read] = '\0'; // terminator string
            printf("%s", buffer);
        }
        printf("\n");

        close(score_pipe[0]);
        waitpid(pid, NULL, 0); // asteptam incheierea copilului
    }
}

// functie pentru a calcula scorurile pentru toate hunt-urile
void calculate_all_scores() 
{
    DIR *dir = opendir(".");
    if (!dir) {
        perror("Eroare la deschiderea directorului curent\n");
        return;
    }

    struct dirent *entry;
    int found = 0;

    printf("Calcularea scorurilor pentru toate hunt-urile...\n");

    while ((entry = readdir(dir)) != NULL) 
    {
        // Sarim peste "." si ".."
        if (strcmp(entry->d_name, ".") == 0 || strcmp(entry->d_name, "..") == 0) 
            continue;

        // Verificam daca e director
        struct stat st;
        if (stat(entry->d_name, &st) == 0 && S_ISDIR(st.st_mode)) 
        {
            // Construim calea catre fisierul de comoara
            char treasure_path[256];
            snprintf(treasure_path, sizeof(treasure_path), "%s/treasures.bin", entry->d_name);

            // Verificam daca fisierul exista
            int fd = open(treasure_path, O_RDONLY);
            if (fd != -1) 
            {
                close(fd);
                found = 1;

                // Pentru a fi consistent, trimitem doar numele hunt-ului (directorul) catre calculate_score
                calculate_score_for_hunt(entry->d_name);
            }
        }
    }

    closedir(dir);

    if (!found) 
    {
        printf("Nu există hunt-uri disponibile pentru calcularea scorurilor.\n");
    }
}

// Functie pentru citirea rezultatelor din pipe-ul de la monitor
void read_from_monitor_pipe() 
{ //stergem continutul pipe_buffer (unde copiem ce scrie in pipe) ca sa fim siguri ca nu raman caractere nedorite
    memset(pipe_buffer, 0, sizeof(pipe_buffer));
    int bytes_read = read(pipe_fd[0], pipe_buffer, sizeof(pipe_buffer)-1);
    // Verificam am citit ceva din pipe
    if (bytes_read > 0) 
    {
        pipe_buffer[bytes_read] = '\0';
        printf("%s", pipe_buffer);
    }
}

int main() 
{
    char input[buffer_length];

    // cream pipe-ul
    if (pipe(pipe_fd) == -1) 
    {
        perror("Eroare la crearea pipe-ului");
        return 1;
    }

    // Configurarea handler-ului pentru SIGCHLD
    struct sigaction sa_chld;
    memset(&sa_chld, 0, sizeof(sa_chld));
    sa_chld.sa_handler = sigchld_handler;
    sigaction(SIGCHLD, &sa_chld, NULL);

    while (1) 
    {
        printf("hub> ");
        fflush(stdout);
        int len = read(0, input, buffer_length-1);
        if (len <= 0) continue;
        input[len-1] = '\0';

        if (strcmp(input, "start_monitor") == 0) 
        {
            if (monitor_pid > 0) 
            {
                printf("Monitor deja pornit\n");
                continue;
            }

            pid_t pid = fork();
            if (pid == 0) 
            {
                // Proces copil - monitorul
                close(pipe_fd[0]); // inchidem capatul de citire in monitor
                
                // Redirectam stdout al procesului monitor spre pipe
                dup2(pipe_fd[1], STDOUT_FILENO);
                close(pipe_fd[1]);
                
                //executam monitorul
                execl("./monitor", "monitor", NULL);
                perror("Eroare la execl");
                exit(1);
            }
            else if (pid > 0) 
            {
                // Proces parinte -hub
                close(pipe_fd[1]); //inchidem capatul de scriere in hub
                monitor_pid = pid; //retinem pid ul monitorului (pid>0 ->suntem in procesul parinte si pid= pid ul copilului)
                printf("Monitor pornit.\n");
                usleep(1000000); // asteptam putin pentru a ne asigura ca monitorul este pornit
                read_from_monitor_pipe();
            }
            else 
            {
                perror("Eroare la fork\n");
            }
        } 
        else if (strncmp(input, "list_hunts", 10) == 0 || 
                 strncmp(input, "list_treasures ", 15) == 0 || 
                 strncmp(input, "view_treasure ", 14) == 0) 
        {
            if (monitor_pid <= 0) 
            {
                printf("Monitorul nu este pornit\n");
                continue;
            }
            write_command_to_file(input);
            kill(monitor_pid, SIGUSR1);
            
            usleep(100000);
            read_from_monitor_pipe();
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
            
            // Citim mesajul de terminare
            usleep(100000);
            read_from_monitor_pipe();
        }
        else if (strcmp(input, "calculate_score") == 0) 
        {
            calculate_all_scores();
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

    // inchidere pipe 
    if (pipe_fd[0] != -1) 
    {
        close(pipe_fd[0]);
    }

    return 0;
}
