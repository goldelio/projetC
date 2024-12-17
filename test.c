#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <wchar.h>
#include <wctype.h>
#include <locale.h>
#include <math.h>
#include <limits.h>

#define LONGUEUR_MOT_MAX 50 // Définit la longueur maximale d'un mot à analyser
#define NB_MOTS_MAX 20000   // Définit le nombre maximal de mots pouvant être analysés
#define LONGUEUR_CHEMIN_MAX 256  // Définit la longueur maximale pour un chemin de fichier
#define TAILLE_HASHTABLE 10007   // Définit la taille de la table de hachage utilisée pour stocker les mots

// Structure représentant un mot analysé
typedef struct {
    wchar_t mot[LONGUEUR_MOT_MAX]; // Tableau pour stocker le mot (en caractères larges, compatible Unicode)
    int frequence;   // Fréquence d'apparition du mot dans le texte
    int longueur;    // Longueur du mot (nombre de caractères)
    int est_verbe;   // Indicateur si le mot est un verbe (1 si vrai, 0 sinon)
    int est_nom_propre;  // Indicateur si le mot est un nom propre (1 si vrai, 0 sinon)
} Mot;
// Structure représentant un nœud dans la table de hachage
typedef struct NoeudHash {
    Mot mot;                     // Le mot stocké dans ce nœud
    struct NoeudHash* suivant;   // Pointeur vers le prochain nœud en cas de collision (chaînage)
} NoeudHash;

// Structure principale pour analyser le texte
typedef struct {
    int nb_espaces;
    int nb_chars_sans_espaces;
    int nb_lignes;                  // Nombre total de lignes
    int nb_caracteres;              // Nombre total de caractères
    wchar_t phrase_plus_longue[1000];  // Pour stocker la phrase la plus longue
    wchar_t phrase_plus_courte[1000];  // Pour stocker la phrase la plus courte
    int longueur_plus_longue;       // Longueur de la plus longue phrase
    int longueur_plus_courte;       // Longueur de la plus courte phrase
    int nb_mots_total;          // Nombre total de mots analysés dans le texte
    int nb_mots_uniques;        // Nombre de mots uniques trouvés (sans répétition)
    int nb_phrases;             // Nombre total de phrases dans le texte
    int nb_paragraphes;         // Nombre total de paragraphes dans le texte
    double longueur_mot_moyenne;    // Longueur moyenne des mots dans le texte
    double longueur_phrase_moyenne; // Longueur moyenne des phrases dans le texte
    double diversite_lexicale;      // Rapport entre les mots uniques et le nombre total de mots
    double complexite_texte;        // Indicateur global de la complexité du texte (combinaison de métriques)
    int nb_verbes;              // Nombre total de verbes identifiés dans le texte
    int nb_noms_propres;        // Nombre total de noms propres identifiés dans le texte
    NoeudHash* table_hash[TAILLE_HASHTABLE];  // Table de hachage pour stocker et retrouver les mots rapidement
} AnalyseTexte;

// Initialise la structure AnalyseTexte à des valeurs par défaut
void initialiserAnalyse(AnalyseTexte* analyse) {
    // Remplit toute la structure AnalyseTexte avec des zéros (initialisation complète)
    memset(analyse, 0, sizeof(AnalyseTexte));
    // Initialise chaque élément de la table de hachage à NULL (aucun mot n'est encore stocké)
    for (int i = 0; i < TAILLE_HASHTABLE; i++) {
        analyse->table_hash[i] = NULL;
    }
    // Initialisation pour la phrase la plus courte
    analyse->longueur_plus_courte = INT_MAX;
    wcscpy(analyse->phrase_plus_courte, L"");
    wcscpy(analyse->phrase_plus_longue, L"");
}
// Nouvelle fonction pour gérer l'extraction des phrases
void gererPhrase(AnalyseTexte* analyse, const wchar_t* phrase, int longueur) {
    if (longueur > 0) {
        // Mise à jour de la phrase la plus longue
        if (longueur > analyse->longueur_plus_longue) {
            analyse->longueur_plus_longue = longueur;
            wcsncpy(analyse->phrase_plus_longue, phrase, 999);
            analyse->phrase_plus_longue[999] = L'\0';
        }
        // Mise à jour de la phrase la plus courte
        if (longueur < analyse->longueur_plus_courte) {
            analyse->longueur_plus_courte = longueur;
            wcsncpy(analyse->phrase_plus_courte, phrase, 999);
            analyse->phrase_plus_courte[999] = L'\0';
        }
    }
}

