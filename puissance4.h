#ifndef PUISSANCE4_H_INCLUDED
#define PUISSANCE4_H_INCLUDED

/**
    Fonctions propres au fonctionnement du jeu.
*/

// Paramètres du jeu
#define LARGEUR_MAX 7 		// nb max de fils pour un noeud (= nb max de coups possibles)

// Macros
#define AUTRE_JOUEUR(i) (1-(i))

/** Critères de fin de partie */
typedef enum { NON, MATCHNUL, ORDI_GAGNE, HUMAIN_GAGNE } FinDePartie;

/** Définition du type Etat (état/position du jeu) */
typedef struct EtatSt {

	int joueur; // à qui de jouer ?

	// 6 lignes et 7 colonnes au Puissance 4
	char plateau[6][7];

} Etat;

/** Définition du type Coup */
typedef struct {

    // un coup de Puissance 4 est simplement défini par sa colonne
	int colonne;

} Coup;

/** Copier un état */
Etat * copieEtat(Etat * src);

/** Etat initial */
Etat * etat_initial(void);

/** Affiche le plateau de jeu */
void afficheJeu(Etat * etat);

/** Nouveau coup */
Coup * nouveauCoup(int col);

/** Demander à l'humain quel coup jouer
    Retourne NULL en cas d'input incorrect */
Coup * demanderCoup(void);

/** Modifier l'état en jouant un coup
    retourne 0 si le coup n'est pas possible */
int jouerCoup(Etat * etat, Coup * coup);

/** Retourne une liste de coups possibles à partir d'un etat
    (tableau de pointeurs de coups se terminant par NULL) */
Coup ** coups_possibles(Etat * etat);

/** Compte le nombre de coups possibles */
int nombre_coups_possibles(Etat * etat);

/** Test si l'état est un état terminal
    et retourne NON, MATCHNUL, ORDI_GAGNE ou HUMAIN_GAGNE */
FinDePartie testFin( Etat * etat );

#endif // PUISSANCE4_H_INCLUDED
