#ifndef UTILS_H_INCLUDED
#define UTILS_H_INCLUDED

#include <stdbool.h>

/**
    Fonctions utilitaires.
*/

/** Clean l'entrée standard */
int clean_stdin(void);

/** Convertit un string (char*) en entier et stocke le résultat dans result.
    Retourne true si la conversion s'est déroulée correctement, false sinon. */
bool convertStringToInt(char * string, int * result);

/** Convertit un string (char*) en double et stocke le résultat dans result.
    Retourne true si la conversion s'est déroulée correctement, false sinon. */
bool convertStringToDouble(char * string, double * result);

#endif // UTILS_H_INCLUDED
