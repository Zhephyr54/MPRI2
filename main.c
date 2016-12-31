#include "puissance4.h"
#include "mcts.h"
#include "utils.h"
#include <stdlib.h>
#include <stdio.h>
#include <stdbool.h>
#include <getopt.h>
#include <ctype.h>

int main(int argc, char **argv) {

    /* Gestion des arguments */

    /*** Niveau d'optimisation de l'algorithme ***
                    0 : fonctionnement basique de l'algorithme MCTS avec UCB (UCT) (les simulations sont réalisées au hasard).
     (par défaut)   1 : (QUESTION 3 :) amélioration des simulations consistant à toujours choisir un coup gagnant lorsque cela est possible.
    *********************************************/
    int optimisationLevel = 1;

    /*** Niveau de verbosité du programme ***
                    0 : aucun affichage autre que la demande de coup et le plateau.
     (par défaut)   1 : (QUESTION 1:) affichage (à chaque coup de l’ordinateur) du coup joué, du nombre total de simulations réalisées (= nombre d'itérations) et d'une estimation de la probabilité de victoire pour l’ordinateur.
                    2 : affichage (à chaque coup de l’ordinateur) du nombre de simulations réalisées pour chaque coup.
                    3 : affichage (à chaque coup de l’ordinateur) de la moyenne des récompenses pour chaque coup.
    ****************************************/
    int verboseLevel = 1;

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
                    fprintf(stderr, "Conflit d'arguments : -%c.\n", opt);
                    fprintf(stderr, "Les options -r et -m ne peuvent être utilisées en même temps.\n");
                    fprintf(stderr, "Utiliser -h ou --help pour obtenir de l'aide.\n");
                    return 1;
                }
                    maxFlag = 1;
                    methodeChoix = MAX;
                break;

            case 'r' :
                if (maxFlag) {   // Si le choix de max a déjà été fait
                    fprintf(stderr, "Conflit d'arguments : -%c.\n", opt);
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
                "\n             0 : fonctionnement basique de l'algorithme MCTS avec UCB (UCT) (les simulations sont réalisées au hasard)."
                "\n(par défaut) 1 : (Question 3) amélioration des simulations consistant à toujours choisir un coup gagnant lorsque cela est possible."

                "\n\n-v arg (ou --verbose) avec arg étant un nombre entier positif non nul."
                "\nPermet de définir le niveau de verbosité du programme, c'est-à-dire :"
                "\n             0 : aucun affichage autre que la demande de coup et le plateau de jeu."
                "\n(par défaut) 1 : (Question 1) affichage (à chaque coup de l’ordinateur) du nombre total de simulations réalisées (= nombre d'itérations) et d'une estimation de la probabilité de victoire pour l’ordinateur en jouant ce coup."
                "\n             2 : affichage (à chaque coup de l’ordinateur) du nombre de simulations réalisées pour chaque coup."
                "\n             3 : affichage (à chaque coup de l’ordinateur) de la moyenne des récompenses pour chaque coup."

                "\n\nmethode : {-r (ou --robuste ou --robust) | -m (ou --max) } :"

                "\n\nPermet de définir la méthode pour choisir le coup à jouer à la fin de l'algorithme MCTS :"
                "\n(par défaut) -r pour robuste (coup avec le plus grand nombre de simulations)."
                "\n             -m pour max (coup avec la plus grande moyenne des récompenses)."
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

			ordijoue_mcts(etat, temps, methodeChoix, optimisationLevel, verboseLevel);

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
