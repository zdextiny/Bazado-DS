// Prediccion: puerto 1:1 de js/ai.js (misma heuristica en las tres
// dificultades). Jugar carta: facil/media siguen la heuristica de
// siempre (media ahora usa lo que antes era la de dificil); dificil de
// verdad es Monte Carlo con determinizacion (ver decide_card_montecarlo
// en ai.c). La version web (bazas-cartas) tambien tiene su propio
// Monte Carlo para dificil ahora, pero son implementaciones separadas
// (JS vs C) -- no hay puerto 1:1 ahi como en prediccion.
#ifndef BAZAS_AI_H
#define BAZAS_AI_H

#include "game.h"

int ai_decide_prediction(const GameState* state, int playerId);
Card ai_decide_card(const GameState* state, int playerId);

#endif
