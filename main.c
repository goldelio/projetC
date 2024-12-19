#include <gtk/gtk.h>
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
    int nb_chars_sans_espaces;      //a
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

typedef struct {
    GtkWidget *main_menu_box;
    GtkWidget *analyze_menu_box;
    GtkWidget *metrics_menu_box;
    GtkWidget *compare_menu_box;
    GtkWidget *entry_file1; //for single file analysis
    GtkWidget *entry_file_1; //for comparison
    GtkWidget *entry_file_2; //for comparison
    GtkWidget *result_label; //element to display results on a single line
    GtkWidget *window;
    AnalyseTexte* current_analysis;
    AnalyseTexte* file1_analysis; //for comparison
    AnalyseTexte* file2_analysis; //for comparison
    GtkWidget *result_text_view;       // For long results
    GtkWidget *result_scroll_window;   // Scrollable container
    GtkTextBuffer *result_buffer;      // Text buffer for text view
} MenuWidgets;


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

// Fonction pour gérer l'extraction des phrases
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

static char* wchar_to_utf8(const wchar_t* str) {
    static char buffer[8192];  // Static buffer for the converted string
    memset(buffer, 0, sizeof(buffer));
    wcstombs(buffer, str, sizeof(buffer) - 1);
    return buffer;
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


void analyserFichier(const char* chemin, AnalyseTexte* analyse) {
    FILE* fichier = fopen(chemin, "r");
    if (fichier == NULL) {
        perror("Erreur à l'ouverture du fichier");
        exit(EXIT_FAILURE);
    }

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
    }

    // Calcul des moyennes et métriques finales
    if (analyse->nb_phrases > 0) {
        analyse->longueur_phrase_moyenne /= analyse->nb_phrases;
    }

    analyse->diversite_lexicale = (double)analyse->nb_mots_uniques / analyse->nb_mots_total;
    analyse->complexite_texte = calculerComplexiteTexte(analyse);

    fclose(fichier);
}

static char* get_detailed_statistics(const AnalyseTexte* analyse) {
    char* result = malloc(1000 * sizeof(char));
    if (result == NULL) {
        return NULL;
    }
    snprintf(result, 1000, "\nStatistiques détaillées du texte:\n"
                            "-----------------------------------\n"
                            "Caractères:\n"
                            "  - Total avec espaces: %d\n"
                            "  - Total sans espaces: %d\n"
                            "  - Nombre d'espaces: %d\n"
                            "\nStructure:\n"
                            "  - Nombre de mots: %d\n"
                            "  - Nombre de phrases: %d\n"
                            "  - Nombre de paragraphes: %d\n"
                            "\nPhrases extrêmes:\n"
                            "Plus longue phrase (%d caractères):\n%ls\n"
                            "\nPlus courte phrase (%d caractères):\n%ls\n",
                            analyse->nb_caracteres,
                            analyse->nb_chars_sans_espaces,
                            analyse->nb_espaces,
                            analyse->nb_mots_total,
                            analyse->nb_phrases,
                            analyse->nb_paragraphes,
                            analyse->longueur_plus_longue,
                            analyse->phrase_plus_longue,
                            analyse->longueur_plus_courte,
                            analyse->phrase_plus_courte);
    return result;
}

// analysis functions for metrics
static char* total_words(const AnalyseTexte* analyse) {
    static char result[100];
    snprintf(result, sizeof(result), "Total Words: %d", analyse->nb_mots_total);
    return result;
}

static char* unique_words(const AnalyseTexte* analyse) {
    static char result[100];
    snprintf(result, sizeof(result), "Unique Words: %d", analyse->nb_mots_uniques);
    return result;
}

static char* sentence_count(const AnalyseTexte* analyse) {
    static char result[100];
    snprintf(result, sizeof(result), "Sentences: %d", analyse->nb_phrases);
    return result;
}

static char* paragraph_count(const AnalyseTexte* analyse) {
    static char result[100];
    snprintf(result, sizeof(result), "Paragraphs: %d", analyse->nb_paragraphes);
    return result;
}

