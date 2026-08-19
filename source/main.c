// Milestone 2: interfaz tactil real. Pantalla de ARRIBA (main/top) = mesa
// (predicciones, penalizaciones, baza en curso). Pantalla de ABAJO
// (sub/bottom) = tu mano y los controles tactiles (elegir prediccion,
// tocar una carta para jugarla). Vos sos siempre el asiento 0; los otros 3
// son bots. Sin online, sin menu todavia — eso es el siguiente paso una
// vez que se sienta bien jugar.
#include <nds.h>
#include <fat.h>
#include <filesystem.h>
#include <maxmod9.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "soundbank.h"
#include "soundbank_bin.h"
#include "deck.h"
#include "game.h"
#include "ai.h"
#include "bg_pattern_0.h"
#include "bg_pattern_1.h"
#include "bg_pattern_2.h"
#include "bg_pattern_3.h"
#include "bg_pattern_4.h"
#include "bg_pattern_5.h"
#include "bg_pattern_6.h"
#include "bg_pattern_7.h"
#include "bg_pattern_8.h"
#include "bg_pattern_9.h"
#include "bg_patterns_shared.h"
#include "score_bg.h"
#include "title_bg_0.h"
#include "title_bg_1.h"
#include "title_bg_shared.h"
#include "title_letter_0.h"
#include "title_letter_1.h"
#include "title_letter_2.h"
#include "title_letter_3.h"
#include "title_letter_4.h"
#include "title_letter_5.h"
#include "title_letters_shared.h"
#include "win_text.h"
#include "lose_text.h"
#include "outcome_text_shared.h"
#include "card_gfx.h"
#include "cards_shared.h"
#include "crown.h"
#include "arrow_right.h"
#include "arrow_down.h"
#include "arrow_left.h"
#include "arrow_up.h"
#include "arrow_shared.h"
#include "leader_frame.h"
#include "heart_full.h"
#include "heart_broken.h"
#include "hearts_shared.h"
#include "icon_pred.h"
#include "icon_won.h"
#include "corner_icons_shared.h"
#include "avatar_placeholder.h"
#include "buttons_shared.h"
#include "button_0.h"
#include "button_1.h"
#include "button_2.h"
#include "button_3.h"
#include "button_4.h"
#include "button_5.h"
#include "button_6.h"
#include "sling_band.h"
#include "particle.h"
#include "play_button_0.h"
#include "play_button_1.h"
#include "play_button_2.h"
#include "play_button_3.h"
#include "play_button_shared.h"
#include "ancho_banner_0.h"
#include "ancho_banner_1.h"
#include "ancho_banner_2.h"
#include "ancho_banner_3.h"
#include "ancho_banner_shared.h"
#include "studio_splash_0.h"
#include "studio_splash_1.h"
#include "studio_splash_shared.h"

#define HUMAN_ID 0

// ---------- Musica/Sonido: on/off desde Opciones (ver run_options_menu) ----------
// Persisten entre partidas (ver load_settings/save_settings, junto al
// nombre guardado). Por defecto los dos prendidos.
static int musicEnabled = 1;
static int soundEnabled = 1;

// ---------- Fondo de gameplay: elegible desde Opciones ----------
// 10 patrones (ver make-bg-patterns.ps1), arriba y abajo pueden ser el
// mismo o distintos. Persisten igual que musica/sonido (ver
// load_settings/save_settings).
#define BG_PATTERN_COUNT 10
static const unsigned int* const BG_PATTERN_TILES[BG_PATTERN_COUNT] = {
  bg_pattern_0Tiles, bg_pattern_1Tiles, bg_pattern_2Tiles, bg_pattern_3Tiles, bg_pattern_4Tiles,
  bg_pattern_5Tiles, bg_pattern_6Tiles, bg_pattern_7Tiles, bg_pattern_8Tiles, bg_pattern_9Tiles,
};
static const unsigned int BG_PATTERN_TILES_LEN[BG_PATTERN_COUNT] = {
  bg_pattern_0TilesLen, bg_pattern_1TilesLen, bg_pattern_2TilesLen, bg_pattern_3TilesLen, bg_pattern_4TilesLen,
  bg_pattern_5TilesLen, bg_pattern_6TilesLen, bg_pattern_7TilesLen, bg_pattern_8TilesLen, bg_pattern_9TilesLen,
};
static const unsigned short* const BG_PATTERN_MAP[BG_PATTERN_COUNT] = {
  bg_pattern_0Map, bg_pattern_1Map, bg_pattern_2Map, bg_pattern_3Map, bg_pattern_4Map,
  bg_pattern_5Map, bg_pattern_6Map, bg_pattern_7Map, bg_pattern_8Map, bg_pattern_9Map,
};
static const unsigned int BG_PATTERN_MAP_LEN[BG_PATTERN_COUNT] = {
  bg_pattern_0MapLen, bg_pattern_1MapLen, bg_pattern_2MapLen, bg_pattern_3MapLen, bg_pattern_4MapLen,
  bg_pattern_5MapLen, bg_pattern_6MapLen, bg_pattern_7MapLen, bg_pattern_8MapLen, bg_pattern_9MapLen,
};
static int bgTopPatternIndex = 0;
static int bgBottomPatternIndex = 0;
static int bgSameTopBottom = 1; // si esta prendido, abajo sigue siempre al de arriba

static int effective_bg_bottom_index(void) {
  return bgSameTopBottom ? bgTopPatternIndex : bgBottomPatternIndex;
}

// Copia el patron elegido a un layer de fondo ya inicializado (arriba
// o abajo, cualquiera de los dos -- bgGetGfxPtr/bgGetMapPtr resuelven
// solas segun el bgId). La paleta NO hace falta volver a copiarla: el
// tinte de rojo da siempre los mismos 2 colores de salida sin importar
// el patron (ver bg_patterns_sharedPal, copiada una sola vez al boot
// en setup_background).
static void apply_bg_pattern(int bgId, int patternIndex) {
  dmaCopy(BG_PATTERN_TILES[patternIndex], bgGetGfxPtr(bgId), BG_PATTERN_TILES_LEN[patternIndex]);
  dmaCopy(BG_PATTERN_MAP[patternIndex], bgGetMapPtr(bgId), BG_PATTERN_MAP_LEN[patternIndex]);
  bgUpdate();
}

// Todos los efectos de sonido del juego pasan por aca (no mmEffect
// directo) para que el toggle de Opciones los calle a todos por igual,
// UI incluida.
static void play_sfx(int id) {
  if (soundEnabled) mmEffect(id);
}

// Tiempos de la pantalla de productora (ver show_studio_splash, mas
// abajo) y del fundido de entrada del menu principal que le sigue (ver
// show_main_menu) -- van ARRIBA de las dos porque show_main_menu esta
// antes en el archivo. Todo medido a 60fps.
#define SPLASH_FADE_FRAMES 20           // ~333ms, fundido de negro <-> normal -- "leve animacion"
#define SPLASH_AUDIO1_HOLD_FRAMES 103   // 1724ms (coin insert) -- incluye el fundido de entrada
#define SPLASH_FLICKER_FRAMES 3         // 50ms
#define SPLASH_AUDIO2_HOLD_FRAMES 59    // 888ms (arcade) + 100ms de mas

// Filas fijas por posicionamiento ANSI explicito (\x1b[fila;colH), NO por
// contar saltos de linea a mano — un renglon de texto de mas/de menos mas
// arriba ya no puede desincronizar donde se imprime de donde se espera el
// toque (paso justo eso: la primera version calculaba mal la fila real y
// el toque nunca coincidia, dejando al jugador trabado sin poder elegir
// nada). La mano son sprites de 32x32 escalados 2x en pantalla (ver
// HAND_Y/CARD_SCALE_PAD mas abajo) -- estas filas de texto tienen que
// quedar libres por DEBAJO del borde visual real de esos sprites, no
// del tamano logico sin escalar.
#define PREDICT_HEADING_ROW 12
#define PREDICT_OPTIONS_ROW 17 // se usa +2 = fila 19, justo debajo de los botones sprite (ver BUTTON_Y)

static volatile int frameCount = 0;
static void vblank_handler(void) { frameCount++; }

static PrintConsole topConsole;
static PrintConsole bottomConsole;

static void top(void) { consoleSelect(&topConsole); }
static void bottom(void) { consoleSelect(&bottomConsole); }

static int bgPatternTop;
static int bgPatternBottom;

// Fondo animado: capa de background APARTE de la consola de texto (layer
// 2, la consola usa layer 3), con su propio banco de paleta (banco 1 —
// el banco 0 ya lo ocupa la fuente de texto, pisarlo dejaria el texto
// con colores rotos).
//
// Arriba: banco B, mapeado a un SLOT de VRAM distinto al banco A (el de
// la consola) — VRAM_B_MAIN_BG cae en el slot 1, que empieza 128KB
// despues del slot 0 (banco A), asi que apunta con tileBase=8
// (8*16KB=128KB) y mapBase=64 (64*2KB=128KB) para caer DENTRO del banco
// B y no volver a pisar la fuente.
//
// Abajo: no existe un "VRAM_D_SUB_BG" (el banco D solo sirve para
// sprites en la pantalla de abajo) — se comparte el banco C con la
// consola, pero con offsets bien separados y con mucho margen (no los
// mas ajustados posibles) para no repetir el bug original: ahi la
// fuente (~15KB desde tileBase 0) y el mapa de la consola (2KB desde
// mapBase 31 = byte 62KB) quedaron pisados por calcular el hueco justo.
static void setup_background(void) {
  // mapBase tiene un tope DURO de 31 en la API (0..31, no importa cuantos
  // bancos de VRAM haya mapeados) — el mapa del patron tiene que vivir
  // en el banco A/C igual que el de la consola (nomas que en otro hueco
  // del mismo banco). tileBase si llega hasta 15, ahi si entra el banco
  // B para arriba.
  vramSetBankB(VRAM_B_MAIN_BG);

  bgPatternTop = bgInit(2, BgType_Text4bpp, BgSize_T_256x256, 16, 8);
  bgPatternBottom = bgInitSub(2, BgType_Text4bpp, BgSize_T_256x256, 16, 5);

  // OJO: bg_patternPalLen son 512 bytes (256 colores) — grit siempre
  // exporta el array de paleta completo con relleno, aunque el patron
  // solo use 2 colores de verdad. Copiar los 512 bytes completos a
  // partir del banco 1 (offset 16) se pasaba del final de BG_PALETTE
  // (solo tiene 256 colores en total) y pisaba memoria de mas —
  // probablemente la causa de que la fuente terminara con el color roto.
  // Esta vez grit SI exporto justo los 2 colores reales, sin relleno
  // (bg_patterns_sharedPalLen = 4 bytes -- copiar 32 a mano, como en
  // otros lados de este archivo, leeria 28 bytes de mas fuera del
  // array). Son los MISMOS 2 colores para cualquiera de los 10 (ver
  // bg_patterns_sharedPal), asi que la paleta se copia UNA sola vez
  // aca, independiente de cual este elegido (eso lo maneja
  // apply_bg_pattern).
  dmaCopy(bg_patterns_sharedPal, &BG_PALETTE[16], bg_patterns_sharedPalLen);
  dmaCopy(bg_patterns_sharedPal, &BG_PALETTE_SUB[16], bg_patterns_sharedPalLen);

  apply_bg_pattern(bgPatternTop, bgTopPatternIndex);
  apply_bg_pattern(bgPatternBottom, effective_bg_bottom_index());

  bgSetPriority(bgPatternTop, 3);
  bgSetPriority(bgPatternBottom, 3);
  bgSetPriority(topConsole.bgId, 0);
  bgSetPriority(bottomConsole.bgId, 0);
}

// Fin de mano: la pantalla de arriba cambia el patron animado por un
// fondo fijo con un panel de color por jugador (score_bg, mismo
// formato que el patron asi se pueden intercambiar en el mismo layer
// sin pedir mas VRAM). Solo afecta arriba -- abajo sigue con el
// patron de siempre.
static int showingScoreBg = 0;
// Pantalla de titulo (remolino + logo "BAZADO"): mismo layer/mecanismo
// que el panel de fin de mano, solo que se muestra antes de arrancar
// el primer partido en vez de entre manos.
static int showingTitleBg = 0;

static void show_score_background(void) {
  if (showingScoreBg) return;
  showingScoreBg = 1;
  showingTitleBg = 0;
  dmaCopy(score_bgTiles, bgGetGfxPtr(bgPatternTop), score_bgTilesLen);
  dmaCopy(score_bgMap, bgGetMapPtr(bgPatternTop), score_bgMapLen);
  dmaCopy(score_bgPal, &BG_PALETTE[16], 32);
  bgSetScroll(bgPatternTop, 0, 0);
  bgUpdate();
}

static void update_title_logo_idle(void); // definida mas abajo, junto al resto de la pantalla de arriba

// El remolino no es un shader en vivo (la DS no tiene) -- son 2 frames
// estaticos del MISMO campo de ruido con el tiempo congelado en un
// valor distinto cada uno (ver make-title-bg.ps1), con el logo
// "BAZADO" dibujado IDENTICO pixel a pixel arriba de los dos. Ciclar
// entre ellos cada tanto (ver title_bg_tick) da la sensacion de que
// "respira" sin que el logo se mueva un pixel.
#define TITLE_BG_FRAME_COUNT 2
#define TITLE_BG_FRAME_HOLD_FRAMES (4 * 60) // ~4s por frame a 60fps

static const unsigned int* const TITLE_BG_TILES[TITLE_BG_FRAME_COUNT] = { title_bg_0Tiles, title_bg_1Tiles };
static const unsigned int TITLE_BG_TILES_LEN[TITLE_BG_FRAME_COUNT] = { title_bg_0TilesLen, title_bg_1TilesLen };
static const unsigned short* const TITLE_BG_MAP[TITLE_BG_FRAME_COUNT] = { title_bg_0Map, title_bg_1Map };
static const unsigned int TITLE_BG_MAP_LEN[TITLE_BG_FRAME_COUNT] = { title_bg_0MapLen, title_bg_1MapLen };
static int titleBgFrame = 0;

static void show_title_background(void) {
  if (showingTitleBg) return;
  showingTitleBg = 1;
  showingScoreBg = 0;
  titleBgFrame = 0;
  // OJO: title_bg_sharedPalLen son 512 bytes (256 colores) -- grit
  // siempre exporta el array de paleta completo con relleno, aunque
  // -mp1 fuerce a que el remolino (5 colores nomas) use un solo banco
  // de 16. Copiar los 512 completos a partir del banco 1 (offset 16)
  // se pasa del final de BG_PALETTE (256 colores en total) y pisa
  // memoria de mas -- eso rompia el color de la fuente de texto. Un
  // banco entero (32 bytes) alcanza de sobra.
  dmaCopy(title_bg_sharedPal, &BG_PALETTE[16], 32);
  dmaCopy(TITLE_BG_TILES[0], bgGetGfxPtr(bgPatternTop), TITLE_BG_TILES_LEN[0]);
  dmaCopy(TITLE_BG_MAP[0], bgGetMapPtr(bgPatternTop), TITLE_BG_MAP_LEN[0]);
  bgSetScroll(bgPatternTop, 0, 0);
  bgUpdate();
}

// Se llama una vez por frame (ver update_background_scroll): cada
// TITLE_BG_FRAME_HOLD_FRAMES cuadros, pasa al siguiente frame del
// remolino. Nunca toca el mapa/tiles si no esta activa esta pantalla.
static void title_bg_tick(void) {
  if (!showingTitleBg) return;
  static int accum = 0;
  accum++;
  if (accum < TITLE_BG_FRAME_HOLD_FRAMES) return;
  accum = 0;
  titleBgFrame = (titleBgFrame + 1) % TITLE_BG_FRAME_COUNT;
  dmaCopy(TITLE_BG_TILES[titleBgFrame], bgGetGfxPtr(bgPatternTop), TITLE_BG_TILES_LEN[titleBgFrame]);
  dmaCopy(TITLE_BG_MAP[titleBgFrame], bgGetMapPtr(bgPatternTop), TITLE_BG_MAP_LEN[titleBgFrame]);
  bgUpdate();
}

// Paneo lento del remolino de titulo: SOLO vertical -- el lienzo mide
// 256x256 pero la pantalla muestra 192 de alto, asi que hay 64px de
// "aire" real abajo para deslizarse sin necesidad de que el patron sea
// perfectamente repetible (a diferencia del ancho, que mide exactamente
// 256 = el ancho de pantalla entero: cualquier scroll horizontal
// mostraria el corte feo donde el lienzo "da la vuelta"). Onda
// triangular (ida y vuelta lineal, no seno) -- visualmente alcanza con
// esto y evita meter trigonometria de mas. El rango se queda chico a
// proposito (28px) para que el logo -- dibujado mas o menos entre las
// filas 50 y 115 del lienzo -- nunca se corte contra el borde de
// arriba de la pantalla en ningun punto del paneo.
#define TITLE_BG_PAN_RANGE 28
#define TITLE_BG_PAN_PERIOD_FRAMES (18 * 60) // ida y vuelta completa en ~18s

static int title_bg_pan_offset(void) {
  static int accum = 0;
  accum = (accum + 1) % TITLE_BG_PAN_PERIOD_FRAMES;
  int half = TITLE_BG_PAN_PERIOD_FRAMES / 2;
  int tri = (accum < half) ? accum : (TITLE_BG_PAN_PERIOD_FRAMES - accum);
  return (tri * TITLE_BG_PAN_RANGE) / half;
}

static void restore_pattern_background(void) {
  if (!showingScoreBg && !showingTitleBg) return;
  showingScoreBg = 0;
  showingTitleBg = 0;
  apply_bg_pattern(bgPatternTop, bgTopPatternIndex);
}

// HSV -> RGB15 (5 bits por canal). hue en grados (0-359), sat/val 0-255.
static u16 hsv_to_rgb15(int hue, int sat255, int val255) {
  int c = (val255 * sat255) / 255;
  int hh = hue / 60;
  int x = c * (60 - abs((hue % 120) - 60)) / 60;
  int r1 = 0, g1 = 0, b1 = 0;
  switch (hh) {
    case 0: r1 = c; g1 = x; b1 = 0; break;
    case 1: r1 = x; g1 = c; b1 = 0; break;
    case 2: r1 = 0; g1 = c; b1 = x; break;
    case 3: r1 = 0; g1 = x; b1 = c; break;
    case 4: r1 = x; g1 = 0; b1 = c; break;
    default: r1 = c; g1 = 0; b1 = x; break;
  }
  int m = val255 - c;
  int r5 = ((r1 + m) * 31) / 255;
  int g5 = ((g1 + m) * 31) / 255;
  int b5 = ((b1 + m) * 31) / 255;
  return RGB15(r5, g5, b5);
}

// Oscurece un color RGB15 (deja el mismo tile, cambia el banco de
// paleta) -- se usa para el boton de prediccion prohibido, sin
// necesitar dibujar un sprite "deshabilitado" aparte.
static u16 dim_rgb15(u16 c) {
  int r = c & 0x1F;
  int g = (c >> 5) & 0x1F;
  int b = (c >> 10) & 0x1F;
  r = (r * 4) / 10;
  g = (g * 4) / 10;
  b = (b * 4) / 10;
  return RGB15(r, g, b);
}

// Ciclo de color muy lento sobre los 2 colores del patron (banco 1,
// indices 0 y 1 -- ver bg_pattern.c: 0x0425 el oscuro, 0x14B9 el
// claro): mantiene el mismo contraste oscuro/claro, pero el TONO va
// rotando de a poco por el espectro entero (~2 minutos por vuelta
// completa) — bastante lento para que no maree.
static void update_background_color_cycle(void) {
  static int accum = 0;
  static int hue = 0;
  accum++;
  if (accum % 20 != 0) return;
  hue = (hue + 1) % 360;

  u16 darkColor = hsv_to_rgb15(hue, 220, 45);
  u16 brightColor = hsv_to_rgb15(hue, 220, 205);

  // Mientras se ve la pantalla de fin de mano o la de titulo (arriba),
  // sus colores fijos no se tocan -- el ciclo de color sigue de largo
  // abajo.
  if (!showingScoreBg && !showingTitleBg) {
    BG_PALETTE[16] = darkColor;
    BG_PALETTE[17] = brightColor;
  }
  BG_PALETTE_SUB[16] = darkColor;
  BG_PALETTE_SUB[17] = brightColor;
}

// Se llama una vez por frame: mueve el fondo en diagonal, lento (1px
// cada varios frames, no todos), y envuelve solo gracias al hardware
// (el patron ya es del mismo tamano que la capa entera, 256x256). La
// pantalla de titulo usa su propio paneo vertical en vez de esto (ver
// title_bg_pan_offset) -- diagonal no sirve ahi porque el remolino no
// es un patron repetible.
static void update_background_scroll(void) {
  update_background_color_cycle();
  title_bg_tick();

  static int accum = 0;
  accum++;

  if (showingTitleBg) {
    bgSetScroll(bgPatternTop, 0, title_bg_pan_offset());
  } else if (!showingScoreBg && accum % 6 == 0) {
    int offset = accum / 6;
    bgSetScroll(bgPatternTop, offset, offset);
  }

  if (accum % 6 == 0) {
    bgSetScroll(bgPatternBottom, accum / 6, accum / 6);
  }

  bgUpdate();
}

// Reemplaza swiWaitForVBlank() en todo el resto del archivo: asi el
// fondo se sigue animando pase lo que pase (esperando un toque, en
// pausa mientras "piensa" un bot, etc.) — nunca se congela.
static void vsync(void) {
  swiWaitForVBlank();
  update_background_scroll();
  mmStreamUpdate(); // musica de fondo (streaming manual, ver setup_music_stream) -- se pide seguido en todo el juego
}

// ---------- Sprites: cartas de verdad (mano abajo, cruz de la baza arriba) ----------

// Un unico formato de carta, 32x32, para la mano Y la cruz de arriba
// (mismo criterio que el juego de referencia: sprite cuadrado, el look
// de carta rectangular sale del margen transparente dibujado adentro
// del arte fuente, no de un tamano de sprite distinto -- ver
// tools/prep-assets.ps1, Convert-CardTo32).
#define CARD_W 32
#define CARD_H 32
#define HAND_SLOT_COUNT MAX_HAND_SIZE
#define HAND_LIFT_PX 6 // cuanto "levanta" la carta con cursor D-pad
#define HAND_Y 36 // deja aire arriba: el borde visual real es HAND_Y-CARD_SCALE_PAD (ver mas abajo)
#define CARD_TILE_BYTES 1024 // 32x32 @ 8bpp
#define DRAG_CONFIRM_PX 16 // cuanto hay que arrastrar (arriba o abajo) para jugar directo, sin segundo toque

// Todas las cartas (mano Y cruz) se ven al DOBLE de grandes en pantalla
// sin generar arte nuevo -- se estira por hardware con una matriz
// afin. oamRotateScale pide el factor INVERSO (1.0 = intToFixed(1,8) =
// 256; para que se vea 2x mas grande hay que pasar la MITAD, 128 --
// ver libnds sprite.h, "sx the inverse scale factor"). Con afin activo
// Y sizeDouble=true el bounding box del sprite se duplica SIEMPRE
// (32x32 -> 64x64) para no recortar el contenido ampliado, asi que la
// posicion x/y que se le pasa a oamSet hay que correrla
// CARD_SCALE_PAD para arriba/izquierda para que el CENTRO visual quede
// donde estaba antes (32x32 sin escalar).
#define CARD_SCALE_INV 128
#define CARD_VISUAL_SIZE 64
#define CARD_SCALE_PAD ((CARD_VISUAL_SIZE - CARD_W) / 2)
#define CARD_AFFINE_ID 0 // un solo indice de matriz por motor, todas las cartas de esa pantalla comparten el mismo escalado

// Matriz APARTE (mismo motor -- oamSub -- indice distinto) solo para
// la carta con el cursor: mientras las demas quedan fijas en el 2x de
// siempre (CARD_AFFINE_ID), esta se anima independiente (punch al
// seleccionar, inclinacion al moverse) sin afectar al resto.
#define CARD_HERO_AFFINE_ID 2
#define CARD_PUNCH_AMOUNT 20 // cuanto se agranda de mas al seleccionar (se resta al inverso -> agranda)
#define CARD_TILT_KICK 1400 // angulo inicial del vaiven al moverse (unidad DS: 182 =~ 1 grado)

// ---------- Animacion con resorte (inspirado en como lo hace Balatro-GBA) ----------
// En vez de saltar de golpe a la posicion nueva, cada sprite tiene una
// posicion ACTUAL y una OBJETIVO en punto fijo (Q8.8 = *256, para que el
// movimiento sub-pixel se vea suave). Cada frame la velocidad se acerca
// al objetivo y se frena con un factor fijo (~0.7) — asi se "asienta"
// solo, sin necesidad de una curva de animacion escrita a mano.
#define ANIM_DAMP_NUM 179   // 179/256 =~ 0.7 de amortiguacion por frame
#define ANIM_DAMP_SHIFT 8
#define ANIM_EPSILON 4      // en unidades Q8.8 (=1/64 de pixel): umbral para clavar el valor final

typedef struct {
  int y;  // Q8.8, posicion actual
  int ty; // Q8.8, objetivo
  int vy; // Q8.8, velocidad
} SpriteAnim;

static void anim_snap(SpriteAnim* a, int px) {
  a->y = px << 8;
  a->ty = a->y;
  a->vy = 0;
}

static void anim_set_target(SpriteAnim* a, int px) {
  a->ty = px << 8;
}

static int anim_is_settled(const SpriteAnim* a) {
  return a->y == a->ty && a->vy == 0;
}

