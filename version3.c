/**
 * @file version2.c
 * @brief Jeu snake autonome SAE1.02
 * @author Noah Le Goff, Sacha Mace
 * @version 2.0
 * @date 18/12/24
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
#define HAUTEUR_PLATEAU 40
// position initiale de la tête du serpent
#define X_INITIAL 40
#define Y_INITIAL 20
// position des trous
#define TROU_HAUT_X 40
#define TROU_HAUT_Y 0
#define TROU_BAS_X 40
#define TROU_BAS_Y 40
#define TROU_GAUCHE_X 0
#define TROU_GAUCHE_Y 20
#define TROU_DROITE_X 80
#define TROU_DROITE_Y 20
// nombre de pommes à manger pour gagner
#define NB_POMMES 10
// temporisation entre deux déplacements du serpent (en microsecondes)
#define ATTENTE 100000
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
// pavés
#define NB_PAVES 6
#define TAILLE_PAVE 5
// valeur renvoyer en fonction de la distance
#define CHEMIN_HAUT 1
#define CHEMIN_BAS 2
#define CHEMIN_GAUCHE 3
#define CHEMIN_DROITE 4
#define CHEMIN_POMME 5

// définition d'un type pour le plateau : tPlateau
// Attention, pour que les indices du tableau 2D (qui commencent à 0) coincident
// avec les coordonées à l'écran (qui commencent à 1), on ajoute 1 aux dimensions
// et on neutralise la ligne 0 et la colonne 0 du tableau 2D (elles ne sont jamais
// utilisées)
typedef char tPlateau[LARGEUR_PLATEAU + 1][HAUTEUR_PLATEAU + 1];

// coordonnées des pommes
const int lesPommesX[NB_POMMES] = {75, 75, 78, 2, 8, 78, 74, 2, 72, 5};
const int lesPommesY[NB_POMMES] = { 8, 39, 2, 2, 5, 39, 33, 38, 35, 2};

// coordonnées des pavés
const int lesPavesX[NB_PAVES] = { 3, 74, 3, 74, 38, 38};
const int lesPavesY[NB_PAVES] = { 3, 3, 34, 34, 21, 15};

void initPlateau(tPlateau plateau);
void dessinerPlateau(tPlateau plateau);
void ajouterPomme(tPlateau plateau, int iPomme);
void afficher(int, int, char);
void effacer(int x, int y);
void dessinerSerpent(int lesX[], int lesY[]);
void directionSerpentVersObjectif(int lesX[], int lesY[], tPlateau plateau, char *direction, int objectifX, int objectifY, bool changement);
bool verifierCollision(int lesX[], int lesY[], tPlateau plateau, char directionProchaine);
int calculerDistance(int lesX[], int lesY[], int pommeX, int pommeY);
void progresser(int lesX[], int lesY[], char direction, tPlateau plateau, bool *collision, bool *pomme, bool *teleporter);
int calculerDistancePommePave(int pommeX, int pommeY, int paveX, int paveY);
void gotoxy(int x, int y);
int kbhit();
void disable_echo();
void enable_echo();

/**
 * @brief  Entrée du programme
 * @return EXIT_SUCCESS : arrêt normal du programme
 */
