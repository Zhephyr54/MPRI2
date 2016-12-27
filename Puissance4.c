/*
	Canvas pour algorithmes de jeux à 2 joueurs

	joueur 0 : humain
	joueur 1 : ordinateur

*/
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>
#include <stdbool.h>
#include <getopt.h>
#include <ctype.h>
#include <limits.h>
#include <errno.h>

// Paramètres du jeu
#define LARGEUR_MAX 7 		// nb max de fils pour un noeud (= nb max de coups possibles)

// Constantes/paramètres Algo MCTS
#define RECOMPENSE_ORDI_GAGNE 1
#define RECOMPENSE_MATCHNUL 0.5
#define RECOMPENSE_HUMAIN_GAGNE 0

#define CONSTANTE_C 1.4142  // ~ Racine carré de 2

// macros
#define AUTRE_JOUEUR(i) (1-(i))
#define min(a, b)       ((a) < (b) ? (a) : (b))
#define max(a, b)       ((a) < (b) ? (b) : (a))

#define ARRAY_LENGTH(x)  (sizeof(x) / sizeof((x)[0]))

// Variables globales avec leurs valeurs par défaut
/*** Niveau d'optimisation de l'algorithme ***
                0 : fonctionnement normal de l'algorithme MCTS avec UCB (UCT).
 (par défaut)   1 : (QUESTION 3 :) amélioration des simulations consistant à toujours choisir un coup gagnant lorsque cela est possible.
*********************************************/
int optimisationLevel = 1;

/*** Niveau de verbosité du programme ***
                0 : aucun affichage autre que la demande de coup et le plateau.
 (par défaut)   1 : (QUESTION 1:) affichage à chaque coup de l’ordinateur du nombre de simulations réalisées pour calculer ce
                    coup et une estimation de la probabilité de victoire pour l’ordinateur.
                2 : affichage du nombre total de simulations réalisées = nombre d'itérations de l'algorithme.
****************************************/
int verboseLevel = 1;

// Méthode du choix du coup à jouer pour MCTS
typedef enum { MAX, ROBUSTE } MethodeChoixCoup;

// Critères de fin de partie
typedef enum { NON, MATCHNUL, ORDI_GAGNE, HUMAIN_GAGNE } FinDePartie;

// Definition du type Etat (état/position du jeu)
typedef struct EtatSt {

	int joueur; // à qui de jouer ?

	// 6 lignes et 7 colonnes au Puissance 4
	char plateau[6][7];

} Etat;

// Definition du type Coup
typedef struct {

    // un coup de Puissance 4 est simplement défini par sa colonne
	int colonne;

} Coup;

// Copier un état
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
Coup * nouveauCoup( int col ) {
	Coup * coup = (Coup *)malloc(sizeof(Coup));

    coup->colonne = col;

	return coup;
}

// Demander à l'humain quel coup jouer
// Retourne NULL en cas d'input incorrect
Coup * demanderCoup () {
	int col = -1;
	char c;
	printf(" quelle colonne ? ") ;
	if ( (scanf("%d%c", &col, &c)!=2 || c!='\n') && clean_stdin() )
        return NULL;

	return nouveauCoup(col);
}

// Modifier l'état en jouant un coup
// retourne 0 si le coup n'est pas possible
int jouerCoup( Etat * etat, Coup * coup ) {

    // le coup est impossible si il est en dehors des limites
    // ou si la colonne est rempli (donc première ligne occupé)
	if ( coup->colonne < 0 || coup->colonne >= ARRAY_LENGTH(etat->plateau[0]) ||
     etat->plateau[0][coup->colonne] != ' ')
		return 0;

    // on cherche la ligne sur laquelle le pion doit être placé
    int ligneJouee = -1;
    int row = ARRAY_LENGTH(etat->plateau) - 1; // on part d'en bas du plateau (dernière ligne donc)
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

    // à l'autre joueur de jouer
    etat->joueur = AUTRE_JOUEUR(etat->joueur);

    return 1;
}

