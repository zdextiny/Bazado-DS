#include "ai.h"
#include <math.h>
#include <string.h>
#include <stdlib.h>

// ---------- Peso de carta (probabilidad aproximada de ganar con ella) ----------

static float card_weight(Card card) {
  int r = card_rank(card);
  if (r >= 15) return 0.9f; // ases especiales
  if (r >= 13) return 0.7f; // 11, 12
  if (r >= 12) return 0.5f; // 10
  if (r >= 9) return 0.4f;  // 7, 8, 9
  if (r >= 6) return 0.2f;  // 4, 5, 6
  if (r >= 4) return 0.1f;  // 2, 3
  return 0.05f;             // ases debiles (oro/copa)
}

static void sort_by_rank_asc(Card* cards, int count) {
  for (int i = 1; i < count; i++) {
    Card key = cards[i];
    int keyRank = card_rank(key);
    int j = i - 1;
    while (j >= 0 && card_rank(cards[j]) > keyRank) {
      cards[j + 1] = cards[j];
      j--;
    }
    cards[j + 1] = key;
  }
}

// Redondeo con sesgo conservador: hace falta que la parte fraccionaria
// llegue a 0.65 (no 0.5) para redondear hacia arriba — mismo criterio que
// la version web, para que el bot no sobre-prediga con manos flojas.
static int clamp_prediction(const GameState* state, float value) {
  int result = (int)floorf(value + 0.35f);
  if (result < 0) result = 0;
  if (result > state->handSize) result = state->handSize;

  int forbidden;
  if (game_get_forbidden_prediction(state, &forbidden) && result == forbidden) {
    result = (result < state->handSize) ? result + 1 : result - 1;
  }
  return result;
}

// El ancho (espada rank16, basto rank15) es la carta mas valiosa que
// existe — se guarda salvo que sea la unica carta en mano (fuerza mayor)
// o haga falta de verdad para superar un Rey (rank14) o algo mas fuerte
// que ya este ganando la baza. currentBestRank = -1 significa "abriendo,
// nada contra que compararse todavia" (equivalente al `null` de JS).
static int is_hoarded_ancho(Card card, int handSize, int currentBestRank) {
  if (card_rank(card) < 15) return 0;
  if (handSize == 1) return 0;
  if (currentBestRank >= 14) return 0;
  return 1;
}

// Si la carta MAS FLOJA de la mano ya le gana a la mesa, cualquier carta
// que tire gana esta baza — no hay riesgo que evitar eligiendo bien.
static int is_guaranteed_win(const Card* byRankAsc, int count, int currentBestRank) {
  return count > 0 && card_rank(byRankAsc[0]) > currentBestRank;
}

// ---------- Prediccion ----------

static int decide_prediction_easy(const GameState* state, int playerId) {
  const Player* player = &state->players[playerId];
  float expected = 0;
  for (int i = 0; i < player->handCount; i++) expected += card_weight(player->hand[i]);
  return clamp_prediction(state, expected);
}

// Con 4 jugadores en la mesa, un bot de media/dificil nunca pide 3.
#define FOUR_PLAYER_PREDICTION_CAP 2

static int decide_prediction_medium(const GameState* state, int playerId) {
  const Player* player = &state->players[playerId];

  int confidentRivals = 0;
  int declaredSum = 0;
  for (int i = 0; i < state->playerCount; i++) {
    const Player* p = &state->players[i];
    if (p->id == playerId || p->eliminated) continue;
    if (p->prediction < 0) continue; // sin predecir todavia
    if (p->prediction >= 1) confidentRivals++;
    declaredSum += p->prediction;
  }
  float claimedRatio = state->handSize > 0 ? (float)declaredSum / (float)state->handSize : 0.0f;

  float expected = 0;
  for (int i = 0; i < player->handCount; i++) {
    Card card = player->hand[i];
    int r = card_rank(card);
    if (r >= 15) {
      expected += card_weight(card);
    } else if (r >= 12) {
      if (confidentRivals >= 2 || claimedRatio >= 0.6f) expected += 0.3f;
      else if (confidentRivals == 1 || claimedRatio >= 0.35f) expected += 0.5f;
      else expected += card_weight(card) - 0.1f;
    } else if (r >= 9) {
      expected += (confidentRivals == 0 && claimedRatio < 0.2f) ? 0.55f : 0.15f;
    } else {
      expected += card_weight(card);
    }
  }

  int result = clamp_prediction(state, expected);
  if (game_active_player_count(state) == 4 && result > FOUR_PLAYER_PREDICTION_CAP) {
    int forbidden;
    int hasForbidden = game_get_forbidden_prediction(state, &forbidden);
    if (hasForbidden && forbidden == FOUR_PLAYER_PREDICTION_CAP) {
      result = FOUR_PLAYER_PREDICTION_CAP - 1;
    } else {
      result = FOUR_PLAYER_PREDICTION_CAP;
    }
  }
  return result;
}

