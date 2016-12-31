#include "mcts.h"

#include <stdlib.h>
#include <stdio.h>
#include <float.h>
#include <math.h>
#include <time.h>

// Constantes/paramètres Algo MCTS
#define RECOMPENSE_ORDI_GAGNE 1
#define RECOMPENSE_MATCHNUL 0.5
#define RECOMPENSE_HUMAIN_GAGNE 0

#define CONSTANTE_C 1.4142  // ~ Racine carré de 2

Noeud * nouveauNoeud (Noeud * parent, Coup * coup) {
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

Noeud * ajouterEnfant(Noeud * parent, Coup * coup) {
	Noeud * enfant = nouveauNoeud (parent, coup) ;
	parent->enfants[parent->nb_enfants] = enfant;
	parent->nb_enfants++;
	return enfant;
}

void freeNoeud (Noeud * noeud) {
	if (noeud->etat != NULL)
		free(noeud->etat);

	while (noeud->nb_enfants > 0) {
		freeNoeud(noeud->enfants[noeud->nb_enfants-1]);
		noeud->nb_enfants --;
	}
	if (noeud->coup != NULL)
		free(noeud->coup);

	free(noeud);
}

double calculerBValeurNoeud(Noeud * noeud) {
    // Si le noeud n'a aucune simulation, il est prioritaire
    if (noeud->nb_simus == 0)
        return DBL_MAX;

    double moyenneRecompense = (double)noeud->sommes_recompenses/noeud->nb_simus;
    // *-1 si le noeud parent est un noeud Min = si le coup joué pour arriver ici a été effectué par l'ordinateur
    if (noeud->parent->joueur == 1)
        moyenneRecompense *= -1;

    return moyenneRecompense + CONSTANTE_C * sqrt( log(noeud->parent->nb_simus) / noeud->nb_simus );
}

Noeud * selectionUCB(Noeud * racine) {
    Noeud * noeudCourant = racine;
    int i = 0;

    // Si on arrive à un noeud terminal ou un dont tous les fils n'ont pas été développés
    if (testFin(noeudCourant->etat) != NON || noeudCourant->nb_enfants != nombre_coups_possibles(noeudCourant->etat))
        return noeudCourant;

    // Sinon, on sélectionne le fils possédant la B-valeur maximale
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

void ordijoue_mcts(Etat * etat, double tempsmax, MethodeChoixCoup methodeChoix, int optimisationLevel, int verboseLevel) {
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

    // Affichage du nombre de simulations réalisées pour chaque coup
    if (verboseLevel >= 2) {
        printf("\n");
        int i;
        for (i=0 ; i < racine->nb_enfants ; i++) {
            Noeud * noeud = racine->enfants[i];
            printf("\nPour le coup en colonne %d :   Nombre de simulations   : %d", noeud->coup->colonne, noeud->nb_simus);
            // et de la récompense moyenne pour chaque coup
            if (verboseLevel >= 3) {
                printf("\n                              Moyenne des récompenses : ");
                if (noeud->nb_simus > 0)
                    printf("%0.4f", (double)noeud->sommes_recompenses/noeud->nb_simus);
                else
                    printf("aucune");
            }
        }
        printf("\n");
    }

    // Affichage du nombre de simulations réalisées pour calculer le meilleur coup
    // et une estimation de la probabilité de victoire pour l'ordinateur
    if (verboseLevel >= 1) {
        printf("\nCoup joué en colonne %d", noeudMeilleurCoup->coup->colonne);
        printf("\nNombre total de simulations : %d", racine->nb_simus);
        printf("\nEstimation de probabilité de victoire pour l'ordinateur : ");
        if (noeudMeilleurCoup->nb_simus > 0)
            printf("%0.2f %%", (double)noeudMeilleurCoup->nb_victoires/noeudMeilleurCoup->nb_simus * 100);
        else
            printf("aucune");
        printf("\n");
    }

	// Jouer le meilleur premier coup
	jouerCoup(etat, meilleur_coup);

	// Penser à libérer la mémoire :
	freeNoeud(racine);
	free (coups);
}
