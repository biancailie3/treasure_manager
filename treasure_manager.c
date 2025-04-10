#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <time.h>
#include <errno.h>

#define USER_LEN 32
#define INDICIU_LENGTH 128
#define LOG_FILE "logged_hunt"

//structura pt comori
typedef struct {
    int treasure_id;
    char username[USER_LEN];
    double latitude;
    double longitude;
    char hint[INDICIU_LENGTH];
    int value;
}Treasure_t;

//functie pentru logarea actiunilor
void log_action(const char *hunt_id, const char *action) {
    char log_path[256];
    snprintf(log_path, sizeof(log_path), "%s/%s", hunt_id, LOG_FILE);

    int log_fd = open(log_path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (log_fd == -1) {
        perror("eroare deschidere fisier log\n");
        return;
    }

    time_t now = time(NULL);
    char log_entry[256];
    snprintf(log_entry, sizeof(log_entry), "%s - %s\n", ctime(&now), action);

    write(log_fd, log_entry, strlen(log_entry));
    close(log_fd);
}

//functie pt adaugare comori in fisier
void add_treasure(const char *hunt_id) 
{
    char dir_path[256], file_path[256];
    snprintf(dir_path, sizeof(dir_path), "%s", hunt_id);
    snprintf(file_path, sizeof(file_path), "%s/treasures.txt", hunt_id);

    // cream directorul daca nu exista
    if (mkdir(dir_path, 0755) == -1 && errno != EEXIST) {
        perror("Eroare la creare director de comori\n");
        return;
    }

    int file_fd = open(file_path, O_WRONLY | O_APPEND | O_CREAT, 0644);
    if (file_fd == -1) {
        perror("Eroare la deschiderea fisierului de comori");
        return;
    }

    Treasure_t new_treasure;
    printf("Introdu ID-ul comorii: ");
    scanf("%d", &new_treasure.treasure_id);
    printf("Introdu username: ");
    scanf("%s", new_treasure.username);
    printf("Introdu latitudinea: ");
    scanf("%lf", &new_treasure.latitude);
    printf("Introdu longitudinea: ");
    scanf("%lf", &new_treasure.longitude);
    printf("Introdu indiciu: ");
    scanf(" %[^\n]", new_treasure.hint);
    printf("Introdu valoarea: ");
    scanf("%d", &new_treasure.value);

    char buffer[512];
    snprintf(buffer, sizeof(buffer), "%d %s %.6lf %.6lf \"%s\" %d\n",
             new_treasure.treasure_id,
             new_treasure.username,
             new_treasure.latitude,
             new_treasure.longitude,
             new_treasure.hint,
             new_treasure.value);

    write(file_fd, buffer, strlen(buffer));
    close(file_fd);

    log_action(hunt_id, "A fost adaugata o comoara noua.\n");
}

// functie pt stergere comori
void remove_hunt(const char *hunt_id) 
{
    char command[256];
    snprintf(command, sizeof(command), "rm -rf %s", hunt_id);

    if (system(command) == -1) {
        perror("eroare stergere treasure hunt\n");
        return;
    }
    log_action(hunt_id, "Treasure hunt sters\n");
}
void view_hunt(const char *hunt_id, int treasure_id) 
{
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/treasures.txt", hunt_id);

    int file_fd = open(file_path, O_RDONLY);
    if (file_fd == -1) {
        perror("Eroare la deschiderea fisierului de comori");
        return;
    }

    char buffer[512];
    ssize_t bytes_read;
    int found = 0;


    while ((bytes_read = read(file_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0'; 

  
        char *line = strtok(buffer, "\n");
        while (line != NULL) 
        {
           Treasure_t treasure;

            if (sscanf(line, "%d %s %lf %lf \"%[^\"]\" %d", &treasure.treasure_id, treasure.username, &treasure.latitude, &treasure.longitude, treasure.hint, &treasure.value) == 6) 
            {
                if (treasure.treasure_id== treasure_id) {
                    printf("ID: %d\nUsername: %s\nLatitude: %.6lf\nLongitude: %.6lf\nClue: %s\nValue: %d\n",
                           treasure.treasure_id, treasure.username, treasure.latitude, treasure.longitude, treasure.hint, treasure.value);
                    found = 1;
                    break;
                }
            }
            line = strtok(NULL, "\n");
        }

        if (found) {
            break;
        }
    }

    if (!found) {
        printf("Comoara cu ID-ul %d nu a fost gasita\n", treasure_id);
    }

    if (bytes_read == -1) {
        perror("Eroare la citire fisier de comori");
    }

    close(file_fd);

    log_action(hunt_id, "Treasure hunt vizualizat\n");
}
//functie pt listare comori
void list_treasures(const char *hunt_id) 
{
    struct stat file_info;
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/treasures.txt", hunt_id);

    if (stat(file_path, &file_info) == -1) {
        perror("Eroare obtinere info din fisier\n");
        return;
    }

    printf("Treasure hunt: %s\n", hunt_id);
    printf("Dimensiune fisier: %lld bytes\n", file_info.st_size);
    printf("Ultima modificare: %s", ctime(&file_info.st_mtime));

    int file_fd = open(file_path, O_RDONLY);
    if (file_fd == -1) {
        perror("Eroare la deschidere fisier de comori");
        return;
    }

    printf("\nLista comori:\n");
    printf("----------------------------------------------------\n");

    char buffer[512];
    ssize_t bytes_read;
    while ((bytes_read = read(file_fd, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0'; 
        printf("%s", buffer);
    }

    if (bytes_read == -1) {
        perror("Eroare la citire fisier de comori\n");
    }

    close(file_fd);
}

void remove_treasure(const char *hunt_id, int treasure_id) 
{
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/treasures.txt", hunt_id);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("Eroare deschidere fisier\n");
        return;
    }

    int count = 0;
    char buffer[512];
    ssize_t bytes_read;
    char line[512];
    int line_pos = 0;

    while ((bytes_read = read(fd, buffer, sizeof(buffer))) > 0) {
        for (ssize_t i = 0; i < bytes_read; i++) {
            if (buffer[i]=='\n')
            {
                line[line_pos]='\0';
                if (line_pos>0) 
                {
                    count++;
                }
                line_pos=0;
            } 
            else 
            {
                if (line_pos< sizeof(line)-1) 
                {
                    line[line_pos++]= buffer[i];
                }
            }
        }
    }

    // daca mai ramane o linie fara newline la final
    if (line_pos > 0) 
    {
        count++;
    }
    close(fd);

    if (count == 1) 
    {
        if (unlink(file_path) == -1) 
        {
            perror("Eroare la stergerea fisierului\n");
            return;
        }
        printf("Ultima comoara a fost stearsa. Fisierul cu comori a fost eliminat\n");
    } 
    else 
    {
        // filtram cu grep comorile
        char command[512];
        snprintf(command, sizeof(command),
                 "grep -v '^%d ' %s > %s.tmp && mv %s.tmp %s",
                 treasure_id, file_path, file_path, file_path, file_path);

        if (system(command) == -1) 
        {
            perror("Eroare la stergerea comorii\n");
            return;
        }

        printf("Comoara cu ID-ul %d a fost stearsa\n", treasure_id);
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("eroare argumente\n");
        return 1;
    }

    if (strcmp(argv[1], "--add") == 0) 
    {
        if (argc < 3) {
            printf("eroare argumente\n");
            return 1;
        }
        add_treasure(argv[2]);
    }
    else if (strcmp(argv[1], "--list") == 0)
    {
        if (argc < 3) {
            printf("eroare argumente\n");    
            return 1;
        }
        list_treasures(argv[2]);
    } 
    else if (strcmp(argv[1], "--remove_hunt") == 0) 
    {
        if (argc < 3) {
            printf("eroare argumente\n");
            return 1;
        }
        remove_hunt(argv[2]);
    } 
    else if (strcmp(argv[1], "--view") == 0) 
    {
        if (argc < 4) {
            printf("eroare argumente\n");
            return 1;
        }
        int treasure_id = atoi(argv[3]); //convertim in numar
        if (treasure_id <= 0) {
            printf("ID-ul comorii trebuie sa fie un numar pozitiv\n");
            return 1;
        }
        view_hunt(argv[2], treasure_id);
    }
    else if (strcmp(argv[1], "--remove") == 0) 
    {
        if (argc < 4) {
            printf("eroare argumente\n");
            return 1;
        }
        int treasure_id = atoi(argv[3]); //convertim in numar
        if (treasure_id <= 0) {
            printf("ID-ul comorii trebuie sa fie un numar pozitiv\n");
            return 1;
        }
        remove_treasure(argv[2], treasure_id);
    }
    else 
    {
        printf("comanda invalida!\n");
    }
    return 0;
}