static char* avg_sentence_length(const AnalyseTexte* analyse) {
    static char result[100];
    snprintf(result, sizeof(result), "Average Sentence Length: %.2f words", analyse->longueur_phrase_moyenne);
    return result;
}

static char* lexical_diversity(const AnalyseTexte* analyse) {
    static char result[100];
    snprintf(result, sizeof(result), "Lexical Diversity: %.2f%%", analyse->diversite_lexicale * 100);
    return result;
}

static char* text_complexity(const AnalyseTexte* analyse) {
    static char result[100];
    snprintf(result, sizeof(result), "Text Complexity: %.2f", analyse->complexite_texte);
    return result;
}

static char* verb_count(const AnalyseTexte* analyse) {
    static char result[100];
    snprintf(result, sizeof(result), "Verbs: %d", analyse->nb_verbes);
    return result;
}

static char* proper_noun_count(const AnalyseTexte* analyse) {
    static char result[100];
    snprintf(result, sizeof(result), "Proper Nouns: %d", analyse->nb_noms_propres);
    return result;
}

static void export_analysis(const AnalyseTexte* analyse) {
    FILE* fichier = fopen("analyse.txt", "w");
    if (fichier == NULL) {
        perror("Erreur à l'ouverture du fichier");
        exit(EXIT_FAILURE);
    }

    fprintf(fichier, "Total Words: %d\n", analyse->nb_mots_total);
    fprintf(fichier, "Unique Words: %d\n", analyse->nb_mots_uniques);
    fprintf(fichier, "Sentences: %d\n", analyse->nb_phrases);
    fprintf(fichier, "Paragraphs: %d\n", analyse->nb_paragraphes);
    fprintf(fichier, "Average Sentence Length: %.2f words\n", analyse->longueur_phrase_moyenne);
    fprintf(fichier, "Lexical Diversity: %.2f%%\n", analyse->diversite_lexicale * 100);
    fprintf(fichier, "Text Complexity: %.2f\n", analyse->complexite_texte);
    fprintf(fichier, "Verbs: %d\n", analyse->nb_verbes);
    fprintf(fichier, "Proper Nouns: %d\n", analyse->nb_noms_propres);

    fprintf(fichier, get_detailed_statistics(analyse));

    fprintf(fichier, "\nComplete Word Frequency:\n");
    fprintf(fichier, "---------------------------------\n");

    // Count the number of unique words
    int unique_words_count = 0;
    for (int i = 0; i < TAILLE_HASHTABLE; i++) {
        NoeudHash* courant = analyse->table_hash[i];
        while (courant != NULL) {
            unique_words_count++;
            courant = courant->suivant;
        }
    }

    // Create an array to store the unique words and their frequencies
    Mot* unique_words_array = malloc(unique_words_count * sizeof(Mot));
    int index = 0;
    for (int i = 0; i < TAILLE_HASHTABLE; i++) {
        NoeudHash* courant = analyse->table_hash[i];
        while (courant != NULL) {
            unique_words_array[index] = courant->mot;
            index++;
            courant = courant->suivant;
        }
    }

    // Sort the array in ascending order of frequency
    for (int i = 0; i < unique_words_count - 1; i++) {
        for (int j = 0; j < unique_words_count - i - 1; j++) {
            if (unique_words_array[j].frequence > unique_words_array[j + 1].frequence) {
                Mot temp = unique_words_array[j];
                unique_words_array[j] = unique_words_array[j + 1];
                unique_words_array[j + 1] = temp;
            }
        }
    }

    // Print the sorted array to the file
    for (int i = 0; i < unique_words_count; i++) {
        fprintf(fichier, "%ls: %d occurrences", unique_words_array[i].mot, unique_words_array[i].frequence);
        if (unique_words_array[i].est_verbe) fprintf(fichier, " (verbe)");
        if (unique_words_array[i].est_nom_propre) fprintf(fichier, " (nom propre)");
        fprintf(fichier, "\n");
    }

    free(unique_words_array);

    fclose(fichier);
}

