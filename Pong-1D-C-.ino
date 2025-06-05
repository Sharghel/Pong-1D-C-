/*
   Code de fonctionnement pour le jeu PONG.
*/

#include <FastLED.h>

//===========================================
// DÉFINITION DES COULEURS
//===========================================
const CRGB NOIR  = CRGB(0,   0,   0  ); // LED éteinte
const CRGB BLANC = CRGB(255, 255, 255); // Couleur Blanc
const CRGB ROUGE = CRGB(255, 0,   0  ); // Couleur Rouge
const CRGB VERT  = CRGB(0,   255, 0  ); // Couleur Vert
const CRGB BLEU  = CRGB(0,   0,   255); // Couleur Bleu
const CRGB JAUNE = CRGB(255, 255, 0  ); // Couleur Jaune
const CRGB ROSE  = CRGB(255, 0,   255); // Couleur Rose
const CRGB CYAN  = CRGB(0,   255, 255); // Couleur Cyan

//===========================================
// SONS
//===========================================
const int NOTE_DO_1 = 33;

//===========================================
// BRANCHEMENTS ÉLECTRONIQUES
//===========================================
const int LED_PIN = 8;          // Numero de branchement de la bande de LEDs
const int BUTTON1_PIN = 3;      // Numero de branchement du premier bouton
const int BUTTON2_PIN = 2;      // Numero de branchement du deuxieme bouton
const int HAUT_PARLEUR_PIN = 6; // Numero de branchement du haut parleur (optionnel)
 
//===========================================
// PARAMETRES GENERAUX DU JEU
//===========================================
const int   NUM_LEDS = 20;      // Nombre de LEDs sur la bande
const float SPEED = 0.5;        // Vitesse de la balle
const float ACCELERATION = 8;   // Accelleration de la balle a chaque tir, si ACCELERATION = 10 on augmente la vitesse de 10 pourcent a chaque tir
const int   HIT_ZONE = 3;       // Nombre de LED pendant lesquelles on peut renvoyer la balle
const int   MAX_SCORE = 3;      // Score nécessaire pour gagner une partie

const CRGB  PLAYER1_COLOR = CYAN;    // Couleur player 1
const CRGB  PLAYER2_COLOR = ROSE;    // Couleur player 2
const CRGB  BALL_COLOR = BLANC;      // Couleur de la balle

// Test pour vérifier que le terrain d'un joueur peut accueillir sa zone et ses points
static_assert((HIT_ZONE + MAX_SCORE) <= (NUM_LEDS / 2), "Erreur: terrain trop petit pour la configuration choisie");

//===========================================
// VARIABLES GLOBALES DU JEU
//===========================================
// LEDs
CRGB leds[NUM_LEDS];

// Players
enum Player
{
  PERSONNE,
  PLAYER1,
  PLAYER2
};

// États possibles du jeu
enum GameState
{
  START,
  GAME
};

// Variables d'état du jeu
GameState gameState = START;
Player player = PLAYER1;      // Prochain joueur a appuyer sur son bouton
Player lastWinner = PERSONNE;

// Variables de la balle
float ballSpeed = SPEED;    // Vitesse de la balle
float ballPosition = 1;     // Position de la balle sur la bande de led (Si ballPosition = 0, la balle est devant le player 1. Si ballPosition = 1, la balle est devant le player 2)

// Score des joueurs
int player1Score = 0;   // Score du player 1
int player2Score = 0;   // Score du player 2

// Variables de chronométrage
unsigned long lastMillis = 0;
unsigned long gameBegin = 0;
unsigned int counter = 0;

//===========================================
// FONCTIONS UTILITAIRES POUR LES LEDS
//===========================================

// Allume une LED à une position donnée avec une couleur
void ledColor(Player player, int pos, CRGB color){
  if (player == PLAYER2){
    leds[NUM_LEDS - pos - 1] = color;
  } else {
    // PLAYER1 ou PERSONNE
    leds[pos] = color;
  }
}

// Allume plusieurs LEDs consécutives d'une certaine couleur
void setZoneColor(int player, CRGB color, int start, int count, int dim = 1) {
  for (int i = 0; i < count; ++i) {
    CRGB c = (dim > 1) ? (color / dim) : color;
    ledColor(player, start + i, c);
  }
}

// Fait clignoter une LED plusieurs fois
void clignotement(Player player, CRGB color, int position, int nombreClignotement = 3) {
  for (int i = 0; i < nombreClignotement; i++)
  {
    // On eteint la LED pendant 0.5s
    delay(500);
    ledColor(player, position, NOIR);
    FastLED.show();

    // On allume la derniere LED pendant 0.5s
    delay(500);
    ledColor(player, position, color);
    FastLED.show();
  }
}

