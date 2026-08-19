#include "game.h"
#include <string.h>
#include <stdlib.h>

static int next_active_seat(const GameState* state, int fromId) {
  int n = state->playerCount;
  for (int step = 1; step <= n; step++) {
    int idx = (fromId + step) % n;
    if (!state->players[idx].eliminated) return idx;
  }
  return -1;
}

static void build_active_order_from(const GameState* state, int startId, int outOrder[MAX_PLAYERS], int* outCount) {
  int activeCount = game_active_player_count(state);
  outOrder[0] = startId;
  int cur = startId;
  for (int i = 1; i < activeCount; i++) {
    cur = next_active_seat(state, cur);
    outOrder[i] = cur;
  }
  *outCount = activeCount;
}

int game_active_player_count(const GameState* state) {
  int count = 0;
  for (int i = 0; i < state->playerCount; i++) {
    if (!state->players[i].eliminated) count++;
  }
  return count;
}

// La mano se muestra ordenada de menor a mayor (el As de espada, rank
// 16, siempre queda ultimo/mas fuerte; el As de basto, rank 15, justo
// antes). No afecta a los bots (deciden por valor, no por posicion), es
// puramente para que la mano del humano se lea bien en pantalla.
static void sort_hand_by_rank(Card* hand, int count) {
  for (int i = 1; i < count; i++) {
    Card key = hand[i];
    int keyRank = card_rank(key);
    int j = i - 1;
    while (j >= 0 && card_rank(hand[j]) > keyRank) {
      hand[j + 1] = hand[j];
      j--;
    }
    hand[j + 1] = key;
  }
}

// Encuentra al primer jugador no eliminado (para "quien gana si nadie mas
// sigue en pie" y casos similares). Devuelve -1 si no queda ninguno.
static int first_active_player(const GameState* state) {
  for (int i = 0; i < state->playerCount; i++) {
    if (!state->players[i].eliminated) return i;
  }
  return -1;
}

static void start_hand(GameState* state) {
  int active = game_active_player_count(state);
  if (active <= 1) {
    state->phase = PHASE_GAME_END;
    state->winner = first_active_player(state);
    return;
  }

  int leaderId = next_active_seat(state, state->dealerId);
  int order[MAX_PLAYERS];
  int orderCount;
  build_active_order_from(state, leaderId, order, &orderCount);

  Card deck[DECK_SIZE];
  deck_create(deck);
  deck_shuffle(deck);

  int cursor = 0;
  for (int i = 0; i < orderCount; i++) {
    int pid = order[i];
    Player* p = &state->players[pid];
    for (int c = 0; c < state->handSize; c++) {
      p->hand[c] = deck[cursor++];
    }
    p->handCount = state->handSize;
    sort_hand_by_rank(p->hand, p->handCount);
    p->prediction = -1;
    p->tricksWon = 0;
  }

  state->handNumber += 1;
  memcpy(state->predictionOrder, order, sizeof(int) * orderCount);
  state->trickLeaderId = leaderId;
  state->turnPointer = 0;
  state->predictionsSum = 0;
  state->trickCardsPlayedCount = 0;
  state->lastTrickCount = 0;
  state->lastTrickWinner = -1;
  state->cardsPlayedThisHandCount = 0;
  state->phase = PHASE_PREDICTING;
}

void game_create_match(GameState* state, const char* names[], const int isAI[], const Difficulty diffs[], int playerCount) {
  memset(state, 0, sizeof(GameState));
  state->playerCount = playerCount;
  for (int i = 0; i < playerCount; i++) {
    Player* p = &state->players[i];
    p->id = i;
    strncpy(p->name, names[i], PLAYER_NAME_LEN - 1);
    p->name[PLAYER_NAME_LEN - 1] = '\0';
    p->isAI = isAI[i];
    p->difficulty = diffs[i];
    p->handCount = 0;
    p->prediction = -1;
    p->tricksWon = 0;
    p->penalties = 0;
    p->eliminated = 0;
  }
  state->dealerId = 0;
  state->handSize = MAX_HAND_SIZE;
  state->cycleDirection = -1;
  state->handNumber = 0;
  state->winner = -1;

  start_hand(state);
}

int game_get_current_predictor_id(const GameState* state) {
  return state->predictionOrder[state->turnPointer];
}

int game_is_last_predictor(const GameState* state) {
  int activeCount = game_active_player_count(state);
  return state->turnPointer == activeCount - 1;
}

int game_get_forbidden_prediction(const GameState* state, int* outForbidden) {
  if (!game_is_last_predictor(state)) return 0;
  *outForbidden = state->handSize - state->predictionsSum;
  return 1;
}

