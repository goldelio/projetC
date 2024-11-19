#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct {
    char mot[50];
    int frequence;
} Mot;

// New comparison function for qsort
int compareMots(const void *a, const void *b) {
    const Mot *motA = (const Mot *)a;
    const Mot *motB = (const Mot *)b;
    // Sort by frequency in descending order
    if (motB->frequence != motA->frequence) {
        return motB->frequence - motA->frequence;
    }
    // If frequencies are equal, sort alphabetically
    return strcmp(motA->mot, motB->mot);
}

void ouvrirFichierLecture(char *chemin, FILE **fichier) {
    *fichier = fopen(chemin, "r");
    if (*fichier == NULL) {
        perror("Erreur à l'ouverture du fichier");
        exit(EXIT_FAILURE);
    }
}

void sauvegarderResultats(char *cheminSortie, int nombreLignes, int nombreMots, 
                         int nombreCaracteres, Mot *tableauMots, int nombreMotsDistincts) {
    FILE *fichierSortie = fopen(cheminSortie, "w");
    if (fichierSortie == NULL) {
        perror("Erreur lors de la création du fichier de sortie");
        return;
    }

    fprintf(fichierSortie, "Résultats de l'analyse du texte:\n");
    fprintf(fichierSortie, "============================\n\n");
    fprintf(fichierSortie, "Statistiques générales:\n");
    fprintf(fichierSortie, "- Nombre de lignes: %d\n", nombreLignes);
    fprintf(fichierSortie, "- Nombre de mots: %d\n", nombreMots);
    fprintf(fichierSortie, "- Nombre de caractères: %d\n", nombreCaracteres);
    fprintf(fichierSortie, "- Nombre de mots distincts: %d\n\n", nombreMotsDistincts);
    
    fprintf(fichierSortie, "Fréquence des mots:\n");
    fprintf(fichierSortie, "==================\n\n");

    // Sort the array before printing
    qsort(tableauMots, nombreMotsDistincts, sizeof(Mot), compareMots);

    // Print frequencies with count
    fprintf(fichierSortie, "Liste triée par fréquence:\n");
    fprintf(fichierSortie, "-------------------------\n");
    for (int i = 0; i < nombreMotsDistincts; i++) {
        fprintf(fichierSortie, "%-20s: %d\n", tableauMots[i].mot, tableauMots[i].frequence);
    }

    // Print words in array format (10 per line)
    fprintf(fichierSortie, "\n\nMots par ordre de fréquence (10 par ligne):\n");
    fprintf(fichierSortie, "----------------------------------------\n");
    for (int i = 0; i < nombreMotsDistincts; i++) {
        fprintf(fichierSortie, "%-15s", tableauMots[i].mot);
        if ((i + 1) % 10 == 0 || i == nombreMotsDistincts - 1) {
            fprintf(fichierSortie, "\n");
        }
    }

    if (fclose(fichierSortie) != 0) {
        perror("Erreur lors de la fermeture du fichier de sortie");
    } else {
        printf("Résultats sauvegardés avec succès dans %s\n", cheminSortie);
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
    *nombreMots = 0;  // Reset the counter
    while (fscanf(fichier, "%s", buffer) == 1) {
        ajouterMotOuIncrementer(buffer, tableauMots, nombreMots);
    }
    // Sort the array after calculating frequencies
    qsort(tableauMots, *nombreMots, sizeof(Mot), compareMots);
    rewind(fichier);
}

void afficherFrequenceMots(Mot *tableauMots, int nombreMots) {
    printf("\nFréquence des mots (triée par ordre décroissant):\n");
    printf("=============================================\n");
    for (int i = 0; i < nombreMots; i++) {
        printf("%-20s: %d\n", tableauMots[i].mot, tableauMots[i].frequence);
    }

    printf("\nMots par ordre de fréquence (10 par ligne):\n");
    printf("=======================================\n");
    for (int i = 0; i < nombreMots; i++) {
        printf("%-15s", tableauMots[i].mot);
        if ((i + 1) % 10 == 0 || i == nombreMots - 1) {
            printf("\n");
        }
    }
}

int main() {
    FILE *fichier;
    char chemin[100], cheminSortie[100];
    int choix;
    Mot tableauMots[1000];
    int nombreMots = 0;
    int totalMots = 0, totalLignes = 0, totalCaracteres = 0;

    printf("Entrez le chemin du fichier texte: ");
    scanf("%s", chemin);
    ouvrirFichierLecture(chemin, &fichier);

    while (1) {
        printf("\nOptions :\n");
        printf("1. Compter lignes\n");
        printf("2. Compter mots\n");
        printf("3. Compter caractères\n");
        printf("4. Calculer fréquence des mots\n");
        printf("5. Sauvegarder les résultats\n");
        printf("6. Quitter\n");
        printf("Choix : ");
        scanf("%d", &choix);

        switch (choix) {
            case 1:
                totalLignes = compterLignes(fichier);
                printf("Nombre de lignes : %d\n", totalLignes);
                break;
            case 2:
                totalMots = compterMots(fichier);
                printf("Nombre de mots : %d\n", totalMots);
                break;
            case 3:
                totalCaracteres = compterCaracteres(fichier);
                printf("Nombre de caractères : %d\n", totalCaracteres);
                break;
            case 4:
                calculerFrequenceMots(fichier, tableauMots, &nombreMots);
                printf("Fréquence des mots calculée.\n");
                afficherFrequenceMots(tableauMots, nombreMots);
                break;
            case 5:
                printf("Entrez le chemin du fichier de sortie: ");
                scanf("%s", cheminSortie);
                totalLignes = compterLignes(fichier);
                totalMots = compterMots(fichier);
                totalCaracteres = compterCaracteres(fichier);
                calculerFrequenceMots(fichier, tableauMots, &nombreMots);
                sauvegarderResultats(cheminSortie, totalLignes, totalMots, 
                                   totalCaracteres, tableauMots, nombreMots);
                break;
            case 6:
                fclose(fichier);
                return 0;
            default:
                printf("Choix invalide\n");
        }
    }
}