// Libère la mémoire allouée dynamiquement pour la structure AnalyseTexte
void libererAnalyse(AnalyseTexte* analyse) {
    // Parcourt chaque entrée de la table de hachage
    for (int i = 0; i < TAILLE_HASHTABLE; i++) {
        NoeudHash* courant = analyse->table_hash[i]; // Pointe vers le premier nœud de la liste chaînée
        // Parcourt la liste chaînée pour libérer chaque nœud
        while (courant != NULL) {
            NoeudHash* temp = courant; // Stocke l'adresse actuelle avant de passer au suivant
            courant = courant->suivant; // Passe au nœud suivant dans la liste
            free(temp); // Libère la mémoire du nœud actuel
        }
        // Réinitialise la table de hachage à NULL après avoir libéré tous les nœuds
        analyse->table_hash[i] = NULL;
    }
}

// Vérifie si un caractère fait partie d'un mot valide
int estCaractereMot(wchar_t c) {
    // Retourne vrai si le caractère est alphanumérique (lettre ou chiffre),
    // ou s'il s'agit d'un tiret, d'une apostrophe ou d'un underscore
    return iswalnum(c) || c == L'-' || c == L'\'' || c == L'_';
}

// Normalise un mot en le convertissant en minuscules et en retirant les caractères non valides
void normaliserMot(wchar_t* mot) {
    int i, j = 0; // `i` parcourt le mot d'origine, `j` construit le mot normalisé
    for (i = 0; mot[i]; i++) {
        // Si le caractère actuel est valide (fait partie d'un mot)
        if (estCaractereMot(mot[i])) {
            mot[j] = towlower(mot[i]); // Convertit le caractère en minuscule
            j++; // Avance l'indice pour le mot normalisé
        }
    }
    mot[j] = L'\0'; // Termine la chaîne de caractères normalisée avec un caractère nul
}

// Calcule la valeur de hachage d'un mot pour la table de hachage
unsigned int calculerHash(const wchar_t* mot) {
    unsigned int hash = 5381; // Initialisation de la valeur de hachage avec une constante
    int c; // Variable pour stocker chaque caractère du mot
    // Parcourt chaque caractère du mot jusqu'à la fin de la chaîne
    while ((c = *mot++)) {
        // Applique l'algorithme de hachage de type DJB2 : hash = hash * 33 + c
        hash = ((hash << 5) + hash) + c;
    }
    // Retourne la valeur de hachage modulo la taille de la table pour rester dans les limites
    return hash % TAILLE_HASHTABLE;
}

// Détecte les types grammaticaux d'un mot (nom propre ou verbe)
void detecterTypeMot(Mot* mot) {
    int len = wcslen(mot->mot); // Calcule la longueur du mot
    // Vérifie si le mot commence par une lettre majuscule, indiquant un nom propre
    mot->est_nom_propre = iswupper(mot->mot[0]);
    mot->est_verbe = 0; // Initialise à 0, supposant que le mot n'est pas un verbe
    // Si le mot a plus de 2 caractères, vérifie les terminaisons possibles d'un verbe
    if (len > 2) {
        wchar_t* fin = mot->mot + len - 2; // Pointe vers les deux derniers caractères
        // Vérifie si le mot se termine par "er", "ir", ou "re" (indicateurs de verbes)
        if (wcscmp(fin, L"er") == 0 || wcscmp(fin, L"ir") == 0 ||
            (len > 3 && wcscmp(fin-1, L"re") == 0)) {
            mot->est_verbe = 1; // Marque le mot comme un verbe
        }
    }
}