// Retourne une liste de coups possibles à partir d'un etat
// (tableau de pointeurs de coups se terminant par NULL)
Coup ** coups_possibles( Etat * etat ) {

	Coup ** coups = (Coup **) malloc((1+LARGEUR_MAX) * sizeof(Coup *) );

	int k = 0;

	int column;
	// on parcourt les colonnes
	for(column=0; column < ARRAY_LENGTH(etat->plateau[0]); column++) {
        // on vérifie que la colonne courante n'est pas remplie (donc première ligne non occupé)
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


// Créer un nouveau noeud en jouant un coup à partir d'un parent
// utiliser nouveauNoeud(NULL, NULL) pour créer la racine
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
	noeud->nb_victoires = 0;
	noeud->sommes_recompenses = 0;
	noeud->nb_simus = 0;


	return noeud;
}

// Ajouter un enfant à un parent en jouant un coup
// retourne le pointeur sur l'enfant ajouté
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


// Test si l'état est un état terminal
// et retourne NON, MATCHNUL, ORDI_GAGNE ou HUMAIN_GAGNE
FinDePartie testFin( Etat * etat ) {

    int largeurPlateau = ARRAY_LENGTH(etat->plateau);
    int hauteurPlateau = ARRAY_LENGTH(etat->plateau[0]);

	// tester si un joueur a gagné
	int i, j, k, n = 0;
	for (i=0; i < largeurPlateau; i++) {
		for(j=0; j < hauteurPlateau; j++) {
			if ( etat->plateau[i][j] != ' ') {
				n++;	// nb coups joués

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

/* Fonctions utilisées pour l'implémentation de l'algorithme MCTS */

// Calcule la B-valeur d'un noeud
double calculerBValeurNoeud(Noeud * noeud) {
    if (noeud->nb_simus == 0)
        return 0;

    double moyenneRecompense = (double)noeud->sommes_recompenses/noeud->nb_simus;
    // *-1 si le noeud parent est un noeud Min = si le coup joué pour arriver ici a été effectué par l'ordinateur
    if (noeud->parent->joueur == 1)
        moyenneRecompense *= -1;

    return moyenneRecompense + CONSTANTE_C * sqrt( log(noeud->parent->nb_simus / noeud->nb_simus) );
}

// Sélectionne récursivement à partir de la racine (passée en paramètre)
// le noeud avec la plus grande B-valeur jusqu'à arriver à un noeud terminal
// ou un dont tous les fils n'ont pas été développés
Noeud * selectionUCB(Noeud * racine) {
    Noeud * noeudCourant = racine;
    Coup ** coups;
    int i, k = 0;

    // Si on arrive à une feuille, on retourne celle-ci
    if (noeudCourant->nb_enfants == 0)
        return noeudCourant;

    coups = coups_possibles(noeudCourant->etat);
    while (coups[k] != NULL) {
        free(coups[k]);
        k++;
    }
    free(coups);
    // Si les fils du noeud courant n'ont pas tous été développés,
    // on retourne ce noeud
    if (noeudCourant->nb_enfants != k)
        return noeudCourant;

    // Si un des fils du noeud n'a aucun fils
    // alors on le sélectionne
    for (i = 0; i < noeudCourant->nb_enfants ; i++)
        if(noeudCourant->enfants[i]->nb_enfants == 0)
            return noeudCourant->enfants[i];

    // On sélectionne le fils possédant la B-valeur maximale
    Noeud * noeudMaxBValeur = noeudCourant->enfants[0];
    double maxBValeur = calculerBValeurNoeud(noeudMaxBValeur);
    for (i = 1 ; i < noeudCourant->nb_enfants ; i++) {
        double bValeurCourante = calculerBValeurNoeud(noeudCourant->enfants[i]);
        if (maxBValeur < bValeurCourante) {
            noeudMaxBValeur = noeudCourant->enfants[i];
            maxBValeur = bValeurCourante;
        }
    }
    // Appel récursif sur le fils possédant la B-valeur maximale
    return selectionUCB(noeudMaxBValeur);
}

// Réalise l'expansion d'un noeud en développant un de ses fils au hasard
// et retourne ce fils.
// Si le noeud représente un état final, retourne simplement celui-ci.
Noeud * expansionNoeud(Noeud * noeud) {
    if (testFin(noeud->etat) != NON)    // Si le noeud représente un état final
        return noeud;                   // on ne le développe pas

    Coup ** coups = coups_possibles(noeud->etat);

    // On enlève les coups correspondant aux fils existants du noeud
    int k = 0;
    while (coups[k] != NULL) {

        bool coupDejaDev = false;

        // Pour chaque fils
        int i;
        for (i = 0 ; i < noeud->nb_enfants ; i++) {
            // Si il s'agit du même coup
            if (coups[k]->colonne == noeud->enfants[i]->coup->colonne) {
                coupDejaDev = true; // ce coup a déjà un fils correspondant
                break;              // on arrête le parcours des fils
            }
        }

        if (coupDejaDev) {  // si ce coup a déjà un fils correspondant
            // On supprime ce coup et on décale la liste de coups
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

    // On développe un fils au hasard
    int choix = rand() % k;

    // On libère la mémoire des coups inutilisés
    k = 0;
    while (coups[k] != NULL) {
        if (k != choix)
            free(coups[k]); // sauf celle du coup joué qui sera libérée dans freenoeud
        k++;
    }

    Noeud * enfant = ajouterEnfant(noeud, coups[choix]);
    free(coups);    // On libère la mémoire de la liste de coups
    return enfant;
}

// Simule le déroulement de la partie à partir d'un état
// jusqu'à la fin et retourne l'état final.
// Si choisirCoupGagnant est à vrai,
// on améliore les simulations en choisissant un coup gagnant lorsque cela est possible.
FinDePartie simulerPartie(Etat * etat, bool choisirCoupGagnant) {
    FinDePartie resultatFinDePartie;
    // Tant que la partie n'est pas terminée
    while ((resultatFinDePartie = testFin(etat)) == NON) {
        Coup** coups = coups_possibles(etat);
        Coup* coupAJoue = NULL;

        // Si on doit choisir un coup gagnant quand cela est possible
        if (choisirCoupGagnant) {
            int k = 0;
            // On teste tous les coups pour voir si un des coups est gagnant
            while (coups[k] != NULL && coupAJoue == NULL) {
                Etat * etatATester = copieEtat(etat);
                jouerCoup(etatATester, coups[k]);

                if (testFin(etatATester) == ORDI_GAGNE)
                    coupAJoue = coups[k];

                free(etatATester);

                k++;
            }
            // Si aucun coup gagnant n'est possible
            if (coupAJoue == NULL)
                coupAJoue = coups[rand() % k];  // On joue un coup aléatoirement
        }
        // Sinon, on choisit le coup aléatoirement
        else {
            int k = 0;
            while (coups[k] != NULL)    k++;
            coupAJoue = coups[rand() % k];  // On joue un coup aléatoirement
        }

        jouerCoup(etat, coupAJoue);  // On joue le coup

        // On libère la mémoire
        int k = 0;
        while (coups[k] != NULL) {
            free(coups[k]);
            k++;
        }
        free(coups);
    }

    return resultatFinDePartie;
}

// Propage le résultat à partir d'un noeud
// en remontant le résultat de la partie
// aux parents de ce noeud.
void propagerResultat(Noeud * noeud, FinDePartie resultat) {

    while (noeud != NULL) {
        noeud->nb_simus++;
        switch(resultat) {
            case ORDI_GAGNE :
                noeud->nb_victoires++;
                noeud->sommes_recompenses += RECOMPENSE_ORDI_GAGNE;
                break;
            case HUMAIN_GAGNE :
                noeud->sommes_recompenses += RECOMPENSE_HUMAIN_GAGNE;
                break;
            case MATCHNUL :
                noeud->sommes_recompenses += RECOMPENSE_MATCHNUL;
                break;
            default:
                break;
        }
        noeud = noeud->parent;
    }

}

// Trouve le noeud correspondant au meilleur coup possible
// en utilisant la méthode spécifié
// à partir de la racine.
Noeud * trouverNoeudMeilleurCoup(Noeud * racine, MethodeChoixCoup methode) {

    Noeud * noeudMeilleurCoup = racine->enfants[0];
    int i = 1, maxSimus;
    double maxValeurs, valeurCourante;

    switch(methode) {   // max simulations
        case ROBUSTE :
            maxSimus = noeudMeilleurCoup->nb_simus;

            for (i = 1 ; i < racine->nb_enfants ; i++) {
                if (maxSimus < racine->enfants[i]->nb_simus) {
                    noeudMeilleurCoup = racine->enfants[i];
                    maxSimus = noeudMeilleurCoup->nb_simus;
                }
            }
            break;

        case MAX :      // max valeurs
            if (noeudMeilleurCoup->nb_simus == 0)
                maxValeurs = 0;
            else
                maxValeurs = (double)noeudMeilleurCoup->sommes_recompenses / noeudMeilleurCoup->nb_simus;

            for (i = 1 ; i < racine->nb_enfants ; i++) {
                if (racine->enfants[i]->nb_simus == 0)
                    valeurCourante = 0;
                else
                    valeurCourante = (double)racine->enfants[i]->sommes_recompenses / racine->enfants[i]->nb_simus;

                if (maxValeurs < valeurCourante) {
                    noeudMeilleurCoup = racine->enfants[i];
                    maxValeurs = valeurCourante;
                }
            }
            break;
    }

    return noeudMeilleurCoup;
}

// Calcule et joue un coup de l'ordinateur avec MCTS-UCT
// en tempsmax secondes
// et en choisissant le coup à l'aide de la méthode methodeChoix.
void ordijoue_mcts(Etat * etat, double tempsmax, MethodeChoixCoup methodeChoix) {
	clock_t tic, toc;
	tic = clock();
	double temps;

	Coup ** coups;
	Coup * meilleur_coup ;

	// Créer l'arbre de recherche
	Noeud * racine = nouveauNoeud(NULL, NULL);
	racine->etat = copieEtat(etat);

	// créer les premiers noeuds:
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
        // Sélection
        Noeud * noeudSelectionne = selectionUCB(racine);
        // Expansion
        enfant = expansionNoeud(noeudSelectionne);
        // Simulation
        Etat * etatCopie = copieEtat(enfant->etat);
        bool choisirCoupGagnant = optimisationLevel >= 1;
        FinDePartie resultat = simulerPartie(etatCopie, choisirCoupGagnant);
        free(etatCopie);
        // Propagation
        propagerResultat(enfant, resultat);

		toc = clock();
		temps = ((double) (toc - tic))/ CLOCKS_PER_SEC;
		iter ++;
	} while ( temps < tempsmax );

    // On cherche le meilleur coup possible
    Noeud * noeudMeilleurCoup = trouverNoeudMeilleurCoup(racine, methodeChoix);
    meilleur_coup = noeudMeilleurCoup->coup;

	/* fin de l'algorithme  */

    if (verboseLevel >= 2) {
        // Affichage du nombre total de simulations / itérations
        printf("\nNombre total de simulations/itérations : %d", iter);
    }

    if (verboseLevel >= 1) {
        // Affichage du nombre de simulations réalisées pour calculer le meilleur coup
        // et une estimation de la probabilité de victoire pour l'ordinateur
        printf("\nNombre de simulations pour ce coup : %d", noeudMeilleurCoup->nb_simus);
        printf("\nEstimation de probabilité de victoire pour l'ordinateur : ");
        if (noeudMeilleurCoup->nb_simus > 0)
            printf("%0.2f %%", (double)noeudMeilleurCoup->nb_victoires/noeudMeilleurCoup->nb_simus * 100);
        else
            printf("inconnue");
        printf("\n");
    }

	// Jouer le meilleur premier coup
	jouerCoup(etat, meilleur_coup);

	// Penser à libérer la mémoire :
	freeNoeud(racine);
	free (coups);
}

/* Fonctions "utilitaires" */

// Convertit un string (char*) en entier et stocke le résultat dans result.
// Retourne true si la conversion s'est déroulée correctement, false sinon.
bool convertStringToInt(char * string, int * result) {
    char * end = NULL;
    errno = 0;
    long temp = strtol(string, &end, 10);

    if (*end == '\0' && errno != ERANGE && temp >= INT_MIN && temp <= INT_MAX) {
        *result = (int)temp;
        return true;
    }
    return false;
}

// Convertit un string (char*) en double et stocke le résultat dans result.
// Retourne true si la conversion s'est déroulée correctement, false sinon.
bool convertStringToDouble(char * string, double * result) {
    char * end = NULL;
    errno = 0;
    double temp = strtod(string, &end);

    if (*end == '\0' && errno != ERANGE) {
        *result = temp;
        return true;
    }
    return false;
}

int clean_stdin() {
    while (getchar()!='\n');
    return 1;
}

int main(int argc, char **argv) {

    /* Gestion des arguments */

    // Valeurs par défaut
    // temps : 5s
    // méthode : robuste
    double temps = 5;  // temps de calcul pour un coup avec MCTS (en secondes)
    MethodeChoixCoup methodeChoix = ROBUSTE; // méthode du choix du meilleur coup à la fin de MCTS
    bool printHelp = false;
    int robustFlag = 0, maxFlag = 0;

    // Spécification des options
    static struct option long_options[] = {
        {"help", no_argument, 0, 'h'},
        {"max", no_argument, 0, 'm'},
        {"robuste", no_argument, 0, 'r'},
        {"robust", no_argument, 0, 'r'},
        {"temps", required_argument, 0, 't'},
        {"time", required_argument, 0, 't'},
        {"optimisation", required_argument, 0, 'o'},
        {"optimization", required_argument, 0, 'o'},
        {"verbose", required_argument, 0, 'v'},
        {0, 0, 0, 0}
    };

    int option_index = 0;
    opterr = 0;
    int opt = 0;

    while ( (opt = getopt_long (argc, argv, "hmrt:o:v:", long_options, &option_index)) != -1) {
        int intResult = 0;
        double doubleResult = 0;

        switch (opt) {
            case 'h' :
                printHelp = true;
                break;

            case 't' :
                if (convertStringToDouble(optarg, &doubleResult) && doubleResult > 0)
                    temps = doubleResult;
                else {
                    fprintf(stderr, "Argument incorrect : %s.\n", optarg);
                    fprintf(stderr, "L'option -t requiert un nombre décimal (1.5 par exemple) positif non nul en argument.\n");
                    fprintf(stderr, "Utiliser -h ou --help pour obtenir de l'aide.\n");
                    return 1;
                }
                break;

            case 'm' :
                if (robustFlag) {   // Si le choix de robuste a déjà été fait
                    fprintf(stderr, "Conflit d'arguments : -%c.\n", optopt);
                    fprintf(stderr, "Les options -r et -m ne peuvent être utilisées en même temps.\n");
                    fprintf(stderr, "Utiliser -h ou --help pour obtenir de l'aide.\n");
                    return 1;
                }
                    maxFlag = 1;
                    methodeChoix = MAX;
                break;

            case 'r' :
                if (maxFlag) {   // Si le choix de max a déjà été fait
                    fprintf(stderr, "Conflit d'arguments : -%c.\n", optopt);
                    fprintf(stderr, "Les options -r et -m ne peuvent être utilisées en même temps.\n");
                    fprintf(stderr, "Utiliser -h ou --help pour obtenir de l'aide.\n");
                    return 1;
                }
                    robustFlag = 1;
                    methodeChoix = ROBUSTE;
                break;

            case 'o' :
                if (convertStringToInt(optarg, &intResult) && intResult >= 0)
                    optimisationLevel = intResult;
                else {
                    fprintf(stderr, "Argument incorrect : %s.\n", optarg);
                    fprintf(stderr, "L'option -o requiert un nombre entier positif ou nul en argument.\n");
                    fprintf(stderr, "Utiliser -h ou --help pour obtenir de l'aide.\n");
                    return 1;
                }
                break;

            case 'v' :
                if (convertStringToInt(optarg, &intResult) && intResult >= 0)
                    verboseLevel = intResult;
                else {
                    fprintf(stderr, "Argument incorrect : %s.\n", optarg);
                    fprintf(stderr, "L'option -v requiert un nombre entier positif ou nul en argument.\n");
                    fprintf(stderr, "Utiliser -h ou --help pour obtenir de l'aide.\n");
                    return 1;
                }
                break;

            case '?':
                // Argument requis
                if (optopt == 't' || optopt == 'o' || optopt == 'v')
                    fprintf(stderr, "Argument requis pour l'option -%c.\n", optopt);
                else if(isprint(optopt))
                    fprintf (stderr, "Option inconnu : `-%c'.\n", optopt);
                else
                    fprintf (stderr, "Option inconnu : `\\x%x'.\n", optopt);

                    fprintf(stderr, "Utiliser -h ou --help pour obtenir de l'aide.\n");
                return 1;
        }
    }

    if (printHelp) {
        printf( "\nutilisation : Puissance4 [options] [methode]"
                "\n\noptions :"

                "\n\n-t arg (ou --temps ou --time) avec arg étant un nombre décimal positif non nul (par exemple 1.5)."
                "\nPermet de définir la limite de temps imposée à l'ordinateur pour l'exécution de l'algorithme MCTS."

                "\n\n-o arg (ou --optimisation ou --optimization) avec arg étant un nombre entier positif non nul."
                "\nPermet de définir le niveau d'optimisation/amélioration de l'algorithme MCTS, c'est-à-dire :"
                "\n             0 : fonctionnement normal de l'algorithme MCTS avec UCB (UCT)."
                "\n(par défaut) 1 : (Question 3) amélioration des simulations consistant à toujours choisir un coup gagnant lorsque cela est possible."

                "\n\n-v arg (ou --verbose) avec arg étant un nombre entier positif non nul."
                "\nPermet de définir le niveau de verbosité du programme, c'est-à-dire :"
                "\n             0 : aucun affichage autre que la demande de coup et le plateau de jeu."
                "\n(par défaut) 1 : (Question 1) affichage à chaque coup de l’ordinateur du nombre de simulations réalisées pour calculer ce coup et une estimation de la probabilité de victoire pour l’ordinateur."
                "\n             2 : affichage du nombre total de simulations réalisées, ce qui correspond également au nombre d'itérations de l'algorithme."

                "\n\nmethode : {-r (ou --robuste ou --robust) | -m (ou --max) } :"

                "\n\nPermet de définir la méthode pour choisir le coup à jouer à la fin de l'algorithme MCTS :"
                "\n(par défaut) -r pour robuste."
                "\n             -m pour max."
                "\n\n");
        return 0;
    }

	Coup * coup = NULL;
	FinDePartie fin;

	// initialisation
	Etat * etat = etat_initial();

    char c;
	// Choisir qui commence :
    do {
        printf("Qui commence (0 : humain, 1 : ordinateur) ? ");
    } while ( ( (scanf("%d%c", &(etat->joueur), &c)!=2 || c!='\n' ) && clean_stdin()) || (etat->joueur != 0 && etat->joueur != 1) );

	// boucle de jeu
	do {
		printf("\n");
		afficheJeu(etat);

		if ( etat->joueur == 0 ) {
			// tour de l'humain

            int coupPossible = 0;
			do {
				coup = demanderCoup();
                if (coup != NULL) {
                    coupPossible = jouerCoup(etat, coup);
                    free(coup);
                }
			} while ( !coupPossible);

		}
		else {
			// tour de l'Ordinateur

			ordijoue_mcts(etat, temps, methodeChoix);

		}

		fin = testFin( etat );
	}	while ( fin == NON ) ;

	printf("\n");
	afficheJeu(etat);

	if ( fin == ORDI_GAGNE )
		printf( "** L'ordinateur a gagné **\n");
	else if ( fin == MATCHNUL )
		printf(" Match nul !  \n");
	else
		printf( "** BRAVO, l'ordinateur a perdu  **\n");

    free(etat);

	return 0;
}
