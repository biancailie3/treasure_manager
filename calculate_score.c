#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>

#define USER_LEN 32
#define INDICIU_LENGTH 128

typedef struct {
    int treasure_id;
    char username[USER_LEN];
    double latitude;
    double longitude;
    char hint[INDICIU_LENGTH];
    int value;
} Treasure_t;

typedef struct {
    char username[USER_LEN];
    int total_score;
    int treasure_count;
} UserScore;

int main(int argc, char *argv[]) 
{
    if (argc < 2) {
        fprintf(stderr, "Utilizare: %s <hunt_id>\n", argv[0]);
        return 1;
    }

    const char *hunt_id = argv[1];
    char file_path[256];
    snprintf(file_path, sizeof(file_path), "%s/treasures.bin", hunt_id);

    // deschidem fisierul de comori
    int fd=open(file_path, O_RDONLY);
    if (fd==-1) 
    {
        fprintf(stderr, "Eroare la deschiderea fisierului %s\n",hunt_id);
        return 1;
    }

    // citim comorile si calculam scorurile
    Treasure_t treasure;
    UserScore *users = NULL;
    int user_count = 0;

    while (read(fd, &treasure, sizeof(Treasure_t)) == sizeof(Treasure_t)) 
    {
        // cautam utilizatorul in lista
        int found = 0;
        for (int i = 0; i < user_count; i++) 
        {
            if (strcmp(users[i].username, treasure.username) == 0) 
            {
                // s-a gasit utilizatorul, actualizam scorul
                users[i].total_score += treasure.value;
                users[i].treasure_count++;
                found = 1;
                break;
            }
        }

        if (!found) 
        {
            // inseamna ca utilizatorul e nou, asa ca il adaugam in lista
            users = realloc(users, sizeof(UserScore) * (user_count + 1));
            strcpy(users[user_count].username, treasure.username);
            users[user_count].total_score = treasure.value;
            users[user_count].treasure_count = 1;
            user_count++;
        }
    }

    close(fd);

    // sortam utilizatorii in functie de scor
    for (int i = 0; i < user_count - 1; i++) 
    {
        for (int j = i + 1; j < user_count; j++) 
        {
            if (users[j].total_score > users[i].total_score) 
            {
                UserScore temp = users[i];
                users[i] = users[j];
                users[j] = temp;
            }
        }
    }

    // afisare scoruri
    printf("Scoruri pentru hunt-ul %s:\n", hunt_id);
    printf("------------------------------------------------\n");
    printf("| %-20s | %-10s | %-13s |\n", "Utilizator", "Scor", "Nr. Comori");
    printf("------------------------------------------------\n");
    
    for (int i = 0; i < user_count; i++) 
    {
        printf("| %-20s | %-10d | %-13d |\n", 
               users[i].username, users[i].total_score, users[i].treasure_count);
    }
    printf("------------------------------------------------\n");

    if (user_count == 0) 
    {
        printf("Nu există utilizatori cu comori în acest hunt.\n");
    }

    free(users);
    return 0;
}