// Avanza un paso y devuelve la posicion actual en pixels normales.
static int anim_step(SpriteAnim* a) {
  if (anim_is_settled(a)) return a->y >> 8;
  a->vy += (a->ty - a->y) / 8;
  if (abs(a->vy) < ANIM_EPSILON && abs(a->ty - a->y) < ANIM_EPSILON) {
    a->vy = 0;
    a->y = a->ty;
  } else {
    a->vy = (a->vy * ANIM_DAMP_NUM) >> ANIM_DAMP_SHIFT;
    a->y += a->vy;
  }
  return a->y >> 8;
}

// Lee la posicion actual SIN avanzar el resorte (para dibujar en medio
// de un paso ya hecho por otra parte del codigo ese mismo frame).
static int anim_current(const SpriteAnim* a) { return a->y >> 8; }

// ---------- "Hero": punch al seleccionar + inclinacion al moverse ----------
// Solo la carta con el cursor usa esto (matriz CARD_HERO_AFFINE_ID); el
// resto de la mano sigue con el escalado 2x fijo de siempre. Los dos
// resortes (escala y angulo) arrancan en un valor "de golpe" (el punch/
// la inclinacion) y decaen solos a 0 -- exactamente el mismo resorte
// que ya se usa para posiciones, aplicado a otra cosa.
typedef struct {
  SpriteAnim scale; // "de mas" sobre el 2x normal, decae a 0
  SpriteAnim tilt;  // angulo (unidad DS), decae a 0
} CardHeroAnim;

static CardHeroAnim heroAnim;

static void hero_anim_reset(void) {
  anim_snap(&heroAnim.scale, 0);
  anim_snap(&heroAnim.tilt, 0);
}

// Se llama cuando el cursor pasa a una carta nueva (touch o D-pad).
static void hero_anim_punch(void) {
  anim_snap(&heroAnim.scale, CARD_PUNCH_AMOUNT);
  anim_set_target(&heroAnim.scale, 0);
}

// Se llama para un salto discreto (D-pad, un paso de carta a carta):
// dir -1 = izquierda, +1 = derecha.
static void hero_anim_tilt_kick(int dir) {
  anim_snap(&heroAnim.tilt, dir * CARD_TILT_KICK);
  anim_set_target(&heroAnim.tilt, 0);
}

// Vaiven CONTINUO mientras se arrastra: en vez de un solo "kick" que
// decae, el angulo objetivo se recalcula cada frame en base a la
// velocidad horizontal actual (cuanto se movio desde el frame
// anterior) -- se llame haya vecina para reordenar o no, la carta
// siempre reacciona a como la estas moviendo, como en la referencia.
// El resorte (con su propia inercia) sigue ese objetivo en vez de
// saltar de golpe, dando el balanceo continuo.
#define CARD_TILT_VEL_FACTOR 150 // cuanto se inclina por pixel de velocidad
#define CARD_TILT_MAX 2000       // tope del angulo, para que no gire de mas con arrastres bruscos
static void hero_anim_tilt_follow(int velocityX) {
  int target = velocityX * CARD_TILT_VEL_FACTOR;
  if (target > CARD_TILT_MAX) target = CARD_TILT_MAX;
  if (target < -CARD_TILT_MAX) target = -CARD_TILT_MAX;
  anim_set_target(&heroAnim.tilt, target);
}

// Un paso por frame: aplica el estado actual de los dos resortes a la
// matriz afin "hero". Barato de llamar siempre (si no hay nada
// animando, deja la matriz en el 2x/0 grados de siempre).
static void hero_anim_step_and_apply(void) {
  int scaleExtra = anim_step(&heroAnim.scale);
  int tiltAngle = anim_step(&heroAnim.tilt);
  int inv = CARD_SCALE_INV - scaleExtra;
  if (inv < 32) inv = 32; // limite de seguridad, nunca invertir el escalado
  oamRotateScale(&oamSub, CARD_HERO_AFFINE_ID, tiltAngle, inv, inv);
  oamUpdate(&oamSub);
}

// Mas juntas que el ancho VISUAL de carta (64px, ya escaladas 2x) a
// proposito -- se superponen un poco, como un abanico de cartas en la
// mano, pero no tanto (32px de paso, la mitad de la carta se tapa).
// Base calculada para que las 6 (HAND_SLOT_COUNT) queden centradas de
// verdad en los 256px de ancho: el ancho total que ocupan de punta a
// punta es (6-1)*32 + 64 (el visual de la ultima) = 224, entran 16px
// de margen libre a cada lado. OJO: hand_slot_x es la posicion LOGICA
// (pre-escalado); el borde visual real es hand_slot_x(i)-CARD_SCALE_PAD,
// por eso la base tiene que arrancar en 32 (= margen 16 + CARD_SCALE_PAD
// 16) y no en 16 -- ese offset de mas fue justo el bug que corria todo
// el abanico hacia un costado.
#define HAND_STRIDE 32
static int hand_slot_x(int i) { return HAND_STRIDE + i * HAND_STRIDE; }

// El id de OAM mas bajo queda ARRIBA en el area de solapamiento -- pero
// el numero de valor de la carta esta impreso del lado DERECHO del
// dibujo, asi que en general conviene que la carta de la DERECHA gane
// el solapamiento (si no, tapa justo el numero de la de al lado). PERO
// la carta con el CURSOR (D-pad o primer toque) tiene que ganar
// siempre, sin importar la posicion -- si no, una carta sin seleccionar
// que este mas a la derecha puede terminar tapando a la seleccionada y
// el "levantado" deja de verse. Se le reserva el id 0 (el mas al
// frente posible) a la carta con cursorIndex, y el resto de las
// cartas se acomodan alrededor con el mismo criterio de siempre
// (derecha gana), corridas para no chocar con ese id ya ocupado.
static int hand_oam_id(int i, int cursorIndex) {
  if (i == cursorIndex) return 0;
  int rank = HAND_SLOT_COUNT - 1 - i;
  if (cursorIndex < 0) return rank;
  int cursorRank = HAND_SLOT_COUNT - 1 - cursorIndex;
  return (rank < cursorRank) ? rank + 1 : rank;
}

// Cruz de la baza en la pantalla de arriba, posicion FIJA por id de
// jugador (mismo criterio que la version web: todas las cartas jugadas
// juntas en una cruz al centro de la mesa). Vos (id 0) siempre abajo.
// Centrada de verdad en (128,96): el CENTRO VISUAL de cada carta (no
// su ancla logica) es TRICK_SLOT_X+16 / TRICK_SLOT_Y+16 (por el
// CARD_SCALE_PAD del escalado 2x) -- izquierda y derecha tienen que
// quedar a la MISMA distancia de 128, arriba y abajo a la misma
// distancia de 96. Antes izquierda estaba a 96px del centro y derecha
// a solo 48px (se corrio en algun ajuste anterior sin volver a
// balancear) -- quedaba visiblemente asimetrico.
static const int TRICK_SLOT_X[4] = { 112, 40, 112, 184 };
static const int TRICK_SLOT_Y[4] = { 132, 80, 28, 80 };

// De que borde de la pantalla "vuela" la carta de cada asiento hacia
// su lugar en la cruz -- el de abajo (Vos) entra por abajo, el de la
// izquierda por la izquierda, etc. (coincide con TRICK_SLOT_X/Y: id0
// abajo, id1 izquierda, id2 arriba, id3 derecha).
typedef enum { ENTER_FROM_BOTTOM, ENTER_FROM_LEFT, ENTER_FROM_TOP, ENTER_FROM_RIGHT } TrickEnterDir;
static const TrickEnterDir TRICK_ENTER_DIR[4] = { ENTER_FROM_BOTTOM, ENTER_FROM_LEFT, ENTER_FROM_TOP, ENTER_FROM_RIGHT };

// Flecha en el CENTRO de la cruz que apunta a quien le toca jugar/
// predecir ahora (reemplaza la flechita chica que iba al lado del
// nombre en la esquina -- se pedia mas clara). Primer intento: UNA
// sola matriz afin rotando en tiempo real con oamRotateScale -- el
// usuario reporto que solo se veia apuntar izquierda/derecha, nunca
// arriba/abajo. En vez de perseguir ese bug, son 4 sprites YA
// rotados como arte estatico (ver make-arrow.ps1, arrowDirGfx mas
// abajo), se elige cual mostrar por indice -- sin matrices, sin
// angulos, sin sorpresas.
#define CENTER_ARROW_X 128
#define CENTER_ARROW_Y 96

// Esquinas de la pantalla de arriba (nombre + lo que pidio + cuantas se
// llevo esta mano), inspirado en la version web -- se ve durante
// predecir Y durante jugar/fin de baza, asi que la esquina de cada
// jugador tiene que coincidir con el LADO de la pantalla donde esta su
// asiento de verdad en la cruz (TRICK_SLOT_X/Y), si no confunde: antes
// el orden era fijo sin relacion espacial y el puntaje de "Vos" (abajo
// del todo en la cruz) terminaba en la esquina de ARRIBA a la
// izquierda. Cada asiento va a la esquina mas cercana que comparte su
// mismo borde (arriba/abajo/izquierda/derecha) -- caminando la mesa en
// sentido horario (arriba, derecha, abajo, izquierda) coincide
// exactamente con caminar las esquinas en sentido horario (arriba-
// izq, arriba-der, abajo-der, abajo-izq), asi que no hay que elegir
// arbitrariamente entre las dos esquinas que tocan cada lado.
// Un poco lejos de los bordes de verdad (antes quedaban pegados,
// row/col 0 o al maximo) -- 2 filas/columnas de aire (16px) de cada
// lado. OJO con el costado derecho: col 20 (no 18) es a proposito --
// es justo donde termina el borde derecho de la carta de ARRIBA de la
// cruz (TRICK_SLOT_X[2]=112, borde visual en x=160=columna 20), si se
// corre mas para adentro el nombre/iconos quedan tapados por esa
// carta (paso justo eso, "Fede" se veia como "ede").
static const int CORNER_ROW[4] = { 18, 18, 2, 2 };
static const int CORNER_COL[4] = { 20, 2, 2, 20 };

// Filas de fin de mano -- una por jugador, calzadas con los paneles de
// color de score_bg.png (PANEL_TOP, ver panelTops en
// make-score-bg.ps1 -- 40px de alto cada uno). TEXT_ROW es la fila de
// texto (nombre). Ya NO hay avatar (se saco el cuadrado placeholder)
// -- los corazones aprovechan ese espacio para verse mas grandes
// (1.5x, ver HAND_END_HEART_AFFINE_ID), centrados verticalmente
// dentro del panel.
static const int HAND_END_TEXT_ROW[4] = { 2, 8, 14, 20 };
static const int HAND_END_PANEL_TOP[4] = { 8, 56, 104, 152 };
#define HAND_END_PANEL_H 40
#define HAND_END_HEART_NATIVE 16
#define HAND_END_HEART_VISUAL 24 // 1.5x
// oamSet con sizeDouble no recentra nada por su cuenta -- la posicion
// visual va directo, sin sumar ni restar ningun padding (confirmado
// con el boton de jugar, ver PLAY_BUTTON_OAM_X -- la variante "centro
// menos mitad del nativo" que se probo antes acá hacia que los
// corazones se salieran del panel por abajo).
#define HAND_END_HEART_VISUAL_Y(panelIdx) (HAND_END_PANEL_TOP[panelIdx] + (HAND_END_PANEL_H - HAND_END_HEART_VISUAL) / 2)
#define HAND_END_HEART_VISUAL_X0 96 // deja lugar de sobra al nombre (8 caracteres = 64px, arranca en x=16)
#define HAND_END_HEART_VISUAL_STRIDE 24

static u16* handGfx[HAND_SLOT_COUNT];
// Posicion X animada de cada LUGAR de la mano (no de cada carta -- el
// contenido de un lugar puede cambiar al reordenar, pero el lugar en
// si sigue siendo el mismo sprite). Normalmente ya esta asentada en
// hand_slot_x(i); cuando un intercambio le cambia el contenido, se
// arranca desde donde estaba la vecina y se deja que llegue sola en
// vez de teletransportarse.
static SpriteAnim handSlideX[HAND_SLOT_COUNT];
static u16* trickGfx[4];
static u16* crownGfx;
static u16* arrowDirGfx[4]; // [right, down, left, up] -- ver make-arrow.ps1
static u16* leaderFrameGfx;
static u16* avatarGfx; // un solo placeholder, se dibuja en 4 posiciones (mismo contenido para los 4)
static u16* heartGfx[4 * 6]; // [jugador*6 + corazon] -- contenido lleno/roto se elige al dibujar
static u16* iconPredGfx; // objetivo -- reemplaza la "P" de "predijo" en las esquinas de puntaje
static u16* iconWonGfx; // trofeo -- reemplaza la "G" de "ganadas" en las esquinas de puntaje
static u16* titleLogoGfx[6]; // logo "BAZADO" letra por letra (B-A-Z-A-D-O), posicion fija, independiente del paneo del fondo
static u16* buttonGfx[7];
static u16* slingBandGfx; // punto de la "tanza", se repite en cadena entre el ancla fija y la carta
static u16* particleGfx; // festejo del ancho
static u16* anchoBannerGfx; // frase del festejo del ancho, como sprite (ver ANCHO_BANNER_*)
static u16* subHeartGfx[GAME_MAX_PENALTIES]; // vida, en la pantalla de ABAJO -- version propia (oamSub tiene su VRAM/paleta aparte de oamMain)
static u16* subIconPredGfx; // objetivo, pantalla de abajo
static u16* subIconWonGfx; // trofeo, pantalla de abajo
static u16* playButtonGfx; // boton "jugar/continuar" animado -- un solo buffer, se pisa con el frame que corresponda (ver render_play_button)
static u16* outcomeTextGfx; // fin de partida (arriba): un solo buffer, se pisa con "Ganaste"/"Perdiste" segun corresponda (ver show_outcome_screen)

#define CROWN_PALETTE_BANK 4 // banco de paleta 4bpp que no pisa los 58 colores (0..57) de las cartas grandes (8bpp)
#define CROWN_OAM_ID 4 // las 4 cartas de la cruz usan 0..3
#define ARROW_PALETTE_BANK 5 // banco propio, aparte del de la corona
#define ARROW_OAM_ID 5
#define CORNER_ICON_PALETTE_BANK 6 // banco que dejo libre la gota al sacarla
#define CORNER_ICON_PRED_OAM_BASE 38 // 38..41, uno por esquina de jugador
#define CORNER_ICON_WON_OAM_BASE 42 // 42..45, idem
#define TITLE_LOGO_PALETTE_BANK 10
#define TITLE_LOGO_LETTER_COUNT 6 // B-A-Z-A-D-O
#define TITLE_LOGO_OAM_BASE 56 // 56..61 -- reubicado (46..49 pisaba a PARTICLE_OAM_BASE=50..55 al pasar de 4 a 6 piezas)
#define TITLE_LOGO_Y 64 // misma altura relativa que tenia horneado en el remolino viejo

// Cada letra es su propio sprite (64x64, pegada contra el borde
// izquierdo del lienzo -- ver make-title-letters.ps1) en vez de la
// palabra entera repartida en 4 piezas parejas de antes. Las letras
// tienen un ANCHO real bien distinto entre si (31-37px) y el sprite
// entero (64px) tiene de sobra relleno magenta a la derecha, asi que
// la posicion en pantalla de cada una se calcula a mano (medido una
// vez, no cambia): ancho real de cada letra + 3px de aire entre una y
// la siguiente, centrado el conjunto en las 256 columnas.
static const int TITLE_LOGO_LETTER_X[TITLE_LOGO_LETTER_COUNT] = { 17, 53, 92, 126, 165, 201 };

// Animacion de entrada/salida del logo (una por letra, ver
// render_title_logo/hide_title_logo mas abajo): cada letra cae desde
// arriba con una demora escalonada entre una y otra, y al llegar hace
// un squash breve (se aplasta un poco en Y y vuelve) para sentir que
// "se acomoda"; al salir suben derecho hacia arriba, mismo escalonado,
// sin squash. El squash necesita una matriz afin PROPIA por letra
// (banco 3..8, oamMain: 0 cartas, 1 banner ancho, 2 corazon fin de
// mano) -- fuera de esas pocas cuadros de squash, las letras se
// dibujan SIN afin, en su posicion final de siempre (identico a como
// se veian antes de esto).
#define TITLE_LOGO_AFFINE_BASE 3 // 3..8, una por letra
#define TITLE_LOGO_DROP_FROM_Y -80 // arranca arriba de la pantalla, fuera de vista
#define TITLE_LOGO_DROP_FRAMES 16
#define TITLE_LOGO_SQUASH_FRAMES 10
#define TITLE_LOGO_STAGGER_FRAMES 6 // demora entre el arranque de una pieza y la siguiente
#define PARTICLE_PALETTE_BANK 11 // oamMain -- festejo del ancho
#define PARTICLE_OAM_BASE 50 // 50..55
#define PARTICLE_COUNT 6

// Banner de la frase del festejo (arte, no texto de consola -- ver
// celebrate_ancho): mas ancho que una carta (128x64 contra los 64x64
// de la carta en pantalla) y SIEMPRE por encima de todo -- prioridad 0
// como la carta/marco de lider/flecha, pero durante el festejo esos
// dos ultimos se esconden (ver celebrate_ancho) para que nada le pueda
// ganar el empate de prioridad.
// OJO: "double size" (sizeDouble) SOLO da el doble del tamano nativo de
// area de recorte -- se probo a 3x (SCALE_INV=85) y el texto quedaba
// CORTADO en los bordes porque el escalado real se pasaba de esa area
// doblada. Maximo seguro: 2x (SCALE_INV=128), igual que todos los
// demas sprites afines del juego.
#define ANCHO_BANNER_PALETTE_BANK 12
#define ANCHO_BANNER_OAM_ID 8 // oamMain: 6,7,8 libres (particulas 50-55, resto ver arriba)
#define ANCHO_BANNER_AFFINE_ID 1 // oamMain solo usa 0 (cartas) hasta ahora
#define ANCHO_BANNER_NATIVE_W 64
#define ANCHO_BANNER_NATIVE_H 32
#define ANCHO_BANNER_SCALE_INV 128 // 2x
#define ANCHO_BANNER_VISUAL_W 128
#define ANCHO_BANNER_VISUAL_H 64
#define ANCHO_BANNER_VISUAL_X ((256 - ANCHO_BANNER_VISUAL_W) / 2)
#define ANCHO_BANNER_VISUAL_Y ((192 - ANCHO_BANNER_VISUAL_H) / 2)
// oamSet con sizeDouble no recentra nada por su cuenta -- ver el mismo
// comentario en PLAY_BUTTON_OAM_X (confirmado con ese boton, mismo
// tamano nativo 64x32 y misma escala 2x: la posicion visual va
// directo, sin sumar ni restar ningun padding).
#define ANCHO_BANNER_OAM_X ANCHO_BANNER_VISUAL_X
#define ANCHO_BANNER_OAM_Y ANCHO_BANNER_VISUAL_Y
#define LEADER_FRAME_PALETTE_BANK 9
#define LEADER_FRAME_OAM_ID 9
#define AVATAR_PALETTE_BANK 7
#define HEART_PALETTE_BANK 8
#define AVATAR_OAM_BASE 10 // 10..13, uno por jugador -- reservado pero ya no se dibuja (ver render_hand_end_rows)
#define HEART_OAM_BASE 14 // 14..37, 4 jugadores x 6 corazones -- solo se usan en fin de mano
#define HAND_END_HEART_AFFINE_ID 2 // oamMain: 0 cartas, 1 banner ancho
#define HAND_END_HEART_SCALE_INV 171 // 1.5x (256/1.5)
#define BUTTON_PALETTE_BANK 4 // idem, en la paleta de ABAJO (58 colores de la mano)
#define BUTTON_PALETTE_BANK_DIM 9 // version oscura, para el numero que no se puede elegir (prohibido)
#define BUTTON_OAM_BASE 6 // la mano de abajo usa ids 0..5 (HAND_SLOT_COUNT), botones 6..12
#define BUTTON_SIZE 16
#define BUTTON_Y 120 // deja aire debajo del titulo -- ver PREDICT_HEADING_ROW

// Resortera (oamSub, pantalla de abajo) -- ids 13 en adelante, libres
// despues de la mano (0..5) y los botones (6..12).
#define SLING_PALETTE_BANK 11
#define SLING_BAND_SEGMENTS 8 // puntitos de la tanza, de la punta fija hasta la carta
#define SLING_BAND_OAM_BASE 14 // 14..21
// Ancla ARRIBA de la pantalla de abajo (cerca de la costura con la de
// arriba), no abajo del todo -- las cartas jugadas salen volando hacia
// ARRIBA (a la cruz de la pantalla de arriba), asi que estirar hacia
// abajo desde un ancla de arriba es lo que se siente intuitivo al
// soltar (tira para el lado contrario de donde se estira, que es para
// arriba, justo donde tiene que ir la carta). Con el ancla abajo era
// al reves: se sentia que iba a salir disparada para abajo.
#define SLING_ANCHOR_X 128
#define SLING_ANCHOR_Y 6

// Barra de estado de ABAJO (vida/pedidas/ganadas, siempre visible, no
// solo en el propio turno) -- ids 22 en adelante en oamSub, libres
// despues de la tanza (14..21). Corazones e iconos NO comparten
// paleta entre si (son artes distintos), asi que van en bancos
// separados.
#define STATUS_HEART_PALETTE_BANK 12
#define STATUS_ICON_PALETTE_BANK 13
#define STATUS_HEART_OAM_BASE 22 // 22..21+GAME_MAX_PENALTIES
#define STATUS_PRED_ICON_OAM_ID (STATUS_HEART_OAM_BASE + GAME_MAX_PENALTIES)
#define STATUS_WON_ICON_OAM_ID (STATUS_PRED_ICON_OAM_ID + 1)
// Iconos de prediccion/ganadas en la fila de ARRIBA, corazones en la de
// ABAJO (al reves de como estaba) -- corazones, iconos Y el numero de
// al lado se ven todos al doble ahora (mismo truco de matriz afin que
// las cartas/botones) -- se pedian mas grandes, se veian chicos
// comparados con el resto. El numero ya NO es texto de consola (no se
// puede escalar un caracter suelto) -- se dibuja con el mismo sprite
// de digito 0-6 que ya existe para los botones de prediccion
// (buttonGfx, ver mas abajo), reusado tal cual sin copiar tiles de
// nuevo.
#define STATUS_ICON_Y 96 // separada de los corazones (STATUS_HEART_VISUAL_Y=136) para que no se pisen -- 128 quedaba encimado
#define STATUS_PRED_NUM_OAM_ID 31
#define STATUS_WON_NUM_OAM_ID 32

#define STATUS_ICON_AFFINE_ID 5 // oamSub: 0 cartas, 1 botones numero, 2 carta hero, 3 boton jugar, 4 corazones
#define STATUS_ICON_SCALE_INV 128 // 2x
#define STATUS_ICON_NATIVE 16
#define STATUS_ICON_VISUAL 32
#define STATUS_ICON_GAP 6 // aire (ya visual) entre el icono y su numero
#define STATUS_GROUP_GAP 24 // aire (ya visual) entre el grupo de prediccion y el de ganadas
// Grupo = icono + numero, 32+6+32=70px visuales; los 2 grupos + el aire
// entre ellos (70*2+24=164) quedan centrados en las 256 columnas (46 de
// margen a cada lado).
#define STATUS_PRED_ICON_VISUAL_X 46
#define STATUS_PRED_NUM_VISUAL_X (STATUS_PRED_ICON_VISUAL_X + STATUS_ICON_VISUAL + STATUS_ICON_GAP)
#define STATUS_WON_ICON_VISUAL_X (STATUS_PRED_NUM_VISUAL_X + STATUS_ICON_VISUAL + STATUS_GROUP_GAP)
#define STATUS_WON_NUM_VISUAL_X (STATUS_WON_ICON_VISUAL_X + STATUS_ICON_VISUAL + STATUS_ICON_GAP)

#define STATUS_HEART_SCALE_INV 128 // 2x
#define STATUS_HEART_NATIVE 16
#define STATUS_HEART_VISUAL 32
#define STATUS_HEART_AFFINE_ID 4 // oamSub ya usa 0 (cartas), 1 (botones numero), 2 (carta hero), 3 (boton jugar)
// Posiciones ya VISUALES (como button_x) -- 6 corazones de 32px
// visuales con 40px de paso, de x=8 a x=240, sin superponerse (8px de
// aire entre uno y el siguiente) ni salirse de las 256 columnas.
#define STATUS_HEART_VISUAL_Y 136 // fila 17 -- corazones, debajo de los iconos
#define STATUS_HEART_VISUAL_X0 8
#define STATUS_HEART_VISUAL_STRIDE 40

// Los botones tambien se ven al doble (mismo truco de matriz afin que
// las cartas, indice DISTINTO -- BUTTON_AFFINE_ID -- para no pisar el
// de las cartas en el mismo motor/oamSub). BUTTON_GAP es el espacio
// entre bordes ya VISUALES (escalados), no logicos, para que los 7
// boxes queden separados de verdad y no se toquen.
#define BUTTON_SCALE_INV 128
#define BUTTON_VISUAL_SIZE 32
#define BUTTON_SCALE_PAD ((BUTTON_VISUAL_SIZE - BUTTON_SIZE) / 2)
#define BUTTON_AFFINE_ID 1
#define BUTTON_GAP 4

static int button_x(int i) {
  return (BUTTON_SCALE_PAD + 4) + i * (BUTTON_VISUAL_SIZE + BUTTON_GAP);
}

static const unsigned int* const BUTTON_TILES[7] = {
  button_0Tiles, button_1Tiles, button_2Tiles, button_3Tiles,
  button_4Tiles, button_5Tiles, button_6Tiles,
};