// Calcule la complexité textuelle en fonction de plusieurs métriques
double calculerComplexiteTexte(const AnalyseTexte* analyse) {
    return (
        0.3 * analyse->longueur_phrase_moyenne + // Pondère la longueur moyenne des phrases
        0.2 * analyse->diversite_lexicale * 100 + // Pondère la diversité lexicale (en pourcentage)
        0.2 * ((double)analyse->nb_verbes / analyse->nb_mots_total) * 100 + // Pondère le ratio de verbes
        0.15 * ((double)analyse->nb_mots_uniques / analyse->nb_mots_total) * 100 + // Pondère la diversité des mots uniques
        0.15 * analyse->longueur_mot_moyenne // Pondère la longueur moyenne des mots
    );
}
// Ajoute un mot à la table de hachage dans l'analyse
void ajouterMot(AnalyseTexte* analyse, const wchar_t* mot) {
    // Calcule l'index de hachage pour le mot
    unsigned int index = calculerHash(mot);
    // Récupère le premier noeud dans la liste chaînée à cet index
    NoeudHash* courant = analyse->table_hash[index];

    // Parcourt la liste chaînée pour vérifier si le mot existe déjà
    while (courant != NULL) {
        if (wcscmp(courant->mot.mot, mot) == 0) { // Si le mot est trouvé
            courant->mot.frequence++; // Incrémente sa fréquence
            return; // Fin de la fonction
        }
        courant = courant->suivant; // Passe au noeud suivant
    }

    // Si le mot n'est pas trouvé, on l'ajoute comme un nouveau noeud
    NoeudHash* nouveau = (NoeudHash*)malloc(sizeof(NoeudHash));
    if (!nouveau) { // Vérifie si l'allocation mémoire a échoué
        perror("Erreur d'allocation mémoire");
        exit(EXIT_FAILURE); // Termine le programme avec une erreur
    }

    // Copie le mot dans la structure du noeud
    wcscpy(nouveau->mot.mot, mot);
    nouveau->mot.frequence = 1; // Initialise la fréquence à 1
    nouveau->mot.longueur = wcslen(mot); // Calcule la longueur du mot
    detecterTypeMot(&nouveau->mot); // Détecte les propriétés grammaticales du mot
    nouveau->suivant = analyse->table_hash[index]; // Pointe vers l'ancien premier noeud
    analyse->table_hash[index] = nouveau; // Met à jour la tête de la liste
    analyse->nb_mots_uniques++; // Incrémente le compteur de mots uniques

    // Met à jour les statistiques si le mot est un verbe ou un nom propre
    if (nouveau->mot.est_verbe) analyse->nb_verbes++;
    if (nouveau->mot.est_nom_propre) analyse->nb_noms_propres++;
}

