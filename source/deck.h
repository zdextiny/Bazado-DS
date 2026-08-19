// Puerto 1:1 de js/deck.js — mazo espanol de 48 cartas y su jerarquia de
// fuerza. Sin dependencias de libnds: esta parte es pura logica, se podria
// compilar y testear igual en cualquier plataforma.
#ifndef BAZAS_DECK_H
#define BAZAS_DECK_H

typedef enum { SUIT_ESPADA, SUIT_BASTO, SUIT_ORO, SUIT_COPA } Suit;

typedef struct {
  int value; // 1..12
  Suit suit;
} Card;

#define DECK_SIZE 48

// Llena `out` con las 48 cartas en orden fijo (sin barajar).
void deck_create(Card out[DECK_SIZE]);

// Fisher-Yates in-place. Requiere haber llamado srand() antes (ver main.c).
void deck_shuffle(Card cards[DECK_SIZE]);

// Jerarquia (mayor a menor): As Espada(16) > As Basto(15) > 12(14) > ... >
// 2(4) > As Oro = As Copa(2).
int card_rank(Card card);

const char* suit_label(Suit suit);

// Escribe "As de Espada", "10 de Oro", etc. en `out` (buffer del llamador).
void card_label(Card card, char* out, int outSize);

#endif