// Boton "jugar/continuar" animado (pantalla de titulo, fin de mano, fin
// de partida -- reemplaza el texto "toca/apreta START" de las tres, ver
// wait_for_play_button). 4 frames del usuario: idle, apretado, rebote,
// vuelta a idle. oamSub, id 30 (libre: la barra de estado termina en
// STATUS_WON_ICON_OAM_ID=29) y banco de paleta 6 (libre en la paleta de
// ABAJO -- el 6 solo esta ocupado en oamMain, paletas independientes).
#define PLAY_BUTTON_PALETTE_BANK 6
#define PLAY_BUTTON_OAM_ID 30
#define PLAY_BUTTON_AFFINE_ID 3 // oamSub ya usa 0 (cartas), 1 (botones numero), 2 (carta hero)
#define PLAY_BUTTON_NATIVE_W 64
#define PLAY_BUTTON_NATIVE_H 32
#define PLAY_BUTTON_SCALE_INV 128 // 2x, mismo truco de matriz afin que las cartas/botones
#define PLAY_BUTTON_VISUAL_W 128
#define PLAY_BUTTON_VISUAL_H 64
// Centrado horizontal en las 256 columnas de la pantalla de abajo; fila
// donde antes iba el texto de "presione para continuar"/"toca START".
#define PLAY_BUTTON_VISUAL_X ((256 - PLAY_BUTTON_VISUAL_W) / 2)
#define PLAY_BUTTON_VISUAL_Y 80
// oamSet con sizeDouble NO recentra nada por su cuenta -- el X/Y que
// se le pasa es directamente el top-left del area YA agrandada (se
// probaron las otras dos variantes -- restar el padding, y restar la
// mitad del nativo -- y las dos quedaron corridas, una para cada lado,
// confirmando que la posicion visual va SIN ningun ajuste).
#define PLAY_BUTTON_OAM_X PLAY_BUTTON_VISUAL_X
#define PLAY_BUTTON_OAM_Y PLAY_BUTTON_VISUAL_Y
#define PLAY_BUTTON_FRAME_HOLD 4 // cuadros que dura cada frame de la animacion de apretado (a 60fps)

static const unsigned int* const PLAY_BUTTON_TILES[4] = {
  play_button_0Tiles, play_button_1Tiles, play_button_2Tiles, play_button_3Tiles,
};
static const unsigned int PLAY_BUTTON_TILES_LEN[4] = {
  play_button_0TilesLen, play_button_1TilesLen, play_button_2TilesLen, play_button_3TilesLen,
};

// Pantalla de fin de partida (arriba): SOLO el texto ("Ganaste"/
// "Perdiste"), sin icono (se probo con un icono de usuario arriba
// tambien pero se saco del todo, ver make-outcome-screen.ps1), sobre
// el fondo de siempre (con su paneo -- ver restore_pattern_background)
// en vez del panel de colores de fin de mano. Banco de paleta propio
// (13), id de sprite bien alto (63) para no pisar nada de lo ya usado
// en oamMain.
#define OUTCOME_SCREEN_PALETTE_BANK 13
#define OUTCOME_TEXT_OAM_ID 63
// El texto nativo (36x10/36x11) se ve minusculo a 1x -- se agranda 2x
// con afin (mismo truco que el boton de jugar: sizeDouble reserva
// EXACTO el doble del nativo cuando el escalado es 2x, asi que no pasa
// nada de clipping, y la posicion visual va DIRECTA, sin ningun offset
// -- formula validada esa vez para sprites anchos como este). El
// propio PNG ya viene agrandado para llenar casi todo el lienzo
// nativo (ver make-outcome-screen.ps1), asi que este 2x es la segunda
// vuelta de agrandado, no la unica.
#define OUTCOME_TEXT_NATIVE_W 64
#define OUTCOME_TEXT_NATIVE_H 32 // no existe SpriteSize_64x16 en hardware, se uso 64x32 (ver make-outcome-screen.ps1)
#define OUTCOME_TEXT_SCALE_INV 128 // 2x
#define OUTCOME_TEXT_VISUAL_W (OUTCOME_TEXT_NATIVE_W * 2)
#define OUTCOME_TEXT_VISUAL_H (OUTCOME_TEXT_NATIVE_H * 2)
#define OUTCOME_TEXT_AFFINE_ID 9 // oamMain: 0 cartas, 1 banner ancho, 2 corazones fin de mano, 3..8 letras del logo
#define OUTCOME_TEXT_X ((256 - OUTCOME_TEXT_VISUAL_W) / 2)
#define OUTCOME_TEXT_Y ((192 - OUTCOME_TEXT_VISUAL_H) / 2)

static void setup_sprites(void) {
  vramSetBankE(VRAM_E_MAIN_SPRITE);
  vramSetBankD(VRAM_D_SUB_SPRITE);

  oamInit(&oamMain, SpriteMapping_1D_32, false);
  oamInit(&oamSub, SpriteMapping_1D_32, false);

  // Matriz de escalado 2x para las cartas, una por motor (todas las
  // cartas de esa pantalla la comparten -- angulo 0, mismo factor en X
  // e Y). La corona no es afin y queda con su tamano normal.
  oamRotateScale(&oamMain, CARD_AFFINE_ID, 0, CARD_SCALE_INV, CARD_SCALE_INV);
  oamRotateScale(&oamSub, CARD_AFFINE_ID, 0, CARD_SCALE_INV, CARD_SCALE_INV);
  // Matriz aparte (mismo motor, indice distinto) para los botones.
  oamRotateScale(&oamSub, BUTTON_AFFINE_ID, 0, BUTTON_SCALE_INV, BUTTON_SCALE_INV);
  // Y otra mas para la carta "hero" (la del cursor) -- arranca igual
  // que las demas (2x, sin inclinar), el resorte la anima despues.
  oamRotateScale(&oamSub, CARD_HERO_AFFINE_ID, 0, CARD_SCALE_INV, CARD_SCALE_INV);
  hero_anim_reset();
  // Y una mas para el boton de jugar/continuar (2x fijo, sin inclinar).
  oamRotateScale(&oamSub, PLAY_BUTTON_AFFINE_ID, 0, PLAY_BUTTON_SCALE_INV, PLAY_BUTTON_SCALE_INV);
  // Y otra mas para los corazones de la barra de estado (2x fijo).
  oamRotateScale(&oamSub, STATUS_HEART_AFFINE_ID, 0, STATUS_HEART_SCALE_INV, STATUS_HEART_SCALE_INV);
  // Y otra para los iconos de prediccion/ganadas y su numero (2x fijo).
  oamRotateScale(&oamSub, STATUS_ICON_AFFINE_ID, 0, STATUS_ICON_SCALE_INV, STATUS_ICON_SCALE_INV);
  // Y otra mas (oamMain esta vez) para el banner del festejo del ancho (~3x fijo).
  oamRotateScale(&oamMain, ANCHO_BANNER_AFFINE_ID, 0, ANCHO_BANNER_SCALE_INV, ANCHO_BANNER_SCALE_INV);
  // Y otra para los corazones del panel de fin de mano (1.5x fijo).
  oamRotateScale(&oamMain, HAND_END_HEART_AFFINE_ID, 0, HAND_END_HEART_SCALE_INV, HAND_END_HEART_SCALE_INV);
  // Y 6 mas para el squash de entrada de las letras del logo -- arrancan
  // en 1x normal (256,256), se tocan en vivo durante el squash nomas
  // (ver render_title_logo).
  for (int i = 0; i < TITLE_LOGO_LETTER_COUNT; i++) {
    oamRotateScale(&oamMain, TITLE_LOGO_AFFINE_BASE + i, 0, 256, 256);
  }
  // Y otra para el texto de fin de partida ("Ganaste"/"Perdiste"), 2x fijo.
  oamRotateScale(&oamMain, OUTCOME_TEXT_AFFINE_ID, 0, OUTCOME_TEXT_SCALE_INV, OUTCOME_TEXT_SCALE_INV);

  // Mismo set de cartas (32x32) para arriba y para abajo -- cada motor
  // (Main/Sub) tiene su propia memoria de paleta de sprites, asi que
  // igual hace falta copiar los datos dos veces, una por lado.
  dmaCopy(cards_sharedPal, SPRITE_PALETTE, cards_sharedPalLen);
  dmaCopy(cards_sharedPal, SPRITE_PALETTE_SUB, cards_sharedPalLen);

  // La corona es 4bpp (16 colores) — convive en la MISMA memoria de
  // paleta que las cartas (8bpp) porque usa un banco de 16 colores
  // aparte (el 4) que no se pisa con los indices que ya ocupan las
  // cartas. OJO: crownPalLen son 512 bytes (256 colores) — grit rellena
  // el array entero aunque la corona solo use un puñado de colores
  // reales; copiar todo se pasaria del final de SPRITE_PALETTE (mismo
  // bug que ya paso una vez con el fondo). Un banco entero (32 bytes)
  // alcanza de sobra.
  dmaCopy(crownPal, &SPRITE_PALETTE[CROWN_PALETTE_BANK * 16], 32);

  // Flecha de turno (las 4 direcciones comparten paleta -- son la
  // misma flecha, solo rotada -- banco propio (5) para no pisarla a
  // la corona).
  dmaCopy(arrow_sharedPal, &SPRITE_PALETTE[ARROW_PALETTE_BANK * 16], arrow_sharedPalLen);

  // Marco verde de la carta lider: idem, banco propio (9).
  dmaCopy(leader_framePal, &SPRITE_PALETTE[LEADER_FRAME_PALETTE_BANK * 16], 32);

  // Avatar (fin de mano): OJO -- este si viene con el relleno de 256
  // colores sin comprimir (no es -pS), asi que igual que la corona
  // solo se copia UN banco (32 bytes), no avatar_placeholderPalLen
  // entero.
  dmaCopy(avatar_placeholderPal, &SPRITE_PALETTE[AVATAR_PALETTE_BANK * 16], 32);

  // Corazones (fin de mano): estos si son -pS (paleta compartida entre
  // lleno/roto), hearts_sharedPalLen ya viene bien chico.
  dmaCopy(hearts_sharedPal, &SPRITE_PALETTE[HEART_PALETTE_BANK * 16], hearts_sharedPalLen);

  // Iconos de esquina (objetivo/trofeo): tambien -pS, banco propio (el
  // que dejo libre la gota).
  dmaCopy(corner_icons_sharedPal, &SPRITE_PALETTE[CORNER_ICON_PALETTE_BANK * 16], corner_icons_sharedPalLen);

  // Logo "BAZADO" de la pantalla de titulo: banco propio.
  dmaCopy(title_letters_sharedPal, &SPRITE_PALETTE[TITLE_LOGO_PALETTE_BANK * 16], title_letters_sharedPalLen);

  // Particulas del festejo del ancho: banco propio (11). OJO -- esta
  // si viene con el relleno de 256 colores sin comprimir (no es -pS),
  // solo se copia UN banco.
  dmaCopy(particlePal, &SPRITE_PALETTE[PARTICLE_PALETTE_BANK * 16], 32);

  // Banner del festejo del ancho: banco propio, oamMain (arriba, donde
  // esta la cruz de la baza).
  dmaCopy(ancho_banner_sharedPal, &SPRITE_PALETTE[ANCHO_BANNER_PALETTE_BANK * 16], ancho_banner_sharedPalLen);

  // Tanza, en la paleta de ABAJO (donde vive la mano) -- un solo
  // archivo (no -pS), asi que igual que la corona solo se copia UN
  // banco (32 bytes), no sling_bandPalLen entero (viene con el
  // relleno de 256 colores sin comprimir).
  dmaCopy(sling_bandPal, &SPRITE_PALETTE_SUB[SLING_PALETTE_BANK * 16], 32);

  // Barra de estado de abajo: mismos artes que arriba (corazones e
  // iconos de esquina), pero cargados de nuevo en la paleta de ABAJO
  // (oamSub tiene su propia VRAM/paleta, independiente de oamMain).
  dmaCopy(hearts_sharedPal, &SPRITE_PALETTE_SUB[STATUS_HEART_PALETTE_BANK * 16], hearts_sharedPalLen);
  dmaCopy(corner_icons_sharedPal, &SPRITE_PALETTE_SUB[STATUS_ICON_PALETTE_BANK * 16], corner_icons_sharedPalLen);

  // Los botones son 4bpp tambien, banco aparte en la paleta de ABAJO
  // (donde vive la mano). buttons_sharedPalLen ya viene bien chico (8
  // bytes = 4 colores reales, no el relleno a 256 que tuvo el bug del
  // fondo), asi que se puede copiar entero sin problema.
  dmaCopy(buttons_sharedPal, &SPRITE_PALETTE_SUB[BUTTON_PALETTE_BANK * 16], buttons_sharedPalLen);

  // Version oscura de esos mismos colores, en un banco aparte -- el
  // boton prohibido usa este banco en vez del normal, mismo tile.
  for (unsigned int i = 0; i < buttons_sharedPalLen / 2; i++) {
    SPRITE_PALETTE_SUB[BUTTON_PALETTE_BANK_DIM * 16 + i] = dim_rgb15(buttons_sharedPal[i]);
  }

  // Boton de jugar/continuar: banco propio, tambien en la paleta de ABAJO.
  dmaCopy(play_button_sharedPal, &SPRITE_PALETTE_SUB[PLAY_BUTTON_PALETTE_BANK * 16], play_button_sharedPalLen);

  // Pantalla de fin de partida (arriba): banco propio, comparte paleta
  // entre "Ganaste"/"Perdiste" -- ver show_outcome_screen, que pisa el
  // contenido de este buffer segun corresponda (mismo truco que
  // playButtonGfx con los frames).
  dmaCopy(outcome_text_sharedPal, &SPRITE_PALETTE[OUTCOME_SCREEN_PALETTE_BANK * 16], outcome_text_sharedPalLen);

  for (int i = 0; i < HAND_SLOT_COUNT; i++) {
    handGfx[i] = oamAllocateGfx(&oamSub, SpriteSize_32x32, SpriteColorFormat_256Color);
  }
  for (int i = 0; i < 4; i++) {
    trickGfx[i] = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_256Color);
  }
  crownGfx = oamAllocateGfx(&oamMain, SpriteSize_32x32, SpriteColorFormat_16Color);
  dmaCopy(crownTiles, crownGfx, crownTilesLen);

  arrowDirGfx[0] = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_16Color);
  dmaCopy(arrow_rightTiles, arrowDirGfx[0], arrow_rightTilesLen);
  arrowDirGfx[1] = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_16Color);
  dmaCopy(arrow_downTiles, arrowDirGfx[1], arrow_downTilesLen);
  arrowDirGfx[2] = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_16Color);
  dmaCopy(arrow_leftTiles, arrowDirGfx[2], arrow_leftTilesLen);
  arrowDirGfx[3] = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_16Color);
  dmaCopy(arrow_upTiles, arrowDirGfx[3], arrow_upTilesLen);

  leaderFrameGfx = oamAllocateGfx(&oamMain, SpriteSize_64x64, SpriteColorFormat_16Color);
  dmaCopy(leader_frameTiles, leaderFrameGfx, leader_frameTilesLen);

  avatarGfx = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_16Color);
  dmaCopy(avatar_placeholderTiles, avatarGfx, avatar_placeholderTilesLen);

  for (int i = 0; i < 4 * 6; i++) {
    heartGfx[i] = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_16Color);
  }

  iconPredGfx = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_16Color);
  dmaCopy(icon_predTiles, iconPredGfx, icon_predTilesLen);
  iconWonGfx = oamAllocateGfx(&oamMain, SpriteSize_16x16, SpriteColorFormat_16Color);
  dmaCopy(icon_wonTiles, iconWonGfx, icon_wonTilesLen);

  titleLogoGfx[0] = oamAllocateGfx(&oamMain, SpriteSize_64x64, SpriteColorFormat_16Color);
  dmaCopy(title_letter_0Tiles, titleLogoGfx[0], title_letter_0TilesLen);
  titleLogoGfx[1] = oamAllocateGfx(&oamMain, SpriteSize_64x64, SpriteColorFormat_16Color);
  dmaCopy(title_letter_1Tiles, titleLogoGfx[1], title_letter_1TilesLen);
  titleLogoGfx[2] = oamAllocateGfx(&oamMain, SpriteSize_64x64, SpriteColorFormat_16Color);
  dmaCopy(title_letter_2Tiles, titleLogoGfx[2], title_letter_2TilesLen);
  titleLogoGfx[3] = oamAllocateGfx(&oamMain, SpriteSize_64x64, SpriteColorFormat_16Color);
  dmaCopy(title_letter_3Tiles, titleLogoGfx[3], title_letter_3TilesLen);
  titleLogoGfx[4] = oamAllocateGfx(&oamMain, SpriteSize_64x64, SpriteColorFormat_16Color);
  dmaCopy(title_letter_4Tiles, titleLogoGfx[4], title_letter_4TilesLen);
  titleLogoGfx[5] = oamAllocateGfx(&oamMain, SpriteSize_64x64, SpriteColorFormat_16Color);
  dmaCopy(title_letter_5Tiles, titleLogoGfx[5], title_letter_5TilesLen);

  for (int i = 0; i < 7; i++) {
    buttonGfx[i] = oamAllocateGfx(&oamSub, SpriteSize_16x16, SpriteColorFormat_16Color);
    dmaCopy(BUTTON_TILES[i], buttonGfx[i], 128);
  }

  particleGfx = oamAllocateGfx(&oamMain, SpriteSize_8x8, SpriteColorFormat_16Color);
  dmaCopy(particleTiles, particleGfx, particleTilesLen);

  anchoBannerGfx = oamAllocateGfx(&oamMain, SpriteSize_64x32, SpriteColorFormat_16Color);
  dmaCopy(ancho_banner_0Tiles, anchoBannerGfx, ancho_banner_0TilesLen);

  slingBandGfx = oamAllocateGfx(&oamSub, SpriteSize_8x8, SpriteColorFormat_16Color);
  dmaCopy(sling_bandTiles, slingBandGfx, sling_bandTilesLen);

  for (int i = 0; i < GAME_MAX_PENALTIES; i++) {
    subHeartGfx[i] = oamAllocateGfx(&oamSub, SpriteSize_16x16, SpriteColorFormat_16Color);
  }
  subIconPredGfx = oamAllocateGfx(&oamSub, SpriteSize_16x16, SpriteColorFormat_16Color);
  dmaCopy(icon_predTiles, subIconPredGfx, icon_predTilesLen);
  subIconWonGfx = oamAllocateGfx(&oamSub, SpriteSize_16x16, SpriteColorFormat_16Color);
  dmaCopy(icon_wonTiles, subIconWonGfx, icon_wonTilesLen);

  playButtonGfx = oamAllocateGfx(&oamSub, SpriteSize_64x32, SpriteColorFormat_16Color);
  dmaCopy(play_button_0Tiles, playButtonGfx, play_button_0TilesLen);

  // Pantalla de fin de partida: buffer reservado, el contenido se
  // copia recien al mostrarla (ver show_outcome_screen).
  outcomeTextGfx = oamAllocateGfx(&oamMain, SpriteSize_64x32, SpriteColorFormat_16Color);
}

// Solo hay que llamar esto fuera de la pantalla de predecir: los
// botones son sprites de verdad, si no se esconden a mano se quedan
// viendose (ej. durante "esta pensando" de un bot, o ya jugando cartas).
static void hide_predict_buttons(void) {
  for (int v = 0; v < 7; v++) {
    oamSet(&oamSub, BUTTON_OAM_BASE + v, 0, 0, 0, BUTTON_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
      buttonGfx[v], -1, false, true, false, false, false);
  }
  oamUpdate(&oamSub);
}

// Tanza: NO es una hondera de caricatura (se probo con una horqueta de
// madera y no gustaba) -- es solo un hilo tenso, una cadena de
// puntitos chicos entre un punto fijo invisible abajo de la pantalla
// (SLING_ANCHOR_X/Y) y la carta arrastrada ahora mismo (cardX/cardY,
// el mismo centro visual que ya usa update_dragged_card_position). Se
// llama SOLO mientras se arrastra.
static void render_slingshot(int cardX, int cardY) {
  for (int i = 0; i < SLING_BAND_SEGMENTS; i++) {
    int t = i + 1; // 1..SEGMENTS -- excluye el ancla (t=0)
    int x = SLING_ANCHOR_X + (cardX - SLING_ANCHOR_X) * t / (SLING_BAND_SEGMENTS + 1);
    int y = SLING_ANCHOR_Y + (cardY - SLING_ANCHOR_Y) * t / (SLING_BAND_SEGMENTS + 1);
    oamSet(&oamSub, SLING_BAND_OAM_BASE + i, x - 4, y - 4, 0, SLING_PALETTE_BANK,
      SpriteSize_8x8, SpriteColorFormat_16Color, slingBandGfx, -1, false, false, false, false, false);
  }
  oamUpdate(&oamSub);
}

static void hide_slingshot(void) {
  for (int i = 0; i < SLING_BAND_SEGMENTS; i++) {
    oamSet(&oamSub, SLING_BAND_OAM_BASE + i, 0, 0, 0, SLING_PALETTE_BANK, SpriteSize_8x8, SpriteColorFormat_16Color,
      slingBandGfx, -1, false, true, false, false, false);
  }
  oamUpdate(&oamSub);
}

// Deja los "lugares" de la mano asentados en su posicion normal, sin
// ningun lerp pendiente -- se llama despues de repartir y despues de
// jugar una carta (ahi los indices se corren, y no queremos que eso
// tambien se vea como un reordenamiento arrastrado a mano).
static void hand_slide_snap_all(void) {
  for (int i = 0; i < HAND_SLOT_COUNT; i++) anim_snap(&handSlideX[i], hand_slot_x(i));
}

// cursorIndex -1 = ninguna carta resaltada (durante predecir, por ej. —
// ahi el cursor es sobre un NUMERO, no una carta). Usa la posicion
// ACTUAL de handSlideX (que puede estar en medio de un lerp por un
// reordenamiento) en vez de saltar directo a hand_slot_x(i).
static void render_hand_sprites(const GameState* state, int cursorIndex) {
  const Player* p = &state->players[HUMAN_ID];
  for (int i = 0; i < HAND_SLOT_COUNT; i++) {
    if (i >= p->handCount) {
      oamSet(&oamSub, hand_oam_id(i, cursorIndex), 0, 0, 0, 0, SpriteSize_32x32, SpriteColorFormat_256Color,
        handGfx[i], -1, false, true, false, false, false);
      continue;
    }
    Card c = p->hand[i];
    dmaCopy(card_tiles_for(c.suit, c.value), handGfx[i], CARD_TILE_BYTES);
    anim_set_target(&handSlideX[i], hand_slot_x(i));
    int x = anim_current(&handSlideX[i]);
    int y = HAND_Y - (i == cursorIndex ? HAND_LIFT_PX : 0);
    int affine = (i == cursorIndex) ? CARD_HERO_AFFINE_ID : CARD_AFFINE_ID;
    oamSet(&oamSub, hand_oam_id(i, cursorIndex), x - CARD_SCALE_PAD, y - CARD_SCALE_PAD, 0, 0, SpriteSize_32x32, SpriteColorFormat_256Color,
      handGfx[i], affine, true, false, false, false, false);
  }
  oamUpdate(&oamSub);
}

// Un paso por frame para los lugares de la mano que estan en medio de
// un lerp (por un reordenamiento) -- excludeIdx es la carta que se
// esta arrastrando en vivo (esa la maneja update_dragged_card_position
// aparte, no hand_slide_tick).
static void hand_slide_tick(const GameState* state, int cursorIndex, int excludeIdx) {
  const Player* p = &state->players[HUMAN_ID];
  int changed = 0;
  for (int i = 0; i < p->handCount; i++) {
    if (i == excludeIdx || anim_is_settled(&handSlideX[i])) continue;
    int x = anim_step(&handSlideX[i]);
    int y = HAND_Y - (i == cursorIndex ? HAND_LIFT_PX : 0);
    int affine = (i == cursorIndex) ? CARD_HERO_AFFINE_ID : CARD_AFFINE_ID;
    oamSet(&oamSub, hand_oam_id(i, cursorIndex), x - CARD_SCALE_PAD, y - CARD_SCALE_PAD, 0, 0, SpriteSize_32x32, SpriteColorFormat_256Color,
      handGfx[i], affine, true, false, false, false, false);
    changed = 1;
  }
  if (changed) oamUpdate(&oamSub);
}

#define CARD_PLAY_EXIT_Y -40 // afuera de la pantalla de abajo, arriba
#define CARD_PLAY_MAX_FRAMES 20
#define TRICK_ENTER_OFFSET 50 // cuanto mas abajo de su lugar arranca la carta que "sube" a la cruz

// Solo la entrada en la cruz de arriba (sin salida de mano) -- la usan
// los bots, que no tienen una fila de mano visible de la que salir. La
// carta entra desde el borde de pantalla que le toca a ese asiento (ver
// TRICK_ENTER_DIR): la de la izquierda entra de izquierda a derecha, la
// de arriba de arriba hacia abajo, etc. -- no siempre de abajo hacia
// arriba como antes.
static void animate_trick_card_entering(int pid, Card card) {
  dmaCopy(card_tiles_for(card.suit, card.value), trickGfx[pid], CARD_TILE_BYTES);

  int startX = TRICK_SLOT_X[pid];
  int startY = TRICK_SLOT_Y[pid];
  switch (TRICK_ENTER_DIR[pid]) {
    case ENTER_FROM_BOTTOM: startY += TRICK_ENTER_OFFSET; break;
    case ENTER_FROM_TOP:    startY -= TRICK_ENTER_OFFSET; break;
    case ENTER_FROM_LEFT:   startX -= TRICK_ENTER_OFFSET; break;
    case ENTER_FROM_RIGHT:  startX += TRICK_ENTER_OFFSET; break;
  }

  SpriteAnim animX, animY;
  anim_snap(&animX, startX);
  anim_set_target(&animX, TRICK_SLOT_X[pid]);
  anim_snap(&animY, startY);
  anim_set_target(&animY, TRICK_SLOT_Y[pid]);

  for (int f = 0; f < CARD_PLAY_MAX_FRAMES && (!anim_is_settled(&animX) || !anim_is_settled(&animY)); f++) {
    int x = anim_step(&animX);
    int y = anim_step(&animY);
    oamSet(&oamMain, pid, x - CARD_SCALE_PAD, y - CARD_SCALE_PAD, 1, 0, SpriteSize_32x32, SpriteColorFormat_256Color,
      trickGfx[pid], CARD_AFFINE_ID, true, false, false, false, false);
    oamUpdate(&oamMain);
    vsync();
  }
}

