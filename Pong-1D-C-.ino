/*
   Code de fonctionnement pour le jeu PONG - Version POO
*/

#include <FastLED.h>

//===========================================
// DÉFINITION DES COULEURS
//===========================================
const CRGB NOIR  = CRGB(0,   0,   0  );
const CRGB BLANC = CRGB(255, 255, 255);
const CRGB ROUGE = CRGB(255, 0,   0  );
const CRGB VERT  = CRGB(0,   255, 0  );
const CRGB BLEU  = CRGB(0,   0,   255);
const CRGB JAUNE = CRGB(255, 255, 0  );
const CRGB ROSE  = CRGB(255, 0,   255);
const CRGB CYAN  = CRGB(0,   255, 255);

//===========================================
// SONS
//===========================================
const int NOTE_DO_1 = 33;

//===========================================
// BRANCHEMENTS ÉLECTRONIQUES
//===========================================
const int LED_PIN = 8;
const int BUTTON1_PIN = 3;
const int BUTTON2_PIN = 2;
const int HAUT_PARLEUR_PIN = 6;

//===========================================
// PARAMETRES GENERAUX DU JEU
//===========================================
const int   NUM_LEDS = 20;
const float SPEED = 0.5;
const float ACCELERATION = 8;
const int   HIT_ZONE = 3;
const int   MAX_SCORE = 3;

const CRGB  PLAYER1_COLOR = CYAN;
const CRGB  PLAYER2_COLOR = ROSE;
const CRGB  BALL_COLOR = BLANC;

static_assert((HIT_ZONE + MAX_SCORE) <= (NUM_LEDS / 2), "Erreur: terrain trop petit pour la configuration choisie");

//===========================================
// ÉNUMÉRATIONS
//===========================================
enum Player {
  PERSONNE,
  PLAYER1,
  PLAYER2
};

enum GameState {
  START,
  GAME
};

//===========================================
// CLASSE LEDMANAGER - Gestion des LEDs
//===========================================
class LedManager {
private:
  CRGB* leds;
  int numLeds;

public:
  LedManager(CRGB* ledArray, int count) : leds(ledArray), numLeds(count) {}

  void ledColor(Player player, int pos, CRGB color) {
    if (player == PLAYER2) {
      leds[numLeds - pos - 1] = color;
    } else {
      leds[pos] = color;
    }
  }

  void setZoneColor(Player player, CRGB color, int start, int count, int dim = 1) {
    for (int i = 0; i < count; ++i) {
      CRGB c = (dim > 1) ? (color / dim) : color;
      ledColor(player, start + i, c);
    }
  }

  void clear() {
    FastLED.clear();
  }

  void show() {
    FastLED.show();
  }

  void clignotement(Player player, CRGB color, int position, int nombreClignotement = 3) {
    for (int i = 0; i < nombreClignotement; i++) {
      delay(500);
      ledColor(player, position, NOIR);
      show();
      delay(500);
      ledColor(player, position, color);
      show();
    }
  }

  void fillRainbow(int startLed, int count, int hue) {
    fill_rainbow(leds + startLed, count, hue, 7);
  }
};

//===========================================
// CLASSE BALL - Gestion de la balle
//===========================================
class Ball {
private:
  float position;
  float speed;
  float baseSpeed;

public:
  Ball(float initialSpeed) : position(1.0f), speed(initialSpeed), baseSpeed(initialSpeed) {}

  float getPosition() const { return position; }
  float getSpeed() const { return speed; }
  
  void setPosition(float pos) { position = pos; }
  void resetSpeed() { speed = baseSpeed; }
  
  void accelerate() {
    speed *= 1.0 + ACCELERATION / 100.0;
  }

  void move(Player currentPlayer, float deltaTime) {
    float vitesse = (currentPlayer == PLAYER1) ? -speed : speed;
    position += vitesse * deltaTime * 0.001f;
  }

  int getLedPosition() const {
    int ballLed = int(position * NUM_LEDS);
    return min(ballLed, NUM_LEDS - 1);
  }