void afficherTop10(const AnalyseTexte* analyse) {
    if (analyse->nb_mots_uniques == 0) {
        printf("Aucun mot à afficher.\n");
        return;
    }

    // Allocation dynamique du tableau de mots
    Mot* mots = (Mot*)malloc(analyse->nb_mots_uniques * sizeof(Mot));
    if (mots == NULL) {
        printf("Erreur d'allocation mémoire\n");
        return;
    }

    int nb_mots = 0;

    // Collecte des mots depuis la table de hachage
    for (int i = 0; i < TAILLE_HASHTABLE && nb_mots < analyse->nb_mots_uniques; i++) {
        NoeudHash* courant = analyse->table_hash[i];
        while (courant != NULL && nb_mots < analyse->nb_mots_uniques) {
            mots[nb_mots] = courant->mot;
            nb_mots++;
            courant = courant->suivant;
        }
    }

    // Tri des 10 premiers mots par fréquence
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

    // Affichage des résultats
    printf("\nTop %d des mots les plus fréquents:\n", (nb_mots < 10) ? nb_mots : 10);
    printf("-----------------------------------\n");
    for (int i = 0; i < 10 && i < nb_mots; i++) {
        printf("%d. %ls : %d occurrence%s",
            i + 1,
            mots[i].mot,
            mots[i].frequence,
            mots[i].frequence > 1 ? "s" : "");

        if (mots[i].est_verbe) printf(" (verbe)");
        if (mots[i].est_nom_propre) printf(" (nom propre)");
        printf("\n");
    }
    printf("-----------------------------------\n");

    // Libération de la mémoire
    free(mots);
}
/* Vérifie si une chaîne est un palindrome */
int estPalindrome(const wchar_t* texte) {
    if (!texte || !*texte) return 0;

    size_t len = wcslen(texte);
    wchar_t* texte_normalise = (wchar_t*)malloc((len + 1) * sizeof(wchar_t));
    if (!texte_normalise) return 0;

    size_t j = 0;
    for (size_t i = 0; i < len; i++) {
        if (iswalnum(texte[i])) {
            texte_normalise[j++] = towlower(texte[i]);
        }
    }
    texte_normalise[j] = L'\0';

    size_t debut = 0;
    size_t fin = j - 1;
    int est_palindrome = 1;

    while (debut < fin) {
        if (texte_normalise[debut] != texte_normalise[fin]) {
            est_palindrome = 0;
            break;
        }
        debut++;
        fin--;
    }

    free(texte_normalise);
    return est_palindrome;
}

/* Trouve et affiche les palindromes */
void trouverPalindromes(const AnalyseTexte* analyse) {
    printf("\nRecherche des palindromes dans le texte:\n");
    int palindromes_trouves = 0;

    for (int i = 0; i < TAILLE_HASHTABLE; i++) {
        NoeudHash* courant = analyse->table_hash[i];
        while (courant != NULL) {
            if (wcslen(courant->mot.mot) > 2 && estPalindrome(courant->mot.mot)) {
                printf("%ls (fréquence: %d)\n",
                       courant->mot.mot,
                       courant->mot.frequence);
                palindromes_trouves++;
            }
            courant = courant->suivant;
        }
    }

    if (palindromes_trouves == 0) {
        printf("Aucun palindrome trouvé dans le texte.\n");
    } else {
        printf("\nTotal des palindromes trouvés: %d\n", palindromes_trouves);
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
    printf("12. Rechercher les palindromes\n");
    printf("13. Statistiques détaillées (lignes, caractères, phrases extrêmes)\n");
    printf("0. Retour au menu précédent\n");
}

// 3. Ajouter ce case dans la fonction afficherMetriqueSpecifique
// (juste après le case 11)


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
        case 12:
            trouverPalindromes(analyse);
        break;
        case 13:
            printf("\nStatistiques détaillées du texte:\n");
        printf("-----------------------------------\n");
        printf("Caractères:\n");
        printf("  - Total avec espaces: %d\n", analyse->nb_caracteres);
        printf("  - Total sans espaces: %d\n", analyse->nb_chars_sans_espaces);
        printf("  - Nombre d'espaces: %d\n", analyse->nb_espaces);
        printf("\nStructure:\n");
        printf("  - Nombre de mots: %d\n", analyse->nb_mots_total);
        printf("  - Nombre de phrases: %d\n", analyse->nb_phrases);
        printf("  - Nombre de paragraphes: %d\n", analyse->nb_paragraphes);
        printf("\nPhrases extrêmes:\n");
        printf("Plus longue phrase (%d caractères):\n%ls\n",
               analyse->longueur_plus_longue,
               analyse->phrase_plus_longue);
        printf("\nPlus courte phrase (%d caractères):\n%ls\n",
               analyse->longueur_plus_courte,
               analyse->phrase_plus_courte);
        break;
    }
    }


