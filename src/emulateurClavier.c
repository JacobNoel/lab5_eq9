#include "emulateurClavier.h"

FILE* initClavier(){
    // Deja implementee pour vous
    FILE* f = fopen(FICHIER_CLAVIER_VIRTUEL, "wb");
    setbuf(f, NULL);        // On desactive le buffering pour eviter tout delai
    return f;
}
 
 
int ecrireCaracteres(FILE* periphClavier, const char* caracteres, size_t len, unsigned int tempsTraitementParPaquetMicroSecondes){
    //Variable pour suivre le nombre de caracteres écrits et buffer de donnees converties en USB-HID
    size_t written = 0;
    int data [8];
    int zeros, r;

    //Tant qu'on a pas tout envoyé au clavier on continu pour envoyer des paquets de 8 octets (--- 6 caracteres max?)
    while (written < len) {
        //On prepare les donnees
        memset(data, 0, sizeof(data));
        //(c>>5)&1 -> min
        for (int i = 0; i<6; i++){ //On prend max 6 caracteres
            //Tant que les caracteres sont de la meme casse ET qu'il y en a encore à écrire
            while (((caracteres[written+i]>='a' && caracteres[written+i]<='z') == ((caracteres[written]>='a' && caracteres[written]<='z'))) &&  (written+i) < len){
                //Conversion en USB-HID avec la table
                if (caracteres[written+i]==44) // ,
                    data[i+2] = 54;
                else if (caracteres[written+i]==46) // .
                    data[i+2] = 55;
                else if (caracteres[written+i]==32) // SPACE
                    data[i+2] = 44;
                else if (caracteres[written+i]==13) // ENTER
                    data[i+2] = 40;
                else if (caracteres[written+i]==48) // chiffre 0
                    data[i+2] = 39;
                else if ((caracteres[written+i]>=49) && (caracteres[written+i]<=57))  { //chiffre 1-9
                    data[i+2] = caracteres[written+i] - 19;
                }
                else {
                    if (caracteres[written+i]>='a' && caracteres[written+i]<='z') //minuscule
                        data[i+2] = caracteres[written+i] - 93;
                    else { //majuscule
                        data[0] = 2;
                        data[i+2] = caracteres[written+i] - 61;
                    }
                }
            }
        }

        //On écrit les donnees
        if  ((r = fwrite(data, LONGUEUR_USB_PAQUET, 1, periphClavier)) < 0){
            printf("Failed to write data\n");
            return -1;
        }
        written += r;

        //On a fini d'envoyer un paquet, on envoie des 0
        memset(data, 0, sizeof(data));
        if ((zeros = fwrite(data, LONGUEUR_USB_PAQUET, 1, periphClavier)) <=0){
            printf("Failed to write 0\n");
            return -1;
        };
        //On attend un peu...
        usleep(tempsTraitementParPaquetMicroSecondes);  
    }

    return written;
}