  bool isOutOfBounds(Player currentPlayer) const {
    return (currentPlayer == PLAYER1) ? (position < 0.0f) : (position >= 1.0f);
  }

  bool isInHitZone(Player currentPlayer) const {
    int ballLed = getLedPosition();
    if (currentPlayer == PLAYER1) {
      return ballLed < HIT_ZONE;
    } else {
      return ballLed >= NUM_LEDS - HIT_ZONE;
    }
  }
};

//===========================================
// CLASSE JOUEUR - Gestion d'un joueur
//===========================================
class Joueur {
private:
  Player playerId;
  int buttonPin;
  int score;
  CRGB color;

public:
  Joueur(Player id, int pin, CRGB playerColor) : 
    playerId(id), buttonPin(pin), score(0), color(playerColor) {
    pinMode(buttonPin, INPUT_PULLUP);
  }

  Player getId() const { return playerId; }
  int getScore() const { return score; }
  CRGB getColor() const { return color; }

  void addPoint() { score++; }
  void resetScore() { score = 0; }

  bool isButtonPressed() const {
    return digitalRead(buttonPin) == LOW;
  }

  bool hasWon() const {
    return score >= MAX_SCORE;
  }
};

//===========================================
// CLASSE PRINCIPALE PONGGAME
//===========================================
class PongGame {
private:
  LedManager ledManager;
  Ball ball;
  Joueur player1;
  Joueur player2;

  GameState gameState;
  Player currentPlayer;
  Player lastWinner;
  
  unsigned long lastMillis;
  unsigned long gameBegin;
  unsigned int counter;

public:
  PongGame(CRGB* leds) : 
    ledManager(leds, NUM_LEDS),
    ball(SPEED),
    player1(PLAYER1, BUTTON1_PIN, PLAYER1_COLOR),
    player2(PLAYER2, BUTTON2_PIN, PLAYER2_COLOR),
    gameState(START),
    currentPlayer(PLAYER1),
    lastWinner(PERSONNE),
    lastMillis(0),
    gameBegin(0),
    counter(0) {
    
    // Initialisation du haut parleur
    pinMode(HAUT_PARLEUR_PIN, OUTPUT);
  }

  void initialize() {
    // Initialisation de FastLED avec le tableau de LEDs global
    extern CRGB leds[NUM_LEDS];
    FastLED.addLeds<WS2812B, LED_PIN, GRB>(leds, NUM_LEDS);
    
    ledManager.clear();
    ledManager.setZoneColor(PLAYER1, player1.getColor(), 0, HIT_ZONE);
    ledManager.setZoneColor(PLAYER2, player2.getColor(), 0, HIT_ZONE);
    ledManager.show();
  }

  void playHitSound() {
    tone(HAUT_PARLEUR_PIN, NOTE_DO_1, 300);
  }

  bool isAnyButtonPressed() const {
    return player1.isButtonPressed() || player2.isButtonPressed();
  }

  Joueur* getCurrentJoueur() {
    return (currentPlayer == PLAYER1) ? &player1 : &player2;
  }

  Joueur* getOpponent(Player player) {
    return (player == PLAYER1) ? &player2 : &player1;
  }

  void showScore() {
    ledManager.clear();
    
    // Affichage des scores
    ledManager.setZoneColor(PLAYER1, player1.getColor(), NUM_LEDS / 2 - player1.getScore(), player1.getScore());
    ledManager.setZoneColor(PLAYER2, player2.getColor(), NUM_LEDS / 2 - player2.getScore(), player2.getScore());
    
    ledManager.show();

    // Clignotement du dernier gagnant
    if (lastWinner == PLAYER1) {
      ledManager.clignotement(PLAYER1, player1.getColor(), NUM_LEDS / 2 - player1.getScore());
    } else if (lastWinner == PLAYER2) {
      ledManager.clignotement(PLAYER2, player2.getColor(), NUM_LEDS / 2 - player2.getScore());
    }

    // Vérifier fin de partie
    if (player1.hasWon() || player2.hasWon()) {
      gameState = START;
      player1.resetScore();
      player2.resetScore();
      ball.resetSpeed();
      ledManager.clear();
    } else {
      gameState = GAME;
      ball.resetSpeed();
    }
  }