int ai_decide_prediction(const GameState* state, int playerId) {
  Difficulty diff = state->players[playerId].difficulty;
  if (diff == DIFF_MEDIUM || diff == DIFF_HARD) return decide_prediction_medium(state, playerId);
  return decide_prediction_easy(state, playerId);
}

// ---------- Jugar carta ----------

static int player_needed(const Player* player) {
  int prediction = player->prediction < 0 ? 0 : player->prediction;
  return prediction - player->tricksWon;
}

static int trick_current_best_rank(const GameState* state) {
  int best = -1;
  for (int i = 0; i < state->trickCardsPlayedCount; i++) {
    int r = card_rank(state->trickCardsPlayed[i].card);
    if (r > best) best = r;
  }
  return best;
}

static Card decide_card_base(const GameState* state, int playerId) {
  const Player* player = &state->players[playerId];
  int needed = player_needed(player);
  int cardsPlayedCount = state->trickCardsPlayedCount;

  Card byRankAsc[MAX_HAND_SIZE];
  memcpy(byRankAsc, player->hand, sizeof(Card) * player->handCount);
  sort_by_rank_asc(byRankAsc, player->handCount);

  if (cardsPlayedCount == 0) {
    // Abre la baza: si no necesita bazas, dumpea la mas debil. Si
    // necesita, arriesga fuerte pero sin tocar el ancho reservado.
    if (needed <= 0) return byRankAsc[0];
    Card free_[MAX_HAND_SIZE]; int freeCount = 0;
    for (int i = 0; i < player->handCount; i++) {
      if (!is_hoarded_ancho(byRankAsc[i], player->handCount, -1)) free_[freeCount++] = byRankAsc[i];
    }
    return freeCount > 0 ? free_[freeCount - 1] : byRankAsc[player->handCount - 1];
  }

  int currentBestRank = trick_current_best_rank(state);

  if (needed > 0) {
    Card winners[MAX_HAND_SIZE]; int winnersCount = 0;
    for (int i = 0; i < player->handCount; i++) {
      if (card_rank(byRankAsc[i]) > currentBestRank) winners[winnersCount++] = byRankAsc[i];
    }
    Card freeWinners[MAX_HAND_SIZE]; int freeWinnersCount = 0;
    for (int i = 0; i < winnersCount; i++) {
      if (!is_hoarded_ancho(winners[i], player->handCount, currentBestRank)) freeWinners[freeWinnersCount++] = winners[i];
    }
    if (freeWinnersCount > 0) {
      if (is_guaranteed_win(byRankAsc, player->handCount, currentBestRank)) return freeWinners[freeWinnersCount - 1];
      return freeWinners[0];
    }
    // El unico ganador disponible era el ancho reservado: mejor dejar
    // pasar esta baza (si hay con que) que gastarlo de mas.
    Card losers[MAX_HAND_SIZE]; int losersCount = 0;
    for (int i = 0; i < player->handCount; i++) {
      if (card_rank(byRankAsc[i]) <= currentBestRank) losers[losersCount++] = byRankAsc[i];
    }
    if (losersCount > 0) return losers[losersCount - 1];
    return winnersCount > 0 ? winners[0] : byRankAsc[0];
  }

  // Ya cumplio: guarda la carta mas segura, tira la mas alta de las que
  // igual pierden esta baza.
  Card losers[MAX_HAND_SIZE]; int losersCount = 0;
  for (int i = 0; i < player->handCount; i++) {
    if (card_rank(byRankAsc[i]) <= currentBestRank) losers[losersCount++] = byRankAsc[i];
  }
  return losersCount > 0 ? losers[losersCount - 1] : byRankAsc[0];
}

// Rank de un 6. Si la carta que va ganando la baza es esta o mas floja,
// senal de que a quien la tiro probablemente no le sirve llevarsela.
#define SABOTAGE_RANK_CEILING 8

static int danger_score(Card card) {
  float w = card_weight(card);
  if (w == 0.9f) return 0;
  if (w == 0.7f) return 1;
  if (w == 0.5f) return 4;
  if (w == 0.4f) return 3;
  if (w == 0.2f) return 2;
  if (w == 0.1f) return 1;
  return 0; // 0.05
}