static char* get_top_ten_words(const AnalyseTexte* analyse) {
    static char result[8192];
    char temp[1024];  // Increased buffer size for UTF-8 characters
    result[0] = '\0';
    
    if (analyse->nb_mots_uniques == 0) {
        strcat(result, "No words to display.\n");
        return result;
    }

    Mot* mots = (Mot*)malloc(analyse->nb_mots_uniques * sizeof(Mot));
    if (mots == NULL) {
        strcat(result, "Memory allocation error\n");
        return result;
    }

    // Collect words from hash table
    int nb_mots = 0;
    for (int i = 0; i < TAILLE_HASHTABLE && nb_mots < analyse->nb_mots_uniques; i++) {
        NoeudHash* courant = analyse->table_hash[i];
        while (courant != NULL && nb_mots < analyse->nb_mots_uniques) {
            mots[nb_mots] = courant->mot;
            nb_mots++;
            courant = courant->suivant;
        }
    }

    // Sort first 10 words by frequency
    for (int i = 0; i < 10 && i < nb_mots; i++) {
        int max_idx = i;
        for (int j = i + 1; j < nb_mots; j++) {
            if (mots[j].frequence > mots[max_idx].frequence) {
                max_idx = j;
            }
        }
        if (max_idx != i) {
            Mot temp_mot = mots[i];
            mots[i] = mots[max_idx];
            mots[max_idx] = temp_mot;
        }
    }

    strcat(result, "Top words by frequency:\n\n");

    // Use wide character formatting
    for (int i = 0; i < 10 && i < nb_mots; i++) {
        temp[0] = '\0';  // Clear temp buffer
        
        // Convert the numeric parts and fixed strings to a temporary buffer
        char prefix[64];
        snprintf(prefix, sizeof(prefix), "%d. ", i + 1);
        strcat(temp, prefix);
        
        // Convert wide string to multibyte
        char word_buffer[512];
        wcstombs(word_buffer, mots[i].mot, sizeof(word_buffer));
        strcat(temp, word_buffer);
        
        // Add frequency and other information
        char suffix[256];
        snprintf(suffix, sizeof(suffix), ": %d occurrence%s%s%s\n",
                mots[i].frequence,
                mots[i].frequence > 1 ? "s" : "",
                mots[i].est_verbe ? " (verb)" : "",
                mots[i].est_nom_propre ? " (proper noun)" : "");
        strcat(temp, suffix);
        
        // Add to final result
        strcat(result, temp);
    }
    free(mots);
    return result;
}



static char* get_palindromes(const AnalyseTexte* analyse) {
    static char result[8192];
    char temp[256];
    int palindromes_trouves = 0;
    
    result[0] = '\0';
    strcat(result, "Palindromes found in text:\n\n");

    for (int i = 0; i < TAILLE_HASHTABLE; i++) {
        NoeudHash* courant = analyse->table_hash[i];
        while (courant != NULL) {
            if (wcslen(courant->mot.mot) > 2 && estPalindrome(courant->mot.mot)) {
                snprintf(temp, sizeof(temp), "%s (frequency: %d)\n",
                        wchar_to_utf8(courant->mot.mot),  // Convert to UTF-8
                        courant->mot.frequence);
                strcat(result, temp);
                palindromes_trouves++;
            }
            courant = courant->suivant;
        }
    }

    if (palindromes_trouves == 0) {
        strcat(result, "No palindromes found in text.\n");
    } else {
        snprintf(temp, sizeof(temp), "\nTotal palindromes found: %d\n", palindromes_trouves);
        strcat(result, temp);
    }

    return result;
}

static char* get_word_frequency(const AnalyseTexte* analyse) {
    static char result[8192];
    char temp[256];
    
    result[0] = '\0';
    strcat(result, "Complete word frequency:\n\n");

    for (int i = 0; i < TAILLE_HASHTABLE; i++) {
        NoeudHash* courant = analyse->table_hash[i];
        while (courant != NULL) {
            snprintf(temp, sizeof(temp), "%s: %d occurrence%s%s%s\n",
                    wchar_to_utf8(courant->mot.mot),  // Convert to UTF-8
                    courant->mot.frequence,
                    courant->mot.frequence > 1 ? "s" : "",
                    courant->mot.est_verbe ? " (verb)" : "",
                    courant->mot.est_nom_propre ? " (proper noun)" : "");
            strcat(result, temp);
            courant = courant->suivant;
        }
    }

    return result;
}

