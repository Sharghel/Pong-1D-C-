/*
   Code de fonctionnement pour le jeu PONG.
*/

#include <FastLED.h>

// Definition des couleurs
const CRGB NOIR  = CRGB(0,   0,   0  );
const CRGB BLANC = CRGB(255, 255, 255);
const CRGB ROUGE = CRGB(255, 0,   0  );
const CRGB VERT  = CRGB(0,   255, 0  );
const CRGB BLEU  = CRGB(0,   0,   255);
const CRGB JAUNE = CRGB(255, 255, 0  );
const CRGB ROSE  = CRGB(255, 0,   255);
const CRGB CYAN  = CRGB(0,   255, 255);

// Definition des notes de musique
const int NOTE_DO_1 = 33;


/******************************
   PARAMETRES DE BRANCHEMENTS
 ******************************/

const int LED_PIN = 8;          // Numero de branchement de la bande de LEDs
const int BUTTON1_PIN = 3;      // Numero de branchement du premier bouton
const int BUTTON2_PIN = 2;      // Numero de branchement du deuxieme bouton
const int HAUT_PARLEUR_PIN = 6; // Numero de branchement du haut parleur (optionnel)


/******************************
   PARAMETRES GENERAUX DU JEU
 ******************************/
const int   NUM_LEDS = 20;      // Nombre de LEDs sur la bande
const float SPEED = 0.5;        // Vitesse de la balle
const float ACCELERATION = 8;  // Accelleration de la balle a chaque tir, si ACCELERATION = 10 on augmente la vitesse de 10 pourcent a chaque tir
const int   HIT_ZONE = 5;       // Nombre de LED pendant lesquelles on peut renvoyer la balle
const int   MAX_SCORE = 3;

const CRGB  PLAYER1_COLOR = CYAN;    // Couleur player 1
const CRGB  PLAYER2_COLOR = ROSE;    // Couleur player 2
const CRGB  BALL_COLOR = BLANC;      // Couleur de la balle


// LEDs
CRGB leds[NUM_LEDS];

// Players
enum Player
{
  PERSONNE,
  PLAYER1,
  PLAYER2
};

// Etats du jeu
enum GameState
{
  START,
  GAME
};
GameState gameState = START;


// Fonctions de fonctionnement du jeu

// Fonction permettant de changer la couleur d'une LED relativement au player 1 ou 2
void ledColor(Player player, int pos, CRGB color)
{
  if (player == PLAYER1)
  {
    leds[pos] = color;
  }
  else // player == PLAYER2
  {
    leds[NUM_LEDS - pos - 1] = color;
  }
}

// Fonction permettant de changer la couleur d'une LED
void ledColor(int pos, CRGB color)
{
  leds[pos] = color;
}

// Variables de fonctionnement du jeu
Player player = PLAYER1; // Prochain joueur a appuyer sur son bouton
float ballSpeed = SPEED;  // Vitesse de la balle
float ballPosition = 1; // Position de la balle sur la bande de led (Si ballPosition = 0, la balle est devant le player 1. Si ballPosition = 1, la balle est devant le player 2)
int player1Score = 0;   // Score du player 1
int player2Score = 0;   // Score du player 2
Player lastWinner = PERSONNE;

unsigned long lastMillis = 0;
unsigned long gameBegin = 0;
unsigned int counter = 0;


