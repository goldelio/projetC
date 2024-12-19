**# Outil d'Analyse de Texte**

Une application basée sur GTK offrant des capacités complètes d'analyse de texte pour les fichiers texte, incluant l'analyse statistique et la comparaison entre fichiers.

**## Fonctionnalités**
- Analyse de fichier unique avec plusieurs métriques :
  - Comptage de mots (total et uniques)
  - Analyse des phrases et paragraphes
  - Calcul de la diversité lexicale
  - Évaluation de la complexité du texte
  - Analyse de la fréquence des mots
  - Détection des verbes et noms propres
  - Identification des palindromes
  - Calcul de la longueur moyenne des phrases
- Fonctionnalité de comparaison entre deux fichiers
- Exportation d'analyse détaillée vers un fichier texte
- Interface GTK conviviale
- Support Unicode/UTF-8

**## Prérequis Techniques**
- GTK 4.0 ou supérieur
- Compilateur C (gcc recommandé)
- Bibliothèques C standard
- Support des caractères larges (wchar)

**## Compilation du Projet**
```bash
# Compiler avec le support GTK
gcc -o text_analyzer main.c `pkg-config --cflags --libs gtk4`
```

**## Utilisation**
1. Lancer l'application :
```bash
./text_analyzer
```
2. Choisir entre :
   - Analyse d'un fichier unique
   - Comparaison de deux fichiers
3. Pour l'analyse d'un fichier unique :
   - Saisir le chemin du fichier
   - Sélectionner parmi les métriques disponibles
   - Voir les résultats dans l'interface
   - Optionnellement exporter l'analyse complète
4. Pour la comparaison de fichiers :
   - Saisir les chemins des deux fichiers
   - Voir l'analyse comparative

**## Implémentation des Fonctionnalités Clés**
- Stockage des mots basé sur une table de hachage pour une recherche efficace
- Calcul avancé des métriques de texte
- Support des fichiers texte encodés en UTF-8
- Traitement du texte économe en mémoire
- Extensible pour les grands fichiers texte (jusqu'à 20 000 mots)