//different function to select the menus 
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
static void show_main_menu(MenuWidgets *widgets) {
    gtk_widget_set_visible(widgets->main_menu_box, TRUE);
    gtk_widget_set_visible(widgets->analyze_menu_box, FALSE);
    gtk_widget_set_visible(widgets->metrics_menu_box, FALSE);
    gtk_widget_set_visible(widgets->compare_menu_box, FALSE);
}

static void show_analyze_menu(MenuWidgets *widgets) {
    gtk_widget_set_visible(widgets->main_menu_box, FALSE);
    gtk_widget_set_visible(widgets->analyze_menu_box, TRUE);
    gtk_widget_set_visible(widgets->metrics_menu_box, FALSE);
    gtk_widget_set_visible(widgets->compare_menu_box, FALSE);
}

static void show_metrics_menu(MenuWidgets *widgets) {
    gtk_widget_set_visible(widgets->main_menu_box, FALSE);
    gtk_widget_set_visible(widgets->analyze_menu_box, FALSE);
    gtk_widget_set_visible(widgets->metrics_menu_box, TRUE);
    gtk_widget_set_visible(widgets->compare_menu_box, FALSE);
}

static void show_compare_menu(MenuWidgets *widgets) {
    gtk_widget_set_visible(widgets->main_menu_box, FALSE);
    gtk_widget_set_visible(widgets->analyze_menu_box, FALSE);
    gtk_widget_set_visible(widgets->metrics_menu_box, FALSE);
    gtk_widget_set_visible(widgets->compare_menu_box, TRUE);
}
//--------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
//end of the different function to select the menus


//function to quit app
static void on_quit_clicked(GtkWidget *button, gpointer user_data) {
    GtkWindow *window = GTK_WINDOW(user_data);
    gtk_window_destroy(window);
}

//function to go to previous menu
static void on_back_clicked(GtkWidget *button, gpointer user_data) {
    MenuWidgets *widgets = (MenuWidgets *)user_data;
    show_main_menu(widgets);
    gtk_widget_set_visible(widgets->result_scroll_window, FALSE); //hide the result text view when back to main menu
    gtk_widget_set_visible(widgets->result_label, FALSE);
}

//function to go back to analysis menu
static void on_back_to_analyze_clicked(GtkWidget *button, gpointer user_data) {
    MenuWidgets *widgets = (MenuWidgets *)user_data;
    show_analyze_menu(widgets);
    gtk_widget_set_visible(widgets->result_scroll_window, FALSE); //hide the result text view when back to main menu
    gtk_widget_set_visible(widgets->result_label, FALSE);
}

static void on_analyze_clicked(GtkWidget *button, gpointer user_data) {
    MenuWidgets *widgets = (MenuWidgets *)user_data;
    show_analyze_menu(widgets); //sends to metrics menu
}

static void on_compare_clicked(GtkWidget *button, gpointer user_data) {
    MenuWidgets *widgets = (MenuWidgets *)user_data;
    show_compare_menu(widgets); //sends to compare menu
}

static void on_analyze_file(GtkWidget *button, gpointer user_data) {
    MenuWidgets *widgets = (MenuWidgets *)user_data;
    const char *filepath = gtk_editable_get_text(GTK_EDITABLE(widgets->entry_file1));

    // Free previous analysis if it exists
    if (widgets->current_analysis != NULL) {
        libererAnalyse(widgets->current_analysis);
        free(widgets->current_analysis);
    }

    // Allocate new analysis
    widgets->current_analysis = malloc(sizeof(AnalyseTexte));
    initialiserAnalyse(widgets->current_analysis);
    analyserFichier(filepath, widgets->current_analysis);
    
    char result[256];
    snprintf(result, sizeof(result), "File loaded: %s\nSelect a metric to analyze", filepath);
    gtk_label_set_text(GTK_LABEL(widgets->result_label), result);
    
    show_metrics_menu(widgets);
}

