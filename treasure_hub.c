#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>

#define CMD_FILE "cmd.txt"
#define buffer_length 256
//monitor pid este -1 pentru a verifica daca monitorul este pornit sau nu
pid_t monitor_pid = -1;

// handler pentru SIGCHLD, detecteaza terminarea procesului monitor
void sigchld_handler(int sig) 
{
    int status;
    pid_t pid = waitpid(-1, &status, WNOHANG);
    if (pid == monitor_pid) {
        printf("Monitorul s-a oprit.\n");
        monitor_pid = -1;
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

int main() 
{
    char input[buffer_length];
// configurarea handler-ului pentru SIGCHLD
    struct sigaction sa_chld;
    memset(&sa_chld, 0, sizeof(sa_chld));
    sa_chld.sa_handler = sigchld_handler;
    sigaction(SIGCHLD, &sa_chld, NULL);

    while (1) 
    {
        printf("hub> ");
        fflush(stdout);

        int len = read(0, input, buffer_length-1);
        if (len<=0) continue;
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
                execl("./monitor", "monitor", NULL);
                perror("Eroare la execl");
                exit(1);
            }
            else if (pid>0) 
            {
                monitor_pid = pid;
                printf("Monitor pornit.\n");
            }
            else 
            {
                perror("Eroare la fork\n");
            }
        } 
        else if (strncmp(input, "list_hunts", 10) == 0 || strncmp(input, "list_treasures ", 15) == 0 || strncmp(input, "view_treasure ", 14) == 0) 
        {
            if (monitor_pid <= 0) 
            {
                printf("Monitorul nu este pornit\n");
                continue;
            }
            write_command_to_file(input);
            kill(monitor_pid, SIGUSR1);
        } 
        else if (strcmp(input, "stop_monitor")==0) 
        {
            if (monitor_pid <= 0) 
            {
                printf("Monitorul nu este pornit\n");
                continue;
            }
            write_command_to_file("stop");
            kill(monitor_pid, SIGUSR1);
        } 
        else if (strcmp(input, "exit")==0) 
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