//===========================================
// FONCTIONS DE GESTION DU JEU
//===========================================

// Affiche les scores des deux joueurs sur la bande de LEDs
void showScore()
{
  // On commence par effacer toutes les couleurs de led
  FastLED.clear();

  // On allume le nombre de led correspondant au score du player 1
  setZoneColor(PLAYER1, PLAYER1_COLOR, NUM_LEDS / 2 - player1Score, player1Score);

  // On allume le nombre de led correspondant au score du player 2
  setZoneColor(PLAYER2, PLAYER2_COLOR, NUM_LEDS / 2 - player2Score, player2Score);

  // On envoie les nouvelles couleurs a la bande de led
  FastLED.show();

  // On fait clignotter trois fois
  if (lastWinner == PLAYER1)
  {
    clignotement(PLAYER1, PLAYER1_COLOR, NUM_LEDS / 2 - player1Score);
  }
  else // lastWinner == PLAYER2
  {
    clignotement(PLAYER2, PLAYER2_COLOR, NUM_LEDS / 2 - player2Score);
  }

  // Si la partie est terminÃ©e on va a l'affichage de fin
  if (player1Score == MAX_SCORE || player2Score == MAX_SCORE)
  {
    gameState = START;

    // On reinitialise les scores
    player1Score = 0;
    player2Score = 0;

    // On reinitialise la vitesse
    ballSpeed = SPEED;

    // On reinitialise les leds
    FastLED.clear();
  }
  // Sinon on reprend le jeu
  else
  {
    gameState = GAME;
    ballSpeed = SPEED;
  }
}

// Gère le tour d'un joueur : vérification du bouton, mouvement de la balle, détection des points
bool gererTour(Player joueurActuel, int pinBouton, int ballLed, unsigned long currentMillis) {
  // Vérifier si le joueur a appuyé sur son bouton (avec délai de début)
  if (digitalRead(pinBouton) == LOW && currentMillis - gameBegin > 500) {
    
    bool balleHorsZoneDeTir;
    Player joueurAdverse;
    float nouvellePositionBalle;
    
    // Configurer les paramètres selon le joueur actuel
    if (joueurActuel == PLAYER1) {
      balleHorsZoneDeTir = (ballLed >= HIT_ZONE);
      joueurAdverse = PLAYER2;
      nouvellePositionBalle = 0.0f;
    } else { // PLAYER2
      balleHorsZoneDeTir = (ballLed < NUM_LEDS - HIT_ZONE);
      joueurAdverse = PLAYER1;
      nouvellePositionBalle = 1.0f;
    }
    
    // Si la balle est hors de la zone de tir
    if (balleHorsZoneDeTir) {
      // L'adversaire marque un point
      if (joueurAdverse == PLAYER1) {
        player1Score += 1;
      } else {
        player2Score += 1;
      }
      
      lastWinner = joueurAdverse;
      ballPosition = nouvellePositionBalle;
      showScore();
      player = joueurAdverse;
      lastMillis = millis();
      
      return true; // POINT MARQUÉ - on arrête tout !
    } 
    // Sinon, tir réussi
    else {
      // Accélérer la balle
      ballSpeed *= 1.0 + ACCELERATION / 100.0;
      
      // Changer de joueur
      player = joueurAdverse;
      
      // Jouer le son
      tone(HAUT_PARLEUR_PIN, NOTE_DO_1, 300);
    }
    
    return false; // Pas de point marqué lors du tir
  }
  
  // Faire avancer la balle
  float vitesseBalle = (joueurActuel == PLAYER1) ? -ballSpeed : ballSpeed;
  ballPosition += vitesseBalle * (currentMillis - lastMillis) * 0.001f;
  
  // Vérifier si la balle est sortie de la zone
  bool balleSortie = (joueurActuel == PLAYER1) ? (ballPosition < 0.0f) : (ballPosition >= 1.0f);
  
  if (balleSortie) {
    // L'adversaire marque un point
    Player joueurAdverse = (joueurActuel == PLAYER1) ? PLAYER2 : PLAYER1;
    
    if (joueurAdverse == PLAYER1) {
      player1Score += 1;
    } else {
      player2Score += 1;
    }
    
    lastWinner = joueurAdverse;
    ballPosition = (joueurActuel == PLAYER1) ? 0.0f : 1.0f;
    showScore();
    player = joueurAdverse;
    lastMillis = millis();
    
    return true; // POINT MARQUÉ - on arrête tout !
  }
  
  return false; // Pas de point marqué
}