int main()
{
	// départ du calcul du temps CPU
	clock_t begin = clock();

	// Total des déplacements
	int deplacement = 0;

	// 2 tableaux contenant les positions des éléments qui constituent le serpent
	int lesX[TAILLE];
	int lesY[TAILLE];

	// représente la touche frappée par l'utilisateur : touche de direction ou pour l'arrêt
	char touche;

	// direction courante du serpent (HAUT, BAS, GAUCHE ou DROITE)
	char direction;

	// le plateau de jeu
	tPlateau lePlateau;

	bool collision = false;
	bool gagne = false;
	bool pommeMangee = false;
	bool teleporter = false;
	bool changement = false;

	// compteur de pommes mangées
	int nbPommesMangee = 0;

	// initialisation de la position du serpent : positionnement de la
	// tête en (X_INITIAL, Y_INITIAL), puis des anneaux à sa gauche
	for (int i = 0; i < TAILLE; i++)
	{
		lesX[i] = X_INITIAL - i;
		lesY[i] = Y_INITIAL;
	}

	// mise en place du plateau
	initPlateau(lePlateau);
	system("clear");
	dessinerPlateau(lePlateau);

	srand(time(NULL));
	ajouterPomme(lePlateau, nbPommesMangee);

	// initialisation : le serpent se dirige vers la DROITE
	dessinerSerpent(lesX, lesY);
	disable_echo();
	direction = DROITE;

	// calcul la meilleur distance à l'initialisation
	int meilleurDistance = calculerDistance(lesX, lesY, lesPommesX[nbPommesMangee], lesPommesY[nbPommesMangee]);

	// boucle de jeu. Arret si touche STOP, si collision avec une bordure ou
	// si toutes les pommes sont mangées
	do
	{
		// choisis la direction en fonction de la meilleur distance
		if (meilleurDistance == CHEMIN_HAUT) // se dirige vers le trou du haut puis quand il s'est téléporter avance vers la pomme
		{
			if (teleporter)
			{
				directionSerpentVersObjectif(lesX, lesY, lePlateau, &direction, lesPommesX[nbPommesMangee], lesPommesY[nbPommesMangee], changement);
			}
			else
			{
				directionSerpentVersObjectif(lesX, lesY, lePlateau, &direction, TROU_HAUT_X, TROU_HAUT_Y, changement);
			}
		}
		else if (meilleurDistance == CHEMIN_BAS) // se dirige vers le trou du bas puis quand il s'est téléporter avance vers la pomme
		{
			if (teleporter)
			{
				directionSerpentVersObjectif(lesX, lesY, lePlateau, &direction, lesPommesX[nbPommesMangee], lesPommesY[nbPommesMangee], changement);
			}
			else
			{
				directionSerpentVersObjectif(lesX, lesY, lePlateau, &direction, TROU_BAS_X, TROU_BAS_Y, changement);
			}
		}
		else if (meilleurDistance == CHEMIN_GAUCHE) // se dirige vers le trou de gauche puis quand il s'est téléporter avance vers la pomme
		{
			if (teleporter)
			{
				directionSerpentVersObjectif(lesX, lesY, lePlateau, &direction, lesPommesX[nbPommesMangee], lesPommesY[nbPommesMangee], changement);
			}
			else
			{
				directionSerpentVersObjectif(lesX, lesY, lePlateau, &direction, TROU_GAUCHE_X, TROU_GAUCHE_Y, changement);
			}
		}
		else if (meilleurDistance == CHEMIN_DROITE) // se dirige vers le trou de droite puis quand il s'est téléporter avance vers la pomme
		{
			if (teleporter)
			{
				directionSerpentVersObjectif(lesX, lesY, lePlateau, &direction, lesPommesX[nbPommesMangee], lesPommesY[nbPommesMangee], changement);
			}
			else
			{
				directionSerpentVersObjectif(lesX, lesY, lePlateau, &direction, TROU_DROITE_X, TROU_DROITE_Y, changement);
			}
		}
		else // sinon se dirige uniquement vers la pomme
		{
			directionSerpentVersObjectif(lesX, lesY, lePlateau, &direction, lesPommesX[nbPommesMangee], lesPommesY[nbPommesMangee], changement);
		}

		progresser(lesX, lesY, direction, lePlateau, &collision, &pommeMangee, &teleporter);
		deplacement++;

		// Ajoute une pomme au compteur de pomme quand elle est mangée et arrete le jeu si score atteint 10
		if (pommeMangee)
		{
			nbPommesMangee++;
			gagne = (nbPommesMangee == NB_POMMES);
			teleporter = false; // remet en false pour pouvoir se retéléporter après avoir manger une pomme
			changement = false;
			if (!gagne)
			{
				ajouterPomme(lePlateau, nbPommesMangee);
				meilleurDistance = calculerDistance(lesX, lesY, lesPommesX[nbPommesMangee], lesPommesY[nbPommesMangee]); // recalcul la meilleur position après l'apparition d'une nouvelle pomme
				pommeMangee = false;
			}
		}
		if (!gagne)
		{
			if (!collision)
			{
				usleep(ATTENTE);
				if (kbhit() == 1)
				{
					touche = getchar();
				}
			}
		}
	} while (touche != STOP && !collision && !gagne);
	enable_echo();
	gotoxy(1, HAUTEUR_PLATEAU + 1);

	clock_t end = clock(); // fin du calcul du temps CPU
	double tmpsCPU = ((end - begin) * 1.0) / CLOCKS_PER_SEC;

	// afficher les performances du programme
	printf("Temps CPU = %.3f secondes\n", tmpsCPU);
	printf("Le serpent c'est déplacer %d fois\n", deplacement);

	return EXIT_SUCCESS;
}

