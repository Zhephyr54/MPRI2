/*
	Canvas pour algorithmes de jeux � 2 joueurs

	joueur 0 : humain
	joueur 1 : ordinateur

*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>

// Param�tres du jeu
#define LARGEUR_MAX 7 		// nb max de fils pour un noeud (= nb max de coups possibles)

#define TEMPS 5		// temps de calcul pour un coup avec MCTS (en secondes)

// Constantes/param�tres Algo MCTS
#define RECOMPENSE_ORDI_GAGNE 1
#define RECOMPENSE_MATCHNUL 0.5
#define RECOMPENSE_HUMAIN_GAGNE 0

#define CONSTANTE_C 1.4142  // ~ Racine carr� de 2

// macros
#define AUTRE_JOUEUR(i) (1-(i))
#define min(a, b)       ((a) < (b) ? (a) : (b))
#define max(a, b)       ((a) < (b) ? (b) : (a))

#define ARRAY_LENGTH(x)  (sizeof(x) / sizeof((x)[0]))

// Crit�res de fin de partie
typedef enum {NON, MATCHNUL, ORDI_GAGNE, HUMAIN_GAGNE } FinDePartie;

// Definition du type Etat (�tat/position du jeu)
typedef struct EtatSt {

	int joueur; // � qui de jouer ?

	// 6 lignes et 7 colonnes au Puissance 4
	char plateau[6][7];

} Etat;

// Definition du type Coup
typedef struct {

    // un coup de Puissance 4 est simplement d�fini par sa colonne
	int colonne;

} Coup;

// Copier un �tat
Etat * copieEtat( Etat * src ) {
	Etat * etat = (Etat *)malloc(sizeof(Etat));

	etat->joueur = src->joueur;

    int i, j;
	for (i=0; i < ARRAY_LENGTH(etat->plateau); i++)
		for ( j=0; j < ARRAY_LENGTH(etat->plateau[0]); j++)
			etat->plateau[i][j] = src->plateau[i][j];

	return etat;
}

// Etat initial
Etat * etat_initial( void ) {
	Etat * etat = (Etat *)malloc(sizeof(Etat));

	int i, j;
	for (i=0; i < ARRAY_LENGTH(etat->plateau); i++)
		for ( j=0; j < ARRAY_LENGTH(etat->plateau[0]); j++)
			etat->plateau[i][j] = ' ';

	return etat;
}


void afficheJeu(Etat * etat) {

	int i, j;
	printf("   |");
	for ( j = 0; j < ARRAY_LENGTH(etat->plateau[0]); j++)
		printf(" %d |", j);
	printf("\n");
	printf("--------------------------------");
	printf("\n");

	for(i=0; i < ARRAY_LENGTH(etat->plateau); i++) {
		printf(" %d |", i);
		for ( j = 0; j < ARRAY_LENGTH(etat->plateau[0]); j++)
			printf(" %c |", etat->plateau[i][j]);
		printf("\n");
		printf("--------------------------------");
		printf("\n");
	}
}


// Nouveau coup
// TODO: adapter la liste de param�tres au jeu
Coup * nouveauCoup( int col ) {
	Coup * coup = (Coup *)malloc(sizeof(Coup));

    coup->colonne = col;

	return coup;
}

// Demander � l'humain quel coup jouer
Coup * demanderCoup () {
	int col;
	printf(" quelle colonne ? ") ;
	scanf("%d",&col);

	return nouveauCoup(col);
}

// Modifier l'�tat en jouant un coup
// retourne 0 si le coup n'est pas possible
int jouerCoup( Etat * etat, Coup * coup ) {

		// le coup est impossible si la colonne est rempli (donc premi�re ligne occup�)
	// ou si il est en dehors des limites
	if ( etat->plateau[0][coup->colonne] != ' ' || coup->colonne < 0 || coup->colonne >= ARRAY_LENGTH(etat->plateau[0]) )
		return 0;

    // on cherche la ligne sur laquelle le pion doit �tre plac�
    int ligneJouee = -1;
    int row = ARRAY_LENGTH(etat->plateau) - 1; // on part d'en bas du plateau (derni�re ligne donc)
    while (row >=0 && ligneJouee == -1) {
        if (etat->plateau[row][coup->colonne] == ' ') // si l'emplacement est libre
            ligneJouee = row;
        else
            row--;
    }

    // cas d'erreur impossible en temps normal
    if (ligneJouee == -1)
        return 0;

    etat->plateau[ligneJouee][coup->colonne] = etat->joueur ? 'O' : 'X';

    // � l'autre joueur de jouer
    etat->joueur = AUTRE_JOUEUR(etat->joueur);

    return 1;
}

// Retourne une liste de coups possibles � partir d'un etat
// (tableau de pointeurs de coups se terminant par NULL)
Coup ** coups_possibles( Etat * etat ) {

	Coup ** coups = (Coup **) malloc((1+LARGEUR_MAX) * sizeof(Coup *) );

	int k = 0;

	int column;
	// on parcourt les colonnes
	for(column=0; column < ARRAY_LENGTH(etat->plateau[0]); column++) {
        // on v�rifie que la colonne courante n'est pas remplie (donc premi�re ligne non occup�)
        if ( etat->plateau[0][column] == ' ' ) {
            coups[k] = nouveauCoup(column);
            k++;
        }
	}

	coups[k] = NULL;

	return coups;
}


// Definition du type Noeud
typedef struct NoeudSt {

	int joueur; // joueur qui a jou� pour arriver ici
	Coup * coup;   // coup jou� par ce joueur pour arriver ici

	Etat * etat; // etat du jeu

	struct NoeudSt * parent;
	struct NoeudSt * enfants[LARGEUR_MAX]; // liste d'enfants : chaque enfant correspond � un coup possible
	int nb_enfants;	// nb d'enfants pr�sents dans la liste

	// POUR MCTS:
	double sommes_recompenses;
	int nb_simus;

} Noeud;


// Cr�er un nouveau noeud en jouant un coup � partir d'un parent
// utiliser nouveauNoeud(NULL, NULL) pour cr�er la racine
Noeud * nouveauNoeud (Noeud * parent, Coup * coup ) {
	Noeud * noeud = (Noeud *)malloc(sizeof(Noeud));

	if ( parent != NULL && coup != NULL ) {
		noeud->etat = copieEtat ( parent->etat );
		jouerCoup ( noeud->etat, coup );
		noeud->coup = coup;
		noeud->joueur = AUTRE_JOUEUR(parent->joueur);
	}
	else {
		noeud->etat = NULL;
		noeud->coup = NULL;
		noeud->joueur = 0;
	}
	noeud->parent = parent;
	noeud->nb_enfants = 0;

	// POUR MCTS:
	noeud->sommes_recompenses = 0;
	noeud->nb_simus = 0;


	return noeud;
}

// Ajouter un enfant � un parent en jouant un coup
// retourne le pointeur sur l'enfant ajout�
Noeud * ajouterEnfant(Noeud * parent, Coup * coup) {
	Noeud * enfant = nouveauNoeud (parent, coup ) ;
	parent->enfants[parent->nb_enfants] = enfant;
	parent->nb_enfants++;
	return enfant;
}

void freeNoeud ( Noeud * noeud) {
	if ( noeud->etat != NULL)
		free(noeud->etat);

	while ( noeud->nb_enfants > 0 ) {
		freeNoeud(noeud->enfants[noeud->nb_enfants-1]);
		noeud->nb_enfants --;
	}
	if ( noeud->coup != NULL)
		free(noeud->coup);

	free(noeud);
}


// Test si l'�tat est un �tat terminal
// et retourne NON, MATCHNUL, ORDI_GAGNE ou HUMAIN_GAGNE
FinDePartie testFin( Etat * etat ) {

    int largeurPlateau = ARRAY_LENGTH(etat->plateau);
    int hauteurPlateau = ARRAY_LENGTH(etat->plateau[0]);

	// tester si un joueur a gagn�
	int i, j, k, n = 0;
	for (i=0; i < largeurPlateau; i++) {
		for(j=0; j < hauteurPlateau; j++) {
			if ( etat->plateau[i][j] != ' ') {
				n++;	// nb coups jou�s

				// lignes
				k=0;
				while ( k < 4 && i+k < largeurPlateau && etat->plateau[i+k][j] == etat->plateau[i][j] )
					k++;
				if ( k == 4 )
					return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;

				// colonnes
				k=0;
				while ( k < 4 && j+k < hauteurPlateau && etat->plateau[i][j+k] == etat->plateau[i][j] )
					k++;
				if ( k == 4 )
					return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;

				// diagonales
				k=0;
				while ( k < 4 && i+k < largeurPlateau && j+k < hauteurPlateau && etat->plateau[i+k][j+k] == etat->plateau[i][j] )
					k++;
				if ( k == 4 )
					return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;

				k=0;
				while ( k < 4 && i+k < largeurPlateau && j-k >= 0 && etat->plateau[i+k][j-k] == etat->plateau[i][j] )
					k++;
				if ( k == 4 )
					return etat->plateau[i][j] == 'O'? ORDI_GAGNE : HUMAIN_GAGNE;
			}
		}
	}

	// et sinon tester le match nul
	if ( n == ARRAY_LENGTH(etat->plateau) * ARRAY_LENGTH(etat->plateau[0]) )
		return MATCHNUL;

	return NON;
}

// Calcule la B-valeur d'un noeud
double calculerBValeurNoeud(Noeud * noeud) {
    if (noeud->nb_simus == 0)
        return 0;

    double moyenneRecompense = noeud->sommes_recompenses/noeud->nb_simus;
    // *-1 si le noeud parent est un noeud Min = si le coup jou� pour arriver ici a �t� effectu� par l'ordinateur
    if (noeud->parent->joueur == 1)
        moyenneRecompense *= -1;

    return moyenneRecompense + CONSTANTE_C * sqrt( log(noeud->parent->nb_simus / noeud->nb_simus) );
}

// S�lectionne r�cursivement � partir de la racine (pass�e en param�tre)
// le noeud avec la plus grande B-valeur jusqu'� arriver � un noeud terminal
// ou un dont tous les fils n'ont pas �t� d�velopp�s
Noeud * selectionUCB(Noeud * racine) {
    Noeud * noeudCourant = racine;
    Coup ** coups;
    int i, k = 0;

    // Si on arrive � une feuille, on retourne celle-ci
    if (noeudCourant->nb_enfants == 0)
        return noeudCourant;

    coups = coups_possibles(noeudCourant->etat);
    while (coups[k] != NULL) {
        free(coups[k]);
        k++;
    }
    free(coups);
    // Si les fils du noeud courant n'ont pas tous �t� d�velopp�s,
    // on retourne ce noeud
    if (noeudCourant->nb_enfants != k)
        return noeudCourant;

    // Si un des fils du noeud n'a aucun fils
    // alors on le s�lectionne
    for (i = 0; i < noeudCourant->nb_enfants ; i++)
        if(noeudCourant->enfants[i]->nb_enfants == 0)
            return noeudCourant->enfants[i];

    // On s�lectionne le fils poss�dant la B-valeur maximale
    Noeud * noeudMaxBValeur = noeudCourant->enfants[0];
    double maxBValeur = calculerBValeurNoeud(noeudMaxBValeur);
    for (i = 1 ; i < noeudCourant->nb_enfants ; i++) {
        double bValeurCourante = calculerBValeurNoeud(noeudCourant->enfants[i]);
        if (maxBValeur < bValeurCourante) {
            noeudMaxBValeur = noeudCourant->enfants[i];
            maxBValeur = bValeurCourante;
        }
    }
    // Appel r�cursif sur le fils poss�dant la B-valeur maximale
    return selectionUCB(noeudMaxBValeur);
}

// R�alise l'expansion d'un noeud en d�veloppant un de ses fils au hasard
// et retourne ce fils.
// Si le noeud repr�sente un �tat final, retourne simplement celui-ci.
Noeud * expansionNoeud(Noeud * noeud) {
    if (testFin(noeud->etat) != NON)    // Si le noeud repr�sente un �tat final
        return noeud;                   // on ne le d�veloppe pas

    Coup ** coups = coups_possibles(noeud->etat);

    // On enl�ve les coups correspondant aux fils existants du noeud
    int k = 0;
    while (coups[k] != NULL) {

        bool coupDejaDev = false;

        // Pour chaque fils
        int i;
        for (i = 0 ; i < noeud->nb_enfants ; i++) {
            // Si il s'agit du m�me coup
            if (coups[k]->colonne == noeud->enfants[i]->coup->colonne) {
                coupDejaDev = true; // ce coup a d�j� un fils correspondant
                break;              // on arr�te le parcours des fils
            }
        }

        if (coupDejaDev) {  // si ce coup a d�j� un fils correspondant
            // On supprime ce coup et on d�cale la liste de coups
            free(coups[k]);
            int j = k;
            while (coups[j] != NULL) {
                coups[j] = coups[j+1];
                j++;
            }
        }
        else
            k++;

    }

    // On d�veloppe un fils au hasard
    int choix = rand() % k;

    // On lib�re la m�moire des coups inutilis�s
    k = 0;
    while (coups[k] != NULL) {
        if (k != choix)
            free(coups[k]); // sauf celle du coup jou� qui sera lib�r�e dans freenoeud
        k++;
    }

    Noeud * enfant = ajouterEnfant(noeud, coups[choix]);
    free(coups);    // On lib�re la m�moire de la liste de coups
    return enfant;
}

// Simule le d�roulement de la partie � partir d'un �tat
// jusqu'� la fin et retourne l'�tat final.
FinDePartie simulerPartie(Etat * etat) {
    FinDePartie resultatFinDePartie;
    // Tant que la partie n'est pas termin�e
    while ((resultatFinDePartie = testFin(etat)) == NON) {
        Coup** coups = coups_possibles(etat);
        int k = 0;
        while (coups[k] != NULL)    k++;

        jouerCoup(etat, coups[rand() % k]);  // On joue un coup al�atoirement
        // On lib�re la m�moire
        k = 0;
        while (coups[k] != NULL) {
            free(coups[k]);
            k++;
        }
        free(coups);
    }

    return resultatFinDePartie;
}

// Propage le r�sultat � partir d'un noeud
// en remontant le r�sultat de la partie
// aux parents de ce noeud.
void propagerResultat(Noeud * noeud, FinDePartie resultat) {

    while (noeud != NULL) {
        noeud->nb_simus++;
        switch(resultat) {
            case ORDI_GAGNE :
                noeud->sommes_recompenses += RECOMPENSE_ORDI_GAGNE;
                break;
            case HUMAIN_GAGNE :
                noeud->sommes_recompenses += RECOMPENSE_HUMAIN_GAGNE;
                break;
            case MATCHNUL :
                noeud->sommes_recompenses += RECOMPENSE_MATCHNUL;
                break;
        }
        noeud = noeud->parent;
    }

}

// Trouve le meilleur coup possible
// � partir de la racine.
Coup * trouverMeilleurCoup(Noeud * racine) {

    // Le meilleur coup actuel correspond au noeud fils de la racine ayant le plus de simulations
    Noeud * noeudMeilleurCoup = racine->enfants[0];
    int maxSimus = noeudMeilleurCoup->nb_simus;
    int i;
    for (i = 1 ; i < racine->nb_enfants ; i++) {
        if (maxSimus < racine->enfants[i]->nb_simus) {
            noeudMeilleurCoup = racine->enfants[i];
            maxSimus = noeudMeilleurCoup->nb_simus;
        }
    }

    return noeudMeilleurCoup->coup;
}

// Calcule et joue un coup de l'ordinateur avec MCTS-UCT
// en tempsmax secondes
void ordijoue_mcts(Etat * etat, int tempsmax) {

	clock_t tic, toc;
	tic = clock();
	int temps;

	Coup ** coups;
	Coup * meilleur_coup ;

	// Cr�er l'arbre de recherche
	Noeud * racine = nouveauNoeud(NULL, NULL);
	racine->etat = copieEtat(etat);

	// cr�er les premiers noeuds:
	coups = coups_possibles(racine->etat);
	int k = 0;
	Noeud * enfant;
	while ( coups[k] != NULL) {
		enfant = ajouterEnfant(racine, coups[k]);
		k++;
	}

	/* Algorithme MCTS-UCS */
	int iter = 0;

	do {
        // S�lection
        Noeud * noeudSelectionne = selectionUCB(racine);
        // Expansion
        enfant = expansionNoeud(noeudSelectionne);
        // Simulation
        Etat * etatCopie = copieEtat(enfant->etat);
        FinDePartie resultat = simulerPartie(etatCopie);
        free(etatCopie);
        // Propagation
        propagerResultat(enfant, resultat);

		toc = clock();
		temps = (int)( ((double) (toc - tic)) / CLOCKS_PER_SEC );
		iter ++;

	} while ( temps < tempsmax );

    // On cherche le meilleur coup possible
    meilleur_coup = trouverMeilleurCoup(racine);

	/* fin de l'algorithme  */

	// Jouer le meilleur premier coup
	jouerCoup(etat, meilleur_coup);

	// Penser � lib�rer la m�moire :
	freeNoeud(racine);
	free (coups);
}

int main(void) {

	Coup * coup;
	FinDePartie fin;

	// initialisation
	Etat * etat = etat_initial();

	// Choisir qui commence :
    do {
        printf("Qui commence (0 : humain, 1 : ordinateur) ? ");
        scanf("%d", &(etat->joueur) );
    } while (etat->joueur != 0 && etat->joueur != 1);

	// boucle de jeu
	do {
		printf("\n");
		afficheJeu(etat);

		if ( etat->joueur == 0 ) {
			// tour de l'humain

			do {
				coup = demanderCoup();
			} while ( !jouerCoup(etat, coup) );
            free(coup);
		}
		else {
			// tour de l'Ordinateur

			ordijoue_mcts( etat, TEMPS );

		}

		fin = testFin( etat );
	}	while ( fin == NON ) ;

	printf("\n");
	afficheJeu(etat);

	if ( fin == ORDI_GAGNE )
		printf( "** L'ordinateur a gagn� **\n");
	else if ( fin == MATCHNUL )
		printf(" Match nul !  \n");
	else
		printf( "** BRAVO, l'ordinateur a perdu  **\n");

    free(etat);

	return 0;
}
