# Bazado

Homebrew para Nintendo DS de **Bazas**, un juego de cartas de predicción con mazo español. Escrito en C con [libnds](https://github.com/devkitPro/libnds), corre en hardware real, flashcarts y emuladores (probado en melonDS).

Es el port a DS de la versión original web/Electron ([bazas-cartas](https://github.com/zdextiny)), rehecho para las dos pantallas, el táctil y el D-pad de la consola.

## Cómo se juega

En cada mano, cada jugador predice por turno cuántas bazas (rondas) va a ganar — y después tiene que cumplirlo **exacto**, ni una más ni una menos. El último en predecir no puede elegir el número que haga que la suma de todas las predicciones dé justo la cantidad de cartas repartidas, así nunca pueden acertar todos a la vez.

- **Mazo**: 48 cartas, 4 palos (oro, copa, espada, basto), del 1 al 12, sin comodines.
- **Jerarquía**: gana la baza quien jugó la carta más fuerte, sin importar el palo. El As de Espada y el As de Basto son las más fuertes de todas (le ganan hasta al 12); el As de Oro y el As de Copa son, al revés, las más débiles.
- **Vidas**: cada jugador arranca con 6. Si no ganaste exacto lo que predijiste, perdés una vida por cada baza de diferencia. Al quedarte sin vidas quedás eliminado — gana quien queda último en pie.
- **Tamaño de mano**: la primera mano se reparten 6 cartas, cada mano siguiente una menos hasta llegar a 1, después vuelve a subir hasta 6, y se repite el ciclo.

El juego incluye un tutorial jugable completo (Menú → Tutorial) con las reglas paso a paso.

## Características

- 3 dificultades de IA (Fácil / Media / Difícil) — la difícil usa una simulación tipo Monte Carlo (determinización + rollouts) para jugar con más anticipación en vez de solo heurísticas por baza.
- Controles duales: D-pad + A/B, o táctil directo sobre las cartas.
- Predicción, mano y penalizaciones siempre visibles en las esquinas de la pantalla de arriba.
- Fondos de pantalla seleccionables (10 patrones), con opción de usar uno distinto arriba y abajo.
- Nombre de jugador editable, música y sonido con toggle independiente — todo persiste entre partidas (requiere tarjeta SD/DLDI real o emulada).
- Pantalla de título animada (remolino con paneo + logo "BAZADO" letra por letra) — soplar al micrófono acelera el vaivén del logo.
- Menú de pausa, pantallas de victoria/derrota con opción de revancha, y splash de estudio al arrancar.

## Controles

| Acción | D-pad / botones | Táctil |
|---|---|---|
| Mover cursor / elegir opción | D-pad | Tocar directo |
| Confirmar | A | Tocar la carta/opción |
| Cancelar / volver | B | — |
| Pausa (in-game) | START | — |

## Compilar desde cero

Hace falta [devkitPro](https://devkitpro.org/wiki/Getting_Started) con el paquete `nds-dev` (devkitARM + libnds + libfat + maxmod).

```sh
export DEVKITPRO=/opt/devkitpro       # o la ruta donde lo hayas instalado
export DEVKITARM=$DEVKITPRO/devkitARM
make
```

Esto genera `bazas-nds.nds` (el nombre sale del nombre de la carpeta del proyecto). El `Makefile` ya arma el banner/ícono de la ROM (visible en TWiLight Menu++ y menús de flashcart) y agrupa la música de fondo dentro de la ROM vía NitroFS, así no hace falta tarjeta SD para el audio principal — sí hace falta una SD (real o emulada vía DLDI) para que se guarden el nombre de jugador, las opciones y el fondo elegido entre partidas.

Si cambiás algo en `gfx/` (cartas, fondos, sprites), corré `tools/prep-assets.ps1` antes de `make` para regenerar los `.c`/`.h` que usa el build — ese script apunta a carpetas de arte fuente fuera del repo, así que solo hace falta si estás tocando el arte original.

## Probarlo

- **Emulador**: [melonDS](https://melonds.kuribo64.net/) — abrí directo el `.nds`.
- **Hardware real / flashcart**: copiá el `.nds` a la tarjeta SD y abrilo desde el menú de tu flashcart (R4, TWiLight Menu++, etc.). Con DLDI habilitado, las opciones y el progreso se guardan entre sesiones.

## Créditos

Creado por **Agustín Cavalié** / **Z-Dextiny** (Dextiny Productions).