/************************************************/
/*		FONCTIONS ET PROCEDURES DU JEU 			*/
/************************************************/

/**
 * @brief Initialise le plateau de jeu avec les bordures et des pavés aléatoires.
 * @param plateau de type tPlateau, qui donne en Entrée le plateau du jeu.
 */
void initPlateau(tPlateau plateau)
{
	// initialisation du plateau avec des espaces
	for (int i = 1; i <= LARGEUR_PLATEAU; i++)
	{
		for (int j = 1; j <= HAUTEUR_PLATEAU; j++)
		{
			plateau[i][j] = VIDE;
		}
	}
	// Mise en place la bordure autour du plateau
	// première ligne
	for (int i = 1; i <= LARGEUR_PLATEAU; i++)
	{
		plateau[i][1] = BORDURE;
		plateau[LARGEUR_PLATEAU / 2][1] = VIDE; // trou du haut
	}
	// lignes intermédiaires
	for (int j = 1; j <= HAUTEUR_PLATEAU; j++)
	{
		plateau[1][j] = BORDURE;
		plateau[1][HAUTEUR_PLATEAU / 2] = VIDE; // trou de gauche
		plateau[LARGEUR_PLATEAU][j] = BORDURE;
		plateau[LARGEUR_PLATEAU][HAUTEUR_PLATEAU / 2] = VIDE; // trou de droite
	}
	// dernière ligne
	for (int i = 1; i <= LARGEUR_PLATEAU; i++)
	{
		plateau[i][HAUTEUR_PLATEAU] = BORDURE;
		plateau[LARGEUR_PLATEAU / 2][HAUTEUR_PLATEAU] = VIDE; // trou du bas
	}

	for (int p = 0; p < NB_PAVES; p++) {
		int xPave, yPave;
		// Générer des coordonnées aléatoires pour le pavé
		xPave = lesPavesX[p];
    	yPave = lesPavesY[p];;

		// Dessiner le pavé sur le plateau
        for (int i = 0; i < TAILLE_PAVE; i++) {
            for (int j = 0; j < TAILLE_PAVE; j++) {
                plateau[xPave + i][yPave + j] = BORDURE;  // Dessine le pavé
            }
        }
    }
}

/**
 * @brief Dessine l'ensemble du plateau de jeu dans le terminal.
 * @param plateau de type tPlateau, qui donne en Entrée le plateau du jeu.
 */
void dessinerPlateau(tPlateau plateau)
{
	// affiche à l'écran le contenu du tableau 2D représentant le plateau
	for (int i = 1; i <= LARGEUR_PLATEAU; i++)
	{
		for (int j = 1; j <= HAUTEUR_PLATEAU; j++)
		{
			afficher(i, j, plateau[i][j]);
		}
	}
}

/**
 * @brief Ajoute une pomme dans une case libre du plateau.
 * @param plateau de type tPlateau, qui donne en Entrée le plateau du jeu.
 * @param iPomme de type int, qui donne la coordonnée de la prochaine pomme.
 */
void ajouterPomme(tPlateau plateau, int iPomme)
{
	// génère aléatoirement la position d'une pomme,
	// vérifie que ça correspond à une case vide
	// du plateau puis l'ajoute au plateau et l'affiche
	int xPomme, yPomme;
	do
	{
		xPomme = lesPommesX[iPomme];
		yPomme = lesPommesY[iPomme];
	} while (plateau[xPomme][yPomme] != ' ');
	plateau[xPomme][yPomme] = POMME;
	afficher(xPomme, yPomme, POMME);
}