// Mientras se mantiene el dedo apoyado sobre la carta ya agarrada, la
// carta se PEGA al toque en tiempo real en las DOS direcciones (no
// solo se levanta una vez y se queda fija) -- inspirado en como
// arrastran las cartas engines tipo Unity/Balatro. Usa la matriz
// "hero" (no la fija) para que el punch/inclinacion se sigan viendo
// mientras se arrastra.
static void update_dragged_card_position(int idx, int liveX, int liveY) {
  oamSet(&oamSub, 0, liveX - CARD_SCALE_PAD, liveY - CARD_SCALE_PAD, 0, 0,
    SpriteSize_32x32, SpriteColorFormat_256Color, handGfx[idx], CARD_HERO_AFFINE_ID, true, false, false, false, false);
  oamUpdate(&oamSub);
}

#define DRAG_SNAP_MAX_FRAMES 14

// Se solto el dedo SIN llegar a confirmar la jugada -- la carta vuelve
// a su lugar (posicion Y de "levantada", posicion X de su lugar ACTUAL
// en la mano -- puede haber cambiado si se reordeno arrastrando al
// costado) con el mismo resorte de siempre, en vez de quedar pegada de
// golpe donde la solto el dedo.
static void animate_drag_snap_back(int idx, int fromX, int fromY) {
  SpriteAnim animX, animY;
  anim_snap(&animX, fromX);
  anim_set_target(&animX, hand_slot_x(idx));
  anim_snap(&animY, fromY);
  anim_set_target(&animY, HAND_Y - HAND_LIFT_PX);
  for (int f = 0; f < DRAG_SNAP_MAX_FRAMES && (!anim_is_settled(&animX) || !anim_is_settled(&animY)); f++) {
    int x = anim_step(&animX);
    int y = anim_step(&animY);
    update_dragged_card_position(idx, x, y);
    vsync();
  }
}

// Se llama justo DESPUES de confirmar que el HUMANO jugo la carta
// (game_play_card ya devolvio true). Dos fases:
//  1) la carta se ACOMODA EN EL CENTRO (mismo X en el que "aparece"
//     arriba, TRICK_SLOT_X[HUMAN_ID]) ANTES de subir -- si no, sale
//     desde donde sea que estuviera en la mano (izquierda, derecha) y
//     aparece siempre en el centro arriba, un salto que rompia la
//     ilusion de que es LA MISMA carta cruzando la costura.
//  2) ya centrada, sube y sale de la pantalla de abajo MIENTRAS su
//     "hermana" sube hasta su lugar en la cruz de arriba -- mismo X
//     las dos, entonces la costura entre pantallas queda invisible.
// El sprite de la mano (handGfx[idx]) todavia tiene el arte cargado en
// este punto -- render_hand_sprites recien reacomoda TODO (con la
// carta ya afuera del estado) despues de que termina la animacion.
// fromX/fromY: de donde arranca -- si se jugo arrastrando, es donde
// quedo la carta pegada al dedo, no su lugar fijo de "levantada".
static void animate_human_card_play(int idx, Card card, int fromX, int fromY) {
  int centerX = TRICK_SLOT_X[HUMAN_ID]; // mismo X de siempre en la cruz de arriba
  int restY = HAND_Y - HAND_LIFT_PX; // altura normal de "levantada"
  int handId = 0; // la que se juega siempre gana el solapamiento mientras sale (era el cursor)

  // Fase 1: se junta hacia el centro Y vuelve a la altura normal A LA
  // VEZ -- si se confirmo arrastrando bien abajo (ahora se puede
  // arrastrar por toda la pantalla), la carta no se queda "colgada"
  // cerca del borde de abajo mientras se acomoda, ya arranca subiendo.
  SpriteAnim centerAnimX, centerAnimY;
  anim_snap(&centerAnimX, fromX);
  anim_set_target(&centerAnimX, centerX);
  anim_snap(&centerAnimY, fromY);
  anim_set_target(&centerAnimY, restY);
  for (int f = 0; f < CARD_PLAY_MAX_FRAMES && (!anim_is_settled(&centerAnimX) || !anim_is_settled(&centerAnimY)); f++) {
    int x = anim_step(&centerAnimX);
    int y = anim_step(&centerAnimY);
    oamSet(&oamSub, handId, x - CARD_SCALE_PAD, y - CARD_SCALE_PAD, 0, 0, SpriteSize_32x32, SpriteColorFormat_256Color,
      handGfx[idx], CARD_HERO_AFFINE_ID, true, false, false, false, false);
    oamUpdate(&oamSub);
    vsync();
  }

  // Fase 2: sube y cruza, X fijo (ya centrado) para las dos mitades.
  SpriteAnim handAnimY;
  anim_snap(&handAnimY, restY);
  anim_set_target(&handAnimY, CARD_PLAY_EXIT_Y);
  int handX = centerX - CARD_SCALE_PAD;

  dmaCopy(card_tiles_for(card.suit, card.value), trickGfx[HUMAN_ID], CARD_TILE_BYTES);
  SpriteAnim trickAnim;
  anim_snap(&trickAnim, TRICK_SLOT_Y[HUMAN_ID] + TRICK_ENTER_OFFSET);
  anim_set_target(&trickAnim, TRICK_SLOT_Y[HUMAN_ID]);
  int trickX = centerX - CARD_SCALE_PAD;

  for (int f = 0; f < CARD_PLAY_MAX_FRAMES && (!anim_is_settled(&handAnimY) || !anim_is_settled(&trickAnim)); f++) {
    int hy = anim_step(&handAnimY);
    oamSet(&oamSub, handId, handX, hy - CARD_SCALE_PAD, 0, 0, SpriteSize_32x32, SpriteColorFormat_256Color,
      handGfx[idx], CARD_HERO_AFFINE_ID, true, false, false, false, false);
    oamUpdate(&oamSub);

    int ty = anim_step(&trickAnim);
    oamSet(&oamMain, HUMAN_ID, trickX, ty - CARD_SCALE_PAD, 1, 0, SpriteSize_32x32, SpriteColorFormat_256Color,
      trickGfx[HUMAN_ID], CARD_AFFINE_ID, true, false, false, false, false);
    oamUpdate(&oamMain);

    vsync();
  }
}

#define CARD_DEAL_START_Y -40 // mismo "afuera de pantalla" que la salida al jugar, pero de entrada
#define CARD_DEAL_STAGGER_FRAMES 5 // cuantos frames se espera entre que arranca una carta y la siguiente

// Animacion de reparto: cada carta de la mano entra boca abajo (dorso)
// desde afuera de la pantalla de abajo, cayendo hasta su lugar en la
// mano, y ahi se da vuelta (se ve la cara). Las 6 cartas arrancan
// escalonadas en el tiempo para que se vea como un reparto en serie, no
// las 6 de golpe.
static void animate_hand_deal(const GameState* state) {
  const Player* p = &state->players[HUMAN_ID];
  int count = p->handCount;

  top();
  consoleClear();
  bottom();
  consoleClear();
  play_sfx(SFX_SHUFFLE);

  SpriteAnim anim[HAND_SLOT_COUNT];
  int started[HAND_SLOT_COUNT] = { 0 };
  int settled[HAND_SLOT_COUNT] = { 0 };

  for (int i = 0; i < HAND_SLOT_COUNT; i++) {
    oamSet(&oamSub, hand_oam_id(i, -1), 0, 0, 0, 0, SpriteSize_32x32, SpriteColorFormat_256Color,
      handGfx[i], -1, false, true, false, false, false);
  }
  oamUpdate(&oamSub);

  int frame = 0;
  int allSettled = 0;
  while (!allSettled) {
    vsync();
    allSettled = 1;
    for (int i = 0; i < count; i++) {
      if (!started[i] && frame >= i * CARD_DEAL_STAGGER_FRAMES) {
        started[i] = 1;
        play_sfx(SFX_DEAL);
        dmaCopy(card_back_tiles(), handGfx[i], CARD_TILE_BYTES);
        anim_snap(&anim[i], CARD_DEAL_START_Y);
        anim_set_target(&anim[i], HAND_Y);
      }
      if (started[i] && !settled[i]) {
        int y = anim_step(&anim[i]);
        oamSet(&oamSub, hand_oam_id(i, -1), hand_slot_x(i) - CARD_SCALE_PAD, y - CARD_SCALE_PAD, 0, 0, SpriteSize_32x32, SpriteColorFormat_256Color,
          handGfx[i], CARD_AFFINE_ID, true, false, false, false, false);
        if (anim_is_settled(&anim[i])) {
          settled[i] = 1;
          Card c = p->hand[i];
          dmaCopy(card_tiles_for(c.suit, c.value), handGfx[i], CARD_TILE_BYTES); // se da vuelta: cara arriba
        }
      }
      if (!settled[i]) allSettled = 0;
    }
    oamUpdate(&oamSub);
    frame++;
  }
  hand_slide_snap_all(); // recien repartida, ningun lugar tiene un lerp pendiente
}

// Toque -> indice de carta en mano, por posicion real del sprite (no por
// fila de texto). Generoso en Y (toda la franja de la fila de mano)
// para no fallar por unos pixeles de mas/menos.
//
// Las cartas se superponen (ver hand_slot_x), asi que mas de una puede
// contener el mismo pixel tocado. En vez de tratar de replicar a mano
// el mismo orden con el que se dibujan (fragil -- dos implementaciones
// separadas que tienen que coincidir SIEMPRE, y ya se desincronizaron
// una vez), se resuelve por CERCANIA: de todas las cartas que
// contienen el toque, gana la que tiene el CENTRO mas cerca del punto
// tocado. Tocar el centro de una carta entonces SIEMPRE la agarra a
// ella (distancia 0), sea cual sea el criterio de superposicion visual.
static int hand_touch_to_index(const GameState* state, int px, int py) {
  int topY = HAND_Y - HAND_LIFT_PX - CARD_SCALE_PAD - 4;
  int botY = HAND_Y - CARD_SCALE_PAD + CARD_VISUAL_SIZE + 4;
  if (py < topY || py > botY) return -1;
  const Player* p = &state->players[HUMAN_ID];
  int best = -1;
  int bestDist = 0;
  for (int i = 0; i < p->handCount; i++) {
    int x = hand_slot_x(i) - CARD_SCALE_PAD;
    if (px < x || px >= x + CARD_VISUAL_SIZE) continue;
    int center = x + CARD_VISUAL_SIZE / 2;
    int dist = abs(px - center);
    if (best < 0 || dist < bestDist) {
      best = i;
      bestDist = dist;
    }
  }
  return best;
}

// Reacomodar la mano tocando y arrastrando funciona en CUALQUIER
// momento (no solo en el propio turno) -- mientras se espera a que
// otro jugador prediga/juegue, o mientras se ve la corona tras ganar
// una baza, se puede tocar y mover las cartas propias igual que en el
// turno de uno. La UNICA diferencia: como no es un turno que se pueda
// confirmar, soltar el dedo SIEMPRE devuelve la carta a su lugar (ya
// reordenado si se arrastro al costado) -- nunca dispara una jugada.
typedef struct {
  int cursorIndex;
  int dragIdx;
  int dragGrabOffsetX;
  int dragStartY;
  int dragLastX;
  int lastTouchX;
  int lastTouchY;
} IdleHandDrag;

static IdleHandDrag idleHandDrag = { -1, -1, 0, 0, 0, 0, 0 };

static void idle_hand_drag_step(GameState* state) {
  IdleHandDrag* d = &idleHandDrag;
  touchPosition touch;
  touchRead(&touch);
  int pressed = keysDown();
  int released = keysUp();
  int handCount = state->players[HUMAN_ID].handCount;
  if (keysHeld() & KEY_TOUCH) {
    d->lastTouchX = touch.px;
    d->lastTouchY = touch.py;
  }

  if (pressed & KEY_TOUCH) {
    int idx = hand_touch_to_index(state, touch.px, touch.py);
    d->dragIdx = idx;
    d->dragStartY = touch.py;
    d->lastTouchX = touch.px;
    d->lastTouchY = touch.py;
    if (idx >= 0) {
      d->dragGrabOffsetX = touch.px - hand_slot_x(idx);
      d->dragLastX = hand_slot_x(idx);
      play_sfx(SFX_BUTTON);
      hero_anim_punch();
      d->cursorIndex = idx;
      render_hand_sprites(state, d->cursorIndex);
    }
  } else if ((keysHeld() & KEY_TOUCH) && d->dragIdx >= 0) {
    int liveX = touch.px - d->dragGrabOffsetX;
    Player* human = &state->players[HUMAN_ID];
    if (d->dragIdx > 0 && liveX < hand_slot_x(d->dragIdx) - HAND_STRIDE / 2) {
      int neighborIdx = d->dragIdx - 1;
      Card tmp = human->hand[d->dragIdx];
      human->hand[d->dragIdx] = human->hand[neighborIdx];
      human->hand[neighborIdx] = tmp;
      anim_snap(&handSlideX[d->dragIdx], hand_slot_x(neighborIdx));
      d->dragIdx = neighborIdx;
      d->cursorIndex = d->dragIdx;
      render_hand_sprites(state, d->cursorIndex);
    } else if (d->dragIdx < handCount - 1 && liveX > hand_slot_x(d->dragIdx) + HAND_STRIDE / 2) {
      int neighborIdx = d->dragIdx + 1;
      Card tmp = human->hand[d->dragIdx];
      human->hand[d->dragIdx] = human->hand[neighborIdx];
      human->hand[neighborIdx] = tmp;
      anim_snap(&handSlideX[d->dragIdx], hand_slot_x(neighborIdx));
      d->dragIdx = neighborIdx;
      d->cursorIndex = d->dragIdx;
      render_hand_sprites(state, d->cursorIndex);
    }
    hero_anim_tilt_follow(liveX - d->dragLastX);
    d->dragLastX = liveX;
    int liveY = (HAND_Y - HAND_LIFT_PX) + (touch.py - d->dragStartY);
    update_dragged_card_position(d->dragIdx, liveX, liveY);
    render_slingshot(liveX, liveY);
  } else if ((released & KEY_TOUCH) && d->dragIdx >= 0) {
    anim_set_target(&heroAnim.tilt, 0);
    int liveX = d->lastTouchX - d->dragGrabOffsetX;
    int liveY = (HAND_Y - HAND_LIFT_PX) + (d->lastTouchY - d->dragStartY);
    hide_slingshot();
    animate_drag_snap_back(d->dragIdx, liveX, liveY);
    d->dragIdx = -1;
  }

  hero_anim_step_and_apply();
  hand_slide_tick(state, d->cursorIndex, d->dragIdx);
}

// Quien va ganando la baza en este momento (mayor rank jugado hasta
// ahora), sin importar si es "sweat-worthy" o no -- -1 si todavia no
// se jugo ninguna carta. Usado tanto para el marco verde (.card-leader
// de la web, se ve desde la primera carta) como de base para el
// temblor/gota (que si necesita el criterio extra de mas abajo).
static int trick_leader_id(const GameState* state) {
  if (state->trickCardsPlayedCount < 1) return -1;
  int leader = -1;
  int bestRank = -1;
  for (int i = 0; i < state->trickCardsPlayedCount; i++) {
    int r = card_rank(state->trickCardsPlayed[i].card);
    if (r > bestRank) {
      bestRank = r;
      leader = state->trickCardsPlayed[i].playerId;
    }
  }
  return leader;
}

// Dibuja la baza en curso en la cruz de arriba; esconde los asientos
// que todavia no tiraron nada esta baza (o si la fase ni siquiera es de
// juego, ej. mientras se predice).
static void render_trick_sprites(const GameState* state) {
  int shown[4] = { 0, 0, 0, 0 };
  int leaderPid = (state->phase == PHASE_PLAYING || state->phase == PHASE_TRICK_END) ? trick_leader_id(state) : -1;

  // Las cartas de la baza van en priority=1 (una capa atras) para que
  // el marco de lider, la corona y las gotitas -- todas en priority=0 --
  // siempre se dibujen POR ENCIMA, sin importar el orden de sus OAM id.
  if (state->phase == PHASE_PLAYING || state->phase == PHASE_TRICK_END) {
    for (int i = 0; i < state->trickCardsPlayedCount; i++) {
      int pid = state->trickCardsPlayed[i].playerId;
      Card c = state->trickCardsPlayed[i].card;
      dmaCopy(card_tiles_for(c.suit, c.value), trickGfx[pid], CARD_TILE_BYTES);
      oamSet(&oamMain, pid, TRICK_SLOT_X[pid] - CARD_SCALE_PAD, TRICK_SLOT_Y[pid] - CARD_SCALE_PAD, 1, 0, SpriteSize_32x32, SpriteColorFormat_256Color,
        trickGfx[pid], CARD_AFFINE_ID, true, false, false, false, false);
      shown[pid] = 1;
    }
  }
  for (int pid = 0; pid < 4; pid++) {
    if (!shown[pid]) {
      oamSet(&oamMain, pid, 0, 0, 1, 0, SpriteSize_32x32, SpriteColorFormat_256Color,
        trickGfx[pid], -1, false, true, false, false, false);
    }
  }

  // Marco verde sobre quien va ganando la baza en este momento (desde
  // la primera carta jugada), igual espiritu que .card-leader de la
  // version web. Mismo tamano 64x64 que el area ya escalada de la
  // carta, asi que queda justo encima de su borde.
  if (leaderPid >= 0) {
    oamSet(&oamMain, LEADER_FRAME_OAM_ID, TRICK_SLOT_X[leaderPid] - CARD_SCALE_PAD, TRICK_SLOT_Y[leaderPid] - CARD_SCALE_PAD, 0, LEADER_FRAME_PALETTE_BANK,
      SpriteSize_64x64, SpriteColorFormat_16Color, leaderFrameGfx, -1, false, false, false, false, false);
  } else {
    oamSet(&oamMain, LEADER_FRAME_OAM_ID, 0, 0, 0, LEADER_FRAME_PALETTE_BANK,
      SpriteSize_64x64, SpriteColorFormat_16Color, leaderFrameGfx, -1, false, true, false, false, false);
  }

  // Corona sobre quien se llevo la baza: ahora mas grande (32x32, el
  // doble que antes) y CENTRADA en la carta en vez de una insignia
  // chica en la esquina. Centro visual de la carta = TRICK_SLOT_X/Y +
  // 16 (ver el comentario grande de mas arriba de TRICK_SLOT_X/Y); para
  // un sprite de 32x32 centrado ahi, el top-left que le toca a
  // oamSet es exactamente TRICK_SLOT_X/Y sin ningun offset mas.
  if (state->phase == PHASE_TRICK_END) {
    int pid = state->lastTrickWinner;
    oamSet(&oamMain, CROWN_OAM_ID, TRICK_SLOT_X[pid], TRICK_SLOT_Y[pid], 0, CROWN_PALETTE_BANK,
      SpriteSize_32x32, SpriteColorFormat_16Color, crownGfx, -1, false, false, false, false, false);
  } else {
    oamSet(&oamMain, CROWN_OAM_ID, 0, 0, 0, CROWN_PALETTE_BANK,
      SpriteSize_32x32, SpriteColorFormat_16Color, crownGfx, -1, false, true, false, false, false);
  }

  // Flecha de turno, en el CENTRO de la cruz -- apunta hacia el
  // asiento de quien le toca predecir/jugar ahora. Sin sentido en fin
  // de baza/mano (no hay "turno" pendiente todavia). id0 abajo, id1
  // izquierda, id2 arriba, id3 derecha (ver TRICK_ENTER_DIR) -> indice
  // dentro de arrowDirGfx (right,down,left,up).
  static const int SEAT_TO_ARROW_DIR[4] = { 1, 2, 3, 0 };
  int turnActor = (state->phase == PHASE_PREDICTING || state->phase == PHASE_PLAYING)
    ? game_get_current_player_to_act_id(state) : -1;
  if (turnActor >= 0) {
    oamSet(&oamMain, ARROW_OAM_ID, CENTER_ARROW_X - 8, CENTER_ARROW_Y - 8, 0, ARROW_PALETTE_BANK,
      SpriteSize_16x16, SpriteColorFormat_16Color, arrowDirGfx[SEAT_TO_ARROW_DIR[turnActor]], -1, false, false, false, false, false);
  } else {
    oamSet(&oamMain, ARROW_OAM_ID, 0, 0, 0, ARROW_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
      arrowDirGfx[0], -1, false, true, false, false, false);
  }

  oamUpdate(&oamMain);
}

static void setup_screens(void) {
  videoSetMode(MODE_0_2D);
  videoSetModeSub(MODE_0_2D);
  vramSetBankA(VRAM_A_MAIN_BG);
  vramSetBankC(VRAM_C_SUB_BG);
  consoleInit(&topConsole, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, true, true);
  consoleInit(&bottomConsole, 3, BgType_Text4bpp, BgSize_T_256x256, 31, 0, false, true);
  setup_background();
  setup_sprites();
}

// Sonido: se cargan todos los efectos una vez al arrancar (mmLoadEffect
// los deja en RAM, mmEffect despues los dispara sin re-cargar nada).
static void setup_audio(void) {
  mmInitDefaultMem((mm_addr)soundbank_bin);
  mmLoadEffect(SFX_SHUFFLE);
  mmLoadEffect(SFX_DEAL);
  mmLoadEffect(SFX_PLAY);
  mmLoadEffect(SFX_BUTTON);
  mmLoadEffect(SFX_CANCEL);
  mmLoadEffect(SFX_TRICKWIN);
  mmLoadEffect(SFX_GAMEWIN);
  mmLoadEffect(SFX_COIN);
  mmLoadEffect(SFX_ANCHO);
  mmLoadEffect(SFX_PRODCOIN);
  mmLoadEffect(SFX_PRODARCADE);
}

// ---------- Microfono: soplido en el menu acelera el vaiven del logo ----------
// Se arranca UNA sola vez al boot (soundMicRecord queda grabando en
// loop solo, nunca hace falta re-armarlo) y se deja corriendo toda la
// partida -- es barato (lo maneja el ARM7) y micLevel solo se lee
// durante las pantallas de menu (ver update_title_logo_idle), asi que
// no hace falta prender/apagarlo en cada entrada/salida del menu.
// 8000Hz hacia el mic (junto con el streaming de musica) saturaba el
// audio -- el muestreo del mic lo maneja el ARM7 a fuerza de
// interrupciones de timer, y compite por CPU con el resto de sus
// tareas (ver el aviso de calico: "muy pesado para la CPU, afecta a
// los demas drivers en proporcion al sample rate"). Para detectar
// solo un soplido (no hace falta fidelidad de audio) alcanza de sobra
// con bastante menos.
#define MIC_SAMPLE_RATE 2000
#define MIC_BUFFER_SAMPLES 128
static s16 micBuffer[MIC_BUFFER_SAMPLES];
// Pico de amplitud (valor absoluto) del ultimo buffer leido -- lo pisa
// mic_handler cada vez que el ARM7 termina de llenarlo (mas o menos
// cada 32ms a este sample rate, varias veces por segundo).
static volatile int micLevel = 0;

static void mic_handler(void* data, int length) {
  DC_InvalidateRange(data, length);
  s16* samples = (s16*)data;
  int count = length / (int)sizeof(s16);
  int peak = 0;
  for (int i = 0; i < count; i++) {
    int v = samples[i];
    if (v < 0) v = -v;
    if (v > peak) peak = v;
  }
  micLevel = peak;
}

static void setup_mic(void) {
  soundMicRecord(micBuffer, sizeof(micBuffer), MicFormat_12Bit, MIC_SAMPLE_RATE, mic_handler);
}

// Musica de fondo: el tema (~1:47, PCM crudo) es demasiado grande para
// cargarlo entero en RAM como un efecto mas (uno de los ~10 SFX
// normales pesa unos pocos KB; esto son ~4.6MB) -- en vez de eso se
// STREAMEA de a pedacitos, leyendo directo del archivo en NitroFS
// (adentro de la ROM en si, ver nitrofiles/ y NITRODATA en el
// Makefile) cada vez que maxmod pide mas muestras. Si algo de esto
// falla (no deberia, pero ej. un .nds viejo sin la parte de NitroFS)
// el juego sigue andando igual, solo que sin musica.
static FILE* musicFile = NULL;

static mm_word music_stream_callback(mm_word length, mm_addr dest, mm_stream_formats format) {
  (void)format; // siempre MM_STREAM_16BIT_MONO, no hace falta mirarlo
  if (!musicEnabled) {
    // Silencio sin tocar la posicion del archivo -- al reactivar la
    // musica sigue exactamente donde se habia quedado (pausa, no salto).
    memset(dest, 0, length * sizeof(s16));
    return length;
  }
  s16* target = (s16*)dest;
  mm_word remaining = length;
  int seekRetries = 0; // por si el archivo estuviera vacio/roto -- no girar para siempre
  while (remaining > 0) {
    size_t got = fread(target, sizeof(s16), remaining, musicFile);
    if (got == 0) {
      seekRetries++;
      if (seekRetries > 2) break;
      fseek(musicFile, 0, SEEK_SET); // se termino el archivo -- vuelve al principio (loop infinito)
      continue;
    }
    target += got;
    remaining -= got;
  }
  return length - remaining;
}