// Fonction servant Ã  afficher les scores
void showScore()
{
  // On commence par effacer toutes les couleurs de led
  FastLED.clear();

  // On allume le nombre de led correspondant au score du player 1
  for (int i = 0; i < player1Score; i++)
  {
    ledColor(PLAYER1, NUM_LEDS / 2 - (i + 1), PLAYER1_COLOR);
  }

  // On allume le nombre de led correspondant au score du player 2
  for (int i = 0; i < player2Score; i++)
  {
    ledColor(PLAYER2, NUM_LEDS / 2 - (i + 1), PLAYER2_COLOR);
  }

  // On envoie les nouvelles couleurs a la bande de led
  FastLED.show();

  // On fait clignotter trois fois
  if (lastWinner == PLAYER1)
  {
    for (int i = 0; i < 3; i++)
    {
      // On eteint la derniere LED pendant 0.5s
      delay(500);
      ledColor(PLAYER1, NUM_LEDS / 2 - player1Score, NOIR);
      FastLED.show();

      // On allume la derniere LED pendant 0.5s
      delay(500);
      ledColor(PLAYER1, NUM_LEDS / 2 - player1Score, PLAYER1_COLOR);
      FastLED.show();
    }
  }
  else // lastWinner == PLAYER2
  {
    for (int i = 0; i < 3; i++)
    {
      // On eteint la derniere LED pendant 0.5s
      delay(500);
      ledColor(PLAYER2, NUM_LEDS / 2 - player2Score, NOIR);
      FastLED.show();

      // On allume la derniere LED pendant 0.5s
      delay(500);
      ledColor(PLAYER2, NUM_LEDS / 2 - player2Score, PLAYER2_COLOR);
      FastLED.show();
    }
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
  ledColor(PLAYER1, 0, PLAYER1_COLOR);
  ledColor(PLAYER1, 1, PLAYER1_COLOR);
  ledColor(PLAYER1, 2, PLAYER1_COLOR);
  ledColor(PLAYER1, 3, PLAYER1_COLOR);
  ledColor(PLAYER1, 4, PLAYER1_COLOR);

  // Couleurs player 2
  ledColor(PLAYER2, 0, PLAYER2_COLOR);
  ledColor(PLAYER2, 1, PLAYER2_COLOR);
  ledColor(PLAYER2, 2, PLAYER2_COLOR);
  ledColor(PLAYER2, 3, PLAYER2_COLOR);
  ledColor(PLAYER2, 4, PLAYER2_COLOR);

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

        // Initialisation du temps de dÃ©but de jeu
        gameBegin = millis();
      }
      break;

    case GAME:
      // Calcul du temps ecoule entre deux boucles
      unsigned long currentMillis = millis();

      // On calcule le numero de la LED allumee
      int ballLed = int(ballPosition * NUM_LEDS);

      // On s'assure que la position de la balle ne dÃ©passe pas la taille de la bande de LED
      ballLed = min(ballLed, NUM_LEDS - 1);

      // On regarde qui est en train de jouer
      if (player == PLAYER1)
      {
        // On regarde si le player a appuye sur son bouton et si le dÃ©lai de dÃ©but de jeu est passÃ©
        if (digitalRead(BUTTON1_PIN) == LOW && currentMillis - gameBegin > 500)
        {
          // Si la balle est hors de la zone de tir, l'autre player marque un point
          if (ballLed >= HIT_ZONE)
          {
            player2Score += 1;
            lastWinner = PLAYER2;
            ballPosition = 0;

            // On passe en mode affichage des scores
            showScore();

            // C'est a l'autre player de jouer
            player = PLAYER2;

            // Actualisation de la variable lastMillis
            lastMillis = millis();
          }
          else
          {
            // On accelere la balle
            ballSpeed *= 1.0 + ACCELERATION / 100;

            // C'est a l'autre player de jouer
            player = PLAYER2;

            // On joue la note de musique
            tone(HAUT_PARLEUR_PIN, NOTE_DO_1, 300);
          }

          break;
        }

        // On fait avancer la balle
        ballPosition -= ballSpeed * (currentMillis - lastMillis) * 0.001f;

        // On regarde si la balle est sortie de la zone
        if (ballPosition < 0.0f)
        {
          // Si oui le player 2 marque un point
          player2Score += 1;
          lastWinner = PLAYER2;
          ballPosition = 0;

          // On passe en mode affichage des scores
          showScore();

          // C'est a l'autre player de jouer
          player = PLAYER2;

          // Actualisation de la variable lastMillis
          lastMillis = millis();
          break;
        }
      }
      else // player == PLAYER2
      {
        // On regarde si le player a appuye sur son bouton et si le dÃ©lai de dÃ©but de jeu est passÃ©
        if (digitalRead(BUTTON2_PIN) == LOW && currentMillis - gameBegin > 500)
        {
          // Si la balle est hors de la zone de tir, l'autre player marque un point
          if (ballLed < NUM_LEDS - HIT_ZONE)
          {
            player1Score += 1;
            lastWinner = PLAYER1;
            ballPosition = 1;

            // On passe en mode affichage des scores
            showScore();

            // C'est a l'autre player de jouer
            player = PLAYER1;

            // Actualisation de la variable lastMillis
            lastMillis = millis();
          }
          else
          {
            // On accelere la balle
            ballSpeed *= 1.1;

            // C'est a l'autre player de jouer
            player = PLAYER1;
          }

          break;
        }

        // On fait avancer la balle dans l'autre sens
        ballPosition += ballSpeed * (currentMillis - lastMillis) * 0.001f;

        // On regarde si la balle est sortie de la zone
        if (ballPosition >= 1)
        {
          // Si oui le player 1 marque un point
          player1Score += 1;
          lastWinner = PLAYER1;
          ballPosition = 1;

          // On passe en mode affichage des scores
          showScore();
          // C'est a l'autre player de jouer
          player = PLAYER1;

          // Actualisation de la variable lastMillis
          lastMillis = millis();
          break;
        }
      }

      ///// AFFICHAGE BANDE DE LEDs /////
      // Premierement on efface toutes les couleurs precedentes
      FastLED.clear();

      // Ensuite on allume faiblement les LEDs correspondant a la zone de chaque cote
      for (int i = 0; i < HIT_ZONE; i++)
      {
        // On allume de chaque cote
        ledColor(PLAYER1, i, PLAYER1_COLOR / 10);  // On divise la couleur par 10 pour la rendre 10 fois moins puissante
        ledColor(PLAYER2, i, PLAYER2_COLOR / 10);
      }

      // Ensuite on allume faiblement les LEDs correspondant aux scores
      // Pour le player 1
      for (int i = 0; i < player1Score; i++)
      {
        ledColor(PLAYER1, NUM_LEDS / 2 - (i + 1), PLAYER1_COLOR / 15);
      }
      // Pour le player 2
      for (int i = 0; i < player2Score; i++)
      {
        ledColor(PLAYER2, NUM_LEDS / 2 - (i + 1), PLAYER2_COLOR / 15);
      }

      // Ensuite on actualise la position de la balle
      // On donne la couleur de la led en fonction de si la balle est dans la zone d'un player ou non

      // Si la balle est dans le camp d'un des player, elle est rouge.
      if (ballLed < HIT_ZONE || ballLed >= NUM_LEDS - HIT_ZONE)
      {
        ledColor(ballLed, ROUGE);
      }
      // Si elle en est proche, elle est jaune
      else if (ballLed < 2 * HIT_ZONE || ballLed >= NUM_LEDS - 2 * HIT_ZONE)
      {
        ledColor(ballLed, JAUNE);
      }
      // Sinon la balle a sa couleur par defaut
      else
      {
        ledColor(ballLed, BALL_COLOR);
      }

      // On envoie la couleur des leds a la bande de leds
      FastLED.show();

      // On actualise la variable lastMillis pour la boucle suivante
      lastMillis = currentMillis;
      break;
  }
}