/**
 * @brief Procédure qui va aux coordonées X et Y, et qui affiche le caractere c entré en parametre
 * @param x de type int, Entrée : la coordonnée de x
 * @param y de type int, Entrée : la coordonnée de y
 * @param c de type char, Entrée : le caractere a afficher
 */
void afficher(int x, int y, char car)
{
	gotoxy(x, y);
	printf("%c", car);
	gotoxy(1, 1);
}

/**
 * @brief Procédure qui va aux coordonées X et Y, et qui affiche un espace pour effacer un caractere
 * @param x de type int, Entrée : la coordonnée de x
 * @param y de type int, Entrée : la coordonnée de y
 */
void effacer(int x, int y)
{
	gotoxy(x, y);
	printf(" ");
	gotoxy(1, 1);
}

/**
 * @brief Procédure qui affiche le corps du serpent, 'O' pour la tete et 'X' pour le corps
 * @param lesX de type int tableau, Entrée : le tableau des X de N élément
 * @param lesY de type int tableau, Entrée : le tableau des Y de N élément
 */
void dessinerSerpent(int lesX[], int lesY[])
{
	// affiche les anneaux puis la tête
	for (int i = 1; i < TAILLE; i++)
	{
		afficher(lesX[i], lesY[i], CORPS);
	}
	afficher(lesX[0], lesY[0], TETE);
}

/**
 * @brief Procédure qui choisit la direction la plus optimiser et courte pour atteindre l'objectif
 * @param lesX de type int tableau, Entrée : le tableau des X de N élément
 * @param lesY de type int tableau, Entrée : le tableau des Y de N élément
 * @param plateau de type tPlateau, qui donne en Entrée le plateau du jeu
 * @param direction de type char, Entrée/Sortie : la direction du serpent attribuée aux touches 'z' 'q' 's' 'd'
 * @param objectifX de type int, Entrée : les coordonnées en X de l'objecif à atteindre
 * @param objectifY de type int, Entrée : les coordonnées en Y de l'objecif à atteindre
 */
void directionSerpentVersObjectif(int lesX[], int lesY[], tPlateau plateau, char *direction, int objectifX, int objectifY, bool changement)
{
	// Calcul des directions possibles
	int dx = objectifX - lesX[0]; // Différence en X
	int dy = objectifY - lesY[0]; // Différence en Y

	if(!changement){
		// Essayer de se déplacer dans la direction verticale
		if (dy != 0)
		{
			*direction = (dy > 0) ? BAS : HAUT;
			if (verifierCollision(lesX, lesY, plateau, *direction))
			{
				// Si collision, essayer la direction horizontale
				*direction = (dx > 0) ? DROITE : GAUCHE;
				if (verifierCollision(lesX, lesY, plateau, *direction))
				{
					// Si collision, essayer l'autre direction horizontale
					*direction = (dx > 0) ? GAUCHE : DROITE;
					if (verifierCollision(lesX, lesY, plateau, *direction))
					{
						// Si collision, essayer l'autre direction verticale
						*direction = (dy > 0) ? HAUT : BAS;
					}
				}
			}
		}

		// Si pas de déplacement verticale possible, essayer horizontale
		else if (dx != 0)
		{
			*direction = (dx > 0) ? DROITE : GAUCHE;
			if (verifierCollision(lesX, lesY, plateau, *direction))
			{
				// Si collision, essayer la direction verticale
				*direction = (dy > 0) ? BAS : HAUT;
				if (verifierCollision(lesX, lesY, plateau, *direction))
				{
					// Si collision, essayer l'autre direction verticale
					*direction = (dy > 0) ? HAUT : BAS;
					if (verifierCollision(lesX, lesY, plateau, *direction))
					{
						// Si collision, essayer l'autre direction horizontale
						*direction = (dx > 0) ? GAUCHE : DROITE;
					}
				}
			}
		}
	}
	else if(changement){
		if (dx != 0)
		{
			*direction = (dx > 0) ? DROITE : GAUCHE;
			if (verifierCollision(lesX, lesY, plateau, *direction))
			{
				// Si collision, essayer la direction verticale
				*direction = (dy > 0) ? BAS : HAUT;
				if (verifierCollision(lesX, lesY, plateau, *direction))
				{
					// Si collision, essayer l'autre direction verticale
					*direction = (dx > 0) ? GAUCHE : DROITE;
					if (verifierCollision(lesX, lesY, plateau, *direction))
					{
						// Si collision, essayer l'autre direction horizontale
						*direction = (dy > 0) ? HAUT : BAS;
					}
				}
			}
		}
		else if (dy != 0)
		{
			*direction = (dy > 0) ? BAS : HAUT;
			if (verifierCollision(lesX, lesY, plateau, *direction))
			{
				// Si collision, essayer la direction horizontale
				*direction = (dx > 0) ? GAUCHE : DROITE;
				if (verifierCollision(lesX, lesY, plateau, *direction))
				{
					// Si collision, essayer l'autre direction horizontale
						*direction = (dy > 0) ? HAUT : BAS;
					if (verifierCollision(lesX, lesY, plateau, *direction))
					{
						// Si collision, essayer l'autre direction verticale
						*direction = (dx > 0) ? DROITE : GAUCHE;
					}
				}
			}
		}
	}
}