static void setup_music_stream(void) {
  if (!nitroFSInit(NULL)) return;
  musicFile = fopen("nitro:/theme.raw", "rb");
  if (!musicFile) return;

  mm_stream stream;
  stream.sampling_rate = 22050;
  stream.buffer_length = 1200;
  stream.callback = music_stream_callback;
  stream.format = MM_STREAM_16BIT_MONO;
  stream.timer = MM_TIMER0;
  stream.manual = true; // se actualiza a mano desde vsync(), que ya se llama seguido en todo el juego
  mmStreamOpen(&stream);
}

static int is_ancho(Card card) {
  return card.value == 1 && (card.suit == SUIT_ESPADA || card.suit == SUIT_BASTO);
}

// Sonidos de jugar una carta (humano o bot, misma logica para los dos):
// el golpe de jugarla siempre; si con esta carta se pasa a liderar la
// baza, ademas la "moneda"; y aparte, si es cualquiera de los dos
// anchos (as de espada o de basto), su sonido propio. Se llama DESPUES
// de que game_play_card ya la agrego a trickCardsPlayed, asi que la
// ultima entrada es justo esta carta.
static void play_card_sfx(const GameState* state, Card card) {
  play_sfx(SFX_PLAY);

  int priorMaxRank = -999;
  for (int i = 0; i < state->trickCardsPlayedCount - 1; i++) {
    int r = card_rank(state->trickCardsPlayed[i].card);
    if (r > priorMaxRank) priorMaxRank = r;
  }
  int rank = card_rank(card);
  if (rank > priorMaxRank) {
    play_sfx(SFX_COIN);
  }
  if (is_ancho(card)) {
    play_sfx(SFX_ANCHO);
  }
}

// Festejo del ancho (as de espada o de basto, la carta mas fuerte del
// mazo) -- calca el "celebrateAncho()" de la version web: una frase
// aleatoria grande + algo alrededor de la carta (alla es un shake de
// pantalla, aca son particulas saliendo disparadas). Se llama DESPUES
// de que la carta ya esta asentada en su lugar de la cruz.
// La frase es un SPRITE (ver ANCHO_BANNER_*), no texto de consola: un
// caracter de consola no se puede escalar, y ademas el texto quedaba
// tapado a veces -- competia con el marco de lider/la flecha (sprites
// de prioridad 0 tambien) y en un empate de prioridad gana el sprite,
// no el fondo de texto. Por eso durante el festejo se esconden esos
// dos (se vuelven a mostrar solos en el proximo render_trick_sprites
// normal, ya con el estado que corresponda).
static const unsigned int* const ANCHO_BANNER_TILES[4] = {
  ancho_banner_0Tiles, ancho_banner_1Tiles, ancho_banner_2Tiles, ancho_banner_3Tiles,
};
static const unsigned int ANCHO_BANNER_TILES_LEN[4] = {
  ancho_banner_0TilesLen, ancho_banner_1TilesLen, ancho_banner_2TilesLen, ancho_banner_3TilesLen,
};
#define ANCHO_CELEBRATE_FRAMES 50

static void celebrate_ancho(const GameState* state, int pid) {
  int bannerIdx = rand() % 4;
  int cx = TRICK_SLOT_X[pid];
  int cy = TRICK_SLOT_Y[pid];

  // Direcciones fijas de las 6 particulas (estallido parejo alrededor
  // de la carta), llegan a su offset final de a poco con el tiempo.
  static const int DX[PARTICLE_COUNT] = { 0, 24, 20, -20, -24, 8 };
  static const int DY[PARTICLE_COUNT] = { -28, -14, 16, 16, -14, 22 };

  dmaCopy(ANCHO_BANNER_TILES[bannerIdx], anchoBannerGfx, ANCHO_BANNER_TILES_LEN[bannerIdx]);

  for (int f = 0; f < ANCHO_CELEBRATE_FRAMES; f++) {
    render_trick_sprites(state);
    // Esconde el marco de lider y la flecha PISANDO lo que
    // render_trick_sprites acaba de dejar -- sin esto, con este mismo
    // frame ya volverian a estar puestos.
    oamSet(&oamMain, LEADER_FRAME_OAM_ID, 0, 0, 0, LEADER_FRAME_PALETTE_BANK,
      SpriteSize_64x64, SpriteColorFormat_16Color, leaderFrameGfx, -1, false, true, false, false, false);
    oamSet(&oamMain, ARROW_OAM_ID, 0, 0, 0, ARROW_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
      arrowDirGfx[0], -1, false, true, false, false, false);
    oamSet(&oamMain, ANCHO_BANNER_OAM_ID, ANCHO_BANNER_OAM_X, ANCHO_BANNER_OAM_Y, 0, ANCHO_BANNER_PALETTE_BANK,
      SpriteSize_64x32, SpriteColorFormat_16Color, anchoBannerGfx, ANCHO_BANNER_AFFINE_ID, true, false, false, false, false);
    for (int i = 0; i < PARTICLE_COUNT; i++) {
      int px = cx + DX[i] * f / ANCHO_CELEBRATE_FRAMES;
      int py = cy + DY[i] * f / ANCHO_CELEBRATE_FRAMES;
      oamSet(&oamMain, PARTICLE_OAM_BASE + i, px - 4, py - 4, 0, PARTICLE_PALETTE_BANK,
        SpriteSize_8x8, SpriteColorFormat_16Color, particleGfx, -1, false, false, false, false, false);
    }
    oamUpdate(&oamMain);
    vsync();
  }

  oamSet(&oamMain, ANCHO_BANNER_OAM_ID, 0, 0, 0, ANCHO_BANNER_PALETTE_BANK, SpriteSize_64x32, SpriteColorFormat_16Color,
    anchoBannerGfx, -1, false, true, false, false, false);
  for (int i = 0; i < PARTICLE_COUNT; i++) {
    oamSet(&oamMain, PARTICLE_OAM_BASE + i, 0, 0, 0, PARTICLE_PALETTE_BANK, SpriteSize_8x8, SpriteColorFormat_16Color,
      particleGfx, -1, false, true, false, false, false);
  }
  oamUpdate(&oamMain);
}

static void pause_frames(int frames) {
  for (int i = 0; i < frames; i++) vsync();
}

static int run_pause_menu(void); // definida mas abajo -- pause_frames_animated la necesita antes

// Igual que pause_frames, pero sigue refrescando la baza (temblor,
// marco de lider, gotas) cuadro a cuadro durante la espera -- sin esto
// la animacion se queda congelada apenas el humano no es quien esta
// jugando (turno de la IA, pausa tras ganar la baza), porque el OAM
// no se toca para nada durante un pause_frames comun.
// Devuelve 1 si desde la pausa (START) se pidio volver al menu
// principal -- en ese caso el llamador tiene que cortar la partida.
static int pause_frames_animated(GameState* state, int frames) {
  for (int i = 0; i < frames; i++) {
    render_trick_sprites(state);
    vsync();
    scanKeys();
    if (keysDown() & KEY_START) {
      if (run_pause_menu()) return 1;
    }
    idle_hand_drag_step(state); // se puede seguir reacomodando la mano aunque no sea el turno propio
  }
  return 0;
}

#define PAUSE_TITLE_ROW 9
#define PAUSE_OPTIONS_ROW0 12
#define PAUSE_OPTIONS_ROW_STRIDE 2

// Menu de pausa: START durante la partida (prediciendo, jugando,
// esperando al bot, en fin de baza/mano/partida) lo abre. Centrado en
// la pantalla de ABAJO -- se esconden TODOS los sprites de esa
// pantalla mientras dura (mano, botones de prediccion, barra de
// estado, boton de jugar/continuar -- lo que sea que hubiera antes),
// el llamador ya vuelve a dibujar todo de nuevo despues.
// Devuelve 1 si el jugador confirmo volver al menu principal, 0 si
// siguio jugando (no toca el estado de la partida en absoluto).
static int run_pause_menu(void) {
  static const char* LABELS[2] = { "Seguir jugando", "Volver al menu" };
  int cursor = 0;
  int chosen = -1;

  for (int i = 0; i < 40; i++) oamClearSprite(&oamSub, i);
  oamUpdate(&oamSub);

  while (chosen < 0) {
    bottom();
    consoleClear();
    iprintf("\x1b[%d;13HPausa", PAUSE_TITLE_ROW);
    for (int i = 0; i < 2; i++) {
      // Centrado sobre el mas largo de los dos ("Volver al menu", 14
      // caracteres + 2 del cursor "> " = 16) -- (32-16)/2 = 8.
      iprintf("\x1b[%d;8H%s%s", PAUSE_OPTIONS_ROW0 + i * PAUSE_OPTIONS_ROW_STRIDE, i == cursor ? "> " : "  ", LABELS[i]);
    }

    vsync();
    scanKeys();
    touchPosition touch;
    touchRead(&touch);
    int pressed = keysDown();

    if (pressed & (KEY_UP | KEY_DOWN)) {
      cursor = !cursor;
      play_sfx(SFX_BUTTON);
    }
    if (pressed & KEY_A) {
      chosen = cursor;
    } else if (pressed & KEY_TOUCH) {
      int row = touch.py / 8;
      for (int i = 0; i < 2; i++) {
        if (row >= PAUSE_OPTIONS_ROW0 + i * PAUSE_OPTIONS_ROW_STRIDE && row < PAUSE_OPTIONS_ROW0 + i * PAUSE_OPTIONS_ROW_STRIDE + PAUSE_OPTIONS_ROW_STRIDE) {
          chosen = i;
        }
      }
    } else if (pressed & KEY_START) {
      chosen = 0; // apretar START de nuevo cierra la pausa sin cambiar nada
    }
  }
  play_sfx(SFX_BUTTON);
  bottom();
  consoleClear();
  return chosen == 1;
}

// Cuando el HUMANO queda eliminado (llega a GAME_MAX_PENALTIES), no
// tiene sentido seguir mirando como terminan de jugar los bots entre
// si solos -- se corta ahi mismo, con la pantalla de arriba mostrando
// el panel de fin de mano de fondo (donde se ven los propios corazones
// rotos, ya dibujado por render_top antes de llegar aca) y este cartel
// abajo. Devuelve 1 si eligio "Volver al menu", 0 si eligio "Otra vez"
// (el llamador arma una partida nueva).
// Menu de fin de partida (abajo): "Otra vez?" / "Volver al menu" --
// mismo menu para victoria Y derrota, solo cambia el titulo (ver
// show_outcome_screen para lo que se ve arriba mientras tanto).
// Devuelve 1 si se eligio "Volver al menu".
static int run_match_end_menu(const char* title) {
  static const char* LABELS[2] = { "Otra vez?", "Volver al menu" };
  int cursor = 0;
  int chosen = -1;
  int titleCol = (32 - (int)strlen(title)) / 2;
  if (titleCol < 0) titleCol = 0;

  while (chosen < 0) {
    bottom();
    consoleClear();
    iprintf("\x1b[6;%dH%s", titleCol, title);
    for (int i = 0; i < 2; i++) {
      iprintf("\x1b[%d;9H%s%s", 10 + i * 2, i == cursor ? "> " : "  ", LABELS[i]);
    }

    vsync();
    scanKeys();
    touchPosition touch;
    touchRead(&touch);
    int pressed = keysDown();

    if (pressed & (KEY_UP | KEY_DOWN)) {
      cursor = !cursor;
      play_sfx(SFX_BUTTON);
    }
    if (pressed & KEY_A) {
      chosen = cursor;
    } else if (pressed & KEY_TOUCH) {
      int row = touch.py / 8;
      if (row == 10) chosen = 0;
      else if (row == 12) chosen = 1;
    }
  }
  play_sfx(SFX_BUTTON);
  return chosen == 1;
}

// Boton "jugar/continuar" animado -- reemplaza el texto "toca/apreta
// START" de la pantalla de titulo, fin de mano y fin de partida (las
// tres eran variantes del mismo "esperar un toque para seguir", ahora
// comparten este unico componente). frame 0 = idle.
static void render_play_button(int frame) {
  dmaCopy(PLAY_BUTTON_TILES[frame], playButtonGfx, PLAY_BUTTON_TILES_LEN[frame]);
  oamSet(&oamSub, PLAY_BUTTON_OAM_ID, PLAY_BUTTON_OAM_X, PLAY_BUTTON_OAM_Y, 0,
    PLAY_BUTTON_PALETTE_BANK, SpriteSize_64x32, SpriteColorFormat_16Color,
    playButtonGfx, PLAY_BUTTON_AFFINE_ID, true, false, false, false, false);
  oamUpdate(&oamSub);
}

static void hide_play_button(void) {
  oamSet(&oamSub, PLAY_BUTTON_OAM_ID, 0, 0, 0, PLAY_BUTTON_PALETTE_BANK,
    SpriteSize_64x32, SpriteColorFormat_16Color, playButtonGfx, -1, false, true, false, false, false);
  oamUpdate(&oamSub);
}

// Se puede tocar el boton o apretar A para confirmarlo -- START ya NO
// es un atajo para apretarlo, abre el menu de pausa (volver al menu
// principal), igual que en cualquier otro momento de la partida.
// Devuelve 1 si desde la pausa se pidio volver al menu principal.
static int wait_for_play_button(void) {
  render_play_button(0);
  while (1) {
    vsync();
    scanKeys();
    int pressed = keysDown();
    if (pressed & KEY_START) {
      if (run_pause_menu()) return 1;
      render_play_button(0); // la pausa piso la pantalla, se vuelve a dibujar
      continue;
    }
    if (pressed & (KEY_TOUCH | KEY_A)) break;
  }
  play_sfx(SFX_BUTTON);
  static const int PRESS_FRAMES[3] = { 1, 2, 3 };
  for (int i = 0; i < 3; i++) {
    render_play_button(PRESS_FRAMES[i]);
    for (int f = 0; f < PLAY_BUTTON_FRAME_HOLD; f++) vsync();
  }
  hide_play_button();
  return 0;
}

// ---------- Nombre del jugador: guardado en la SD (libfat) ----------
#define PLAYER_NAME_MAX 8
#define PLAYER_NAME_SAVE_PATH "/bazado_name.sav"

// fatInitDefault() salio bien -- si no (no hay tarjeta SD/DLDI, ej.
// melonDS sin una imagen configurada), se juega igual pero el nombre
// no se guarda de una vez para la otra, solo dura mientras esta
// prendida.
static int fatReady = 0;

static void load_player_name(char* outName) {
  strcpy(outName, "Vos"); // default si no hay nada guardado (o no hay FAT)
  if (!fatReady) return;
  FILE* f = fopen(PLAYER_NAME_SAVE_PATH, "rb");
  if (!f) return;
  char buf[PLAYER_NAME_MAX + 1] = { 0 };
  size_t n = fread(buf, 1, PLAYER_NAME_MAX, f);
  fclose(f);
  if (n > 0) {
    buf[n] = '\0';
    if (buf[0] != '\0') strcpy(outName, buf);
  }
}

static void save_player_name(const char* name) {
  if (!fatReady) return;
  FILE* f = fopen(PLAYER_NAME_SAVE_PATH, "wb");
  if (!f) return;
  fwrite(name, 1, strlen(name), f);
  fclose(f);
}

// ---------- Musica/Sonido: guardado junto al nombre (2 bytes, 1 por toggle) ----------
#define SETTINGS_SAVE_PATH "/bazado_settings.sav"

static void load_settings(void) {
  musicEnabled = 1; // default si no hay guardado (o no hay FAT)
  soundEnabled = 1;
  bgSameTopBottom = 1;
  bgTopPatternIndex = 0;
  bgBottomPatternIndex = 0;
  if (!fatReady) return;
  FILE* f = fopen(SETTINGS_SAVE_PATH, "rb");
  if (!f) return;
  unsigned char buf[5];
  size_t n = fread(buf, 1, 5, f);
  fclose(f);
  if (n >= 2) {
    musicEnabled = buf[0] != 0;
    soundEnabled = buf[1] != 0;
  }
  // Los 3 campos de fondo se agregaron despues -- un .sav viejo (solo 2
  // bytes) se sigue leyendo bien, nomas se queda con los defaults de arriba.
  if (n >= 5) {
    bgSameTopBottom = buf[2] != 0;
    bgTopPatternIndex = buf[3] % BG_PATTERN_COUNT;
    bgBottomPatternIndex = buf[4] % BG_PATTERN_COUNT;
  }
}

static void save_settings(void) {
  if (!fatReady) return;
  FILE* f = fopen(SETTINGS_SAVE_PATH, "wb");
  if (!f) return;
  unsigned char buf[5] = {
    (unsigned char)musicEnabled, (unsigned char)soundEnabled,
    (unsigned char)bgSameTopBottom, (unsigned char)bgTopPatternIndex, (unsigned char)bgBottomPatternIndex,
  };
  fwrite(buf, 1, 5, f);
  fclose(f);
}

// Pantalla de "elegi tu nombre": grid de A-Z + borrar + listo (28
// celdas, 7x4), D-pad se mueve en las 2 direcciones o se toca directo
// la letra. Si ya habia un nombre guardado arranca precargado --
// START lo deja tal cual sin tener que tocar nada (juego rapido).
// La pantalla de abajo tiene 32 columnas de texto en total -- con 7
// columnas de grilla, X0=20 (lo que habia antes) se iba mucho mas
// alla del borde (20 + 6*4 = 44a col), por eso se veia todo
// amontonado/cortado. Con X0=2 la ultima columna (26) mas la etiqueta
// mas ancha ("OK", 2 caracteres) termina en 28, adentro de las 32.
// 5ta fila agregada para la tecla de mayus/minuscula (SHIFT, celda 28)
// -- antes la grilla terminaba justo en 28 celdas (26 letras + DEL +
// OK), asi que no habia donde meterla sin agrandar. Las celdas 29..34
// (resto de la 5ta fila) quedan sin usar, en blanco.
#define NAME_GRID_COLS 7
#define NAME_GRID_ROWS 5
#define NAME_GRID_CELL_DEL 26
#define NAME_GRID_CELL_OK 27
#define NAME_GRID_CELL_SHIFT 28
#define NAME_GRID_CELL_BACK 29 // "volver": descarta la edicion, no guarda nada
#define NAME_GRID_X0 2
#define NAME_GRID_Y0 8
#define NAME_GRID_STEP_X 4
#define NAME_GRID_STEP_Y 2

// La etiqueta de SHIFT muestra a que caso CAMBIA si se toca (no el
// caso actual) -- mismo criterio que un teclado de telefono comun.
static void name_grid_label(int cell, int lowercase, char* out) {
  if (cell == NAME_GRID_CELL_DEL) { strcpy(out, "<-"); return; }
  if (cell == NAME_GRID_CELL_OK) { strcpy(out, "OK"); return; }
  if (cell == NAME_GRID_CELL_SHIFT) { strcpy(out, lowercase ? "AB" : "ab"); return; }
  if (cell == NAME_GRID_CELL_BACK) { strcpy(out, "X"); return; }
  if (cell < 26) {
    out[0] = (lowercase ? 'a' : 'A') + cell;
    out[1] = '\0';
    return;
  }
  out[0] = '\0'; // relleno de la 5ta fila, sin uso
}

static void render_name_entry(const char* name, int cursor, int lowercase) {
  bottom();
  consoleClear();
  iprintf("\x1b[2;3HEscribi tu nombre:");
  iprintf("\x1b[4;3H%-8s", name);

  for (int cell = 0; cell < NAME_GRID_COLS * NAME_GRID_ROWS; cell++) {
    char label[3];
    name_grid_label(cell, lowercase, label);
    if (label[0] == '\0') continue;
    int gr = cell / NAME_GRID_COLS;
    int gc = cell % NAME_GRID_COLS;
    int row = NAME_GRID_Y0 + gr * NAME_GRID_STEP_Y;
    int col = NAME_GRID_X0 + gc * NAME_GRID_STEP_X;
    iprintf("\x1b[%d;%dH%s%s", row, col, cell == cursor ? ">" : " ", label);
  }
  iprintf("\x1b[18;2HOK guarda -- X vuelve sin guardar");
}

static int name_grid_touch_to_cell(int px, int py) {
  int row0px = NAME_GRID_Y0 * 8;
  int col0px = NAME_GRID_X0 * 8;
  int rowStridePx = NAME_GRID_STEP_Y * 8;
  int colStridePx = NAME_GRID_STEP_X * 8;
  if (py < row0px || px < col0px) return -1;
  int gr = (py - row0px) / rowStridePx;
  int gc = (px - col0px) / colStridePx;
  if (gr < 0 || gr >= NAME_GRID_ROWS || gc < 0 || gc >= NAME_GRID_COLS) return -1;
  return gr * NAME_GRID_COLS + gc;
}

// Pantalla de "cambiar nombre" (Opciones). Arranca SIEMPRE vacia (no
// precarga el nombre actual -- "por defecto lo borra el anterior").
// OK guarda lo que haya escrito en nameOut (devuelve 1); X vuelve sin
// tocar nameOut para nada (devuelve 0, el llamador se queda con el
// nombre que ya tenia).
static int edit_player_name(char* nameOut) {
  char buf[PLAYER_NAME_MAX + 1] = { 0 };
  int cursor = 0;
  int lowercase = 0;

  while (1) {
    int len = (int)strlen(buf);
    render_name_entry(buf, cursor, lowercase);
    vsync();
    scanKeys();
    touchPosition touch;
    touchRead(&touch);
    int pressed = keysDown();

    int gr = cursor / NAME_GRID_COLS;
    int gc = cursor % NAME_GRID_COLS;
    if (pressed & KEY_LEFT) { gc = (gc + NAME_GRID_COLS - 1) % NAME_GRID_COLS; cursor = gr * NAME_GRID_COLS + gc; play_sfx(SFX_BUTTON); }
    if (pressed & KEY_RIGHT) { gc = (gc + 1) % NAME_GRID_COLS; cursor = gr * NAME_GRID_COLS + gc; play_sfx(SFX_BUTTON); }
    if (pressed & KEY_UP) { gr = (gr + NAME_GRID_ROWS - 1) % NAME_GRID_ROWS; cursor = gr * NAME_GRID_COLS + gc; play_sfx(SFX_BUTTON); }
    if (pressed & KEY_DOWN) { gr = (gr + 1) % NAME_GRID_ROWS; cursor = gr * NAME_GRID_COLS + gc; play_sfx(SFX_BUTTON); }

    int activated = -1;
    if (pressed & KEY_A) {
      activated = cursor;
    } else if (pressed & KEY_TOUCH) {
      int cell = name_grid_touch_to_cell(touch.px, touch.py);
      if (cell >= 0) {
        cursor = cell;
        activated = cell;
      }
    }
    if (activated < 0) continue;

    if (activated == NAME_GRID_CELL_OK) {
      if (len == 0) continue; // no se puede confirmar un nombre vacio
      strcpy(nameOut, buf);
      play_sfx(SFX_BUTTON);
      return 1;
    }
    if (activated == NAME_GRID_CELL_BACK) {
      play_sfx(SFX_CANCEL);
      return 0;
    }
    if (activated == NAME_GRID_CELL_DEL) {
      if (len > 0) {
        buf[len - 1] = '\0';
        play_sfx(SFX_BUTTON);
      }
      continue;
    }
    if (activated == NAME_GRID_CELL_SHIFT) {
      lowercase = !lowercase;
      play_sfx(SFX_BUTTON);
      continue;
    }
    if (activated >= 26) continue; // relleno de la 5ta fila, sin uso
    if (len < PLAYER_NAME_MAX) {
      buf[len] = (lowercase ? 'a' : 'A') + activated;
      buf[len + 1] = '\0';
      play_sfx(SFX_BUTTON);
    }
  }
}

// Nombres para los 3 bots -- un pool mas grande que 3, se sortean sin
// repetir en cada partida nueva (antes eran siempre Tito/Coco/Fede
// fijos).
#define AI_NAME_POOL_COUNT 8
static const char* AI_NAME_POOL[AI_NAME_POOL_COUNT] = {
  "Tito", "Coco", "Fede", "Nacho", "Lucho", "Pipo", "Chino", "Toto"
};

// Sorteo sin repetir de 3 nombres del pool (Fisher-Yates parcial sobre
// una copia de los indices).
static void pick_ai_names(const char* outNames[3]) {
  int idx[AI_NAME_POOL_COUNT];
  for (int i = 0; i < AI_NAME_POOL_COUNT; i++) idx[i] = i;
  for (int i = 0; i < 3; i++) {
    int r = i + rand() % (AI_NAME_POOL_COUNT - i);
    int tmp = idx[i];
    idx[i] = idx[r];
    idx[r] = tmp;
    outNames[i] = AI_NAME_POOL[idx[i]];
  }
}

// Pantalla de arranque: elegir que tan dificiles son los 3 bots -- LOS
// TRES juegan con la MISMA dificultad elegida aca (ni una fija por bot
// como antes), igual que el selector unico "Dificultad" de la version
// web (cfg.difficulty se aplica por igual a cada isAI ahi). D-pad
// arriba/abajo + A, o tocar directamente la opcion.
#define DIFF_PICK_ROW0 8 // fila del primer renglon ("Facil")
#define DIFF_PICK_ROW_STRIDE 2 // 2 filas de texto por opcion

