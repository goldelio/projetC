#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char mot[50];
    int frequence;
} Mot;

void ouvrirFichierLecture(char *chemin, FILE **fichier) {
    *fichier = fopen(chemin, "r");
    if (*fichier == NULL) {
        perror("Erreur à l'ouverture du fichier");
        exit(EXIT_FAILURE);
    }
}

int compterLignes(FILE *fichier) {
    int lignes = 0;
    char ch;
    while ((ch = fgetc(fichier)) != EOF) {
        if (ch == '\n') lignes++;
    }
    rewind(fichier);
    return lignes;
}

int compterMots(FILE *fichier) {
    int mots = 0;
    char buffer[50];
    while (fscanf(fichier, "%s", buffer) == 1) mots++;
    rewind(fichier);
    return mots;
}

int compterCaracteres(FILE *fichier) {
    int caracteres = 0;
    char ch;
    while ((ch = fgetc(fichier)) != EOF) caracteres++;
    rewind(fichier);
    return caracteres;
}

void ajouterMotOuIncrementer(char *mot, Mot *tableauMots, int *nombreMots) {
    for (int i = 0; i < *nombreMots; i++) {
        if (strcmp(tableauMots[i].mot, mot) == 0) {
            tableauMots[i].frequence++;
            return;
        }
    }
    strcpy(tableauMots[*nombreMots].mot, mot);
    tableauMots[*nombreMots].frequence = 1;
    (*nombreMots)++;
}

void calculerFrequenceMots(FILE *fichier, Mot *tableauMots, int *nombreMots) {
    char buffer[50];
    while (fscanf(fichier, "%s", buffer) == 1) {
        ajouterMotOuIncrementer(buffer, tableauMots, nombreMots);
    }
    rewind(fichier);
}

int main() {
    FILE *fichier;
    char chemin[100];
    int choix;

    Mot tableauMots[1000];
    int nombreMots = 0;

    printf("Entrez le chemin du fichier texte: ");
    scanf("%s", chemin);
    ouvrirFichierLecture(chemin, &fichier);

    while (1) {
        printf("\nOptions :\n");
        printf("1. Compter lignes\n");
        printf("2. Compter mots\n");
        printf("3. Compter caractères\n");
        printf("4. Calculer fréquence des mots\n");
        printf("5. Quitter\n");
        printf("Choix : ");
        scanf("%d", &choix);

        switch (choix) {
            case 1:
                printf("Nombre de lignes : %d\n", compterLignes(fichier));
                break;
            case 2:
                printf("Nombre de mots : %d\n", compterMots(fichier));
                break;
            case 3:
                printf("Nombre de caractères : %d\n", compterCaracteres(fichier));
                break;
            case 4:
                calculerFrequenceMots(fichier, tableauMots, &nombreMots);
                printf("Fréquence des mots calculée.\n");
                for (int i = 0; i < nombreMots; i++) {
                    printf("%s : %d\n", tableauMots[i].mot, tableauMots[i].frequence);
                }
                break;
            case 5:
                fclose(fichier);
                return 0;
            default:
                printf("Choix invalide\n");
        }
    }
}