bool calculAvecPavesPommeSerpent(int nbPommesMangee){
	if((lesPommesX[nbPommesMangee]<LARGEUR_PLATEAU && lesPommesX[nbPommesMangee]>lesPavesX+4) && ){

	}
	return changement;
}

/**
 * @brief Fonction qui calcule puis renvoie le chemin le plus rapide
 * @param lesX de type int tableau, Entrée : le tableau des X de N élément
 * @param lesY de type int tableau, Entrée : le tableau des Y de N élément
 * @param pommeX de type int, Entrée : les coordonnées des pommes en X
 * @param pommeY de type int, Entrée : les coordonnées des pommes en Y
 */
int calculerDistance(int lesX[], int lesY[], int pommeX, int pommeY)
{
	// définition des variables
	int passageTrouGauche, passageTrouDroit, passageTrouHaut, passageTrouBas, passageDirectPomme, resultat;

	// calcul la distance pour chaque chemin du serpent vers la pomme
	passageTrouGauche = abs(lesX[0] - TROU_GAUCHE_X) + abs(lesY[0] - TROU_GAUCHE_Y) + abs(pommeX - TROU_DROITE_X) + abs(pommeY - TROU_DROITE_Y);
	passageTrouDroit = abs(lesX[0] - TROU_DROITE_X) + abs(lesY[0] - TROU_DROITE_Y) + abs(pommeX - TROU_GAUCHE_X) + abs(pommeY - TROU_GAUCHE_Y);
	passageTrouHaut = abs(lesX[0] - TROU_HAUT_X) + abs(lesY[0] - TROU_HAUT_Y) + abs(pommeX - TROU_BAS_X) + abs(pommeY - TROU_BAS_Y);
	passageTrouBas = abs(lesX[0] - TROU_BAS_X) + abs(lesY[0] - TROU_BAS_Y) + abs(pommeX - TROU_HAUT_X) + abs(pommeY - TROU_HAUT_Y);
	passageDirectPomme = abs(lesX[0] - pommeX) + abs(lesY[0] - pommeY);

	// compare les résultats pour obtenir le meilleur chemin
	if (passageDirectPomme <= passageTrouHaut && passageDirectPomme <= passageTrouBas &&
		passageDirectPomme <= passageTrouGauche && passageDirectPomme <= passageTrouDroit) // chemin direct vers la pomme sans passer dans un trou
	{
		resultat = CHEMIN_POMME;
	}
	else if (passageTrouHaut <= passageTrouBas && passageTrouHaut <= passageTrouGauche && passageTrouHaut <= passageTrouDroit) // chemin vers la pomme en passant par le trou du haut
	{
		resultat = CHEMIN_HAUT;
	}
	else if (passageTrouBas <= passageTrouGauche && passageTrouBas <= passageTrouDroit) // chemin vers la pomme en passant par le trou du bas
	{
		resultat = CHEMIN_BAS;
	}
	else if (passageTrouGauche <= passageTrouDroit) // chemin vers la pomme en passant par le trou de gauche
	{
		resultat = CHEMIN_GAUCHE;
	}
	else // chemin vers la pomme en passant par le trou de droite
	{
		resultat = CHEMIN_DROITE;
	}

	return resultat;
}