// Devuelve la dificultad elegida (0..2), o -1 si se cancelo con B --
// en ese caso el que llama tiene que volver al menu principal sin
// arrancar ninguna partida.
static int choose_difficulty(void) {
  static const char* LABELS[3] = { "Facil", "Media", "Dificil" };
  static const Difficulty VALUES[3] = { DIFF_EASY, DIFF_MEDIUM, DIFF_HARD };
  int cursor = 0; // "Facil" primero, mismo default que el <select> web

  int chosen = -1;
  int cancelled = 0;
  while (chosen < 0 && !cancelled) {
    top();
    consoleClear();
    bottom();
    consoleClear();
    iprintf("\x1b[4;6HElegi la dificultad\n\x1b[5;9Hde los bots");
    for (int i = 0; i < 3; i++) {
      iprintf("\x1b[%d;6H%s %s", DIFF_PICK_ROW0 + i * DIFF_PICK_ROW_STRIDE, i == cursor ? ">" : " ", LABELS[i]);
    }

    vsync();
    update_title_logo_idle(); // sigue a la vista de fondo, con su vaiven -- hasta que arranca la partida
    scanKeys();
    touchPosition touch;
    touchRead(&touch);
    int pressed = keysDown();

    if (pressed & KEY_UP) {
      cursor = (cursor + 2) % 3;
      play_sfx(SFX_BUTTON);
    }
    if (pressed & KEY_DOWN) {
      cursor = (cursor + 1) % 3;
      play_sfx(SFX_BUTTON);
    }
    if (pressed & KEY_A) {
      chosen = cursor;
    } else if (pressed & KEY_TOUCH) {
      int row = touch.py / 8;
      for (int i = 0; i < 3; i++) {
        if (row >= DIFF_PICK_ROW0 + i * DIFF_PICK_ROW_STRIDE && row < DIFF_PICK_ROW0 + i * DIFF_PICK_ROW_STRIDE + DIFF_PICK_ROW_STRIDE) {
          cursor = i;
          chosen = i;
        }
      }
    } else if (pressed & KEY_B) {
      cancelled = 1;
    }
  }
  play_sfx(SFX_BUTTON);
  if (cancelled) return -1;

  // Pausa breve (0.1s, 6 cuadros a 60fps) con la flechita de siempre
  // (">", NADA de corchetes) sobre la elegida, asi se ve un toque antes
  // de arrancar la partida -- no un frenazo largo, solo lo justo para
  // que no se sienta instantaneo.
  for (int f = 0; f < 6; f++) {
    bottom();
    consoleClear();
    iprintf("\x1b[4;6HElegi la dificultad\n\x1b[5;9Hde los bots");
    for (int i = 0; i < 3; i++) {
      iprintf("\x1b[%d;6H%s %s", DIFF_PICK_ROW0 + i * DIFF_PICK_ROW_STRIDE, i == chosen ? ">" : " ", LABELS[i]);
    }
    vsync();
    update_title_logo_idle(); // sigue a la vista de fondo, con su vaiven -- hasta que arranca la partida
  }
  return VALUES[chosen];
}

// ---------- Opciones (desde el menu principal) ----------
#define OPTIONS_ROW0 8
#define OPTIONS_ROW_STRIDE 2
// Cambiar nombre, Musica, Sonido, Mismo fondo?, Fondo(s) (1 o 2 filas
// segun ese toggle), Volver -- 6 filas si es el mismo fondo arriba y
// abajo, 7 si no.
#define OPTIONS_MAX_COUNT 7

typedef enum {
  OPT_NAME, OPT_MUSIC, OPT_SOUND, OPT_SAME_BG, OPT_BG_SINGLE, OPT_BG_TOP, OPT_BG_BOTTOM, OPT_BACK
} OptionAction;

// Cambiar nombre / Musica SI-NO / Sonido SI-NO / Mismo fondo? SI-NO /
// Fondo(s) / Volver. D-pad arriba/abajo + A, o tocar directamente la
// opcion -- mismo criterio que choose_difficulty. La cantidad de filas
// varia segun "Mismo fondo?" (una fila de fondo si es SI, dos si es
// NO), asi que cada fila se arma con una ACCION (OptionAction) en vez
// de un indice fijo -- la posicion de "Volver" se corre segun el caso.
static void run_options_menu(char* playerName) {
  int cursor = 0;
  while (1) {
    char labels[OPTIONS_MAX_COUNT][32];
    OptionAction actions[OPTIONS_MAX_COUNT];
    int count = 0;

    strcpy(labels[count], "Cambiar nombre");
    actions[count] = OPT_NAME;
    count++;
    snprintf(labels[count], sizeof(labels[count]), "Musica: %s", musicEnabled ? "SI" : "NO");
    actions[count] = OPT_MUSIC;
    count++;
    snprintf(labels[count], sizeof(labels[count]), "Sonido: %s", soundEnabled ? "SI" : "NO");
    actions[count] = OPT_SOUND;
    count++;
    snprintf(labels[count], sizeof(labels[count]), "Fondo arriba = abajo: %s", bgSameTopBottom ? "SI" : "NO");
    actions[count] = OPT_SAME_BG;
    count++;
    if (bgSameTopBottom) {
      snprintf(labels[count], sizeof(labels[count]), "Fondo: %d", bgTopPatternIndex + 1);
      actions[count] = OPT_BG_SINGLE;
      count++;
    } else {
      snprintf(labels[count], sizeof(labels[count]), "Fondo de arriba: %d", bgTopPatternIndex + 1);
      actions[count] = OPT_BG_TOP;
      count++;
      snprintf(labels[count], sizeof(labels[count]), "Fondo de abajo: %d", bgBottomPatternIndex + 1);
      actions[count] = OPT_BG_BOTTOM;
      count++;
    }
    strcpy(labels[count], "Volver");
    actions[count] = OPT_BACK;
    count++;

    if (cursor >= count) cursor = count - 1; // veniamos de "NO" con el cursor en la 2da fila de fondo

    top();
    consoleClear();
    bottom();
    consoleClear();
    iprintf("\x1b[4;6HOpciones");
    for (int i = 0; i < count; i++) {
      iprintf("\x1b[%d;4H%s%s", OPTIONS_ROW0 + i * OPTIONS_ROW_STRIDE, i == cursor ? "> " : "  ", labels[i]);
    }

    vsync();
    update_title_logo_idle(); // sigue a la vista de fondo, con su vaiven
    scanKeys();
    touchPosition touch;
    touchRead(&touch);
    int pressed = keysDown();

    if (pressed & KEY_UP) { cursor = (cursor + count - 1) % count; play_sfx(SFX_BUTTON); }
    if (pressed & KEY_DOWN) { cursor = (cursor + 1) % count; play_sfx(SFX_BUTTON); }

    int activated = -1;
    if (pressed & KEY_A) {
      activated = cursor;
    } else if (pressed & KEY_TOUCH) {
      int row = touch.py / 8;
      for (int i = 0; i < count; i++) {
        if (row >= OPTIONS_ROW0 + i * OPTIONS_ROW_STRIDE && row < OPTIONS_ROW0 + i * OPTIONS_ROW_STRIDE + OPTIONS_ROW_STRIDE) {
          cursor = i;
          activated = i;
        }
      }
    } else if (pressed & (KEY_START | KEY_B)) {
      activated = count - 1; // START/B tambien vuelven, atajo rapido -- Volver siempre es la ultima fila
    }
    if (activated < 0) continue;
    play_sfx(SFX_BUTTON);

    switch (actions[activated]) {
      case OPT_NAME: {
        char newName[PLAYER_NAME_MAX + 1];
        if (edit_player_name(newName)) {
          strcpy(playerName, newName);
          save_player_name(playerName);
        }
        break;
      }
      case OPT_MUSIC:
        musicEnabled = !musicEnabled;
        save_settings();
        break;
      case OPT_SOUND:
        soundEnabled = !soundEnabled;
        save_settings();
        break;
      case OPT_SAME_BG:
        bgSameTopBottom = !bgSameTopBottom;
        save_settings();
        apply_bg_pattern(bgPatternBottom, effective_bg_bottom_index());
        break;
      case OPT_BG_SINGLE:
        bgTopPatternIndex = (bgTopPatternIndex + 1) % BG_PATTERN_COUNT;
        save_settings();
        apply_bg_pattern(bgPatternBottom, effective_bg_bottom_index());
        break;
      case OPT_BG_TOP:
        bgTopPatternIndex = (bgTopPatternIndex + 1) % BG_PATTERN_COUNT;
        save_settings();
        break;
      case OPT_BG_BOTTOM:
        bgBottomPatternIndex = (bgBottomPatternIndex + 1) % BG_PATTERN_COUNT;
        save_settings();
        apply_bg_pattern(bgPatternBottom, effective_bg_bottom_index());
        break;
      case OPT_BACK:
        return; // Volver
    }
  }
}

// ---------- Tutorial ----------
// Mismo contenido que TUTORIAL_STEPS de la version web (js/ui.js,
// proyecto bazas-cartas) condensado para la pantalla de abajo de la DS
// (32x24, sin acentos ni ñ -- ver la limitacion de fuente ya conocida).
// Paginas cortas, D-pad izq/der (o A) para pasar, START para salir en
// cualquier momento.
#define TUTORIAL_STEP_COUNT 9
#define TUTORIAL_MAX_LINES 14
#define TUTORIAL_MAX_CARDS 5

typedef struct {
  const char* title;
  const char* lines[TUTORIAL_MAX_LINES];
  int lineCount;
  Suit cardSuits[TUTORIAL_MAX_CARDS];
  int cardValues[TUTORIAL_MAX_CARDS];
  int cardCount; // 0 = pagina sin cartas
} TutorialStep;

static const TutorialStep TUTORIAL_STEPS[TUTORIAL_STEP_COUNT] = {
  { "Bienvenido a Bazas", {
      "Bazas es un juego de",
      "PREDICCION: en cada mano",
      "tenes que adivinar cuantas",
      "bazas (rondas) vas a",
      "ganar, y despues cumplirlo",
      "justo -- ni una mas, ni",
      "una menos.",
      "",
      "Te mostramos como se juega",
      "en pocos pasos cortos.",
    }, 10, {0}, {0}, 0 },
  { "El mazo", {
      "Se juega con un mazo de",
      "48 cartas: 4 palos (oro,",
      "copa, espada, basto), del",
      "1 al 12, sin comodines.",
    }, 4,
    { SUIT_ORO, SUIT_COPA, SUIT_ESPADA, SUIT_BASTO },
    { 7, 7, 7, 7 }, 4 },
  { "La jerarquia de las cartas", {
      "Gana la baza quien jugo la",
      "carta mas FUERTE -- no",
      "importa el palo. En",
      "general, mas alto el",
      "numero, mas fuerte.",
      "",
      "OJO: el As de Espada y el",
      "As de Basto son las MAS",
      "FUERTES de todas (le ganan",
      "hasta al 12). El As de Oro",
      "y el As de Copa son al",
      "reves: las mas debiles.",
    }, 12,
    { SUIT_COPA, SUIT_ORO, SUIT_BASTO, SUIT_BASTO, SUIT_ESPADA },
    { 1, 1, 12, 1, 1 }, 5 },
  { "Predecir", {
      "Antes de jugar, cada uno",
      "predice por turno cuantas",
      "bazas va a ganar (entre 0",
      "y la cantidad de cartas).",
      "",
      "El ultimo en predecir NO",
      "puede elegir el numero",
      "que haga que la suma de",
      "todos de EXACTO la",
      "cantidad de cartas --",
      "asi nunca pueden acertar",
      "todos a la vez.",
    }, 12, {0}, {0}, 0 },
  { "Jugar una baza", {
      "Quien abre juega cualquier",
      "carta. Los demas, en",
      "orden, tambien -- NO hace",
      "falta tener el mismo palo.",
      "",
      "Se lleva la baza quien",
      "jugo la carta mas fuerte.",
      "Esa persona abre la",
      "baza siguiente.",
      "",
      "Si hay EMPATE (misma",
      "carta repetida), gana",
      "quien la jugo PRIMERO.",
    }, 13, {0}, {0}, 0 },
  { "Vidas", {
      "Cada jugador arranca con",
      "6 vidas (los corazones que",
      "ves abajo en pantalla).",
      "",
      "Si no ganaste EXACTO lo",
      "que predijiste, perdes UNA",
      "vida por cada baza de",
      "diferencia.",
      "",
      "Ej: predijiste 2 y",
      "ganaste 4 -> perdes 2",
      "vidas. Al quedarte sin",
      "vidas quedas eliminado --",
      "gana quien queda ultimo.",
    }, 14, {0}, {0}, 0 },
  { "El tamano de mano cambia", {
      "La primera mano se",
      "reparten 6 cartas. Cada",
      "mano siguiente, una menos,",
      "hasta llegar a 1 -- ahi",
      "cada jugada pesa",
      "muchisimo.",
      "",
      "Despues vuelve a subir",
      "hasta 6, y se repite",
      "el ciclo.",
    }, 10, {0}, {0}, 0 },
  { "Consejos", {
      "- Predecir 0 es mas facil",
      "con cartas bajas y",
      "variadas en palo.",
      "",
      "- Guardate los anchos",
      "fuertes para el momento",
      "justo.",
      "",
      "- Fijate que jugaron los",
      "demas antes de tirar.",
      "",
      "- Si ya llegaste a tu",
      "numero, juga tus cartas",
      "mas debiles.",
    }, 14, {0}, {0}, 0 },
  { "Listo para jugar", {
      "Eso es todo lo que",
      "necesitas saber para",
      "arrancar.",
      "",
      "Volve al menu y elegi",
      "Jugar cuando quieras.",
    }, 6, {0}, {0}, 0 },
};

#define TUTORIAL_CARD_Y 128 // entre el texto (la pagina con cartas mas larga termina en fila 14, y=120) y el indicador de pagina (fila 21, y=168)

static void render_tutorial_step(int step) {
  const TutorialStep* s = &TUTORIAL_STEPS[step];
  top();
  consoleClear();
  bottom();
  consoleClear();
  iprintf("\x1b[1;2H%s", s->title);
  for (int i = 0; i < s->lineCount; i++) {
    iprintf("\x1b[%d;2H%s", 3 + i, s->lines[i]);
  }
  iprintf("\x1b[21;2HPagina %d/%d", step + 1, TUTORIAL_STEP_COUNT);
  if (step == TUTORIAL_STEP_COUNT - 1) {
    iprintf("\x1b[22;2HA o START: al menu");
  } else {
    iprintf("\x1b[22;2HA avanza  START sale");
  }

  // Cartas de ejemplo (paginas "El mazo" y "La jerarquia") -- se
  // reusan los mismos ids/buffers de sprite que la mano de juego
  // (0..HAND_SLOT_COUNT-1, handGfx) ya que el tutorial nunca corre al
  // mismo tiempo que una partida.
  int cardsW = s->cardCount * CARD_W + (s->cardCount - 1) * 2;
  int startX = (256 - cardsW) / 2;
  for (int i = 0; i < s->cardCount; i++) {
    dmaCopy(card_tiles_for(s->cardSuits[i], s->cardValues[i]), handGfx[i], CARD_TILE_BYTES);
    oamSet(&oamSub, i, startX + i * (CARD_W + 2), TUTORIAL_CARD_Y, 0, 0, SpriteSize_32x32, SpriteColorFormat_256Color,
      handGfx[i], -1, false, false, false, false, false);
  }
  for (int i = s->cardCount; i < HAND_SLOT_COUNT; i++) {
    oamClearSprite(&oamSub, i);
  }
  oamUpdate(&oamSub);
}

static void run_tutorial(void) {
  int step = 0;
  render_tutorial_step(step);
  while (1) {
    vsync();
    update_title_logo_idle(); // sigue a la vista de fondo, con su vaiven
    scanKeys();
    int pressed = keysDown();

    if (pressed & (KEY_START | KEY_B)) {
      play_sfx(SFX_BUTTON);
      break;
    }
    if (pressed & KEY_LEFT) {
      if (step > 0) {
        step--;
        play_sfx(SFX_BUTTON);
        render_tutorial_step(step);
      }
      continue;
    }
    if (pressed & (KEY_RIGHT | KEY_A | KEY_TOUCH)) {
      play_sfx(SFX_BUTTON);
      if (step < TUTORIAL_STEP_COUNT - 1) {
        step++;
        render_tutorial_step(step);
      } else {
        break; // ultima pagina -- confirma y vuelve al menu
      }
      continue;
    }
  }
  for (int i = 0; i < HAND_SLOT_COUNT; i++) oamClearSprite(&oamSub, i);
  oamUpdate(&oamSub);
}

static void run_match(char* playerName, Difficulty botDiff); // definida mas abajo, junto al resto del loop principal
static void render_title_logo(void); // definida mas abajo, junto al resto de la pantalla de arriba
static void hide_title_logo(void); // idem

// ---------- Menu principal ----------
#define MAIN_MENU_ROW0 10
#define MAIN_MENU_ROW_STRIDE 2
#define MAIN_MENU_COUNT 3

// Si se vuelve al menu a mitad de partida (pausa -> "Volver al menu")
// run_match() corta de golpe, dejando prendidos los sprites de
// cualquier pantalla que estuviera mostrando en ese momento (mano,
// botones de prediccion, iconos de esquina, barra de estado, etc.) --
// nada los esconde a mano porque nunca se llega al render_top/hide_*
// normal de esa fase. Se esconden TODOS de una (oamMain y oamSub
// enteros) antes de volver a mostrar el menu; el logo de titulo se
// vuelve a prender aparte, despues, con render_title_logo().
static void hide_all_gameplay_sprites(void) {
  for (int i = 0; i < SPRITE_COUNT; i++) {
    oamClearSprite(&oamMain, i);
    oamClearSprite(&oamSub, i);
  }
  oamUpdate(&oamMain);
  oamUpdate(&oamSub);
}

// Pantalla de fin de partida (arriba). Se llama justo antes de esperar
// la respuesta del jugador (run_match_end_menu, tanto en victoria como
// en derrota) -- ese menu no vuelve a tocar la pantalla de arriba por
// su cuenta mientras espera, asi que hay que dejarla lista antes.
//
// SOLO el texto ("Ganaste"/"Perdiste", agrandado 2x), centrado, sobre
// el fondo de siempre CON su paneo (restore_pattern_background), no el
// panel de colores de fin de mano que deja render_top() en la mano en
// la que el jugador quedo eliminado.
//
// Limpia TODOS los sprites de la partida en curso primero (mesa, mano,
// corazones, corona, etc.) y el texto de consola de arriba, para que
// no quede nada de la ultima baza/panel asomando detras.
static void show_outcome_screen(int won) {
  hide_all_gameplay_sprites();
  top();
  consoleClear();
  restore_pattern_background();

  const unsigned int* textTiles = won ? win_textTiles : lose_textTiles;
  unsigned int textTilesLen = won ? win_textTilesLen : lose_textTilesLen;
  dmaCopy(textTiles, outcomeTextGfx, textTilesLen);
  oamSet(&oamMain, OUTCOME_TEXT_OAM_ID, OUTCOME_TEXT_X, OUTCOME_TEXT_Y, 0, OUTCOME_SCREEN_PALETTE_BANK,
    SpriteSize_64x32, SpriteColorFormat_16Color, outcomeTextGfx, OUTCOME_TEXT_AFFINE_ID, true, false, false, false, false);
  oamUpdate(&oamMain);
}

static void hide_outcome_screen(void) {
  oamClearSprite(&oamMain, OUTCOME_TEXT_OAM_ID);
  oamUpdate(&oamMain);
}

// Primera pantalla real del juego (reemplaza al boton "jugar" solo):
// Jugar lleva directo al selector de dificultad y arranca una partida
// con el nombre ya guardado (no se vuelve a preguntar cada vez -- eso
// ahora es cosa de Opciones); Opciones abre el submenu de arriba;
// Tutorial (debajo de Opciones, como se pidio) explica las reglas.
static void show_main_menu(char* playerName) {
  static const char* LABELS[MAIN_MENU_COUNT] = { "Jugar", "Opciones", "Tutorial" };
  int cursor = 0;
  // show_main_menu se llama UNA sola vez en todo el programa (ver
  // main()) -- esta bandera solo distingue el primerisimo cuadro
  // dibujado, para hacer el fundido de entrada justo esa vez (las dos
  // pantallas arrancan en negro desde el fundido de salida del splash,
  // ver main()). Nunca se vuelve a disparar despues (volver de una
  // partida/opciones/tutorial sigue reusando el mismo while(1)).
  int firstFrame = 1;

  while (1) {
    top();
    consoleClear();
    bottom();
    consoleClear();
    for (int i = 0; i < MAIN_MENU_COUNT; i++) {
      iprintf("\x1b[%d;9H%s%s", MAIN_MENU_ROW0 + i * MAIN_MENU_ROW_STRIDE, i == cursor ? "> " : "  ", LABELS[i]);
    }
    // Dos lineas a proposito (no una sola larga que se corte/desborde
    // en un wrap automatico impredecible) -- la fila 23 (la ultima del
    // todo) queda parcialmente tapada por el borde de la pantalla real,
    // asi que ademas se corrio todo un par de filas mas arriba.
    iprintf("\x1b[20;2HCreado por Agustin Cavalie");
    iprintf("\x1b[21;2H/Z-Dextiny");

    vsync();
    update_title_logo_idle(); // sigue a la vista de fondo, con su vaiven

    if (firstFrame) {
      firstFrame = 0;
      // Solo la de ABAJO -- la de arriba ya esta en brillo normal desde
      // antes de render_title_logo() (ver main()), para que ESA
      // animacion (el logo entrando) se vea de una.
      for (int f = 0; f < SPLASH_FADE_FRAMES; f++) {
        setBrightness(2, -16 + (16 * f) / SPLASH_FADE_FRAMES);
        swiWaitForVBlank();
      }
      setBrightness(2, 0);
    }

    scanKeys();
    touchPosition touch;
    touchRead(&touch);
    int pressed = keysDown();

    if (pressed & KEY_UP) { cursor = (cursor + MAIN_MENU_COUNT - 1) % MAIN_MENU_COUNT; play_sfx(SFX_BUTTON); }
    if (pressed & KEY_DOWN) { cursor = (cursor + 1) % MAIN_MENU_COUNT; play_sfx(SFX_BUTTON); }

    int activated = -1;
    if (pressed & KEY_A) {
      activated = cursor;
    } else if (pressed & KEY_TOUCH) {
      int row = touch.py / 8;
      for (int i = 0; i < MAIN_MENU_COUNT; i++) {
        if (row >= MAIN_MENU_ROW0 + i * MAIN_MENU_ROW_STRIDE && row < MAIN_MENU_ROW0 + i * MAIN_MENU_ROW_STRIDE + MAIN_MENU_ROW_STRIDE) {
          cursor = i;
          activated = i;
        }
      }
    }
    if (activated < 0) continue;
    play_sfx(SFX_BUTTON);

    if (activated == 0) {
      int botDiff = choose_difficulty(); // -1 = cancelado con B, volver directo al menu
      if (botDiff < 0) continue;
      // OJO: el microfono (soplido del logo, ver update_title_logo_idle)
      // solo hace falta en el menu, pero apagarlo/prenderlo de nuevo
      // aca (soundMicOff antes de la partida + setup_mic al volver)
      // dejaba TODO el audio mudo (efectos, a veces la musica tambien)
      // si se entraba a jugar "de una" desde el arranque -- entrar a
      // Opciones/Tutorial primero (mas tiempo con el mic ya estable
      // antes de tocarlo) lo evitaba, lo que apunta a una condicion de
      // carrera al reconfigurar el timer del mic a mitad de sesion.
      // Se deja corriendo TODO el tiempo en vez de esto (arranca una
      // sola vez en main(), nunca se vuelve a tocar) -- ya viene a un
      // ritmo bajo (2000Hz, ver MIC_SAMPLE_RATE) que en su momento
      // resolvio el audio saturado sin necesidad de apagarlo.
      run_match(playerName, (Difficulty)botDiff); // vuelve aca cuando se pide "volver al menu" desde la pausa
      // La partida pudo cortarse a mitad de cualquier pantalla -- se
      // limpia TODO (sprites Y el texto de las dos consolas -- este
      // ultimo quedaba pegado, por ejemplo "Mano N (X cartas)" o los
      // puntajes de las esquinas, con el logo animando por encima)
      // antes de restaurar el fondo/logo del menu, para que primero
      // desaparezca lo de la partida y RECIEN AHI aparezca el menu.
      hide_all_gameplay_sprites();
      top();
      consoleClear();
      bottom();
      consoleClear();
      show_title_background();
      render_title_logo();
    } else if (activated == 1) {
      run_options_menu(playerName);
    } else {
      run_tutorial();
    }
  }
}

// ---------- Pantalla de arriba: la mesa ----------

// Esconde el avatar y los corazones de fin de mano -- solo tienen
// sentido en esa pantalla, si no se esconden a mano se quedan viendose
// pegados encima de la cruz/las esquinas en las demas fases.
static void hide_hand_end_icons(void) {
  for (int i = 0; i < 4; i++) {
    oamSet(&oamMain, AVATAR_OAM_BASE + i, 0, 0, 0, AVATAR_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
      avatarGfx, -1, false, true, false, false, false);
  }
  for (int i = 0; i < 4 * 6; i++) {
    oamSet(&oamMain, HEART_OAM_BASE + i, 0, 0, 0, HEART_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
      heartGfx[i], -1, false, true, false, false, false);
  }
  oamUpdate(&oamMain);
}

// Esconde los iconos de objetivo/trofeo de las 4 esquinas -- solo
// tienen sentido mientras se predice/juega (ver render_score_corners),
// si no se esconden a mano se quedan pegados encima del panel de fin
// de mano.
static void hide_corner_icons(void) {
  for (int i = 0; i < 4; i++) {
    oamSet(&oamMain, CORNER_ICON_PRED_OAM_BASE + i, 0, 0, 0, CORNER_ICON_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
      iconPredGfx, -1, false, true, false, false, false);
    oamSet(&oamMain, CORNER_ICON_WON_OAM_BASE + i, 0, 0, 0, CORNER_ICON_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
      iconWonGfx, -1, false, true, false, false, false);
  }
  oamUpdate(&oamMain);
}