void menuAnalyseFichierUnique(AnalyseTexte* analyse) {
    int choix;
    
    do {
        afficherMenuMetriques();
        printf("\nChoisissez une métrique (0 pour revenir): ");
        scanf("%d", &choix);
        getchar();

        if (choix >= 1 && choix <= 13) {
            afficherMetriqueSpecifique(analyse, choix);
        } else if (choix != 0) {
            printf("Choix invalide\n");
        }
    } while (choix != 0);
}
void analyserFichier(const char* chemin, AnalyseTexte* analyse) {
    FILE* fichier = fopen(chemin, "r");
    if (fichier == NULL) {
        perror("Erreur à l'ouverture du fichier");
        exit(EXIT_FAILURE);
    }

    // Initialisation des variables
    wchar_t mot_courant[LONGUEUR_MOT_MAX];
    wchar_t phrase_courante[1000] = {0};
    wint_t c;
    int pos_mot = 0;
    int pos_phrase = 0;
    int en_mot = 0;
    int mots_dans_phrase = 0;
    int en_paragraphe = 0;

    // Initialisation des compteurs
    analyse->nb_lignes = 1;
    analyse->nb_caracteres = 0;
    analyse->nb_espaces = 0;
    analyse->nb_chars_sans_espaces = 0;
    analyse->longueur_plus_longue = 0;
    analyse->longueur_plus_courte = INT_MAX;

    while ((c = fgetwc(fichier)) != WEOF) {
        // Gestion du comptage des caractères
        if (c == L' ') {
            analyse->nb_espaces++;
            analyse->nb_caracteres++;
        } else if (c != L'\n' && c != L'\r') {
            analyse->nb_chars_sans_espaces++;
            analyse->nb_caracteres++;
        }

        if (estCaractereMot(c)) {
            if (!en_mot) {
                en_mot = 1;
                analyse->nb_mots_total++;
                mots_dans_phrase++;
            }
            if (pos_mot < LONGUEUR_MOT_MAX - 1) {
                mot_courant[pos_mot++] = c;
            }
            if (pos_phrase < 999) {
                phrase_courante[pos_phrase++] = c;
                phrase_courante[pos_phrase] = L'\0';
            }
        } else {
            if (en_mot) {
                mot_courant[pos_mot] = L'\0';
                normaliserMot(mot_courant);
                ajouterMot(analyse, mot_courant);
                pos_mot = 0;
                en_mot = 0;
            }

            if (c == L'\n') {
                analyse->nb_lignes++;
                if (!en_paragraphe) {
                    analyse->nb_paragraphes++;
                    en_paragraphe = 1;
                }
            } else {
                en_paragraphe = 0;
            }

            if (c != L'\n' && c != L'\r') {
                if (pos_phrase < 999) {
                    phrase_courante[pos_phrase++] = c;
                    phrase_courante[pos_phrase] = L'\0';
                }
            }

            if (c == L'.' || c == L'!' || c == L'?') {
                analyse->nb_phrases++;
                analyse->longueur_phrase_moyenne += mots_dans_phrase;

                // Gérer la phrase complète
                if (pos_phrase > 0) {
                    if (pos_phrase > analyse->longueur_plus_longue) {
                        analyse->longueur_plus_longue = pos_phrase;
                        wcsncpy(analyse->phrase_plus_longue, phrase_courante, 999);
                        analyse->phrase_plus_longue[999] = L'\0';
                    }
                    if (pos_phrase < analyse->longueur_plus_courte && mots_dans_phrase > 0) {
                        analyse->longueur_plus_courte = pos_phrase;
                        wcsncpy(analyse->phrase_plus_courte, phrase_courante, 999);
                        analyse->phrase_plus_courte[999] = L'\0';
                    }
                }

                pos_phrase = 0;
                phrase_courante[0] = L'\0';
                mots_dans_phrase = 0;
            }
        }
    }

    // Traiter le dernier mot s'il y en a un
    if (en_mot) {
        mot_courant[pos_mot] = L'\0';
        normaliserMot(mot_courant);
        ajouterMot(analyse, mot_courant);
    }

    // Traiter la dernière phrase si elle ne se termine pas par un point
    if (mots_dans_phrase > 0) {
        analyse->nb_phrases++;
        analyse->longueur_phrase_moyenne += mots_dans_phrase;

        if (pos_phrase > analyse->longueur_plus_longue) {
            analyse->longueur_plus_longue = pos_phrase;
            wcsncpy(analyse->phrase_plus_longue, phrase_courante, 999);
            analyse->phrase_plus_longue[999] = L'\0';
        }
        if (pos_phrase < analyse->longueur_plus_courte) {
            analyse->longueur_plus_courte = pos_phrase;
            wcsncpy(analyse->phrase_plus_courte, phrase_courante, 999);
            analyse->phrase_plus_courte[999] = L'\0';
        }
    }

    // Calcul des moyennes et métriques finales
    if (analyse->nb_phrases > 0) {
        analyse->longueur_phrase_moyenne /= analyse->nb_phrases;
    }

    analyse->diversite_lexicale = (double)analyse->nb_mots_uniques / analyse->nb_mots_total;
    analyse->complexite_texte = calculerComplexiteTexte(analyse);

    fclose(fichier);
}
void menuComparaisonFichiers(const char* chemin1, const char* chemin2) {
    AnalyseTexte analyse1, analyse2;
    int choix;

    initialiserAnalyse(&analyse1);
    initialiserAnalyse(&analyse2);

    analyserFichier(chemin1, &analyse1);
    analyserFichier(chemin2, &analyse2);

    do {
        printf("\nMenu de comparaison:\n");
        printf("1. Voir métriques du premier fichier\n");
        printf("2. Voir métriques du deuxième fichier\n");
        printf("3. Voir comparaison des métriques\n");
        printf("4. Top 10 des mots du premier fichier\n");
        printf("5. Top 10 des mots du deuxième fichier\n");
        printf("6. Fréquence complète des mots du premier fichier\n");
        printf("7. Fréquence complète des mots du deuxième fichier\n");
        printf("8. Palindromes du premier fichier\n");
        printf("9. Palindromes du deuxième fichier\n");
        printf("0. Retour au menu principal\n");
        printf("Choix: ");
        scanf("%d", &choix);
        getchar();

        switch (choix) {
            case 0:
                break;

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
                    abs(analyse1.nb_noms_propres - analyse2.nb_noms_propres));
                break;
            case 4:
                printf("\nTop 10 des mots du premier fichier:\n");
            afficherTop10(&analyse1);
            break;
            case 5:
                printf("\nTop 10 des mots du deuxième fichier:\n");
            afficherTop10(&analyse2);
            break;
            case 6:
                printf("\nFréquence complète des mots du premier fichier:\n");
            afficherFrequenceComplete(&analyse1);
            break;
            case 7:
                printf("\nFréquence complète des mots du deuxième fichier:\n");
            afficherFrequenceComplete(&analyse2);
            break;
            case 8:
                printf("\nPalindromes du premier fichier:\n");
            trouverPalindromes(&analyse1);
            break;
            case 9:
                printf("\nPalindromes du deuxième fichier:\n");
            trouverPalindromes(&analyse2);
            break;

            default:
                printf("Choix invalide\n");
                break;
        }
    } while (choix != 0);

    libererAnalyse(&analyse1);
    libererAnalyse(&analyse2);
}


int main() {
    setlocale(LC_ALL, "");  // Support des caractères Unicode
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
            case 0:
                return EXIT_SUCCESS;

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