  bool handlePlayerTurn(Joueur* joueur, unsigned long currentMillis) {
    if (joueur->isButtonPressed() && currentMillis - gameBegin > 500) {
      
      if (!ball.isInHitZone(joueur->getId())) {
        // Point pour l'adversaire
        Joueur* opponent = getOpponent(joueur->getId());
        opponent->addPoint();
        lastWinner = opponent->getId();
        ball.setPosition((joueur->getId() == PLAYER1) ? 0.0f : 1.0f);
        showScore();
        currentPlayer = opponent->getId();
        lastMillis = millis();
        return true; // Point marqué
      } else {
        // Tir réussi
        ball.accelerate();
        currentPlayer = (joueur->getId() == PLAYER1) ? PLAYER2 : PLAYER1;
        playHitSound();
      }
      return false;
    }

    // Faire avancer la balle
    ball.move(joueur->getId(), currentMillis - lastMillis);

    // Vérifier si la balle sort
    if (ball.isOutOfBounds(joueur->getId())) {
      Joueur* opponent = getOpponent(joueur->getId());
      opponent->addPoint();
      lastWinner = opponent->getId();
      ball.setPosition((joueur->getId() == PLAYER1) ? 0.0f : 1.0f);
      showScore();
      currentPlayer = opponent->getId();
      lastMillis = millis();
      return true; // Point marqué
    }

    return false;
  }

  void renderGame() {
    ledManager.clear();

    // Zones de tir faibles
    ledManager.setZoneColor(PLAYER1, player1.getColor(), 0, HIT_ZONE, 10);
    ledManager.setZoneColor(PLAYER2, player2.getColor(), 0, HIT_ZONE, 10);

    // Scores faibles
    ledManager.setZoneColor(PLAYER1, player1.getColor(), NUM_LEDS / 2 - player1.getScore(), player1.getScore(), 15);
    ledManager.setZoneColor(PLAYER2, player2.getColor(), NUM_LEDS / 2 - player2.getScore(), player2.getScore(), 15);

    // Balle avec couleur selon position
    int ballLed = ball.getLedPosition();
    CRGB ballColor;
    
    if (ballLed < HIT_ZONE || ballLed >= NUM_LEDS - HIT_ZONE) {
      ballColor = ROUGE;
    } else if (ballLed < 2 * HIT_ZONE || ballLed >= NUM_LEDS - 2 * HIT_ZONE) {
      ballColor = JAUNE;
    } else {
      ballColor = BALL_COLOR;
    }
    
    ledManager.ledColor(PERSONNE, ballLed, ballColor);
    ledManager.show();
  }

  void update() {
    switch (gameState) {
      case START:
        // Arc-en-ciel pour le gagnant
        if (lastWinner == PLAYER1) {
          ledManager.fillRainbow(0, NUM_LEDS / 2, counter++);
        } else if (lastWinner == PLAYER2) {
          ledManager.fillRainbow(NUM_LEDS / 2, NUM_LEDS / 2, counter++);
        }
        ledManager.show();

        // Démarrage du jeu
        if (isAnyButtonPressed()) {
          gameState = GAME;
          lastMillis = millis();
          gameBegin = millis();
        }
        break;

      case GAME:
        unsigned long currentMillis = millis();
        
        // Gestion du tour du joueur actuel
        Joueur* joueurActuel = getCurrentJoueur();
        bool pointMarque = handlePlayerTurn(joueurActuel, currentMillis);

        // Affichage seulement si pas de point marqué
        if (!pointMarque) {
          renderGame();
          lastMillis = currentMillis;
        }
        break;
    }
  }
};

//===========================================
// VARIABLES GLOBALES
//===========================================
CRGB leds[NUM_LEDS];
PongGame* game;

//===========================================
// FONCTIONS PRINCIPALES
//===========================================
void setup() {
  Serial.begin(115200);
  game = new PongGame(leds);
  game->initialize();
}

void loop() {
  game->update();
}