static void set_text_buffer_safely(GtkTextBuffer *buffer, const char *text) {
    //function created to make sure the text is utf8 legal before inputing it inside a GTK buffer (otherwise would display nothing and a error message in console)
    if (!text) {
        gtk_text_buffer_set_text(buffer, "", -1);
        return;
    }

    // First check if it's valid UTF-8
    if (g_utf8_validate(text, -1, NULL)) {
        // Text is valid UTF-8, use it directly
        gtk_text_buffer_set_text(buffer, text, -1);
        return;
    }

    // Text is not valid UTF-8, try to convert from local encoding
    GError *error = NULL;
    char *utf8_text = g_locale_to_utf8(text, -1, NULL, NULL, &error);
    
    if (utf8_text) {
        // Successfully converted to UTF-8
        gtk_text_buffer_set_text(buffer, utf8_text, -1);
        g_free(utf8_text);
    } else {
        // Conversion failed
        g_warning("Failed to conve rt text to UTF-8: %s", error->message);
        g_error_free(error);
        // Set a fallback message
        gtk_text_buffer_set_text(buffer, "[Invalid text encoding]", -1);
    }
}

static void on_compare_files(GtkWidget *button, gpointer user_data) {
    MenuWidgets *widgets = (MenuWidgets *)user_data;
    const char *filepath1 = gtk_editable_get_text(GTK_EDITABLE(widgets->entry_file_1));
    const char *filepath2 = gtk_editable_get_text(GTK_EDITABLE(widgets->entry_file_2));
    
    // Create two separate analyses for each file
    AnalyseTexte *analyse1 = malloc(sizeof(AnalyseTexte));
    AnalyseTexte *analyse2 = malloc(sizeof(AnalyseTexte));
    
    if (!analyse1 || !analyse2) {
        gtk_label_set_text(GTK_LABEL(widgets->result_label), "Memory allocation error!");
        free(analyse1);
        free(analyse2);
        return;
    }

    // Initialize and analyze both files
    initialiserAnalyse(analyse1);
    initialiserAnalyse(analyse2);
    analyserFichier(filepath1, analyse1);
    analyserFichier(filepath2, analyse2);

    // Create formatted comparison text
    char result[4096];
    snprintf(result, sizeof(result),
        "Comparison of metrics between files:\n\n"
        "Total Words difference: %d\n"
        "Unique Words difference: %d\n"
        "Sentences difference: %d\n"
        "Average Sentence Length difference: %.2f\n"
        "Lexical Diversity difference: %.2f%%\n"
        "Text Complexity difference: %.2f\n"
        "Verbs difference: %d\n"
        "Proper Nouns difference: %d\n\n"
        "Individual Statistics:\n"
        "File 1: %s\n"
        "- Total Words: %d\n"
        "- Unique Words: %d\n"
        "- Text Complexity: %.2f\n\n"
        "File 2: %s\n"
        "- Total Words: %d\n"
        "- Unique Words: %d\n"
        "- Text Complexity: %.2f",
        abs(analyse1->nb_mots_total - analyse2->nb_mots_total),
        abs(analyse1->nb_mots_uniques - analyse2->nb_mots_uniques),
        abs(analyse1->nb_phrases - analyse2->nb_phrases),
        fabs(analyse1->longueur_phrase_moyenne - analyse2->longueur_phrase_moyenne),
        fabs(analyse1->diversite_lexicale - analyse2->diversite_lexicale) * 100,
        fabs(analyse1->complexite_texte - analyse2->complexite_texte),
        abs(analyse1->nb_verbes - analyse2->nb_verbes),
        abs(analyse1->nb_noms_propres - analyse2->nb_noms_propres),
        filepath1,
        analyse1->nb_mots_total,
        analyse1->nb_mots_uniques,
        analyse1->complexite_texte,
        filepath2,
        analyse2->nb_mots_total,
        analyse2->nb_mots_uniques,
        analyse2->complexite_texte
    );

    // Use the text view for displaying the comparison results
    set_text_buffer_safely(widgets->result_buffer, result);
    gtk_widget_set_visible(widgets->result_label, FALSE);
    gtk_widget_set_visible(widgets->result_scroll_window, TRUE);

    // Clean up
    libererAnalyse(analyse1);
    libererAnalyse(analyse2);
    free(analyse1);
    free(analyse2);
}