// Logo "BAZADO": posicion FIJA en pantalla (4 sprites de 64x64 en
// fila, cubren el ancho entero), independiente de como panee/cicle el
// remolino de atras -- justo lo que se pidio ("deja el texto de
// bazado quieto").
//
// Con doble tamano (sizeDouble) el hardware reserva SIEMPRE el doble
// del nativo (64->128) para el recorte, sin importar la escala real
// que se use -- confirmado con el boton de jugar (ver PLAY_BUTTON_OAM_X)
// y los corazones (ver HAND_END_HEART_VISUAL_Y): la posicion durante
// el squash necesita ese padding fijo (128-64)/2=32 sumado.
#define TITLE_LOGO_SQUASH_PAD 32

// Curva del squash: simetrica, comprime a mitad de camino y vuelve a
// 1x. Devuelve el factor INVERSO de escala en Y (256 = 1x normal, mas
// alto = mas comprimido -- ver el comentario grande de CARD_SCALE_INV
// mas arriba sobre el inverso que pide oamRotateScale).
static int title_logo_squash_scale_inv(int f) {
  int half = TITLE_LOGO_SQUASH_FRAMES / 2;
  int amount = (f <= half) ? f : (TITLE_LOGO_SQUASH_FRAMES - f);
  return 256 + (amount * 110) / half; // 256 (1x) hasta ~366 (~0.7x) y vuelta
}

// Entrada animada: cada pieza cae desde arriba (ease-out) con una
// demora escalonada, y al llegar a su lugar hace un squash breve antes
// de asentarse del todo. Bloqueante (dura ANIM_TOTAL_FRAMES cuadros) --
// se llama una sola vez por pantalla de titulo, no hace falta que sea
// asincronica.
static void render_title_logo(void) {
  int totalFrames = (TITLE_LOGO_LETTER_COUNT - 1) * TITLE_LOGO_STAGGER_FRAMES + TITLE_LOGO_DROP_FRAMES + TITLE_LOGO_SQUASH_FRAMES;

  for (int frame = 0; frame < totalFrames; frame++) {
    for (int i = 0; i < TITLE_LOGO_LETTER_COUNT; i++) {
      int local = frame - i * TITLE_LOGO_STAGGER_FRAMES;
      int visualX = TITLE_LOGO_LETTER_X[i];

      if (local < 0) {
        oamSet(&oamMain, TITLE_LOGO_OAM_BASE + i, 0, 0, 0, TITLE_LOGO_PALETTE_BANK,
          SpriteSize_64x64, SpriteColorFormat_16Color, titleLogoGfx[i], -1, false, true, false, false, false);
      } else if (local < TITLE_LOGO_DROP_FRAMES) {
        // Cayendo: ease-out cuadratico (rapido al arrancar, frena
        // llegando) desde arriba de pantalla hasta el lugar final.
        int t = (local * 256) / TITLE_LOGO_DROP_FRAMES; // 0..256
        int eased = 256 - (((256 - t) * (256 - t)) >> 8);
        int y = TITLE_LOGO_DROP_FROM_Y + ((TITLE_LOGO_Y - TITLE_LOGO_DROP_FROM_Y) * eased) / 256;
        oamSet(&oamMain, TITLE_LOGO_OAM_BASE + i, visualX, y, 0, TITLE_LOGO_PALETTE_BANK,
          SpriteSize_64x64, SpriteColorFormat_16Color, titleLogoGfx[i], -1, false, false, false, false, false);
      } else if (local < TITLE_LOGO_DROP_FRAMES + TITLE_LOGO_SQUASH_FRAMES) {
        int squashScaleInv = title_logo_squash_scale_inv(local - TITLE_LOGO_DROP_FRAMES);
        oamRotateScale(&oamMain, TITLE_LOGO_AFFINE_BASE + i, 0, 256, squashScaleInv);
        oamSet(&oamMain, TITLE_LOGO_OAM_BASE + i, visualX - TITLE_LOGO_SQUASH_PAD, TITLE_LOGO_Y - TITLE_LOGO_SQUASH_PAD, 0,
          TITLE_LOGO_PALETTE_BANK, SpriteSize_64x64, SpriteColorFormat_16Color, titleLogoGfx[i],
          TITLE_LOGO_AFFINE_BASE + i, true, false, false, false, false);
      } else {
        // Ya se acomodo del todo -- posicion final de siempre, sin afin.
        oamSet(&oamMain, TITLE_LOGO_OAM_BASE + i, visualX, TITLE_LOGO_Y, 0, TITLE_LOGO_PALETTE_BANK,
          SpriteSize_64x64, SpriteColorFormat_16Color, titleLogoGfx[i], -1, false, false, false, false, false);
      }
    }
    oamUpdate(&oamMain);
    vsync();
  }

  for (int i = 0; i < TITLE_LOGO_LETTER_COUNT; i++) {
    oamSet(&oamMain, TITLE_LOGO_OAM_BASE + i, TITLE_LOGO_LETTER_X[i], TITLE_LOGO_Y, 0, TITLE_LOGO_PALETTE_BANK,
      SpriteSize_64x64, SpriteColorFormat_16Color, titleLogoGfx[i], -1, false, false, false, false, false);
  }
  oamUpdate(&oamMain);
}

// Vaiven leve tipo "levitando" mientras el logo esta quieto en su
// lugar (fuera de las animaciones de entrada/salida de arriba) -- se
// llama UNA vez por cuadro desde el loop de las pantallas que lo dejan
// a la vista (menu principal, Opciones, Tutorial). Onda triangular
// simple (misma idea que title_bg_pan_offset, sin trigonometria) con
// una fase distinta por letra para que floten como una ola en vez de
// subir y bajar todas juntas en bloque.
#define TITLE_LOGO_IDLE_AMPLITUDE 3 // px para cada lado en reposo -- "leve"
#define TITLE_LOGO_IDLE_BLOW_AMPLITUDE 14 // mientras se sopla -- bien mas violento, para que se note la diferencia
#define TITLE_LOGO_IDLE_PERIOD_FRAMES 180 // ~3s a 60fps, un ciclo completo arriba-abajo-arriba
#define TITLE_LOGO_IDLE_PHASE_STEP 14 // corrimiento entre el vaiven de una letra y la siguiente

static int title_logo_idle_offset(int accum, int amplitude) {
  int half = TITLE_LOGO_IDLE_PERIOD_FRAMES / 2;
  int a = accum % TITLE_LOGO_IDLE_PERIOD_FRAMES;
  int tri = (a < half) ? a : (TITLE_LOGO_IDLE_PERIOD_FRAMES - a); // 0..half..0
  return (tri * amplitude * 2) / half - amplitude; // -amplitude..+amplitude..-amplitude
}

// Soplando fuerte al microfono, el pico de amplitud se dispara bien
// por encima del ruido ambiente normal -- el umbral esta alto a
// proposito (hace falta un soplido bien directo, cerca del microfono,
// no cualquier ruido de fondo) -- capaz hace falta ajustarlo probando
// en hardware real (en melonDS depende de si hay un microfono real
// pasado al emulador).
#define MIC_BLOW_THRESHOLD 12000
#define MIC_BLOW_SPEED_MULT 4 // cuanto mas rapido corre el vaiven mientras dura el soplido a fondo

// Transicion entre reposo y soplido a fondo: un lerp ("intensity", 0 =
// reposo, 256 = soplido a fondo, punto fijo /256) en vez de saltar de
// golpe -- se not aba brusco pasar de un salto en seco a estatico.
// Maneja amplitud Y velocidad JUNTAS desde el mismo valor asi las dos
// suben/bajan a la vez, sin quedar descoordinadas. La fase del vaiven
// tambien se guarda en punto fijo (accum256) para que la velocidad
// pueda variar de a poco sin ningun salto brusco de fase tampoco.
#define TITLE_LOGO_IDLE_LERP_STEP 12 // ~21 cuadros (~0.35s a 60fps) para ir de un extremo al otro

static void update_title_logo_idle(void) {
  static int accum256 = 0;
  static int intensity = 0; // 0..256

  int target = (micLevel > MIC_BLOW_THRESHOLD) ? 256 : 0;
  if (intensity < target) {
    intensity += TITLE_LOGO_IDLE_LERP_STEP;
    if (intensity > target) intensity = target;
  } else if (intensity > target) {
    intensity -= TITLE_LOGO_IDLE_LERP_STEP;
    if (intensity < target) intensity = target;
  }

  int amplitude = TITLE_LOGO_IDLE_AMPLITUDE + ((TITLE_LOGO_IDLE_BLOW_AMPLITUDE - TITLE_LOGO_IDLE_AMPLITUDE) * intensity) / 256;
  int speed256 = 256 + ((MIC_BLOW_SPEED_MULT * 256 - 256) * intensity) / 256; // 256 = 1x .. MULT*256 = MULTx
  accum256 += speed256;
  int accum = accum256 / 256;

  for (int i = 0; i < TITLE_LOGO_LETTER_COUNT; i++) {
    int y = TITLE_LOGO_Y + title_logo_idle_offset(accum + i * TITLE_LOGO_IDLE_PHASE_STEP, amplitude);
    oamSet(&oamMain, TITLE_LOGO_OAM_BASE + i, TITLE_LOGO_LETTER_X[i], y, 0, TITLE_LOGO_PALETTE_BANK,
      SpriteSize_64x64, SpriteColorFormat_16Color, titleLogoGfx[i], -1, false, false, false, false, false);
  }
  oamUpdate(&oamMain);
}

// Salida animada: cada pieza sube derecho hacia arriba (ease-in) con
// el mismo escalonado que la entrada, sin squash -- "cuando arranca a
// jugar las letras suben una a una hacia arriba". Bloqueante, se llama
// justo antes de arrancar la partida (ver run_match).
static void hide_title_logo(void) {
  int totalFrames = (TITLE_LOGO_LETTER_COUNT - 1) * TITLE_LOGO_STAGGER_FRAMES + TITLE_LOGO_DROP_FRAMES;

  for (int frame = 0; frame < totalFrames; frame++) {
    for (int i = 0; i < TITLE_LOGO_LETTER_COUNT; i++) {
      int local = frame - i * TITLE_LOGO_STAGGER_FRAMES;
      int visualX = TITLE_LOGO_LETTER_X[i];

      if (local < 0) {
        oamSet(&oamMain, TITLE_LOGO_OAM_BASE + i, visualX, TITLE_LOGO_Y, 0, TITLE_LOGO_PALETTE_BANK,
          SpriteSize_64x64, SpriteColorFormat_16Color, titleLogoGfx[i], -1, false, false, false, false, false);
      } else if (local < TITLE_LOGO_DROP_FRAMES) {
        int t = (local * 256) / TITLE_LOGO_DROP_FRAMES; // 0..256
        int eased = (t * t) >> 8; // ease-in: arranca lento, acelera subiendo
        int y = TITLE_LOGO_Y - ((TITLE_LOGO_Y - TITLE_LOGO_DROP_FROM_Y) * eased) / 256;
        oamSet(&oamMain, TITLE_LOGO_OAM_BASE + i, visualX, y, 0, TITLE_LOGO_PALETTE_BANK,
          SpriteSize_64x64, SpriteColorFormat_16Color, titleLogoGfx[i], -1, false, false, false, false, false);
      } else {
        oamSet(&oamMain, TITLE_LOGO_OAM_BASE + i, 0, 0, 0, TITLE_LOGO_PALETTE_BANK,
          SpriteSize_64x64, SpriteColorFormat_16Color, titleLogoGfx[i], -1, false, true, false, false, false);
      }
    }
    oamUpdate(&oamMain);
    vsync();
  }

  for (int i = 0; i < TITLE_LOGO_LETTER_COUNT; i++) {
    oamSet(&oamMain, TITLE_LOGO_OAM_BASE + i, 0, 0, 0, TITLE_LOGO_PALETTE_BANK,
      SpriteSize_64x64, SpriteColorFormat_16Color, titleLogoGfx[i], -1, false, true, false, false, false);
  }
  oamUpdate(&oamMain);
}

// Fin de mano: nombre (a la izquierda, centrado verticalmente en su
// panel) + una fila de 6 corazones por jugador -- llenos son vidas que
// quedan, rotos (tachados con una X) las que ya perdio. Sin avatar (se
// saco el cuadrado placeholder); los corazones se ven mas grandes
// (1.5x) aprovechando ese espacio. Cada fila va sobre su panel de
// color propio (ver score_bg.png).
static void render_hand_end_rows(const GameState* state) {
  for (int i = 0; i < state->playerCount; i++) {
    const Player* p = &state->players[i];
    int textRow = HAND_END_TEXT_ROW[i];
    int heartY = HAND_END_HEART_VISUAL_Y(i);

    if (p->eliminated) {
      iprintf("\x1b[%d;2H%-8s ELIM", textRow, p->name);
    } else {
      iprintf("\x1b[%d;2H%-8s", textRow, p->name);
    }

    for (int h = 0; h < GAME_MAX_PENALTIES; h++) {
      u16* gfx = heartGfx[i * GAME_MAX_PENALTIES + h];
      int lost = h < p->penalties; // los primeros "penalties" corazones se ven rotos
      dmaCopy(lost ? heart_brokenTiles : heart_fullTiles, gfx, heart_fullTilesLen);
      int x = HAND_END_HEART_VISUAL_X0 + h * HAND_END_HEART_VISUAL_STRIDE;
      oamSet(&oamMain, HEART_OAM_BASE + i * GAME_MAX_PENALTIES + h, x, heartY, 0, HEART_PALETTE_BANK,
        SpriteSize_16x16, SpriteColorFormat_16Color, gfx, HAND_END_HEART_AFFINE_ID, true, false, false, false, false);
    }
  }
  oamUpdate(&oamMain);
}

// Mientras se predice, pantalla completa (todavia no hay cartas en la
// mesa asi que no compite con la cruz de sprites): un jugador por
// esquina con su nombre, lo que predijo y cuantas bazas lleva esta
// mano, inspirado en la version web -- y una flechita al lado del
// nombre de quien tiene que predecir ahora. Una vez que se empieza a
// jugar, la cruz de cartas ocupa la mayor parte de la pantalla y el
// texto desaparece del todo para que quede limpia.
// Nombre + lo que predijo + cuantas se llevo esta mano, una esquina por
// jugador (ver CORNER_ROW/COL) -- se usa mientras se predice Y mientras
// se juega, para tener siempre a la vista quien va ganando/perdiendo.
// Icono de objetivo (cuanto predijo) + icono de trofeo (cuantas lleva
// ganadas), en vez de las letras sueltas "P"/"G" -- arte del usuario
// (ver make-corner-icons.ps1). La prediccion y las bazas ganadas nunca
// pasan de MAX_HAND_SIZE (6), asi que el numero siempre es UN solo
// digito ("?" tambien) -- no hace falta reservarle 2 columnas, eso era
// justo el aire de mas que sobraba entre cada icono y su numero.
// Verticalmente arranca DEBAJO del renglon del nombre con un pequeño
// aire (2px), sin invadirlo como antes.
static void render_score_corners(const GameState* state) {
  for (int i = 0; i < state->playerCount; i++) {
    const Player* p = &state->players[i];
    int row = CORNER_ROW[i];
    int col = CORNER_COL[i];
    iprintf("\x1b[%d;%dH%-8s", row, col, p->name);

    int iconY = (row + 1) * 8 + 2;
    int iconPredX = col * 8;
    int iconWonX = (col + 4) * 8;

    if (p->eliminated) {
      iprintf("\x1b[%d;%dHELIMINADO", row + 1, col);
      oamSet(&oamMain, CORNER_ICON_PRED_OAM_BASE + i, 0, 0, 0, CORNER_ICON_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
        iconPredGfx, -1, false, true, false, false, false);
      oamSet(&oamMain, CORNER_ICON_WON_OAM_BASE + i, 0, 0, 0, CORNER_ICON_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
        iconWonGfx, -1, false, true, false, false, false);
      continue;
    }

    char pred[2];
    if (p->prediction < 0) snprintf(pred, sizeof(pred), "?");
    else snprintf(pred, sizeof(pred), "%d", p->prediction);
    // icono(2) + numero(1) + aire(1) + icono(2) + numero(1).
    iprintf("\x1b[%d;%dH  %s   %d", row + 1, col, pred, p->tricksWon);

    oamSet(&oamMain, CORNER_ICON_PRED_OAM_BASE + i, iconPredX, iconY, 0, CORNER_ICON_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
      iconPredGfx, -1, false, false, false, false, false);
    oamSet(&oamMain, CORNER_ICON_WON_OAM_BASE + i, iconWonX, iconY, 0, CORNER_ICON_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
      iconWonGfx, -1, false, false, false, false, false);
  }
  oamUpdate(&oamMain);
}

static void render_top(const GameState* state) {
  render_trick_sprites(state);

  top();
  consoleClear();

  if (state->phase == PHASE_PREDICTING) {
    restore_pattern_background();
    hide_hand_end_icons();
    char heading[32];
    snprintf(heading, sizeof(heading), "Mano %d (%d cartas)", state->handNumber, state->handSize);
    int headingCol = (32 - (int)strlen(heading)) / 2;
    if (headingCol < 0) headingCol = 0;
    iprintf("\x1b[10;%dH%s", headingCol, heading);

    // Quien arranca la mano (state->trickLeaderId) ya esta decidido
    // desde el principio -- por la rotacion del que reparte, no tiene
    // nada que ver con el orden de predecir -- asi que se muestra
    // siempre aca, no solo despues de que el humano prediga (antes
    // quedaba escondido si el humano no era el ULTIMO en predecir esa
    // mano, aunque le tocara arrancar la baza igual).
    char leaderMsg[32];
    if (state->trickLeaderId == HUMAN_ID) {
      snprintf(leaderMsg, sizeof(leaderMsg), "Vos empezas");
    } else {
      snprintf(leaderMsg, sizeof(leaderMsg), "Empieza %s", state->players[state->trickLeaderId].name);
    }
    int leaderCol = (32 - (int)strlen(leaderMsg)) / 2;
    if (leaderCol < 0) leaderCol = 0;
    // Fila 14, no 11 -- la flecha de turno del centro de la cruz ocupa
    // las filas 11 a 13 (fija ahi), asi que mas arriba quedaba tapado.
    iprintf("\x1b[14;%dH%s", leaderCol, leaderMsg);

    render_score_corners(state);
    return;
  }

  // Jugando/fin de baza: las esquinas se quedan (para saber en todo
  // momento cuanto pidio y cuanto lleva cada uno, uno mismo incluido),
  // sin el titulo de "Mano N" -- ya no aporta nada util una vez que se
  // esta jugando. La flecha de turno (en el centro de la cruz, ver
  // render_trick_sprites) sigue apuntando a quien le toca jugar.
  if (state->phase == PHASE_PLAYING || state->phase == PHASE_TRICK_END) {
    restore_pattern_background();
    hide_hand_end_icons();
    render_score_corners(state);
    return;
  }

  // Fin de mano: panel de color por jugador (ver score_bg.png / make-
  // score-bg.ps1) con avatar (placeholder) + corazones de vida
  // (llenos/rotos).
  if (state->phase == PHASE_HAND_END) {
    hide_corner_icons();
    show_score_background();
    render_hand_end_rows(state);
    return;
  }

  restore_pattern_background();
  hide_hand_end_icons();
  hide_corner_icons();
  if (state->phase == PHASE_GAME_END) {
    iprintf("*** %s GANO LA PARTIDA ***\n", state->winner >= 0 ? state->players[state->winner].name : "Nadie");
  }
}

// ---------- Pantalla de abajo: tu mano y los controles ----------

// Se imprime DEBAJO de donde termina la fila de sprites de la mano
// (y=20..84, hasta la fila 10.5) — arriba de eso quedaba tapado por la
// primera carta, se veia "cortado".
static void render_bottom_message(const char* msg) {
  hide_predict_buttons();
  bottom();
  consoleClear();
  iprintf("\x1b[%d;0H  %s", PREDICT_HEADING_ROW, msg);
}


static void hide_bottom_status(void) {
  for (int i = 0; i < GAME_MAX_PENALTIES; i++) {
    oamSet(&oamSub, STATUS_HEART_OAM_BASE + i, 0, 0, 0, STATUS_HEART_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
      subHeartGfx[i], STATUS_HEART_AFFINE_ID, true, true, false, false, false);
  }
  oamSet(&oamSub, STATUS_PRED_ICON_OAM_ID, 0, 0, 0, STATUS_ICON_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
    subIconPredGfx, STATUS_ICON_AFFINE_ID, true, true, false, false, false);
  oamSet(&oamSub, STATUS_WON_ICON_OAM_ID, 0, 0, 0, STATUS_ICON_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
    subIconWonGfx, STATUS_ICON_AFFINE_ID, true, true, false, false, false);
  oamSet(&oamSub, STATUS_PRED_NUM_OAM_ID, 0, 0, 0, BUTTON_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
    buttonGfx[0], STATUS_ICON_AFFINE_ID, true, true, false, false, false);
  oamSet(&oamSub, STATUS_WON_NUM_OAM_ID, 0, 0, 0, BUTTON_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
    buttonGfx[0], STATUS_ICON_AFFINE_ID, true, true, false, false, false);
  oamUpdate(&oamSub);
}

// Barra de estado de la pantalla de abajo -- vida (corazones, igual
// criterio que en fin de mano: los primeros "penalties" corazones se
// ven rotos), cuanto predijo (icono objetivo) y cuantas lleva ganadas
// esta mano (icono trofeo), del HUMANO. A diferencia de la version
// anterior (una linea de texto en render_bottom_play nomas), esto se
// llama SIEMPRE desde el loop principal -- se pedia que se mantuviera
// visible tambien mientras juegan los bots, no solo en el propio
// turno.
static void render_bottom_status(const GameState* state) {
  // Solo mientras se juega -- durante predecir esa misma altura la
  // ocupan los botones de prediccion (se pisarian), y en fin de
  // mano/partida no aporta nada.
  if (state->phase != PHASE_PLAYING && state->phase != PHASE_TRICK_END) {
    hide_bottom_status();
    return;
  }
  const Player* me = &state->players[HUMAN_ID];

  for (int i = 0; i < GAME_MAX_PENALTIES; i++) {
    int lost = i < me->penalties;
    dmaCopy(lost ? heart_brokenTiles : heart_fullTiles, subHeartGfx[i], heart_fullTilesLen);
    // Con doble tamano el hardware mantiene fijo el centro del sprite
    // NATIVO (ver comentario de PLAY_BUTTON_OAM_X) -- centro deseado
    // (en base a la posicion/tamano VISUAL) menos la mitad del nativo.
    int x = STATUS_HEART_VISUAL_X0 + i * STATUS_HEART_VISUAL_STRIDE + STATUS_HEART_VISUAL / 2 - STATUS_HEART_NATIVE / 2;
    int y = STATUS_HEART_VISUAL_Y + STATUS_HEART_VISUAL / 2 - STATUS_HEART_NATIVE / 2;
    oamSet(&oamSub, STATUS_HEART_OAM_BASE + i, x, y, 0, STATUS_HEART_PALETTE_BANK,
      SpriteSize_16x16, SpriteColorFormat_16Color, subHeartGfx[i], STATUS_HEART_AFFINE_ID, true, false, false, false, false);
  }
  // Mismo criterio de centro-menos-mitad-del-nativo que los corazones
  // (ver mas arriba) para las 4 posiciones de esta fila (2 iconos + 2
  // numeros), todas con el mismo tamano nativo/visual.
  int iconY = STATUS_ICON_Y + STATUS_ICON_VISUAL / 2 - STATUS_ICON_NATIVE / 2;
  int predIconX = STATUS_PRED_ICON_VISUAL_X + STATUS_ICON_VISUAL / 2 - STATUS_ICON_NATIVE / 2;
  int predNumX = STATUS_PRED_NUM_VISUAL_X + STATUS_ICON_VISUAL / 2 - STATUS_ICON_NATIVE / 2;
  int wonIconX = STATUS_WON_ICON_VISUAL_X + STATUS_ICON_VISUAL / 2 - STATUS_ICON_NATIVE / 2;
  int wonNumX = STATUS_WON_NUM_VISUAL_X + STATUS_ICON_VISUAL / 2 - STATUS_ICON_NATIVE / 2;

  oamSet(&oamSub, STATUS_PRED_ICON_OAM_ID, predIconX, iconY, 0, STATUS_ICON_PALETTE_BANK,
    SpriteSize_16x16, SpriteColorFormat_16Color, subIconPredGfx, STATUS_ICON_AFFINE_ID, true, false, false, false, false);
  oamSet(&oamSub, STATUS_WON_ICON_OAM_ID, wonIconX, iconY, 0, STATUS_ICON_PALETTE_BANK,
    SpriteSize_16x16, SpriteColorFormat_16Color, subIconWonGfx, STATUS_ICON_AFFINE_ID, true, false, false, false, false);

  // Numero como sprite (digito 0-6, mismo arte que ya existe para los
  // botones de prediccion) en vez de texto de consola -- un caracter
  // suelto no se puede escalar, un sprite si. Prediccion siempre esta
  // elegida a esta altura (esta fila solo se ve en PHASE_PLAYING o
  // PHASE_TRICK_END, despues de predecir).
  oamSet(&oamSub, STATUS_PRED_NUM_OAM_ID, predNumX, iconY, 0, BUTTON_PALETTE_BANK,
    SpriteSize_16x16, SpriteColorFormat_16Color, buttonGfx[me->prediction], STATUS_ICON_AFFINE_ID, true, false, false, false, false);
  oamSet(&oamSub, STATUS_WON_NUM_OAM_ID, wonNumX, iconY, 0, BUTTON_PALETTE_BANK,
    SpriteSize_16x16, SpriteColorFormat_16Color, buttonGfx[me->tricksWon], STATUS_ICON_AFFINE_ID, true, false, false, false, false);
  oamUpdate(&oamSub);
}

