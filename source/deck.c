#include "deck.h"
#include <stdlib.h>
#include <stdio.h>

void deck_create(Card out[DECK_SIZE]) {
  int i = 0;
  for (int suit = 0; suit < 4; suit++) {
    for (int value = 1; value <= 12; value++) {
      out[i].value = value;
      out[i].suit = (Suit)suit;
      i++;
    }
  }
}

void deck_shuffle(Card cards[DECK_SIZE]) {
  for (int i = DECK_SIZE - 1; i > 0; i--) {
    int j = rand() % (i + 1);
    Card tmp = cards[i];
    cards[i] = cards[j];
    cards[j] = tmp;
  }
}

int card_rank(Card card) {
  if (card.value == 1) {
    if (card.suit == SUIT_ESPADA) return 16;
    if (card.suit == SUIT_BASTO) return 15;
    return 2; // oro o copa
  }
  return card.value + 2;
}

const char* suit_label(Suit suit) {
  switch (suit) {
    case SUIT_ESPADA: return "Espada";
    case SUIT_BASTO: return "Basto";
    case SUIT_ORO: return "Oro";
    case SUIT_COPA: return "Copa";
  }
  return "?";
}

void card_label(Card card, char* out, int outSize) {
  const char* suit = suit_label(card.suit);
  if (card.value == 1) snprintf(out, outSize, "As de %s", suit);
  else if (card.value == 10) snprintf(out, outSize, "Sota de %s", suit);
  else if (card.value == 11) snprintf(out, outSize, "Caballo de %s", suit);
  else if (card.value == 12) snprintf(out, outSize, "Rey de %s", suit);
  else snprintf(out, outSize, "%d de %s", card.value, suit);
}