static void on_metric_clicked(GtkWidget *button, gpointer user_data) {
    //function called when the user clicks on a button for the single file analysis
    MenuWidgets *widgets = (MenuWidgets *)user_data;
    const char *label = gtk_button_get_label(GTK_BUTTON(button));
    char *result = NULL;
    
    if (!widgets->current_analysis) {
        gtk_label_set_text(GTK_LABEL(widgets->result_label), "No file analyzed yet!");
        gtk_widget_set_visible(widgets->result_scroll_window, FALSE);
        gtk_widget_set_visible(widgets->result_label, TRUE);
        return;
    }
    
    // For shorter metrics, use label
    if (strstr(label, "1. Total Words") ||
        strstr(label, "2. Unique Words") ||
        strstr(label, "3. Sentences") ||
        strstr(label, "4. Paragraphs") ||
        strstr(label, "5. Average Sentence Length") ||
        strstr(label, "6. Lexical Diversity") ||
        strstr(label, "7. Text Complexity") ||
        strstr(label, "8. Verbs") ||
        strstr(label, "9. Proper Nouns")||
        strstr(label, "14. Download analysis")) {
        
        if (strstr(label, "1. Total Words")) 
            result = total_words(widgets->current_analysis);
        else if (strstr(label, "2. Unique Words")) 
            result = unique_words(widgets->current_analysis);
        else if (strstr(label, "3. Sentences")) 
            result = sentence_count(widgets->current_analysis);
        else if (strstr(label, "4. Paragraphs")) 
            result = paragraph_count(widgets->current_analysis);
        else if (strstr(label, "5. Average Sentence Length")) 
            result = avg_sentence_length(widgets->current_analysis);
        else if (strstr(label, "6. Lexical Diversity")) 
            result = lexical_diversity(widgets->current_analysis);
        else if (strstr(label, "7. Text Complexity")) 
            result = text_complexity(widgets->current_analysis);
        else if (strstr(label, "8. Verbs")) 
            result = verb_count(widgets->current_analysis);
        else if (strstr(label, "9. Proper Nouns")) 
            result = proper_noun_count(widgets->current_analysis);
        else if (strstr(label, "14. Download analysis")){
            export_analysis(widgets->current_analysis);
            result = "Analysis saved in file 'analyse.txt'";
        } 
            

        gtk_label_set_text(GTK_LABEL(widgets->result_label), result);
        gtk_widget_set_visible(widgets->result_scroll_window, FALSE);
        gtk_widget_set_visible(widgets->result_label, TRUE);
    }
    // For longer metrics, use scrollable text view
    else {
        if (strstr(label, "10. Top 10 Words")) 
            result = get_top_ten_words(widgets->current_analysis);
        else if (strstr(label, "11. Word Frequency")) 
            result = get_word_frequency(widgets->current_analysis);
        else if (strstr(label, "12. Palindromes")) 
            result = get_palindromes(widgets->current_analysis);
        else if(strstr(label, "13. Detailed statistics")) 
            result = get_detailed_statistics(widgets->current_analysis);
        if (result) {
            set_text_buffer_safely(widgets->result_buffer, result);
            gtk_widget_set_visible(widgets->result_label, FALSE);
            gtk_widget_set_visible(widgets->result_scroll_window, TRUE);
        }
        
    }
}

static void cleanup_widgets(MenuWidgets *widgets) {
    if (widgets->current_analysis != NULL) {
        libererAnalyse(widgets->current_analysis);
        free(widgets->current_analysis);
    }
    g_free(widgets);
}