int calculerDistancePommePave(int pommeX, int pommeY, int paveX, int paveY)
{
	// définition des variables
	int passageTrouGauche, passageTrouDroit, passageTrouHaut, passageTrouBas, passageDirectPomme, resultat;

	// calcul la distance pour chaque chemin du serpent vers la pomme
	passageTrouGauche = abs(lesX[0] - TROU_GAUCHE_X) + abs(lesY[0] - TROU_GAUCHE_Y) + abs(pommeX - TROU_DROITE_X) + abs(pommeY - TROU_DROITE_Y);
	passageTrouDroit = abs(lesX[0] - TROU_DROITE_X) + abs(lesY[0] - TROU_DROITE_Y) + abs(pommeX - TROU_GAUCHE_X) + abs(pommeY - TROU_GAUCHE_Y);
	passageTrouHaut = abs(lesX[0] - TROU_HAUT_X) + abs(lesY[0] - TROU_HAUT_Y) + abs(pommeX - TROU_BAS_X) + abs(pommeY - TROU_BAS_Y);
	passageTrouBas = abs(lesX[0] - TROU_BAS_X) + abs(lesY[0] - TROU_BAS_Y) + abs(pommeX - TROU_HAUT_X) + abs(pommeY - TROU_HAUT_Y);
	passageDirectPomme = abs(lesX[0] - pommeX) + abs(lesY[0] - pommeY);

	// compare les résultats pour obtenir le meilleur chemin
	if (passageDirectPomme <= passageTrouHaut && passageDirectPomme <= passageTrouBas &&
		passageDirectPomme <= passageTrouGauche && passageDirectPomme <= passageTrouDroit) // chemin direct vers la pomme sans passer dans un trou
	{
		resultat = CHEMIN_POMME;
	}
	else if (passageTrouHaut <= passageTrouBas && passageTrouHaut <= passageTrouGauche && passageTrouHaut <= passageTrouDroit) // chemin vers la pomme en passant par le trou du haut
	{
		resultat = CHEMIN_HAUT;
	}
	else if (passageTrouBas <= passageTrouGauche && passageTrouBas <= passageTrouDroit) // chemin vers la pomme en passant par le trou du bas
	{
		resultat = CHEMIN_BAS;
	}
	else if (passageTrouGauche <= passageTrouDroit) // chemin vers la pomme en passant par le trou de gauche
	{
		resultat = CHEMIN_GAUCHE;
	}
	else // chemin vers la pomme en passant par le trou de droite
	{
		resultat = CHEMIN_DROITE;
	}

	return resultat;
}

/**
 * @brief Vérifie si le prochain déplacement du serpent dans la direction spécifiée entraîne une collision.
 * @param lesX tableau contenant les positions X du serpent.
 * @param lesY tableau contenant les positions Y du serpent.
 * @param plateau tableau représentant le plateau de jeu.
 * @param directionProchaine direction vers laquelle le serpent va se déplacer.
 * @return true si une collision est détectée, false sinon.
 */
