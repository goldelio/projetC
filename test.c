#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <wchar.h>
#include <wctype.h>
#include <locale.h>
#include <ctype.h>
#include <math.h>

#define LONGUEUR_MOT_MAX 50
#define NB_MOTS_MAX 20000
#define LONGUEUR_CHEMIN_MAX 256
#define TAILLE_HASHTABLE 10007

typedef struct {
    wchar_t mot[LONGUEUR_MOT_MAX];
    int frequence;
    int longueur;
    int est_verbe;
    int est_nom_propre;
} Mot;

typedef struct NoeudHash {
    Mot mot;
    struct NoeudHash* suivant;
} NoeudHash;

typedef struct {
    int nb_mots_total;
    int nb_mots_uniques;
    int nb_phrases;
    int nb_paragraphes;
    double longueur_mot_moyenne;
    double longueur_phrase_moyenne;
    double diversite_lexicale;
    double densite_lexicale;
    double complexite_texte;
    int nb_verbes;
    int nb_noms_propres;
    NoeudHash* table_hash[TAILLE_HASHTABLE];
} AnalyseTexte;

void initialiserAnalyse(AnalyseTexte* analyse) {
    memset(analyse, 0, sizeof(AnalyseTexte));
    for (int i = 0; i < TAILLE_HASHTABLE; i++) {
        analyse->table_hash[i] = NULL;
    }
}

void libererAnalyse(AnalyseTexte* analyse) {
    for (int i = 0; i < TAILLE_HASHTABLE; i++) {
        NoeudHash* courant = analyse->table_hash[i];
        while (courant != NULL) {
            NoeudHash* temp = courant;
            courant = courant->suivant;
            free(temp);
        }
        analyse->table_hash[i] = NULL;
    }
}

int estCaractereMot(wchar_t c) {
    return iswalnum(c) || c == L'-' || c == L'\'' || c == L'_';
}

void normaliserMot(wchar_t* mot) {
    int i, j = 0;
    for (i = 0; mot[i]; i++) {
        if (estCaractereMot(mot[i])) {
            mot[j] = towlower(mot[i]);
            j++;
        }
    }
    mot[j] = L'\0';
}

void afficherTempsExecution(struct timespec debut, struct timespec fin, const char* nom_fonction) {
    double duree = (fin.tv_sec - debut.tv_sec) + (fin.tv_nsec - debut.tv_nsec) / 1e9;
    printf("Temps d'exécution (%s): %.3f s\n", nom_fonction, duree);
}

