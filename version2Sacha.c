/**
* @file version1.c
* @brief Jeu snake autonome SAE1.01
* @author Noah Le Goff, Sacha Mace
* @version 1.0
* @date 10/12/24
*
* Le serpent avance automatiquement et peut changer de direction automatiquement
* Le serpent va donc se diriger vers les pommes sans toucher d'obstacle.
* Le jeu se termine lorsque la touche 'a' est pressée ou lorsque le serpent a manger 10 pommes.
*
*/

/* Fichiers inclus */
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <unistd.h>
#include <termios.h>
#include <fcntl.h>
#include <time.h>


// taille du serpent
#define TAILLE 10
// dimensions du plateau
#define LARGEUR_PLATEAU 80	
#define HAUTEUR_PLATEAU 40	//  
// position initiale de la tête du serpent
#define X_INITIAL 40
#define Y_INITIAL 20
// nombre de pommes à manger pour gagner
#define NB_POMMES 10
// temporisation entre deux déplacements du serpent (en microsecondes)
#define ATTENTE 200000
// caractères pour représenter le serpent
#define CORPS 'X'
#define TETE 'O'
// touches de direction ou d'arrêt du jeu
#define HAUT 'z'
#define BAS 's'
#define GAUCHE 'q'
#define DROITE 'd'
#define STOP 'a'
// caractères pour les éléments du plateau
#define BORDURE '#'
#define VIDE ' '
#define POMME '6'


// définition d'un type pour le plateau : tPlateau
// Attention, pour que les indices du tableau 2D (qui commencent à 0) coincident
// avec les coordonées à l'écran (qui commencent à 1), on ajoute 1 aux dimensions
// et on neutralise la ligne 0 et la colonne 0 du tableau 2D (elles ne sont jamais
// utilisées)
typedef char tPlateau[LARGEUR_PLATEAU+1][HAUTEUR_PLATEAU+1];

const int lesPommesX[NB_POMMES] = {75, 75, 78, 2, 8, 78, 74, 2, 72, 5};
const int lesPommesY[NB_POMMES] = { 8, 39, 2, 2, 5, 39, 33, 38, 35, 2};


void initPlateau(tPlateau plateau);
void dessinerPlateau(tPlateau plateau);
void ajouterPomme(tPlateau plateau, int iPomme);
void afficher(int, int, char);
void effacer(int x, int y);
void dessinerSerpent(int lesX[], int lesY[]);
void progresser(int lesX[], int lesY[], char direction, tPlateau plateau, bool * collision, bool * pomme);
void gotoxy(int x, int y);
int kbhit();
void disable_echo();
void enable_echo();

/**
* @brief  Entrée du programme
* @return EXIT_SUCCESS : arrêt normal du programme
*/
int main(){
	// départ du calcul du temps CPU
	clock_t begin = clock();
	// 2 tableaux contenant les positions des éléments qui constituent le serpent
    int lesX[TAILLE];
	int lesY[TAILLE];

	// représente la touche frappée par l'utilisateur : touche de direction ou pour l'arrêt
	char touche;

	//direction courante du serpent (HAUT, BAS, GAUCHE ou DROITE)
	char direction;

	// le plateau de jeu
	tPlateau lePlateau;

	bool collision=false;
	bool gagne = false;
	bool pommeMangee = false;

	// compteur de pommes mangées
	int nbPommes = 0;
   
	// initialisation de la position du serpent : positionnement de la
	// tête en (X_INITIAL, Y_INITIAL), puis des anneaux à sa gauche
	for(int i=0 ; i<TAILLE ; i++){
		lesX[i] = X_INITIAL-i;
		lesY[i] = Y_INITIAL;
	}

	// mise en place du plateau
	initPlateau(lePlateau);
	system("clear");
	dessinerPlateau(lePlateau);


	srand(time(NULL));
	ajouterPomme(lePlateau, nbPommes);

	// initialisation : le serpent se dirige vers la DROITE
	dessinerSerpent(lesX, lesY);
	disable_echo();
	direction = DROITE;

	// boucle de jeu. Arret si touche STOP, si collision avec une bordure ou
	// si toutes les pommes sont mangées
	do {
		// Condition pour que le serpent change de direction automatiquement
		if (lesPommesX[nbPommes] < lesX[0]){
			direction = GAUCHE;
		}
		else if (lesPommesX[nbPommes] > lesX[0]){
			direction = DROITE;
		}
		else if (lesPommesY[nbPommes] < lesY[0]){
			direction = HAUT;
		}
		else if (lesPommesY[nbPommes] > lesY[0]){
			direction = BAS;
		}

		progresser(lesX, lesY, direction, lePlateau, &collision, &pommeMangee);

		// Ajoute une pomme au compteur de pomme quand elle est mangée et arrete le jeu si score atteint 10
		if (pommeMangee){
            nbPommes++;
			gagne = (nbPommes==NB_POMMES);
			if (!gagne){
				ajouterPomme(lePlateau, nbPommes);
				pommeMangee = false;
			}	
			
		}
		if (!gagne){
			if (!collision){
				usleep(ATTENTE);
				if (kbhit()==1){
					touche = getchar();
				}
			}
		}
	} while (touche != STOP && !collision && !gagne);
    enable_echo();
	gotoxy(1, HAUTEUR_PLATEAU+1);

	clock_t end = clock();
	double tmpsCPU = ((end - begin)*1.0) / CLOCKS_PER_SEC;
	printf( "Temps CPU = %.3f secondes\n",tmpsCPU);

	return EXIT_SUCCESS;
}