bool verifierCollision(int lesX[], int lesY[], tPlateau plateau, char directionProchaine)
{
	int nouvelleX = lesX[0]; // projeter des coordonnées en X
	int nouvelleY = lesY[0]; // projeter des coordonnées en Y

	// Calcul de la nouvelle position en fonction de la direction donnée
	switch (directionProchaine)
	{
	case HAUT:
		nouvelleY--;
		break;
	case BAS:
		nouvelleY++;
		break;
	case GAUCHE:
		nouvelleX--;
		break;
	case DROITE:
		nouvelleX++;
		break;
	}

	// Vérification des collisions avec les bords du tableau
	if (plateau[nouvelleX][nouvelleY] == BORDURE)
	{
		return true; // Collision avec une bordure
	}

	// Vérification des collisions avec le corps du serpent
	for (int i = 0; i < TAILLE; i++)
	{
		if (lesX[i] == nouvelleX && lesY[i] == nouvelleY)
		{
			return true; // Collision avec le corps du serpent
		}
	}

	return false; // Pas de collision
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
 * @param teleporter de type bool, vérifie si le serpent s'est téléporter
 */
void progresser(int lesX[], int lesY[], char direction, tPlateau plateau, bool *collision, bool *pomme, bool *teleporter)
{
	// efface le dernier élément avant d'actualiser la position de tous les
	// élémentds du serpent avant de le  redessiner et détecte une
	// collision avec une pomme ou avec une bordure
	effacer(lesX[TAILLE - 1], lesY[TAILLE - 1]);

	for (int i = TAILLE - 1; i > 0; i--)
	{
		lesX[i] = lesX[i - 1];
		lesY[i] = lesY[i - 1];
	}
	// faire progresser la tete dans la nouvelle direction
	switch (direction)
	{
	case HAUT:
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

	// Faire des trous dans les bordures
	for (int i = 1; i < TAILLE; i++)
	{
		if (lesX[0] <= 0)
		{
			lesX[0] = LARGEUR_PLATEAU; // faire apparaitre à gauche
			*teleporter = true;		   // quand le serpent traverse le trou
		}
		else if (lesX[0] > LARGEUR_PLATEAU)
		{
			lesX[0] = 1;		// faire apparaitre à droite
			*teleporter = true; // quand le serpent traverse le trou
		}
		else if (lesY[0] <= 0)
		{
			lesY[0] = HAUTEUR_PLATEAU; // faire apparaitre en haut
			*teleporter = true;		   // quand le serpent traverse le trou
		}
		else if (lesY[0] > HAUTEUR_PLATEAU)
		{
			lesY[0] = 1;		// faire apparaitre en bas
			*teleporter = true; // quand le serpent traverse le trou
		}
	}

	*pomme = false;
	// détection d'une "collision" avec une pomme
	if (plateau[lesX[0]][lesY[0]] == POMME)
	{
		*pomme = true;
		// la pomme disparait du plateau
		plateau[lesX[0]][lesY[0]] = VIDE;
	}
	// détection d'une collision avec la bordure
	else if (plateau[lesX[0]][lesY[0]] == BORDURE)
	{
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
void gotoxy(int x, int y)
{
	printf("\033[%d;%df", y, x);
}

/**
 * @brief Fonction qui vérifie si une touche est tapée dans le terminal
 * @return 1 si un caractere est present, 0 si pas de caractere present
 */
int kbhit()
{
	// la fonction retourne :
	// 1 si un caractere est present
	// 0 si pas de caractere présent
	int unCaractere = 0;
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

	if (ch != EOF)
	{
		ungetc(ch, stdin);
		unCaractere = 1;
	}
	return unCaractere;
}

// Fonction pour désactiver l'echo
void disable_echo()
{
	struct termios tty;

	// Obtenir les attributs du terminal
	if (tcgetattr(STDIN_FILENO, &tty) == -1)
	{
		perror("tcgetattr");
		exit(EXIT_FAILURE);
	}

	// Désactiver le flag ECHO
	tty.c_lflag &= ~ECHO;

	// Appliquer les nouvelles configurations
	if (tcsetattr(STDIN_FILENO, TCSANOW, &tty) == -1)
	{
		perror("tcsetattr");
		exit(EXIT_FAILURE);
	}
}

// Fonction pour réactiver l'echo
void enable_echo()
{
	struct termios tty;

	// Obtenir les attributs du terminal
	if (tcgetattr(STDIN_FILENO, &tty) == -1)
	{
		perror("tcgetattr");
		exit(EXIT_FAILURE);
	}

	// Réactiver le flag ECHO
	tty.c_lflag |= ECHO;

	// Appliquer les nouvelles configurations
	if (tcsetattr(STDIN_FILENO, TCSANOW, &tty) == -1)
	{
		perror("tcsetattr");
		exit(EXIT_FAILURE);
	}
}