static void sort_by_danger_desc(Card* cards, int count) {
  for (int i = 1; i < count; i++) {
    Card key = cards[i];
    int keyDanger = danger_score(key);
    int j = i - 1;
    while (j >= 0 && danger_score(cards[j]) < keyDanger) {
      cards[j + 1] = cards[j];
      j--;
    }
    cards[j + 1] = key;
  }
}

// Dificil: mientras tenga margen (mas bazas por jugar que las que
// necesita ganar), prioriza perder a proposito (sabotaje incluido) y
// guardarse la informacion para mas adelante. Forzado a ganar TODO lo que
// le queda, recien ahi juega en serio.
static Card decide_card_hard(const GameState* state, int playerId) {
  const Player* player = &state->players[playerId];
  int cardsPlayedCount = state->trickCardsPlayedCount;
  int needed = player_needed(player);
  int remainingTricks = player->handCount; // incluye la que esta por jugar
  int mustWinEverything = needed > 0 && remainingTricks <= needed;

  Card byRankAsc[MAX_HAND_SIZE];
  memcpy(byRankAsc, player->hand, sizeof(Card) * player->handCount);
  sort_by_rank_asc(byRankAsc, player->handCount);

  if (cardsPlayedCount > 0 && !mustWinEverything) {
    int currentBestRank = trick_current_best_rank(state);
    if (currentBestRank <= SABOTAGE_RANK_CEILING) {
      for (int i = 0; i < player->handCount; i++) {
        if (card_rank(byRankAsc[i]) < currentBestRank) return byRankAsc[i]; // la mas floja que se queda por debajo
      }
    }
  }

  if (cardsPlayedCount == 0) {
    if (!mustWinEverything) return byRankAsc[0];
    Card free_[MAX_HAND_SIZE]; int freeCount = 0;
    for (int i = 0; i < player->handCount; i++) {
      if (!is_hoarded_ancho(byRankAsc[i], player->handCount, -1)) free_[freeCount++] = byRankAsc[i];
    }
    return freeCount > 0 ? free_[freeCount - 1] : byRankAsc[player->handCount - 1];
  }

  int currentBestRank = trick_current_best_rank(state);

  if (mustWinEverything) {
    Card winners[MAX_HAND_SIZE]; int winnersCount = 0;
    for (int i = 0; i < player->handCount; i++) {
      if (card_rank(byRankAsc[i]) > currentBestRank) winners[winnersCount++] = byRankAsc[i];
    }
    Card freeWinners[MAX_HAND_SIZE]; int freeWinnersCount = 0;
    for (int i = 0; i < winnersCount; i++) {
      if (!is_hoarded_ancho(winners[i], player->handCount, currentBestRank)) freeWinners[freeWinnersCount++] = winners[i];
    }
    Card* usable = freeWinnersCount > 0 ? freeWinners : winners;
    int usableCount = freeWinnersCount > 0 ? freeWinnersCount : winnersCount;
    if (usableCount > 0) {
      if (is_guaranteed_win(byRankAsc, player->handCount, currentBestRank)) return usable[usableCount - 1];
      return usable[0];
    }
    return byRankAsc[0]; // no hay con que ganar: perdio esta igual, la mas barata
  }

  // Con margen: prioriza perder esta baza a proposito, sacandose de
  // encima la carta mas peligrosa entre las que igual pierden.
  Card losers[MAX_HAND_SIZE]; int losersCount = 0;
  for (int i = 0; i < player->handCount; i++) {
    if (card_rank(byRankAsc[i]) <= currentBestRank) losers[losersCount++] = byRankAsc[i];
  }
  if (losersCount > 0) {
    sort_by_danger_desc(losers, losersCount);
    return losers[0];
  }
  // No hay ninguna carta que pierda: forzado a ganar pese a tener
  // margen — aprovecha la baza "gratis" para descartar lo mas alto
  // posible, sin tocar el ancho reservado.
  Card winners[MAX_HAND_SIZE]; int winnersCount = 0;
  for (int i = 0; i < player->handCount; i++) {
    if (card_rank(byRankAsc[i]) > currentBestRank) winners[winnersCount++] = byRankAsc[i];
  }
  Card freeWinners[MAX_HAND_SIZE]; int freeWinnersCount = 0;
  for (int i = 0; i < winnersCount; i++) {
    if (!is_hoarded_ancho(winners[i], player->handCount, currentBestRank)) freeWinners[freeWinnersCount++] = winners[i];
  }
  Card* usable = freeWinnersCount > 0 ? freeWinners : winners;
  int usableCount = freeWinnersCount > 0 ? freeWinnersCount : winnersCount;
  return usableCount > 0 ? usable[usableCount - 1] : byRankAsc[0];
}