// Pantalla de "jugar carta": la mano (sprites) mas la barra de estado.
// cursorIndex -1 = ninguna carta levantada todavia (sin tocar D-pad).
static void render_bottom_play(const GameState* state, int cursorIndex) {
  hide_predict_buttons();
  render_hand_sprites(state, cursorIndex);
  bottom();
  consoleClear();
  render_bottom_status(state);
}

#define BUTTON_LIFT_PX 4 // igual que HAND_LIFT_PX, pero mas chico (el sprite tambien es mas chico)

// Fila de botones sprite [0]..[handSize], centrada — reemplaza los
// numeros de texto de antes: ahora son sprites de verdad (fondo, borde
// biselado y el digito dibujado adentro), se ven como botones tocables
// en serio. El que tiene el cursor D-pad se "levanta" un poco, igual
// que la carta resaltada en la mano.
static void render_predict_buttons(const GameState* state, int cursorIndex) {
  int forbidden;
  int hasForbidden = game_get_forbidden_prediction(state, &forbidden);

  for (int v = 0; v < 7; v++) {
    if (v > state->handSize) {
      oamSet(&oamSub, BUTTON_OAM_BASE + v, 0, 0, 0, BUTTON_PALETTE_BANK, SpriteSize_16x16, SpriteColorFormat_16Color,
        buttonGfx[v], -1, false, true, false, false, false);
      continue;
    }
    int y = BUTTON_Y - (v == cursorIndex ? BUTTON_LIFT_PX : 0);
    int bank = (hasForbidden && v == forbidden) ? BUTTON_PALETTE_BANK_DIM : BUTTON_PALETTE_BANK;
    oamSet(&oamSub, BUTTON_OAM_BASE + v, button_x(v) - BUTTON_SCALE_PAD, y - BUTTON_SCALE_PAD, 0, bank, SpriteSize_16x16, SpriteColorFormat_16Color,
      buttonGfx[v], BUTTON_AFFINE_ID, true, false, false, false, false);
  }
  oamUpdate(&oamSub);
}

// Toque -> que boton (0..handSize), por posicion real del sprite.
static int predict_touch_to_value(const GameState* state, int px, int py) {
  int topY = BUTTON_Y - BUTTON_LIFT_PX - BUTTON_SCALE_PAD - 4;
  int botY = BUTTON_Y - BUTTON_SCALE_PAD + BUTTON_VISUAL_SIZE + 4;
  if (py < topY || py > botY) return -1;
  for (int v = 0; v <= state->handSize; v++) {
    int x = button_x(v) - BUTTON_SCALE_PAD;
    if (px >= x && px < x + BUTTON_VISUAL_SIZE) return v;
  }
  return -1;
}

// Pantalla de "predecir": la mano (sprites, sin levantar ninguna — el
// cursor aca es sobre un NUMERO) mas el selector de cuantas bazas, como
// una fila de botones cuadrados centrada.
// cursorIndex -1 = ningun numero marcado todavia (sin tocar D-pad).
static void render_bottom_predict(const GameState* state, int cursorIndex) {
  render_hand_sprites(state, -1);
  render_predict_buttons(state, cursorIndex);
  bottom();
  consoleClear();

  const char* heading = "Cuantas bazas predecis?";
  int headingCol = (32 - (int)strlen(heading)) / 2;
  if (headingCol < 0) headingCol = 0;
  iprintf("\x1b[%d;%dH%s", PREDICT_HEADING_ROW, headingCol, heading);

  int forbidden;
  if (game_get_forbidden_prediction(state, &forbidden)) {
    char msg[32];
    snprintf(msg, sizeof(msg), "(no podes elegir %d)", forbidden);
    int msgCol = (32 - (int)strlen(msg)) / 2;
    if (msgCol < 0) msgCol = 0;
    iprintf("\x1b[%d;%dH%s", PREDICT_OPTIONS_ROW + 2, msgCol, msg);
  }
}

// ---------- Loop principal ----------

static void new_match(GameState* state, const char* names[4], const int isAI[4], const Difficulty diffs[4]) {
  game_create_match(state, names, isAI, diffs, 4);
}

// Una partida completa: elegir dificultad, armar el match y correr el
// loop hasta que el jugador pide volver al menu principal desde la
// pausa (START, ver run_pause_menu) -- esa es la UNICA forma de salir
// de aca antes de tiempo. Fin de partida normal (GAME_END) sigue
// arrancando una mano nueva sola, como siempre.
static void run_match(char* playerName, Difficulty botDiff) {
  srand(frameCount);

  hide_title_logo();

  const char* names[4];
  names[0] = playerName;
  pick_ai_names(&names[1]); // los 3 bots, sorteados sin repetir del pool
  int isAI[4] = { 0, 1, 1, 1 };
  Difficulty diffs[4] = { botDiff, botDiff, botDiff, botDiff }; // los 3 bots juegan a la MISMA dificultad elegida

  GameState state;
  new_match(&state, names, isAI, diffs);
  animate_hand_deal(&state);

  while (1) {
    render_top(&state);
    render_bottom_status(&state); // se auto-esconde en fin de mano/partida; en las demas fases se refresca aca en cada vuelta del loop principal

    if (state.phase == PHASE_GAME_END) {
      if (state.players[HUMAN_ID].eliminated) {
        // El humano perdio (llego a este fin de partida ya eliminado --
        // el ultimo bot en pie se llevo la partida entre ellos) -- no
        // tiene sentido el festejo de victoria.
        show_outcome_screen(0);
        if (run_match_end_menu("Perdiste la partida")) return; // "volver al menu"
        hide_outcome_screen();
        pick_ai_names(&names[1]);
        new_match(&state, names, isAI, diffs);
        animate_hand_deal(&state);
        continue;
      }
      play_sfx(SFX_GAMEWIN);
      render_bottom_message("");
      show_outcome_screen(1);
      if (run_match_end_menu("Ganaste la partida")) return; // "volver al menu"
      hide_outcome_screen();
      pick_ai_names(&names[1]); // nombres nuevos de los bots cada partida
      new_match(&state, names, isAI, diffs);
      animate_hand_deal(&state);
      continue;
    }

    if (state.phase == PHASE_TRICK_END) {
      // Se ve la corona sobre quien se la llevo un momento y sigue sola
      // — no hace falta tocar nada.
      play_sfx(SFX_TRICKWIN);
      if (pause_frames_animated(&state, 70)) return;
      game_continue_after_trick(&state);
      continue;
    }

    if (state.phase == PHASE_HAND_END) {
      if (state.players[HUMAN_ID].eliminated) {
        // El humano acaba de quedar eliminado en esta mano -- se corta
        // ahi, no tiene sentido seguir viendo jugar a los bots solos
        // entre ellos hasta que quede uno.
        show_outcome_screen(0);
        if (run_match_end_menu("Perdiste la partida")) return;
        hide_outcome_screen();
        pick_ai_names(&names[1]);
        new_match(&state, names, isAI, diffs);
        animate_hand_deal(&state);
        continue;
      }
      render_bottom_message("");
      if (wait_for_play_button()) return;
      game_start_next_hand(&state);
      animate_hand_deal(&state);
      continue;
    }

    int actorId = game_get_current_player_to_act_id(&state);

    if (actorId != HUMAN_ID) {
      // Antes decia "X esta pensando..." aca abajo -- ahora quien le
      // toca se indica con la flecha en su esquina de arriba (ver
      // render_top), asi que solo hace falta despejar la pantalla de
      // abajo (esconder los botones/la mano de la vez anterior).
      // render_bottom_message ya limpia el texto -- se vuelve a pedir
      // la barra de estado enseguida (el consoleClear tambien borraba
      // los numeros de al lado de los iconos, aunque los sprites en si
      // se quedaran) para que se mantenga visible tambien en el turno
      // de los bots, no solo en el propio.
      render_bottom_message("");
      render_bottom_status(&state);
      if (pause_frames_animated(&state, 30)) return;
      if (state.phase == PHASE_PREDICTING) {
        int value = ai_decide_prediction(&state, actorId);
        game_submit_prediction(&state, actorId, value);
      } else {
        Card card = ai_decide_card(&state, actorId);
        if (game_play_card(&state, actorId, card)) {
          play_card_sfx(&state, card);
          animate_trick_card_entering(actorId, card);
          if (is_ancho(card)) celebrate_ancho(&state, actorId);
        }
      }
      continue;
    }

    // Turno del humano. Se puede jugar con el touch (como antes) O con
    // el D-pad + A: mientras no se toque ninguna flecha no aparece
    // ningun cursor (cursorIndex en -1) — recien al tocar izq/der (o
    // arriba/abajo en la mano) aparece el ">"/"[ ]" y desde ahi A
    // confirma la opcion marcada. Las dos formas conviven: usar el
    // touch en cualquier momento sigue funcionando igual.
    if (state.phase == PHASE_PREDICTING) {
      int cursorIndex = -1;
      render_bottom_predict(&state, cursorIndex);
      int chosen = 0;
      while (!chosen) {
        vsync();
        scanKeys();
        touchPosition touch;
        touchRead(&touch);
        int pressed = keysDown();

        if (pressed & KEY_START) {
          if (run_pause_menu()) return;
          render_bottom_predict(&state, cursorIndex);
          continue;
        }

        if (pressed & KEY_LEFT) {
          cursorIndex = (cursorIndex < 0) ? 0 : cursorIndex - 1;
          if (cursorIndex < 0) cursorIndex = 0;
          play_sfx(SFX_BUTTON);
          render_bottom_predict(&state, cursorIndex);
        }
        if (pressed & KEY_RIGHT) {
          cursorIndex = (cursorIndex < 0) ? 0 : cursorIndex + 1;
          if (cursorIndex > state.handSize) cursorIndex = state.handSize;
          play_sfx(SFX_BUTTON);
          render_bottom_predict(&state, cursorIndex);
        }

        int value = -1;
        if (cursorIndex >= 0 && (pressed & KEY_A)) {
          value = cursorIndex;
        } else if (pressed & KEY_TOUCH) {
          value = predict_touch_to_value(&state, touch.px, touch.py);
        }
        if (value < 0) continue;

        if (game_submit_prediction(&state, HUMAN_ID, value)) {
          chosen = 1;
        } else {
          play_sfx(SFX_CANCEL);
          render_bottom_message("El ultimo tiene que desempatar\npuntos. Pedi mas o menos");
          pause_frames(45);
          render_bottom_predict(&state, cursorIndex);
        }
      }
    } else if (state.phase == PHASE_PLAYING) {
      int cursorIndex = -1;
      render_bottom_play(&state, cursorIndex);
      int played = 0;

      // Estado del gesto de touch en curso (-1 = no hay ninguno). El
      // primer toque sobre una carta la SELECCIONA nomas (la levanta,
      // igual que el D-pad) -- recien juega si: (a) se vuelve a tocar
      // la carta que YA estaba seleccionada, o (b) se arrastra el dedo
      // hacia abajo mas de DRAG_CONFIRM_PX sin soltar antes (como si la
      // bajaras de la mano hacia la mesa). Asi un toque de mas no juega
      // una carta por error. Ademas, arrastrando al COSTADO se puede
      // reordenar la mano (intercambia con la vecina de ese lado, se
      // pueden encadenar varios intercambios en un solo gesto).
      int dragIdx = -1;
      int dragStartIdx = -1; // indice con el que arranco ESTE toque -- si dragIdx termina siendo otro, hubo reordenamiento
      int dragGrabOffsetX = 0; // touch.px - hand_slot_x(idx) AL AGARRAR -- fijo todo el gesto
      int dragStartY = 0;
      int dragPreselected = 0;
      int dragLastX = 0; // liveX del frame anterior, para sacar la velocidad y hacer el vaiven
      int lastTouchX = 0;
      int lastTouchY = 0;

      while (!played) {
        vsync();
        scanKeys();
        touchPosition touch;
        touchRead(&touch);
        int pressed = keysDown();
        int released = keysUp();
        int handCount = state.players[HUMAN_ID].handCount;
        if (keysHeld() & KEY_TOUCH) {
          lastTouchX = touch.px;
          lastTouchY = touch.py;
        }

        if (pressed & KEY_START) {
          if (run_pause_menu()) return;
          render_bottom_play(&state, cursorIndex);
          continue;
        }

        // La mano es una fila horizontal de sprites ahora — izquierda y
        // derecha para moverse entre cartas (antes era arriba/abajo,
        // cuando la mano era una lista de texto vertical). Cada vez que
        // el cursor cambia de carta: un "punch" chiquito (se agranda un
        // toque y vuelve) mas una inclinacion breve en la direccion del
        // movimiento -- decaen solas (ver hero_anim_step_and_apply).
        if (pressed & KEY_LEFT) {
          int prev = cursorIndex;
          cursorIndex = (cursorIndex < 0) ? 0 : cursorIndex - 1;
          if (cursorIndex < 0) cursorIndex = 0;
          if (cursorIndex != prev) {
            play_sfx(SFX_BUTTON);
            hero_anim_punch();
            hero_anim_tilt_kick(-1);
          }
          render_bottom_play(&state, cursorIndex);
        }
        if (pressed & KEY_RIGHT) {
          int prev = cursorIndex;
          cursorIndex = (cursorIndex < 0) ? 0 : cursorIndex + 1;
          if (cursorIndex > handCount - 1) cursorIndex = handCount - 1;
          if (cursorIndex != prev) {
            play_sfx(SFX_BUTTON);
            hero_anim_punch();
            hero_anim_tilt_kick(1);
          }
          render_bottom_play(&state, cursorIndex);
        }

        if (pressed & KEY_TOUCH) {
          int idx = hand_touch_to_index(&state, touch.px, touch.py);
          dragIdx = idx;
          dragStartIdx = idx;
          dragStartY = touch.py;
          lastTouchX = touch.px;
          lastTouchY = touch.py;
          if (idx >= 0) {
            dragGrabOffsetX = touch.px - hand_slot_x(idx);
            dragLastX = hand_slot_x(idx); // arranca en 0 la velocidad, no en un salto
            dragPreselected = (idx == cursorIndex);
            if (!dragPreselected) {
              play_sfx(SFX_BUTTON);
              hero_anim_punch();
            }
            cursorIndex = idx;
            render_bottom_play(&state, cursorIndex);
          }
        } else if ((keysHeld() & KEY_TOUCH) && dragIdx >= 0) {
          // Arrastre libre: la carta sigue al dedo en las dos
          // direcciones. La posicion X sale SIEMPRE del toque actual
          // menos el offset fijo de cuando se agarro (dragGrabOffsetX,
          // que no cambia en todo el gesto) -- nunca se acumula un
          // delta relativo, asi no se puede "perder" arrastre de mas al
          // encadenar varios intercambios (eso hacia que la carta
          // terminara lejos del dedo). Cruzar la mitad hacia una vecina
          // intercambia con ella.
          int liveX = touch.px - dragGrabOffsetX;
          Player* human = &state.players[HUMAN_ID];
          if (dragIdx > 0 && liveX < hand_slot_x(dragIdx) - HAND_STRIDE / 2) {
            int neighborIdx = dragIdx - 1;
            Card tmp = human->hand[dragIdx];
            human->hand[dragIdx] = human->hand[neighborIdx];
            human->hand[neighborIdx] = tmp;
            // La vecina desplazada (ahora en dragIdx) arranca desde
            // donde estaba ANTES (neighborIdx) y llega sola a su lugar
            // nuevo -- no se teletransporta.
            anim_snap(&handSlideX[dragIdx], hand_slot_x(neighborIdx));
            dragIdx = neighborIdx;
            cursorIndex = dragIdx;
            render_hand_sprites(&state, cursorIndex);
          } else if (dragIdx < handCount - 1 && liveX > hand_slot_x(dragIdx) + HAND_STRIDE / 2) {
            int neighborIdx = dragIdx + 1;
            Card tmp = human->hand[dragIdx];
            human->hand[dragIdx] = human->hand[neighborIdx];
            human->hand[neighborIdx] = tmp;
            anim_snap(&handSlideX[dragIdx], hand_slot_x(neighborIdx));
            dragIdx = neighborIdx;
            cursorIndex = dragIdx;
            render_hand_sprites(&state, cursorIndex);
          }
          // Vaiven continuo: se recalcula la velocidad TODOS los frames
          // que se este arrastrando, haya o no vecina para reordenar --
          // antes solo se inclinaba en el instante de un intercambio.
          hero_anim_tilt_follow(liveX - dragLastX);
          dragLastX = liveX;
          int liveY = (HAND_Y - HAND_LIFT_PX) + (touch.py - dragStartY);
          update_dragged_card_position(dragIdx, liveX, liveY);
          render_slingshot(liveX, liveY);
        }

        hero_anim_step_and_apply();
        hand_slide_tick(&state, cursorIndex, dragIdx);
        render_trick_sprites(&state); // sigue animando el temblor/gota mientras el humano piensa

        int idxToPlay = -1;
        int exitFromX = hand_slot_x(cursorIndex >= 0 ? cursorIndex : 0);
        int exitFromY = HAND_Y - HAND_LIFT_PX;
        if (cursorIndex >= 0 && (pressed & KEY_A)) {
          idxToPlay = cursorIndex;
        } else if ((released & KEY_TOUCH) && dragIdx >= 0) {
          anim_set_target(&heroAnim.tilt, 0); // se solto -- el vaiven vuelve a 0 solo
          hide_slingshot();
          int dragDistY = lastTouchY - dragStartY; // + = arrastro hacia abajo, - = hacia arriba
          int liveX = lastTouchX - dragGrabOffsetX;
          int liveY = (HAND_Y - HAND_LIFT_PX) + dragDistY;
          int reordered = (dragIdx != dragStartIdx); // se uso el gesto para acomodar la mano, no para jugar
          // "Segunda vez que se toca la ya seleccionada" solo cuenta
          // como confirmar si NO se reordeno nada en el medio -- si no,
          // reordenar una carta que ya estaba resaltada y soltarla
          // dentro de la mano la jugaba sola por error. Aparte de eso,
          // confirma arrastrar bastante para CUALQUIER lado (arriba O
          // abajo, no solo abajo) -- adentro de la mano nunca se juega.
          if ((dragPreselected && !reordered) || abs(dragDistY) >= DRAG_CONFIRM_PX) {
            idxToPlay = dragIdx;
            exitFromX = liveX;
            exitFromY = liveY; // sigue el viaje desde donde quedo pegada al dedo
          } else {
            animate_drag_snap_back(dragIdx, liveX, liveY);
          }
          dragIdx = -1;
        }
        if (idxToPlay < 0) continue;

        Card card = state.players[HUMAN_ID].hand[idxToPlay];
        if (game_play_card(&state, HUMAN_ID, card)) {
          played = 1;
          play_card_sfx(&state, card);
          animate_human_card_play(idxToPlay, card, exitFromX, exitFromY);
          if (is_ancho(card)) celebrate_ancho(&state, HUMAN_ID);
          // Redibuja la mano YA, con la carta ya sacada — antes se
          // quedaba viendose hasta el proximo turno (o la proxima mano
          // si esta era la ultima baza), porque el sprite no se tocaba
          // de nuevo hasta la siguiente vez que se armaba esta pantalla.
          // Los indices se corren al sacar una carta -- se asienta todo
          // de una, sin lerp (eso no es un reordenamiento a mano).
          hand_slide_snap_all();
          render_hand_sprites(&state, -1);
        }
      }
    }
  }
}

// ---------- Pantalla de la productora (arranque) ----------

// Bitmap de pantalla completa (256x192, 8bpp) en vez de tiles/sprites
// -- necesita "Mode 5" (fondos extendidos), distinto del Mode 0 (solo
// texto/tiles) que usa el resto del juego. Por eso arma su PROPIO modo
// de video/banco de VRAM, aca nomas, y lo deja como estaba (Mode 0)
// antes de seguir -- setup_screens() lo reconfigura todo de cero
// despues, sin que quede ningun rastro de esto.
//
// Secuencia pedida: fundido de negro al frame 1 (arte del usuario,
// D:\PixelArt123\2026\DextinyProductions1.png), se mantiene mientras
// suena el primer efecto ("coin insert", 1724ms), parpadea al frame 2
// (DextinyProductions2.png) y de vuelta al 1 (50ms cada uno), y
// termina en el frame 2 de nuevo, que se queda 100ms mas de lo que
// dura el segundo efecto ("dragon studio arcade", 888ms) mientras
// suena. Todo medido a 60fps (swiWaitForVBlank por cuadro). Las
// constantes de tiempo (SPLASH_*) estan mas arriba en el archivo --
// show_main_menu tambien usa SPLASH_FADE_FRAMES para el fundido de
// entrada del menu, que va ANTES que esta funcion en el archivo.

static void show_studio_splash(void) {
  videoSetMode(MODE_5_2D);
  vramSetBankA(VRAM_A_MAIN_BG);
  int splashBg = bgInit(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
  dmaCopy(studio_splash_sharedPal, BG_PALETTE, studio_splash_sharedPalLen);

  // Pantalla de ABAJO: el "negro" del arte de arriba en realidad es
  // gris oscuro (0x272727, indice 0 de la paleta) -- sin ARMAR un
  // layer de verdad en el motor B (solo escribir BG_PALETTE_SUB[0] NO
  // alcanza: sin videoSetModeSub/bgInitSub el motor B ni esta prendido,
  // por eso se seguia viendo negro posta) queda todo el bitmap en 0
  // (indice 0 = el mismo gris) para que las dos pantallas se vean
  // exactamente igual mientras dura la presentacion.
  videoSetModeSub(MODE_5_2D);
  vramSetBankC(VRAM_C_SUB_BG);
  int splashBgSub = bgInitSub(3, BgType_Bmp8, BgSize_B8_256x256, 0, 0);
  dmaCopy(studio_splash_sharedPal, BG_PALETTE_SUB, studio_splash_sharedPalLen);
  dmaFillWords(0, bgGetGfxPtr(splashBgSub), 256 * 192);
  bgUpdate();

  setBrightness(3, -16); // arranca en negro, las dos pantallas
  dmaCopy(studio_splash_0Bitmap, bgGetGfxPtr(splashBg), studio_splash_0BitmapLen);
  bgUpdate();

  play_sfx(SFX_PRODCOIN);
  for (int f = 0; f < SPLASH_AUDIO1_HOLD_FRAMES; f++) {
    if (f < SPLASH_FADE_FRAMES) {
      setBrightness(3, -16 + (16 * f) / SPLASH_FADE_FRAMES);
    }
    swiWaitForVBlank();
  }
  setBrightness(3, 0); // asegura que el fundido termino del todo

  dmaCopy(studio_splash_1Bitmap, bgGetGfxPtr(splashBg), studio_splash_1BitmapLen);
  bgUpdate();
  for (int f = 0; f < SPLASH_FLICKER_FRAMES; f++) swiWaitForVBlank();

  dmaCopy(studio_splash_0Bitmap, bgGetGfxPtr(splashBg), studio_splash_0BitmapLen);
  bgUpdate();
  for (int f = 0; f < SPLASH_FLICKER_FRAMES; f++) swiWaitForVBlank();

  dmaCopy(studio_splash_1Bitmap, bgGetGfxPtr(splashBg), studio_splash_1BitmapLen);
  bgUpdate();
  play_sfx(SFX_PRODARCADE);
  for (int f = 0; f < SPLASH_AUDIO2_HOLD_FRAMES; f++) swiWaitForVBlank();

  // Fundido de salida a negro -- sin esto, setup_screens() de main()
  // reconfigura todo (Mode 0, VRAM de cero) apenas esto vuelve y el
  // corte de la animacion al menu se sentia brusco/de golpe.
  for (int f = 0; f < SPLASH_FADE_FRAMES; f++) {
    setBrightness(3, 0 - (16 * f) / SPLASH_FADE_FRAMES);
    swiWaitForVBlank();
  }
  setBrightness(3, -16);
}

// ---------- Arranque ----------

int main(void) {
  irqSet(IRQ_VBLANK, vblank_handler);
  irqEnable(IRQ_VBLANK);

  // Para poder guardar el nombre/las opciones entre partidas -- si no
  // hay tarjeta SD/DLDI configurada (ej. melonDS sin una imagen armada)
  // esto da false y se sigue jugando igual, solo que sin persistencia.
  fatReady = fatInitDefault();

  char playerName[PLAYER_NAME_MAX + 1];
  load_player_name(playerName);
  load_settings(); // asi la pantalla de productora ya respeta "Sonido: NO" si estaba guardado

  setup_audio(); // antes del splash -- necesita el soundbank cargado para sus 2 efectos
  setup_mic(); // soplido en el menu -- ver update_title_logo_idle
  show_studio_splash(); // termina con las dos pantallas en negro (fundido de salida)

  setup_screens(); // arma Mode 0 + VRAM de cero, sin rastro del Mode 5 que uso el splash
  setup_music_stream();

  // Pantalla de titulo: remolino paneando + logo "BAZADO" fijo encima
  // (sprite aparte, ver render_title_logo -- inspirado en la pantalla
  // de inicio de Balatro que mando el usuario). Se queda de fondo del
  // menu principal (y de Opciones/dificultad, que cuelgan de el).
  show_title_background();
  // La de ARRIBA pasa a brillo normal ANTES del logo -- si se quedara
  // en negro (como sale de show_studio_splash) la animacion de entrada
  // del logo (ver render_title_logo) pasaria entera a oscuras, invisible.
  // La de ABAJO se queda en negro (show_main_menu hace su propio
  // fundido de entrada la primera vez que corre, con el menu ya
  // dibujado) para que el corte de la animacion al menu no sea de golpe.
  setBrightness(1, 0);
  render_title_logo();

  show_main_menu(playerName); // no vuelve nunca -- adentro corre para siempre (Jugar/Opciones)
  return 0;
}
