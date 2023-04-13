#include "emulateurClavier.h"

FILE* initClavier(){
    // Deja implementee pour vous
    printf("init clavier\n");
    FILE* f = fopen(FICHIER_CLAVIER_VIRTUEL, "wb");
    setbuf(f, NULL);        // On desactive le buffering pour eviter tout delai
    return f;
}

unsigned char asciiVersCodeClavier(char c) {
    if (c==44) // ,
        return 54;
    else if (c==46) // .
        return 55;
    else if (c==32) // SPACE
        return 44;
    else if (c==13 || c==10) // ENTER
        return 40;
    else if (c==48) // chiffre 0
        return 39;
    else if ((c>=49) && (c<=57)) //chiffre 1-9
        return c - 19;
    else if (c>=97 && c<=122) //minuscule
        return c - 93;
    else if (c>=65 && c<=90) { //majuscule
        return c - 61;
    }
    return 0;
}

void relacherTouches(FILE* periphClavier, unsigned char buffer[]) {
    // Buffer pour stocker les 0 à envoyer sur le bus USB
    memset(buffer, 0, LONGUEUR_USB_PAQUET);

    fwrite(buffer, LONGUEUR_USB_PAQUET, 1, periphClavier);
}

 
int ecrireCaracteres(FILE* periphClavier, const char* caracteres, size_t len, unsigned int tempsTraitementParPaquetMicroSecondes){
    // Taille du buffer contenant les données à envoyer sur le bus USB
    const size_t TAILLE_BUF = LONGUEUR_USB_PAQUET;

    // Buffer pour stocker les données à envoyer sur le bus USB
    unsigned char buffer[TAILLE_BUF];
    buffer[0] = 0;
    buffer[1] = 0;

    // Indice dans le buffer pour les données à envoyer
    size_t bufIndex = 2;

    // On boucle sur chaque caractère
    for (size_t i = 0; i < len; i++) {
        if (buffer[0] != 2 && caracteres[i] >= 65 && caracteres[i] <= 90) {
            // Si on a encore des données dans le buffer, on les envoie
            if (bufIndex > 2) {
                // On remplit le reste du buffer avec des 0
                memset(buffer + bufIndex, 0, LONGUEUR_USB_PAQUET - bufIndex);

                size_t result = fwrite(buffer, LONGUEUR_USB_PAQUET, 1, periphClavier);
                if (result != 1) {
                    perror("Erreur lors de l'envoi des donnees sur le bus USB");
                    return -1;
                }
                relacherTouches(periphClavier, buffer);

                // On attend le temps de traitement par paquet
                usleep(tempsTraitementParPaquetMicroSecondes);
            }

            // On réinitialise l'indice dans le buffer
            bufIndex = 2;
            buffer[0] = 2;
        }
        else if (buffer[0] != 0 && (caracteres[i] < 65 || caracteres[i] > 90)) {
            // Si on a encore des données dans le buffer, on les envoie
            if (bufIndex > 2) {
                // On remplit le reste du buffer avec des 0
                memset(buffer + bufIndex, 0, LONGUEUR_USB_PAQUET - bufIndex);

                size_t result = fwrite(buffer, LONGUEUR_USB_PAQUET, 1, periphClavier);
                if (result != 1) {
                    perror("Erreur lors de l'envoi des donnees sur le bus USB");
                    return -1;
                }
                relacherTouches(periphClavier, buffer);

                // On attend le temps de traitement par paquet
                usleep(tempsTraitementParPaquetMicroSecondes);
            }

            // On réinitialise l'indice dans le buffer
            bufIndex = 2;
            buffer[0] = 0;
        }
        // On convertit le caractère ASCII en code clavier
        unsigned char clavierCode = asciiVersCodeClavier(caracteres[i]);

        // On vérifie que la conversion a réussi
        if (clavierCode == 0) {
            fprintf(stderr, "Erreur: caractere non supporte: %c\n", caracteres[i]);
            return -1;
        }

        // On ajoute le code clavier dans le buffer
        buffer[bufIndex++] = clavierCode;

        // Si on a rempli un paquet, on l'envoie
        if (bufIndex >= LONGUEUR_USB_PAQUET) {
            size_t result = fwrite(buffer, LONGUEUR_USB_PAQUET, 1, periphClavier);
            if (result != 1) {
                perror("Erreur lors de l'envoi des donnees sur le bus USB");
                return -1;
            }
            relacherTouches(periphClavier, buffer);

            // On attend le temps de traitement par paquet
            usleep(tempsTraitementParPaquetMicroSecondes);

            // On réinitialise l'indice dans le buffer
            bufIndex = 2;
        }
    }

    // Si on a encore des données dans le buffer, on les envoie
    if (bufIndex > 2) {
        // On remplit le reste du buffer avec des 0
        memset(buffer + bufIndex, 0, LONGUEUR_USB_PAQUET - bufIndex);

        size_t result = fwrite(buffer, LONGUEUR_USB_PAQUET, 1, periphClavier);
        if (result != 1) {
            perror("Erreur lors de l'envoi des donnees sur le bus USB");
            return -1;
        }
        relacherTouches(periphClavier, buffer);

        // On attend le temps de traitement par paquet
        usleep(tempsTraitementParPaquetMicroSecondes);
    }

    return len;
}
