// GENERADO por tools/prep-assets.ps1 -- no editar a mano.
#include "card_gfx.h"
#include "card_espada_1.h"
#include "card_espada_2.h"
#include "card_espada_3.h"
#include "card_espada_4.h"
#include "card_espada_5.h"
#include "card_espada_6.h"
#include "card_espada_7.h"
#include "card_espada_8.h"
#include "card_espada_9.h"
#include "card_espada_10.h"
#include "card_espada_11.h"
#include "card_espada_12.h"
#include "card_basto_1.h"
#include "card_basto_2.h"
#include "card_basto_3.h"
#include "card_basto_4.h"
#include "card_basto_5.h"
#include "card_basto_6.h"
#include "card_basto_7.h"
#include "card_basto_8.h"
#include "card_basto_9.h"
#include "card_basto_10.h"
#include "card_basto_11.h"
#include "card_basto_12.h"
#include "card_oro_1.h"
#include "card_oro_2.h"
#include "card_oro_3.h"
#include "card_oro_4.h"
#include "card_oro_5.h"
#include "card_oro_6.h"
#include "card_oro_7.h"
#include "card_oro_8.h"
#include "card_oro_9.h"
#include "card_oro_10.h"
#include "card_oro_11.h"
#include "card_oro_12.h"
#include "card_copa_1.h"
#include "card_copa_2.h"
#include "card_copa_3.h"
#include "card_copa_4.h"
#include "card_copa_5.h"
#include "card_copa_6.h"
#include "card_copa_7.h"
#include "card_copa_8.h"
#include "card_copa_9.h"
#include "card_copa_10.h"
#include "card_copa_11.h"
#include "card_copa_12.h"
#include "card_back.h"

static const unsigned int* const CARD_TILES[4][12] = {
  { card_espada_1Tiles, card_espada_2Tiles, card_espada_3Tiles, card_espada_4Tiles, card_espada_5Tiles, card_espada_6Tiles, card_espada_7Tiles, card_espada_8Tiles, card_espada_9Tiles, card_espada_10Tiles, card_espada_11Tiles, card_espada_12Tiles },
  { card_basto_1Tiles, card_basto_2Tiles, card_basto_3Tiles, card_basto_4Tiles, card_basto_5Tiles, card_basto_6Tiles, card_basto_7Tiles, card_basto_8Tiles, card_basto_9Tiles, card_basto_10Tiles, card_basto_11Tiles, card_basto_12Tiles },
  { card_oro_1Tiles, card_oro_2Tiles, card_oro_3Tiles, card_oro_4Tiles, card_oro_5Tiles, card_oro_6Tiles, card_oro_7Tiles, card_oro_8Tiles, card_oro_9Tiles, card_oro_10Tiles, card_oro_11Tiles, card_oro_12Tiles },
  { card_copa_1Tiles, card_copa_2Tiles, card_copa_3Tiles, card_copa_4Tiles, card_copa_5Tiles, card_copa_6Tiles, card_copa_7Tiles, card_copa_8Tiles, card_copa_9Tiles, card_copa_10Tiles, card_copa_11Tiles, card_copa_12Tiles }
};

const unsigned int* card_tiles_for(Suit suit, int value) {
  return CARD_TILES[suit][value - 1];
}

const unsigned int* card_back_tiles(void) {
  return card_backTiles;
}
