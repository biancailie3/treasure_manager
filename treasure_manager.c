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

typedef struct {
    int treasure_id;
    char username[USER_LEN];
    double latitude;
    double longitude;
    char hint[INDICIU_LENGTH];
    int value;
} Treasure_t;

//functie pentru logarea actiunilor
// in fisierul de log

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


void add_treasure(const char *hunt_id) {
    char dir_path[256], file_path[256];
    snprintf(dir_path, sizeof(dir_path), "%s", hunt_id);
    snprintf(file_path, sizeof(file_path), "%s/treasures.bin", hunt_id);

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

    write(file_fd, &new_treasure, sizeof(Treasure_t));
    close(file_fd);

    log_action(hunt_id, "A fost adaugata o comoara noua.\n");
}

void list_treasures(const char *hunt_id) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/treasures.bin", hunt_id);

    struct stat file_info;
    if (stat(file_path, &file_info) == -1) {
        perror("Eroare obtinere info din fisier\n");
        return;
    }

    printf("Treasure hunt: %s\n", hunt_id);
    printf("Dimensiune fisier: %lld bytes\n", file_info.st_size);
    printf("Ultima modificare: %s", ctime(&file_info.st_mtime));
    printf("\nLista comori:\n");
    printf("---------------------------------------------------------------\n");

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
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

void view_hunt(const char *hunt_id, int treasure_id) {
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/treasures.bin", hunt_id);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("Eroare la deschiderea fisierului de comori");
        return;
    }

    Treasure_t treasure;
    int found = 0;

    while (read(fd, &treasure, sizeof(Treasure_t)) == sizeof(Treasure_t)) {
        if (treasure.treasure_id == treasure_id) {
            printf("ID: %d\nUsername: %s\nLatitude: %.6lf\nLongitude: %.6lf\nClue: %s\nValue: %d\n",
                   treasure.treasure_id, treasure.username, treasure.latitude,
                   treasure.longitude, treasure.hint, treasure.value);
            found = 1;
            break;
        }
    }

    close(fd);

    if (!found) {
        printf("Comoara cu ID-ul %d nu a fost gasita\n", treasure_id);
    }

    log_action(hunt_id, "Treasure hunt vizualizat\n");
}


void remove_treasure(const char *hunt_id, int treasure_id) 
{
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/treasures.bin", hunt_id);

    int fd = open(file_path, O_RDONLY);
    if (fd == -1) {
        perror("Eroare deschidere fisier");
        return;
    }

    Treasure_t *treasures = NULL;
    int count = 0;
    Treasure_t temp;

    //citim toate comorile si le salvam in memorie
    while (read(fd, &temp, sizeof(Treasure_t)) == sizeof(Treasure_t)) {
        if (temp.treasure_id != treasure_id) {
            treasures = realloc(treasures, sizeof(Treasure_t) * (count + 1));
            treasures[count++] = temp;
        }
    }
    close(fd);

    // daca nu mai e nicio comoara, stergem direct tot fisierul
    if (count == 0) {
        if (unlink(file_path) == -1) {
            perror("Eroare la stergerea fisierului de comori\n");
        } else {
            printf("Fisier de comori sters.\n");
        }
        free(treasures);
        return;
    }

    // rescriem fisierul cu comorile ramase
    fd = open(file_path, O_WRONLY | O_TRUNC);
    if (fd == -1) {
        perror("Eroare rescriere fisier");
        free(treasures);
        return;
    }
    
    // Scriem datele ramase inapoi in fisier
    for (int i = 0; i < count; i++) {
        if (write(fd, &treasures[i], sizeof(Treasure_t)) != sizeof(Treasure_t)) {
            perror("Eroare la scrierea în fișier");
            break;
        }
    }
    
    close(fd);
    free(treasures);
    
    char action[100];
    snprintf(action, sizeof(action), "Comoara cu ID-ul %d a fost ștearsă", treasure_id);
    log_action(hunt_id, action);
}

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

int main(int argc, char *argv[]) 
{
    if (argc < 2)
    {
        printf("eroare argumente\n");
        return 1;
    }

    if (strcmp(argv[1], "--add") == 0) 
    {
        if (argc < 3) 
        {
            printf("eroare argumente\n");
            return 1;
        }
        add_treasure(argv[2]);
    }
    else if (strcmp(argv[1], "--list") == 0) 
    {
        if (argc < 3) 
        {
            printf("eroare argumente\n");
            return 1;
        }
        list_treasures(argv[2]);   
    } 
    else if (strcmp(argv[1], "--view") == 0) 
    {
        if (argc < 4) 
        {
            printf("eroare argumente\n");
            return 1;
        }
        int treasure_id = atoi(argv[3]); //convertim id-ul in int
        view_hunt(argv[2], treasure_id);
    } 
    else if (strcmp(argv[1], "--remove") == 0) 
    {
        if (argc < 4) 
        {
            printf("eroare argumente\n");
            return 1;
        }
        int treasure_id = atoi(argv[3]);
        remove_treasure(argv[2], treasure_id);
    } else if (strcmp(argv[1], "--remove-hunt") == 0) {
        if (argc < 3) {
            printf("eroare argumente\n");
            return 1;
        }
        remove_hunt(argv[2]);
    } else {
        printf("eroare argumente\n");
    }
    return 0;
}