/************************************************/
/*		FONCTIONS ET PROCEDURES DU JEU 			*/
/************************************************/

/**
 * @brief Initialise le plateau de jeu avec les bordures et des pavés aléatoires.
 * @param plateau de type tPlateau, qui donne en Entrée le plateau du jeu.
 */
void initPlateau(tPlateau plateau){
	// initialisation du plateau avec des espaces
	for (int i=1 ; i<=LARGEUR_PLATEAU ; i++){
		for (int j=1 ; j<=HAUTEUR_PLATEAU ; j++){
			plateau[i][j] = VIDE;
		}
	}
	// Mise en place la bordure autour du plateau
	// première ligne
	for (int i=1 ; i<=LARGEUR_PLATEAU ; i++){
		plateau[i][1] = BORDURE;
		plateau[LARGEUR_PLATEAU/2][1] = VIDE;
	}
	// lignes intermédiaires
	for (int j=1 ; j<=HAUTEUR_PLATEAU ; j++){
			plateau[1][j] = BORDURE;
			plateau[1][HAUTEUR_PLATEAU / 2] = VIDE;
			plateau[LARGEUR_PLATEAU][j] = BORDURE;
			plateau[LARGEUR_PLATEAU][HAUTEUR_PLATEAU/ 2] = VIDE;
		}
	// dernière ligne
	for (int i=1 ; i<=LARGEUR_PLATEAU ; i++){
		plateau[i][HAUTEUR_PLATEAU] = BORDURE;
		plateau[LARGEUR_PLATEAU / 2 ][HAUTEUR_PLATEAU] = VIDE;
	}
}

/**
* @brief Dessine l'ensemble du plateau de jeu dans le terminal.
* @param plateau de type tPlateau, qui donne en Entrée le plateau du jeu.
*/
void dessinerPlateau(tPlateau plateau){
	// affiche eà l'écran le contenu du tableau 2D représentant le plateau
	for (int i=1 ; i<=LARGEUR_PLATEAU ; i++){
		for (int j=1 ; j<=HAUTEUR_PLATEAU ; j++){
			afficher(i, j, plateau[i][j]);
		}
	}
}

/**
 * @brief Ajoute une pomme dans une case libre du plateau.
 * @param plateau de type tPlateau, qui donne en Entrée le plateau du jeu.
 * @param iPomme de type int, qui donne la coordonnée de la prochaine pomme.
 */
void ajouterPomme(tPlateau plateau, int iPomme){
	// génère aléatoirement la position d'une pomme,
	// vérifie que ça correspond à une case vide
	// du plateau puis l'ajoute au plateau et l'affiche
	int xPomme, yPomme;
	do{
		xPomme = lesPommesX[iPomme];
		yPomme = lesPommesY[iPomme];
	} while (plateau[xPomme][yPomme]!=' ');
	plateau[xPomme][yPomme]=POMME;
	afficher(xPomme, yPomme, POMME);
}

/**
* @brief Procédure qui va aux coordonées X et Y, et qui affiche le caractere c entré en parametre
* @param x de type int, Entrée : la coordonnée de x
* @param y de type int, Entrée : la coordonnée de y
* @param c de type char, Entrée : le caractere a afficher
*/
void afficher(int x, int y, char car){
	gotoxy(x, y);
	printf("%c", car);
	gotoxy(1,1);
}

/**
* @brief Procédure qui va aux coordonées X et Y, et qui affiche un espace pour effacer un caractere
* @param x de type int, Entrée : la coordonnée de x
* @param y de type int, Entrée : la coordonnée de y
*/
void effacer(int x, int y){
	gotoxy(x, y);
	printf(" ");
	gotoxy(1,1);
}

