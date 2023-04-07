#include "tamponCirculaire.h"

// Plusieurs variables globales statiques (pour qu'elles ne soient accessible que dans les
// fonctions de ce fichier) sont declarees ici. Elle servent a conserver l'etat du tampon
// circulaire ainsi qu'a mesurer certains elements utiles au calcul des statistiques.
// Vous etes libres d'en creer d'autres si vous en voyez le besoin.

// Pointe vers la memoire allouee pour le tampon circulaire
static char* memoire;

// Taile du tampon circulaire (en nombre d'elements de type struct requete)
static size_t memoireTaille;

// Positions de lecture et d'ecriture, et longueur actuelle du tampon circulaire
static unsigned int posLecture, posEcriture, longueurCourante;

// Mutex permettant de proteger les acces au tampon circulaire
// N'oubliez pas que _deux_ threads vont tenter de faire des operations en parallele!
pthread_mutex_t mutexTampon;

// Pour les statistiques
static unsigned int nombreRequetesRecues, nombreRequetesTraitees, nombreRequetesPerdues;

// Le tempsDebutPeriode permet de se rappeler du debut de la periode ou les statistiques sont mesurees
// sommeTempsAttente contient la somme de toutes les periodes d'attente pour les requetes
// (vous pourrez donc calculer la moyenne du temps d'attente en utilisant les autres variables sur le
// nombre de requetes).
static double tempsDebutPeriode, sommeTempsAttente;

int initTamponCirculaire(size_t taille){
    // Initialisez ici:
    // La memoire, en utilisant malloc ou calloc (rappelez-vous que votre tampon circulaire doit
    // pouvoir contenir _taille_ fois la taille d'une struct requete)
    memoire = calloc(taille, sizeof(struct requete));
    if (memoire == NULL) {
        printf("calloc failed\n");
        return -1;
    }

    // Les positions de lecture, d'ecriture et de longueur courante.
    posEcriture = 0;
    posLecture = 0;
    longueurCourante = 0;

    // Le mutex
    if (pthread_mutex_init(&mutexTampon,NULL) < 0){
        printf("Failed mutex initialisation\n");
        return -1;
    }

    // Les variables de statistiques
    nombreRequetesRecues = 0;
    nombreRequetesTraitees = 0;
    nombreRequetesPerdues = 0;
    tempsDebutPeriode = 0;
    sommeTempsAttente = 0;
    return 0;
}

void resetStats(){
    // Reinitialise les variables de statistique
    nombreRequetesRecues = 0;
    nombreRequetesTraitees = 0;
    nombreRequetesPerdues = 0;
    tempsDebutPeriode = 0;
    sommeTempsAttente = 0;
}

void calculeStats(struct statistiques *stats){
    stats->nombreRequetesEnAttente = nombreRequetesRecues-nombreRequetesTraitees;
    stats->nombreRequetesTraitees = nombreRequetesTraitees;
    stats->nombreRequetesPerdues = nombreRequetesPerdues;
    stats->lambda = nombreRequetesRecues/(get_time()-tempsDebutPeriode);
    stats->mu = nombreRequetesTraitees/(get_time()-tempsDebutPeriode);
    stats->rho=(stats->lambda)/(stats->mu);
}

int insererDonnee(struct requete *req){
    // Dans cette fonction, vous devez :
    //
    // Determiner a quel endroit copier la requete req dans le tampon circulaire
    //
    // Copier celle-ci
    pthread_mutex_lock(&mutexTampon);
    
    memcpy(memoire+posEcriture, req, sizeof(struct requete));

    // Mettre a jour posEcriture et longueurCourante (toujours) et possiblement
    // posLecture (si vous vous etes "mordu la queue" et que vous etes revenu au
    // debut de votre tampon circulaire)
    //MaJ PosEcriture
    if (posEcriture==memoireTaille) {
        posLecture = 1;
        posEcriture = 0;
    }
    else
        posEcriture += 1;
    
    //MaJ longueur courante
    if (posEcriture > posLecture)
        longueurCourante = posEcriture-posLecture;
    else
        longueurCourante = memoireTaille-posLecture+posEcriture;

    // Mettre a jour les variables necessaires aux statistiques (comme nombreRequetesRecues, par exemple)
    nombreRequetesRecues += 1;
    
    // N'oubliez pas de proteger les operations qui le necessitent par un mutex!
    pthread_mutex_unlock(&mutexTampon);
    
    return 0;
}

int consommerDonnee(struct requete *req){
    // Determiner si une requete est disponible dans le tampon circulaire
    // S'il n'y en a _pas_, retourner 0.
    if (longueurCourante == 0){
        printf("Tampon circulaire vide, aucune requete à traiter\n");
        return 0;
    }
    
    pthread_mutex_lock(&mutexTampon);
    //Copier cette requete dans la structure passee en argument
    memcpy(req, memoire+posLecture, sizeof(struct requete));

    //Modifier la valeur de posLecture et longueurCourante
    if (posLecture ==  memoireTaille)
        posLecture = 0;
    else
        posLecture += 1;
    longueurCourante -= 1;

    //Mettre a jour les variables necessaires aux statistiques (comme sommeTempsAttente)
    //Temps actuel a la fin de la consommation de donnée
    double tempsFin = get_time();
    sommeTempsAttente += tempsFin-req->tempsReception;

    pthread_mutex_unlock(&mutexTampon);

    return 1;
}

unsigned int longueurFile(){
    // Retourne la longueur courante de la file contenue dans votre tampon circulaire.
    return longueurCourante;
}