/****************************************************************
   Cette fonction s'execute une fois lorsque la carte s'allume.
 ****************************************************************/
void setup() {
  Serial.begin(115200);
  // Initialisation des LEDs
  FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);

  // Initialisations des boutons
  pinMode(BUTTON1_PIN, INPUT_PULLUP);
  pinMode(BUTTON2_PIN, INPUT_PULLUP);

  // Initialisation du haut parleur
  pinMode(HAUT_PARLEUR_PIN, OUTPUT);

  // COULEUR DES LEDS EN DEBUT DE PARTIE
  FastLED.clear();

  // Couleurs player 1
  setZoneColor(PLAYER1, PLAYER1_COLOR, 0, HIT_ZONE);

  // Couleurs player 2
  setZoneColor(PLAYER2, PLAYER2_COLOR, 0, HIT_ZONE);

  // On envoie les changements a la bande de leds
  FastLED.show();
}

/*************************************************************
   Cette fonction s'execute en continue tout au long du jeu.
 *************************************************************/
void loop() {

  switch (gameState)
  {
    case START:
      // Si un player a gagne, on affiche l'arc en ciel du cote du vainqueur
      if (lastWinner == PLAYER1)
      {
        fill_rainbow(leds, NUM_LEDS / 2, counter++, 7);
        FastLED.show();
      }
      else if (lastWinner == PLAYER2)
      {
        fill_rainbow(leds + NUM_LEDS / 2, NUM_LEDS / 2, counter++, 7);
        FastLED.show();
      }

      // On regarde si un des boutons est appuye
      if (digitalRead(BUTTON1_PIN) == LOW || digitalRead(BUTTON2_PIN) == LOW)
      {
        // La partie commence
        gameState = GAME;

        // Initialisation de la varable lastMillis
        lastMillis = millis();

        // Initialisation du temps de début de jeu
        gameBegin = millis();
      }
      break;

    case GAME:
      // Calcul du temps ecoule entre deux boucles
      unsigned long currentMillis = millis();

      // On calcule le numero de la LED allumee
      int ballLed = int(ballPosition * NUM_LEDS);

      // On s'assure que la position de la balle ne dépasse pas la taille de la bande de LED
      ballLed = min(ballLed, NUM_LEDS - 1);

      // Gérer le tour - SI un point est marqué, on arrête tout
      bool pointMarque = false;
      if (player == PLAYER1) {
        pointMarque = gererTour(PLAYER1, BUTTON1_PIN, ballLed, currentMillis);
      } else {
        pointMarque = gererTour(PLAYER2, BUTTON2_PIN, ballLed, currentMillis);
      }
      
      // Si un point a été marqué, on arrête le traitement pour ce tour
      if (pointMarque) {
        break; // Sortir du case GAME
      }

      ///// AFFICHAGE BANDE DE LEDs /////
      // Premierement on efface toutes les couleurs precedentes
      FastLED.clear();

      // Ensuite on allume faiblement les LEDs correspondant a la zone de chaque cote
      setZoneColor(PLAYER1, PLAYER1_COLOR, 0, HIT_ZONE, 10);
      setZoneColor(PLAYER2, PLAYER2_COLOR, 0, HIT_ZONE, 10);

      // Ensuite on allume faiblement les LEDs correspondant aux scores
      // Pour le player 1
      setZoneColor(PLAYER1, PLAYER1_COLOR, NUM_LEDS / 2 - player1Score, player1Score, 15);
      
      // Pour le player 2
      setZoneColor(PLAYER2, PLAYER2_COLOR, NUM_LEDS / 2 - player2Score, player2Score, 15);

      // Ensuite on actualise la position de la balle
      // On donne la couleur de la led en fonction de si la balle est dans la zone d'un player ou non

      // Si la balle est dans le camp d'un des player, elle est rouge.
      if (ballLed < HIT_ZONE || ballLed >= NUM_LEDS - HIT_ZONE)
      {
        ledColor(PERSONNE, ballLed, ROUGE);
      }
      // Si elle en est proche, elle est jaune
      else if (ballLed < 2 * HIT_ZONE || ballLed >= NUM_LEDS - 2 * HIT_ZONE)
      {
        ledColor(PERSONNE, ballLed, JAUNE);
      }
      // Sinon la balle a sa couleur par defaut
      else
      {
        ledColor(PERSONNE, ballLed, BALL_COLOR);
      }

      // On envoie la couleur des leds a la bande de leds
      FastLED.show();

      // On actualise la variable lastMillis pour la boucle suivante
      lastMillis = currentMillis;
      break;
  }
}