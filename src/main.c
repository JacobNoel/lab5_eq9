#include <pthread.h>
#include <fcntl.h>
#include <string.h>
#include <unistd.h>
#include <math.h>

#include "utils.h"
#include "emulateurClavier.h"
#include "tamponCirculaire.h"


static void* threadFonctionClavier(void* args){
    // Implementez ici votre fonction de thread pour l'ecriture sur le bus USB
    // La premiere des choses est de recuperer les arguments (deja fait pour vous)
    struct infoThreadClavier *infos = (struct infoThreadClavier *)args;

    // Vous devez ensuite attendre sur la barriere passee dans les arguments
    // pour etre certain de commencer au meme moment que le thread lecteur
    // TODO
    pthread_barrier_wait(infos->barriere);

    // Finalement, ecrivez dans cette boucle la logique du thread, qui doit:
    // 1) Tenter d'obtenir une requete depuis le tampon circulaire avec consommerDonnee()
    // 2) S'il n'y en a pas, attendre un cours laps de temps (par exemple usleep(500))
    // 3) S'il y en a une, appeler ecrireCaracteres avec les informations requises
    // 4) Liberer la memoire du champ data de la requete avec la fonction free(), puisque
    //      la requete est maintenant terminee
    
    // Créer une requête pour stocker les données à envoyer sur le bus USB
    struct requete req;

    while(1){
       // TODO

        // Tenter de récupérer une requête depuis le tampon circulaire
        if(consommerDonnee(&req)){
            // S'il y en a une, appeler ecrireCaracteres avec les informations requises
            ecrireCaracteres(infos->pointeurClavier, req.data, req.taille, infos->tempsTraitementParCaractereMicroSecondes);
            // Libérer la mémoire du champ data de la requête avec la fonction free(),
            // puisque la requête est maintenant terminée
            free(req.data);
        } else{
            // S'il n'y en a pas, attendre un court laps de temps
            usleep(500);
        }
    }
    return NULL;
}

static void* threadFonctionLecture(void *args){
    // Implementez ici votre fonction de thread pour la lecture sur le named pipe
    // La premiere des choses est de recuperer les arguments (deja fait pour vous)
    struct infoThreadLecture *infos = (struct infoThreadLecture *)args;
    
    // Ces champs vous seront utiles pour l'appel a select()
    fd_set setFd;
    int nfds = infos->pipeFd + 1;

    // Vous devez ensuite attendre sur la barriere passee dans les arguments
    // pour etre certain de commencer au meme moment que le thread lecteur

    // TODO
    pthread_barrier_wait(infos->barriere);

    // Finalement, ecrivez dans cette boucle la logique du thread, qui doit:
    // 1) Remplir setFd en utilisant FD_ZERO et FD_SET correctement, pour faire en sorte
    //      d'attendre sur infos->pipeFd
    // 2) Appeler select(), sans timeout, avec setFd comme argument de lecture (on veut bien
    //      lire sur le pipe)
    // 3) Lire les valeurs sur le named pipe
    // 4) Si une de ses valeurs est le caracteres ASCII EOT (0x4), alors c'est la fin d'un
    //      message. Vous creez alors une nouvelle requete et utilisez insererDonnee() pour
    //      l'inserer dans le tampon circulaire. Notez que le caractere EOT ne doit PAS se
    //      retrouver dans le champ data de la requete! N'oubliez pas egalement de donner
    //      la bonne valeur aux champs taille et tempsReception.


    int offset = 0;
    // On est sûr de lire moins de 2048 octets (on espère)
    char buf[2048];

            struct requete req;
    // Boucle principale du thread
    while(1){
        // Remplissage du set de descripteurs a surveiller pour le select()
        FD_ZERO(&setFd);
        FD_SET(infos->pipeFd, &setFd);

        // Attente de la disponibilite de donnees sur le named pipe
        if (select(nfds, &setFd, NULL, NULL, NULL) < 0){
            perror("Erreur lors de l'appel a select()");
            exit(EXIT_FAILURE);
        }

        // Lecture des donnees sur le named pipe
        int nBytes = read(infos->pipeFd, buf + offset, 1);
        if (nBytes < 0){
            perror("Erreur lors de la lecture sur le named pipe");
            exit(EXIT_FAILURE);
        }
        if (*(buf + offset) == 0x4) {
            // Creation d'une nouvelle requete
            req.taille = offset;
            req.data = malloc(offset);
            memcpy(req.data, buf, offset);
            req.tempsReception = get_time();

            // Insertion de la requete dans le tampon circulaire
            if (insererDonnee(&req) < 0){
                fprintf(stderr, "Erreur lors de l'insertion de donnees dans le tampon circulaire\n");
                exit(EXIT_FAILURE);
            }
            offset = 0;
        }
        else {
            offset++;
        }
    }
    return NULL;
}