int game_submit_prediction(GameState* state, int playerId, int value) {
  if (state->phase != PHASE_PREDICTING) return 0;
  if (game_get_current_predictor_id(state) != playerId) return 0;
  if (value < 0 || value > state->handSize) return 0;

  int forbidden;
  if (game_get_forbidden_prediction(state, &forbidden) && value == forbidden) return 0;

  state->players[playerId].prediction = value;
  state->predictionsSum += value;
  state->turnPointer += 1;

  int activeCount = game_active_player_count(state);
  if (state->turnPointer >= activeCount) {
    state->phase = PHASE_PLAYING;
    int order[MAX_PLAYERS], orderCount;
    build_active_order_from(state, state->trickLeaderId, order, &orderCount);
    memcpy(state->trickPlayOrder, order, sizeof(int) * orderCount);
    state->turnPointer = 0;
    state->trickCardsPlayedCount = 0;
  }
  return 1;
}

int game_get_current_player_to_act_id(const GameState* state) {
  if (state->phase == PHASE_PREDICTING) return game_get_current_predictor_id(state);
  if (state->phase == PHASE_PLAYING) return state->trickPlayOrder[state->turnPointer];
  return -1;
}

static int cards_equal(Card a, Card b) {
  return a.suit == b.suit && a.value == b.value;
}

int game_play_card(GameState* state, int playerId, Card card) {
  if (state->phase != PHASE_PLAYING) return 0;
  if (game_get_current_player_to_act_id(state) != playerId) return 0;

  Player* player = &state->players[playerId];
  int cardIndex = -1;
  for (int i = 0; i < player->handCount; i++) {
    if (cards_equal(player->hand[i], card)) { cardIndex = i; break; }
  }
  if (cardIndex == -1) return 0;

  Card playedCard = player->hand[cardIndex];
  for (int i = cardIndex; i < player->handCount - 1; i++) {
    player->hand[i] = player->hand[i + 1];
  }
  player->handCount -= 1;

  int slot = state->trickCardsPlayedCount;
  state->trickCardsPlayed[slot].playerId = playerId;
  state->trickCardsPlayed[slot].card = playedCard;
  state->trickCardsPlayedCount += 1;
  state->cardsPlayedThisHand[state->cardsPlayedThisHandCount++] = playedCard;
  state->turnPointer += 1;

  int activeCount = game_active_player_count(state);
  if (state->turnPointer < activeCount) {
    return 1;
  }

  // Se jugaron todas las cartas de la baza: resolver ganador (primero
  // jugado gana los empates, igual que Deck.rank + orden de juego en JS).
  int winningIdx = 0;
  for (int i = 1; i < state->trickCardsPlayedCount; i++) {
    if (card_rank(state->trickCardsPlayed[i].card) > card_rank(state->trickCardsPlayed[winningIdx].card)) {
      winningIdx = i;
    }
  }
  int winnerId = state->trickCardsPlayed[winningIdx].playerId;
  state->players[winnerId].tricksWon += 1;

  memcpy(state->lastTrick, state->trickCardsPlayed, sizeof(TrickEntry) * state->trickCardsPlayedCount);
  state->lastTrickCount = state->trickCardsPlayedCount;
  state->lastTrickWinner = winnerId;
  state->phase = PHASE_TRICK_END;
  return 1;
}

static void resolve_hand_end(GameState* state) {
  for (int i = 0; i < state->playerCount; i++) {
    Player* p = &state->players[i];
    if (p->eliminated) continue;
    int diff = abs(p->prediction - p->tricksWon);
    if (diff > 0) {
      p->penalties += diff;
      if (p->penalties >= GAME_MAX_PENALTIES) p->eliminated = 1;
    }
  }

  int stillActive = game_active_player_count(state);
  if (stillActive <= 1) {
    state->phase = PHASE_GAME_END;
    state->winner = first_active_player(state);
    return;
  }

  state->dealerId = next_active_seat(state, state->dealerId);

  if (state->cycleDirection == -1) {
    if (state->handSize > MIN_HAND_SIZE) {
      state->handSize -= 1;
    } else {
      state->cycleDirection = 1;
      state->handSize += 1;
    }
  } else {
    if (state->handSize < MAX_HAND_SIZE) {
      state->handSize += 1;
    } else {
      state->cycleDirection = -1;
      state->handSize -= 1;
    }
  }

  state->phase = PHASE_HAND_END;
}

void game_continue_after_trick(GameState* state) {
  if (state->phase != PHASE_TRICK_END) return;

  int handFinished = 1;
  for (int i = 0; i < state->playerCount; i++) {
    if (state->players[i].eliminated) continue;
    if (state->players[i].handCount > 0) { handFinished = 0; break; }
  }

  if (!handFinished) {
    int leaderId = state->lastTrickWinner;
    state->trickLeaderId = leaderId;
    int order[MAX_PLAYERS], orderCount;
    build_active_order_from(state, leaderId, order, &orderCount);
    memcpy(state->trickPlayOrder, order, sizeof(int) * orderCount);
    state->turnPointer = 0;
    state->trickCardsPlayedCount = 0;
    state->phase = PHASE_PLAYING;
    return;
  }

  resolve_hand_end(state);
}

void game_start_next_hand(GameState* state) {
  if (state->phase != PHASE_HAND_END) return;
  start_hand(state);
}