// ---------- Dificil: Monte Carlo con determinizacion ----------
//
// En vez de una heuristica fija, para cada carta candidata se juegan
// muchas manos simuladas COMPLETAS a partir de ahi (repartiendo al
// azar, pero de forma consistente con lo que ya se sabe, las cartas
// que todavia no se vieron entre los rivales -- "determinizacion",
// la misma tecnica que usan IAs de bridge/hearts/spades de verdad) y
// se elige la carta cuyo promedio de |prediccion - bazas ganadas al
// final| salga mas bajo. Ese es el objetivo REAL del juego -- clavarla
// justo, no ganar todas las bazas posibles -- asi que es lo que se
// mide, no cuantas bazas se ganan a secas.
//
// El resto de los jugadores (rivales Y el propio bot en sus turnos
// futuros dentro de la simulacion) juegan con decide_card_hard como
// "politica" de relleno -- la MISMA heuristica que de verdad usan los
// bots de "media" en la mesa real (dispatch en ai_decide_card mas
// abajo), rapida y ya probada, no hace falta que sea perfecta: el
// valor de Monte Carlo sale de promediar muchas determinizaciones, no
// de que cada partida simulada este jugada de forma optima.
// OJO: antes esto llamaba a decide_card_medium -- una funcion que
// tenia el MISMO bug que se encontro y arreglo en la version web hoy
// (decideCardMedium era una copia literal de decideCardBase, sin
// ninguna disciplina de margen) y que ademas NUNCA se usaba para nada
// mas (ai_decide_card manda "media" derecho a decide_card_hard, no a
// decide_card_medium). Osea que el relleno de cada simulacion jugaba
// como "facil" (siempre a ganar apenas puede) en vez de como los
// rivales de "media"/"dificil" de verdad, lo que sesgaba todas las
// evaluaciones de Monte Carlo. Se saco decide_card_medium entero (no
// lo llamaba nadie mas) y esto pasa a usar decide_card_hard.
//
// GameState es POD (sin punteros), se puede copiar con una asignacion
// comun -- eso es lo que permite simular sin tocar para nada el
// estado real ni sus efectos (sonido, animacion, todo eso vive en
// main.c, game.c es un motor de reglas puro).
//
// Ademas de la propia precision, dificil juega SUCIO contra el HUMANO
// a proposito: cada simulacion tambien mide cuanto termina desviandose
// el humano de lo que predijo (de mas o de menos), y esa desviacion
// suma a favor de la carta candidata -- con mas peso que la propia
// precision del bot. La idea no es "cada uno a lo suyo", es arruinarle
// la mano al humano aunque el bot se pase de lo que predijo el.
#define MC_ROLLOUT_BUDGET 120 // total repartido entre las cartas candidatas, ver decide_card_montecarlo
#define HUMAN_PLAYER_ID 0 // Vos es siempre el asiento 0 (ver HUMAN_ID en main.c)
#define MC_SABOTAGE_WEIGHT 1.5f // cuanto pesa arruinarle la prediccion al humano contra la propia precision del bot

static void mc_shuffle(Card* cards, int count) {
  for (int i = count - 1; i > 0; i--) {
    int j = rand() % (i + 1);
    Card tmp = cards[i];
    cards[i] = cards[j];
    cards[j] = tmp;
  }
}

// Todo el mazo menos la mano propia y menos lo que ya se jugo esta
// mano (cardsPlayedThisHand incluye la baza en curso tambien, se
// agrega ahi mismo en game_play_card) -- son las cartas que, desde el
// punto de vista de `deciderId`, todavia podrian estar en la mano de
// CUALQUIER rival.
static int mc_collect_unknown(const GameState* state, int deciderId, Card* out) {
  Card fullDeck[DECK_SIZE];
  deck_create(fullDeck);
  const Player* me = &state->players[deciderId];

  int count = 0;
  for (int i = 0; i < DECK_SIZE; i++) {
    Card c = fullDeck[i];
    int known = 0;
    for (int h = 0; h < me->handCount && !known; h++) {
      if (me->hand[h].suit == c.suit && me->hand[h].value == c.value) known = 1;
    }
    for (int p = 0; p < state->cardsPlayedThisHandCount && !known; p++) {
      if (state->cardsPlayedThisHand[p].suit == c.suit && state->cardsPlayedThisHand[p].value == c.value) known = 1;
    }
    if (!known) out[count++] = c;
  }
  return count;
}