unsigned int calculerHash(const wchar_t* mot) {
    unsigned int hash = 5381;
    int c;
    while ((c = *mot++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash % TAILLE_HASHTABLE;
}

void detecterTypeMot(Mot* mot) {
    int len = wcslen(mot->mot);
    mot->est_nom_propre = iswupper(mot->mot[0]);
    mot->est_verbe = 0;
    if (len > 2) {
        wchar_t* fin = mot->mot + len - 2;
        if (wcscmp(fin, L"er") == 0 || wcscmp(fin, L"ir") == 0 ||
            (len > 3 && wcscmp(fin-1, L"re") == 0)) {
            mot->est_verbe = 1;
        }
    }
}

double calculerComplexiteTexte(const AnalyseTexte* analyse) {
    return (
        0.3 * analyse->longueur_phrase_moyenne +
        0.2 * analyse->diversite_lexicale * 100 +
        0.2 * ((double)analyse->nb_verbes / analyse->nb_mots_total) * 100 +
        0.15 * ((double)analyse->nb_mots_uniques / analyse->nb_mots_total) * 100 +
        0.15 * analyse->longueur_mot_moyenne
    );
}

void ajouterMot(AnalyseTexte* analyse, const wchar_t* mot) {
    unsigned int index = calculerHash(mot);
    NoeudHash* courant = analyse->table_hash[index];

    while (courant != NULL) {
        if (wcscmp(courant->mot.mot, mot) == 0) {
            courant->mot.frequence++;
            return;
        }
        courant = courant->suivant;
    }

    NoeudHash* nouveau = (NoeudHash*)malloc(sizeof(NoeudHash));
    wcscpy(nouveau->mot.mot, mot);
    nouveau->mot.frequence = 1;
    nouveau->mot.longueur = wcslen(mot);
    detecterTypeMot(&nouveau->mot);
    nouveau->suivant = analyse->table_hash[index];
    analyse->table_hash[index] = nouveau;
    analyse->nb_mots_uniques++;

    if (nouveau->mot.est_verbe) analyse->nb_verbes++;
    if (nouveau->mot.est_nom_propre) analyse->nb_noms_propres++;
}

void afficherMenuMetriques(void) {
    printf("\nMétriques disponibles:\n");
    printf("1. Nombre total de mots\n");
    printf("2. Nombre de mots uniques\n");
    printf("3. Nombre de phrases\n");
    printf("4. Nombre de paragraphes\n");
    printf("5. Longueur moyenne des phrases\n");
    printf("6. Diversité lexicale\n");
    printf("7. Complexité du texte\n");
    printf("8. Nombre de verbes\n");
    printf("9. Nombre de noms propres\n");
    printf("10. Top 10 des mots\n");
    printf("11. Fréquence complète des mots\n");
    printf("0. Retour au menu précédent\n");
}

void afficherTop10(const AnalyseTexte* analyse) {
    Mot mots[NB_MOTS_MAX];
    int nb_mots = 0;

    for (int i = 0; i < TAILLE_HASHTABLE; i++) {
        NoeudHash* courant = analyse->table_hash[i];
        while (courant != NULL && nb_mots < NB_MOTS_MAX) {
            mots[nb_mots++] = courant->mot;
            courant = courant->suivant;
        }
    }

    for (int i = 0; i < 10 && i < nb_mots; i++) {
        int max_idx = i;
        for (int j = i + 1; j < nb_mots; j++) {
            if (mots[j].frequence > mots[max_idx].frequence) {
                max_idx = j;
            }
        }
        if (max_idx != i) {
            Mot temp = mots[i];
            mots[i] = mots[max_idx];
            mots[max_idx] = temp;
        }
    }

    printf("\nTop 10 des mots les plus fréquents:\n");
    for (int i = 0; i < 10 && i < nb_mots; i++) {
        printf("%ls: %d occurrences\n", mots[i].mot, mots[i].frequence);
    }
}

void afficherFrequenceComplete(const AnalyseTexte* analyse) {
    printf("\nFréquence complète des mots:\n");
    for (int i = 0; i < TAILLE_HASHTABLE; i++) {
        NoeudHash* courant = analyse->table_hash[i];
        while (courant != NULL) {
            printf("%ls: %d occurrences", courant->mot.mot, courant->mot.frequence);
            if (courant->mot.est_verbe) printf(" (verbe)");
            if (courant->mot.est_nom_propre) printf(" (nom propre)");
            printf("\n");
            courant = courant->suivant;
        }
    }
}

void afficherMetriqueSpecifique(const AnalyseTexte* analyse, int choix) {
    switch (choix) {
        case 1:
            printf("Nombre total de mots: %d\n", analyse->nb_mots_total);
            break;
        case 2:
            printf("Mots uniques: %d\n", analyse->nb_mots_uniques);
            break;
        case 3:
            printf("Nombre de phrases: %d\n", analyse->nb_phrases);
            break;
        case 4:
            printf("Nombre de paragraphes: %d\n", analyse->nb_paragraphes);
            break;
        case 5:
            printf("Longueur moyenne des phrases: %.2f mots\n", analyse->longueur_phrase_moyenne);
            break;
        case 6:
            printf("Diversité lexicale: %.2f%%\n", analyse->diversite_lexicale * 100);
            break;
        case 7:
            printf("Complexité du texte: %.2f\n", analyse->complexite_texte);
            break;
        case 8:
            printf("Nombre de verbes: %d\n", analyse->nb_verbes);
            break;
        case 9:
            printf("Nombre de noms propres: %d\n", analyse->nb_noms_propres);
            break;
        case 10:
            afficherTop10(analyse);
            break;
        case 11:
            afficherFrequenceComplete(analyse);
            break;
    }
}

void analyserFichier(const char* chemin, AnalyseTexte* analyse) {
    FILE* fichier = fopen(chemin, "r");
    if (fichier == NULL) {
        perror("Erreur à l'ouverture du fichier");
        exit(EXIT_FAILURE);
    }

    struct timespec debut, fin;
    clock_gettime(CLOCK_MONOTONIC, &debut);

    wchar_t mot_courant[LONGUEUR_MOT_MAX];
    wint_t c;
    int pos_mot = 0;
    int en_mot = 0;
    int mots_dans_phrase = 0;
    int en_paragraphe = 0;

    while ((c = fgetwc(fichier)) != WEOF) {
        if (estCaractereMot(c)) {
            if (!en_mot) {
                en_mot = 1;
                analyse->nb_mots_total++;
                mots_dans_phrase++;
            }
            if (pos_mot < LONGUEUR_MOT_MAX - 1) {
                mot_courant[pos_mot++] = c;
            }
        } else {
            if (en_mot) {
                mot_courant[pos_mot] = L'\0';
                normaliserMot(mot_courant);
                ajouterMot(analyse, mot_courant);
                pos_mot = 0;
                en_mot = 0;
            }

            if (c == L'.' || c == L'!' || c == L'?') {
                analyse->nb_phrases++;
                analyse->longueur_phrase_moyenne += mots_dans_phrase;
                mots_dans_phrase = 0;
            } else if (c == L'\n') {
                if (!en_paragraphe) {
                    analyse->nb_paragraphes++;
                    en_paragraphe = 1;
                }
            } else {
                en_paragraphe = 0;
            }
        }
    }

    if (analyse->nb_phrases > 0) {
        analyse->longueur_phrase_moyenne /= analyse->nb_phrases;
    }

    analyse->diversite_lexicale = (double)analyse->nb_mots_uniques / analyse->nb_mots_total;
    analyse->complexite_texte = calculerComplexiteTexte(analyse);

    clock_gettime(CLOCK_MONOTONIC, &fin);
    afficherTempsExecution(debut, fin, "analyserFichier");

    fclose(fichier);
}

void menuAnalyseFichierUnique(AnalyseTexte* analyse) {
    int choix;

    while (1) {
        afficherMenuMetriques();
        printf("\nChoisissez une métrique (0 pour revenir): ");
        scanf("%d", &choix);
        getchar();

        if (choix == 0) break;
        if (choix >= 1 && choix <= 11) {
            afficherMetriqueSpecifique(analyse, choix);
        } else {
            printf("Choix invalide\n");
        }
    }
}

void menuComparaisonFichiers(const char* chemin1, const char* chemin2) {
    AnalyseTexte analyse1, analyse2;
    int choix;

    initialiserAnalyse(&analyse1);
    initialiserAnalyse(&analyse2);

    analyserFichier(chemin1, &analyse1);
    analyserFichier(chemin2, &analyse2);

    while (1) {
        printf("\nMenu de comparaison:\n");
        printf("1. Voir métriques du premier fichier\n");
        printf("2. Voir métriques du deuxième fichier\n");
        printf("3. Voir comparaison des métriques\n");
        printf("4. Calculer similarité\n");
        printf("0. Retour au menu principal\n");
        printf("Choix: ");
        scanf("%d", &choix);
        getchar();

        switch (choix) {
            case 0:
                libererAnalyse(&analyse1);
            libererAnalyse(&analyse2);
            return;

            case 1:
                printf("\nAnalyse du fichier 1: %s\n", chemin1);
            menuAnalyseFichierUnique(&analyse1);
            break;

            case 2:
                printf("\nAnalyse du fichier 2: %s\n", chemin2);
            menuAnalyseFichierUnique(&analyse2);
            break;

            case 3:
                printf("\nComparaison des métriques:\n");
            printf("Différence de mots totaux: %d\n",
                abs(analyse1.nb_mots_total - analyse2.nb_mots_total));
            printf("Différence de mots uniques: %d\n",
                abs(analyse1.nb_mots_uniques - analyse2.nb_mots_uniques));
            printf("Différence de phrases: %d\n",
                abs(analyse1.nb_phrases - analyse2.nb_phrases));

            printf("Différence de longueur moyenne des phrases: %.2f\n",
                                fabs(analyse1.longueur_phrase_moyenne - analyse2.longueur_phrase_moyenne));
            printf("Différence de diversité lexicale: %.2f%%\n",
                fabs(analyse1.diversite_lexicale - analyse2.diversite_lexicale) * 100);
            printf("Différence de complexité: %.2f\n",
                fabs(analyse1.complexite_texte - analyse2.complexite_texte));
            printf("Différence de verbes: %d\n",
                abs(analyse1.nb_verbes - analyse2.nb_verbes));
            printf("Différence de noms propres: %d\n",
                abs(analyse1.nb_noms_propres - analyse2.nb_noms_propres)); break;
            default:
                printf("Choix invalide\n");break;



        }
    }
}

    int main()
    {
        setlocale(LC_ALL, "");
        char chemin1[LONGUEUR_CHEMIN_MAX];
        char chemin2[LONGUEUR_CHEMIN_MAX];
        int choix;

        while (1) {
            printf("\nMenu principal:\n");
            printf("1. Analyser un fichier\n");
            printf("2. Comparer deux fichiers\n");
            printf("0. Quitter\n");
            printf("Choix: ");
            scanf("%d", &choix);
            getchar();

            switch (choix) {

                case 1: {
                    printf("Entrez le chemin du fichier à analyser: ");
                    fgets(chemin1, LONGUEUR_CHEMIN_MAX, stdin);
                    chemin1[strcspn(chemin1, "\n")] = 0;

                    AnalyseTexte analyse;
                    initialiserAnalyse(&analyse);
                    analyserFichier(chemin1, &analyse);
                    menuAnalyseFichierUnique(&analyse);
                    libererAnalyse(&analyse);
                    break;
                }

                case 2:
                    printf("Entrez le chemin du premier fichier: ");
                fgets(chemin1, LONGUEUR_CHEMIN_MAX, stdin);
                chemin1[strcspn(chemin1, "\n")] = 0;

                printf("Entrez le chemin du deuxième fichier: ");
                fgets(chemin2, LONGUEUR_CHEMIN_MAX, stdin);
                chemin2[strcspn(chemin2, "\n")] = 0;

                menuComparaisonFichiers(chemin1, chemin2);
                break;

                default:
                    printf("Choix invalide\n");
                break;
            }
        }
    }
