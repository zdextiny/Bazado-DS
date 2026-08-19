// Puerto 1:1 de js/game.js — motor de reglas puro (sin dibujar nada).
// Sin online ni anotador: solo lo necesario para jugar una partida
// completa contra bots en la propia consola.
#ifndef BAZAS_GAME_H
#define BAZAS_GAME_H

#include "deck.h"

#define MAX_PLAYERS 4
#define MAX_HAND_SIZE 6
#define MIN_HAND_SIZE 1
#define GAME_MAX_PENALTIES 6
#define PLAYER_NAME_LEN 16

typedef enum { DIFF_EASY, DIFF_MEDIUM, DIFF_HARD } Difficulty;

typedef struct {
  int id;
  char name[PLAYER_NAME_LEN];
  int isAI;
  Difficulty difficulty;
  Card hand[MAX_HAND_SIZE];
  int handCount;
  int prediction; // -1 = todavia no predijo esta mano
  int tricksWon;
  int penalties;
  int eliminated;
} Player;

typedef enum {
  PHASE_PREDICTING,
  PHASE_PLAYING,
  PHASE_TRICK_END,
  PHASE_HAND_END,
  PHASE_GAME_END,
} Phase;

typedef struct {
  int playerId;
  Card card;
} TrickEntry;

typedef struct {
  Player players[MAX_PLAYERS];
  int playerCount;

  int dealerId;
  int handSize;
  int cycleDirection; // -1 o 1
  int handNumber;
  Phase phase;

  int predictionOrder[MAX_PLAYERS];
  int turnPointer;
  int predictionsSum;

  int trickLeaderId;
  int trickPlayOrder[MAX_PLAYERS];
  TrickEntry trickCardsPlayed[MAX_PLAYERS];
  int trickCardsPlayedCount;

  TrickEntry lastTrick[MAX_PLAYERS];
  int lastTrickCount;
  int lastTrickWinner;

  // Todas las cartas jugadas esta mano (no solo la baza actual/anterior
  // como trickCardsPlayed/lastTrick) -- la usa la IA dificil para saber
  // que cartas quedan sin repartir entre los rivales al simular
  // (Monte Carlo, ver ai.c). Maximo teorico: todos los jugadores
  // juegan toda su mano.
  Card cardsPlayedThisHand[MAX_PLAYERS * MAX_HAND_SIZE];
  int cardsPlayedThisHandCount;

  int winner; // -1 = sin ganador todavia
} GameState;

void game_create_match(GameState* state, const char* names[], const int isAI[], const Difficulty diffs[], int playerCount);

int game_active_player_count(const GameState* state);
int game_get_current_predictor_id(const GameState* state);
int game_is_last_predictor(const GameState* state);

// Devuelve 1 y llena *outForbidden si hay un numero prohibido para el
// predictor actual (el ultimo en predecir esta mano); 0 si no aplica.
int game_get_forbidden_prediction(const GameState* state, int* outForbidden);

// Devuelve 1 si se aplico, 0 si la jugada no es valida.
int game_submit_prediction(GameState* state, int playerId, int value);

int game_get_current_player_to_act_id(const GameState* state); // -1 = sin turno activo

// Devuelve 1 si se aplico, 0 si la jugada no es valida.
int game_play_card(GameState* state, int playerId, Card card);

void game_continue_after_trick(GameState* state);
void game_start_next_hand(GameState* state);

#endif
