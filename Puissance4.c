#include "Puissance4.h"
#include "utils.h"
#include <stdio.h>
#include <stdlib.h>

// Macros
#define ARRAY_LENGTH(x)  (sizeof(x) / sizeof((x)[0]))

Etat * copieEtat(Etat * src) {
	Etat * etat = (Etat *)malloc(sizeof(Etat));

	etat->joueur = src->joueur;

    int i, j;
	for (i=0; i < ARRAY_LENGTH(etat->plateau); i++)
		for ( j=0; j < ARRAY_LENGTH(etat->plateau[0]); j++)
			etat->plateau[i][j] = src->plateau[i][j];

	return etat;
}

Etat * etat_initial(void) {
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

Coup * nouveauCoup(int col) {
	Coup * coup = (Coup *)malloc(sizeof(Coup));

    coup->colonne = col;

	return coup;
}

Coup * demanderCoup(void) {
	int col = -1;
	char c;
	printf(" quelle colonne ? ") ;
	if ( (scanf("%d%c", &col, &c)!=2 || c!='\n') && clean_stdin() )
        return NULL;

	return nouveauCoup(col);
}

int jouerCoup(Etat * etat, Coup * coup) {

    // le coup est impossible si il est en dehors des limites
    // ou si la colonne est rempli (donc premiére ligne occupée)
	if ( coup->colonne < 0 || coup->colonne >= ARRAY_LENGTH(etat->plateau[0]) ||
     etat->plateau[0][coup->colonne] != ' ')
		return 0;

    // on cherche la ligne sur laquelle le pion doit être placée
    int ligneJouee = -1;
    int row = ARRAY_LENGTH(etat->plateau) - 1; // on part d'en bas du plateau (derniére ligne donc)
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

Coup ** coups_possibles(Etat * etat) {

	Coup ** coups = (Coup **) malloc((1+LARGEUR_MAX) * sizeof(Coup *) );

	int k = 0;

	int column;
	// on parcourt les colonnes
	for(column=0; column < ARRAY_LENGTH(etat->plateau[0]); column++) {
        // on vérifie que la colonne courante n'est pas remplie (donc première ligne non occupée)
        if ( etat->plateau[0][column] == ' ' ) {
            coups[k] = nouveauCoup(column);
            k++;
        }
	}

	coups[k] = NULL;

	return coups;
}

int nombre_coups_possibles(Etat * etat) {
    int count = 0;
	int column;
	// on parcourt les colonnes
	for(column=0; column < ARRAY_LENGTH(etat->plateau[0]); column++)
        // on vérifie que la colonne courante n'est pas remplie (donc premiére ligne non occupée)
        if ( etat->plateau[0][column] == ' ' )
            count++;

    return count;
}

FinDePartie testFin(Etat * etat) {

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