int main(int argc, char* argv[]){
    if(argc < 4){
        printf("Pas assez d'arguments! Attendu : ./emulateurClavier cheminPipe tempsAttenteParPaquet tailleTamponCirculaire\n");
    }

    // A ce stade, vous pouvez consider que:
    // argv[1] contient un chemin valide vers un named pipe
    // argv[2] contient un entier valide (que vous pouvez convertir avec atoi()) representant le nombre de microsecondes a
    //      attendre a chaque envoi de paquet
    // argv[3] contient un entier valide (que vous pouvez convertir avec atoi()) contenant la taille voulue pour le tampon
    //      circulaire



    // Vous avez plusieurs taches d'initialisation a faire :
    //

    // 1) Ouvrir le named pipe
    // TODO
    int pipeFd = open(argv[1], O_RDONLY);
    if(pipeFd == -1){
        perror("Erreur lors de l'ouverture du named pipe");
        return -1;
    }

    // 2) Declarer et initialiser la barriere

    // Convertir les arguments en entiers
    int tempsAttente = atoi(argv[2]);
    int tailleTampon = atoi(argv[3]);
    
    // TODO
    pthread_barrier_t barrier;
    if(pthread_barrier_init(&barrier, NULL, 3) != 0){
        perror("Erreur lors de l'initialisation de la barriere");
        return -1;
    }

    // 3) Initialiser le tampon circulaire avec la bonne taille

    // TODO
    if(initTamponCirculaire(tailleTampon) != 0){
        perror("Erreur lors de l'initialisation du tampon circulaire");
        return -1;
    }

    // 4) Creer et lancer les threads clavier et lecteur, en leur passant les bons arguments dans leur struct de configuration respective
    
    // TODO
     // Creer les structures de configuration pour les threads
    struct infoThreadLecture infosLecture = {pipeFd, &barrier};
    struct infoThreadClavier infosClavier = {initClavier(), (unsigned int)tempsAttente, &barrier};

    // Creer et lancer les threads clavier et lecteur
    pthread_t threadClavier, threadLecture;
    if(pthread_create(&threadClavier, NULL, &threadFonctionClavier, (void*)&infosClavier) != 0){
        perror("Erreur lors de la creation du thread clavier");
        return -1;
    }
    if(pthread_create(&threadLecture, NULL, &threadFonctionLecture, (void*)&infosLecture) != 0){
        perror("Erreur lors de la creation du thread lecteur");
        return -1;
    }

    // Attendre que les deux threads aient atteint la barriere avant de continuer
    pthread_barrier_wait(&barrier);


    // La boucle de traitement est deja implementee pour vous. Toutefois, si vous voulez eviter l'affichage des statistiques
    // (qui efface le terminal a chaque fois), vous pouvez commenter la ligne afficherStats().
    struct statistiques stats;
    double tempsDebut = get_time();
    while(1){
        // Affichage des statistiques toutes les secondes
        calculeStats(&stats);
        afficherStats((unsigned int)(round(get_time() - tempsDebut)), &stats);
        resetStats();
        usleep(1e6);
    }
    return 0;
}