/**
* @brief Procédure qui affiche le corps du serpent, '^' ou '<' ou '>' ou 'v' pour la tete et 'X' pour le corps
* @param lesX de type int tableau, Entrée : le tableau des X de N élément
* @param lesY de type int tableau, Entrée : le tableau des Y de N élément
*/
void dessinerSerpent(int lesX[], int lesY[]){
	// affiche les anneaux puis la tête
	for(int i=1 ; i<TAILLE ; i++){
		afficher(lesX[i], lesY[i], CORPS);
	}
	afficher(lesX[0], lesY[0],TETE);
}

/**
* @brief Procédure qui calcule la prochaine position du serpent et qui l'affiche,
* elle permet aussi de savoir si le serpent entre en collision avec une bordure, un pavé, ou une pomme.
* @param lesX de type int tableau, Entrée : le tableau des X de N élément
* @param lesY de type int tableau, Entrée : le tableau des Y de N élément
* @param plateau de type tPlateau, qui donne en Entrée le plateau du jeu
* @param direction de type char, Entrée : la direction du serpent attribuée aux touches 'z' 'q' 's' 'd'
* @param collision de type bool, vérifie si il y a une collision
* @param pomme de type bool, vérifie si une pomme est mangée
*/
void progresser(int lesX[], int lesY[], char direction, tPlateau plateau, bool * collision, bool * pomme){
	// efface le dernier élément avant d'actualiser la position de tous les 
	// élémentds du serpent avant de le  redessiner et détecte une
	// collision avec une pomme ou avec une bordure
	effacer(lesX[TAILLE-1], lesY[TAILLE-1]);

	for(int i=TAILLE-1 ; i>0 ; i--){
		lesX[i] = lesX[i-1];
		lesY[i] = lesY[i-1];
	}
	//faire progresser la tete dans la nouvelle direction
	switch(direction){
		case HAUT : 
			lesY[0] = lesY[0] - 1;
			break;
		case BAS:
			lesY[0] = lesY[0] + 1;
			break;
		case DROITE:
			lesX[0] = lesX[0] + 1;
			break;
		case GAUCHE:
			lesX[0] = lesX[0] - 1;
			break;
	}
	*pomme = false;
	// détection d'une "collision" avec une pomme
	if (plateau[lesX[0]][lesY[0]] == POMME){
		*pomme = true;
		// la pomme disparait du plateau
		plateau[lesX[0]][lesY[0]] = VIDE;
	}
	// détection d'une collision avec la bordure
	else if (plateau[lesX[0]][lesY[0]] == BORDURE){
		*collision = true;
	}
   	dessinerSerpent(lesX, lesY);
}



/************************************************/
/*				 FONCTIONS UTILITAIRES 			*/
/************************************************/

/**
* @brief Procédure qui va aux coordonées X et Y donnée en parametre
* @param x de type int, Entrée : la coordonnée de x
* @param y de type int, Entrée : la coordonnée de y
*/
void gotoxy(int x, int y) { 
    printf("\033[%d;%df", y, x);
}

/**
* @brief Fonction qui vérifie si une touche est tapée dans le terminal
* @return 1 si un caractere est present, 0 si pas de caractere present
*/
int kbhit(){
	// la fonction retourne :
	// 1 si un caractere est present
	// 0 si pas de caractere présent
	int unCaractere=0;
	struct termios oldt, newt;
	int ch;
	int oldf;

	// mettre le terminal en mode non bloquant
	tcgetattr(STDIN_FILENO, &oldt);
	newt = oldt;
	newt.c_lflag &= ~(ICANON | ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &newt);
	oldf = fcntl(STDIN_FILENO, F_GETFL, 0);
	fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);
 
	ch = getchar();

	// restaurer le mode du terminal
	tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
	fcntl(STDIN_FILENO, F_SETFL, oldf);
 
	if(ch != EOF){
		ungetc(ch, stdin);
		unCaractere=1;
	} 
	return unCaractere;
}

// Fonction pour désactiver l'echo
void disable_echo() {
    struct termios tty;

    // Obtenir les attributs du terminal
    if (tcgetattr(STDIN_FILENO, &tty) == -1) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }

    // Désactiver le flag ECHO
    tty.c_lflag &= ~ECHO;

    // Appliquer les nouvelles configurations
    if (tcsetattr(STDIN_FILENO, TCSANOW, &tty) == -1) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }
}

// Fonction pour réactiver l'echo
void enable_echo() {
    struct termios tty;

    // Obtenir les attributs du terminal
    if (tcgetattr(STDIN_FILENO, &tty) == -1) {
        perror("tcgetattr");
        exit(EXIT_FAILURE);
    }

    // Réactiver le flag ECHO
    tty.c_lflag |= ECHO;

    // Appliquer les nouvelles configurations
    if (tcsetattr(STDIN_FILENO, TCSANOW, &tty) == -1) {
        perror("tcsetattr");
        exit(EXIT_FAILURE);
    }
}