static void on_activate(GtkApplication *app) {
    setlocale(LC_ALL, "");

    MenuWidgets *widgets = g_new(MenuWidgets, 1);
    widgets->current_analysis = NULL;
    
    widgets->window = gtk_application_window_new(app);
    gtk_window_set_title(GTK_WINDOW(widgets->window), "Text Analysis Tool");
    gtk_window_set_default_size(GTK_WINDOW(widgets->window), 500, 400);

    GtkWidget *main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_margin_start(main_box, 20);
    gtk_widget_set_margin_end(main_box, 20);
    gtk_widget_set_margin_top(main_box, 20);
    gtk_widget_set_margin_bottom(main_box, 20);

    // Main menu
    widgets->main_menu_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    GtkWidget *analyze_button = gtk_button_new_with_label("1. Analyze a file");
    GtkWidget *compare_button = gtk_button_new_with_label("2. Compare two files");
    GtkWidget *quit_button = gtk_button_new_with_label("0. Quit");
    
    gtk_box_append(GTK_BOX(widgets->main_menu_box), analyze_button);
    gtk_box_append(GTK_BOX(widgets->main_menu_box), compare_button);
    gtk_box_append(GTK_BOX(widgets->main_menu_box), quit_button);

    // Analyze menu
    widgets->analyze_menu_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    widgets->entry_file1 = gtk_entry_new();
    GtkWidget *analyze_label = gtk_label_new("Enter file path to analyze:");
    GtkWidget *analyze_file_button = gtk_button_new_with_label("Analyze");
    GtkWidget *back_button1 = gtk_button_new_with_label("Back to main menu");
    
    gtk_box_append(GTK_BOX(widgets->analyze_menu_box), analyze_label);
    gtk_box_append(GTK_BOX(widgets->analyze_menu_box), widgets->entry_file1);
    gtk_box_append(GTK_BOX(widgets->analyze_menu_box), analyze_file_button);
    gtk_box_append(GTK_BOX(widgets->analyze_menu_box), back_button1);

    // Metrics menu with 2-column layout
    widgets->metrics_menu_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    GtkWidget *metrics_label = gtk_label_new("Available Metrics:");
    gtk_box_append(GTK_BOX(widgets->metrics_menu_box), metrics_label);

    const char *metric_labels[] = {
        "1. Total Words", "2. Unique Words", "3. Sentences",
        "4. Paragraphs", "5. Average Sentence Length",
        "6. Lexical Diversity", "7. Text Complexity",
        "8. Verbs", "9. Proper Nouns", "10. Top 10 Words",
        "11. Word Frequency", "12. Palindromes", "13. Detailed statistics","14. Download analysis"
    };

    // Create pairs of metric buttons
    for (int i = 0; i < 14; i += 2) {
        GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
        // Make the row expand horizontally
        gtk_widget_set_hexpand(row, TRUE);
        
        // First button in pair
        GtkWidget *metric_button1 = gtk_button_new_with_label(metric_labels[i]);
        // Make the button expand to fill available space
        gtk_widget_set_hexpand(metric_button1, TRUE);
        gtk_widget_set_vexpand(metric_button1, FALSE);
        gtk_box_append(GTK_BOX(row), metric_button1);
        g_signal_connect(metric_button1, "clicked", G_CALLBACK(on_metric_clicked), widgets);
        
        // Second button in pair (if it exists)
        if (i + 1 < 14) {
            GtkWidget *metric_button2 = gtk_button_new_with_label(metric_labels[i + 1]);
            // Make the button expand to fill available space
            gtk_widget_set_hexpand(metric_button2, TRUE);
            gtk_widget_set_vexpand(metric_button2, FALSE);
            gtk_box_append(GTK_BOX(row), metric_button2);
            g_signal_connect(metric_button2, "clicked", G_CALLBACK(on_metric_clicked), widgets);
        } else {
            // If we have an odd number of buttons, add an invisible placeholder
            // to maintain layout consistency
            GtkWidget *placeholder = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
            gtk_widget_set_hexpand(placeholder, TRUE);
            gtk_box_append(GTK_BOX(row), placeholder);
        }
        
        gtk_box_append(GTK_BOX(widgets->metrics_menu_box), row);
    }

    GtkWidget *back_button_metrics = gtk_button_new_with_label("Back to file selection");
    gtk_box_append(GTK_BOX(widgets->metrics_menu_box), back_button_metrics);

    // Compare menu
    widgets->compare_menu_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    widgets->entry_file_1 = gtk_entry_new();
    widgets->entry_file_2 = gtk_entry_new();
    GtkWidget *compare_label1 = gtk_label_new("Enter first file path:");
    GtkWidget *compare_label2 = gtk_label_new("Enter second file path:");
    GtkWidget *compare_files_button = gtk_button_new_with_label("Compare");
    GtkWidget *back_button2 = gtk_button_new_with_label("Back to main menu");
    
    gtk_box_append(GTK_BOX(widgets->compare_menu_box), compare_label1);
    gtk_box_append(GTK_BOX(widgets->compare_menu_box), widgets->entry_file_1);
    gtk_box_append(GTK_BOX(widgets->compare_menu_box), compare_label2);
    gtk_box_append(GTK_BOX(widgets->compare_menu_box), widgets->entry_file_2);
    gtk_box_append(GTK_BOX(widgets->compare_menu_box), compare_files_button);
    gtk_box_append(GTK_BOX(widgets->compare_menu_box), back_button2);

    // Result label
    widgets->result_label = gtk_label_new("");
    gtk_label_set_wrap(GTK_LABEL(widgets->result_label), TRUE);

    // Create scrollable text view for long results
    widgets->result_text_view = gtk_text_view_new();
    widgets->result_buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(widgets->result_text_view));
    widgets->result_scroll_window = gtk_scrolled_window_new();
    
    // Configure text view
    gtk_text_view_set_wrap_mode(GTK_TEXT_VIEW(widgets->result_text_view), GTK_WRAP_WORD);
    gtk_text_view_set_editable(GTK_TEXT_VIEW(widgets->result_text_view), FALSE);
    
    // Configure scroll window
    gtk_scrolled_window_set_child(GTK_SCROLLED_WINDOW(widgets->result_scroll_window), 
                                widgets->result_text_view);
    gtk_widget_set_size_request(widgets->result_scroll_window, -1, 200);

    // Add all containers to main box
    gtk_box_append(GTK_BOX(main_box), widgets->main_menu_box);
    gtk_box_append(GTK_BOX(main_box), widgets->analyze_menu_box);
    gtk_box_append(GTK_BOX(main_box), widgets->metrics_menu_box);
    gtk_box_append(GTK_BOX(main_box), widgets->compare_menu_box);
    gtk_box_append(GTK_BOX(main_box), widgets->result_label);
    gtk_box_append(GTK_BOX(main_box), widgets->result_scroll_window);

    // Initially hide both result displays
    gtk_widget_set_visible(widgets->result_scroll_window, FALSE);
    gtk_widget_set_visible(widgets->result_label, FALSE);

    // Connect signals
    g_signal_connect(quit_button, "clicked", G_CALLBACK(on_quit_clicked), widgets->window);
    g_signal_connect(analyze_button, "clicked", G_CALLBACK(on_analyze_clicked), widgets);
    g_signal_connect(compare_button, "clicked", G_CALLBACK(on_compare_clicked), widgets);
    g_signal_connect(back_button1, "clicked", G_CALLBACK(on_back_clicked), widgets);
    g_signal_connect(back_button2, "clicked", G_CALLBACK(on_back_clicked), widgets);
    g_signal_connect(back_button_metrics, "clicked", G_CALLBACK(on_back_to_analyze_clicked), widgets);
    g_signal_connect(analyze_file_button, "clicked", G_CALLBACK(on_analyze_file), widgets);
    g_signal_connect(compare_files_button, "clicked", G_CALLBACK(on_compare_files), widgets);

    // Show main menu, hide others
    show_main_menu(widgets);

    // Setup window
    gtk_window_set_child(GTK_WINDOW(widgets->window), main_box);
    gtk_window_present(GTK_WINDOW(widgets->window));

    g_signal_connect_swapped(widgets->window, "destroy", G_CALLBACK(cleanup_widgets), widgets);
}



int main(int argc, char **argv) {
    GtkApplication *app;
    int status;
    
    app = gtk_application_new("org.gtk.textanalysis", G_APPLICATION_DEFAULT_FLAGS);
    g_signal_connect(app, "activate", G_CALLBACK(on_activate), NULL);
    status = g_application_run(G_APPLICATION(app), argc, argv);
    g_object_unref(app);
    
    return status;
}