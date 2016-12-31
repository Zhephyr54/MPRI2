#ifndef MCTS_H_INCLUDED
#define MCTS_H_INCLUDED

#include "puissance4.h"
#include <stdbool.h>

/**
    Fonctions d'implémentation de l'algorithme MCTS avec UCB (UCT).
*/

/** Méthode du choix du coup à jouer pour MCTS */
typedef enum { MAX, ROBUSTE } MethodeChoixCoup;

/** Definition du type Noeud */
typedef struct NoeudSt {

	int joueur; // joueur qui a joué pour arriver ici
	Coup * coup;   // coup joué par ce joueur pour arriver ici

	Etat * etat; // etat du jeu

	struct NoeudSt * parent;
	struct NoeudSt * enfants[LARGEUR_MAX]; // liste d'enfants : chaque enfant correspond à un coup possible
	int nb_enfants;	// nb d'enfants présents dans la liste

	// POUR MCTS:
	int nb_victoires;   // Pour calculer les stats
	double sommes_recompenses;  // Pour calculer la B-valeur (car match nul pris en compte)
	int nb_simus;

} Noeud;

/** Créer un nouveau noeud en jouant un coup à partir d'un parent
    utiliser nouveauNoeud(NULL, NULL) pour créer la racine */
Noeud * nouveauNoeud (Noeud * parent, Coup * coup);

/** Ajouter un enfant à un parent en jouant un coup
    retourne le pointeur sur l'enfant ajouté */
Noeud * ajouterEnfant(Noeud * parent, Coup * coup);

/** Libère la mémoire d'un noeud et de ses enfants récursivement */
void freeNoeud (Noeud * noeud);

/** Calcule la B-valeur d'un noeud */
double calculerBValeurNoeud(Noeud * noeud);

/** Sélectionne récursivement à partir de la racine (passée en paramètre)
    le noeud avec la plus grande B-valeur jusqu'à arriver à un noeud terminal
    ou un dont tous les fils n'ont pas été développés */
Noeud * selectionUCB(Noeud * racine);

/** Réalise l'expansion d'un noeud en développant un de ses fils au hasard
    et retourne ce fils.
    Si le noeud représente un état final, retourne simplement celui-ci. */
Noeud * expansionNoeud(Noeud * noeud);

/** Simule le déroulement de la partie à partir d'un état
    jusqu'à la fin et retourne l'état final.
    Si choisirCoupGagnant est à vrai,
    on améliore les simulations en choisissant un coup gagnant lorsque cela est possible. */
FinDePartie simulerPartie(Etat * etat, bool choisirCoupGagnant);

/** Propage le résultat à partir d'un noeud
    en remontant le résultat de la partie
    aux parents de ce noeud.*/
void propagerResultat(Noeud * noeud, FinDePartie resultat);

/** Trouve le noeud correspondant au meilleur coup possible
    en utilisant la méthode spécifié
    à partir de la racine. */
Noeud * trouverNoeudMeilleurCoup(Noeud * racine, MethodeChoixCoup methode);

/** Calcule et joue un coup de l'ordinateur avec MCTS-UCT
    en tempsmax secondes ou avec iterationxmax itérations (selon le plus limitant)
    (l'un de ces deux paramètres peut être ignoré en le mettant à une valeur nulle ou négative)
    et en choisissant le coup à l'aide de la méthode methodeChoix.

    *** Niveau d'optimisation de l'algorithme ***
                   0 : fonctionnement basique de l'algorithme MCTS avec UCB (UCT) (les simulations sont réalisées au hasard).
    (par défaut)   1 : (QUESTION 3 :) amélioration des simulations consistant à toujours choisir un coup gagnant lorsque cela est possible.

    *** Niveau de verbosité du programme ***
                   0 : aucun affichage autre que la demande de coup et le plateau.
    (par défaut)   1 : (QUESTION 1:) affichage (à chaque coup de l’ordinateur) du coup joué, du nombre total de simulations réalisées (= nombre d'itérations) et d'une estimation de la probabilité de victoire pour l’ordinateur.
                   2 : affichage (à chaque coup de l’ordinateur) du temps passé dans la boucle principale de l'algorithme MCTS et du nombre d'itérations réalisées.
                   3 : affichage (à chaque coup de l’ordinateur) du nombre de simulations réalisées pour chaque coup.
                   4 : affichage (à chaque coup de l’ordinateur) de la moyenne des récompenses pour chaque coup.
    */
void ordijoue_mcts(Etat * etat, double tempsmax, int iterationsmax, MethodeChoixCoup methodeChoix, int optimisationLevel, int verboseLevel);

#endif // MCTS_H_INCLUDED