// Termina de jugar la mano simulada (bot incluido) usando
// decide_card_hard para todos, y devuelve cuantas bazas termino
// ganando `deciderId`. `sim` YA tiene la jugada candidata hecha antes
// de llamar esto.
static int mc_playout(GameState* sim, int deciderId) {
  int guard = 0; // limite de seguridad, nunca deberia hacer falta de verdad
  while (sim->phase != PHASE_HAND_END && sim->phase != PHASE_GAME_END && guard < 64) {
    guard++;
    if (sim->phase == PHASE_TRICK_END) {
      game_continue_after_trick(sim);
      continue;
    }
    if (sim->phase != PHASE_PLAYING) break; // no deberia pasar: predicciones ya cerradas antes de jugar cartas
    int actor = game_get_current_player_to_act_id(sim);
    if (actor < 0) break;
    Card card = decide_card_hard(sim, actor);
    if (!game_play_card(sim, actor, card)) break;
  }
  return sim->players[deciderId].tricksWon;
}

static Card decide_card_montecarlo(const GameState* state, int playerId) {
  const Player* player = &state->players[playerId];
  if (player->handCount <= 1) return player->hand[0]; // unica opcion, no hace falta simular nada

  Card unknown[DECK_SIZE];
  int unknownCount = mc_collect_unknown(state, playerId, unknown);

  // Presupuesto de simulaciones repartido entre las cartas candidatas
  // -- asi una mano con mas opciones no tarda proporcionalmente mas.
  int rollouts = MC_ROLLOUT_BUDGET / player->handCount;
  if (rollouts < 12) rollouts = 12;

  float bestScore = 1e9f;
  Card bestCard = player->hand[0];

  for (int c = 0; c < player->handCount; c++) {
    Card candidate = player->hand[c];
    float totalScore = 0.0f; // propia precision MENOS sabotaje -- puede terminar negativo, y esta bien (mejor todavia)

    for (int r = 0; r < rollouts; r++) {
      GameState sim = *state; // copia completa (POD) -- la real no se toca

      // Reparto al azar, entre los rivales activos, de exactamente la
      // cantidad de cartas que cada uno tiene de verdad -- las que
      // sobran del pool (manos chicas dejan cartas sin repartir del
      // mazo de 48 esta mano) simplemente no le tocan a nadie.
      Card pool[DECK_SIZE];
      memcpy(pool, unknown, sizeof(Card) * unknownCount);
      mc_shuffle(pool, unknownCount);
      int cursor = 0;
      for (int i = 0; i < sim.playerCount; i++) {
        if (i == playerId || sim.players[i].eliminated) continue;
        Player* p = &sim.players[i];
        for (int h = 0; h < p->handCount; h++) {
          p->hand[h] = pool[cursor++];
        }
      }

      if (!game_play_card(&sim, playerId, candidate)) continue;
      if (sim.phase == PHASE_TRICK_END) game_continue_after_trick(&sim);

      int finalTricks = mc_playout(&sim, playerId);
      int ownPenalty = finalTricks - player->prediction;
      if (ownPenalty < 0) ownPenalty = -ownPenalty;

      // Sabotaje: cuanto mas lejos termina el HUMANO de lo que predijo
      // en esta simulacion (para cualquier lado, de mas o de menos),
      // mas atractiva se ve esta carta -- resta del puntaje (mas bajo
      // = mejor), asi que empuja a elegirla aunque empeore un poco la
      // propia precision. Si el humano ya quedo eliminado en la
      // simulacion no tiene sentido seguir apuntandole.
      float sabotageBonus = 0.0f;
      if (!state->players[HUMAN_PLAYER_ID].eliminated) {
        int humanDeviation = sim.players[HUMAN_PLAYER_ID].tricksWon - state->players[HUMAN_PLAYER_ID].prediction;
        if (humanDeviation < 0) humanDeviation = -humanDeviation;
        sabotageBonus = MC_SABOTAGE_WEIGHT * (float)humanDeviation;
      }

      totalScore += (float)ownPenalty - sabotageBonus;
    }

    float avgScore = totalScore / rollouts;
    if (avgScore < bestScore) {
      bestScore = avgScore;
      bestCard = candidate;
    }
  }

  return bestCard;
}

Card ai_decide_card(const GameState* state, int playerId) {
  Difficulty diff = state->players[playerId].difficulty;
  // Dificil ahora es el sistema de Monte Carlo (mas arriba) -- la
  // heuristica que antes era "dificil" pasa a ser la de "media" (sigue
  // siendo un buen salto respecto de facil, pero dificil de verdad
  // ahora es el que practica miles de manos posibles antes de tirar).
  if (diff == DIFF_HARD) return decide_card_montecarlo(state, playerId);
  if (diff == DIFF_MEDIUM) return decide_card_hard(state, playerId);
  return decide_card_base(state, playerId);
}
