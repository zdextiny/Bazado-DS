# Prepara los assets graficos para la version DS: recorta las 48 cartas
# de free_deck/ a 32x32 (un unico formato de carta, con el margen
# transparente ya en el arte fuente) y tine de rojo el patron de fondo
# elegido. Se corre a mano cuando cambian los assets de origen; el
# resultado queda en gfx/ para que el Makefile lo procese con grit en
# cada build.
Add-Type -AssemblyName System.Drawing

$deckSrc = "C:\Users\Dex\Desktop\Resourses\free_deck"
$gfxOut = "D:\Proyectos\bazas-nds\gfx"

New-Item -ItemType Directory -Path $gfxOut -Force | Out-Null

# Cartas: un unico formato, 32x32, con el margen transparente ya
# dibujado adentro del arte en si (mismo criterio que el juego de
# referencia -- ver deck_gfx.png de balatro-gba: sprite cuadrado, el
# "look" de carta rectangular sale del arte, no de un tamano de sprite
# distinto). Si la fuente ya es 32x32 (las Pico) se usa tal cual. Si es
# mas grande, SOLO se recorta al centro a 32x32 -- nada de escalar ni
# de elegir color dominante. Los pixeles con alpha bajo de la fuente
# (si la tiene, como las Pico) se llevan como magenta (color llave que
# grit despues mapea a transparente con -gT); una fuente sin alpha
# simplemente sale opaca de punta a punta.
# Caja del contenido REALMENTE visible (no transparente) de la fuente.
# Para una fuente sin alpha (opaca de punta a punta, ej. los recortes
# de free_deck) esto da la imagen entera -- mismo comportamiento de
# siempre. Para una fuente CON alpha (las Pico) da solo el dibujo en
# si, ignorando el margen transparente que ya traiga el archivo -- asi
# el centrado es sobre lo que se VE, no sobre el tamano del archivo
# (que puede venir descentrado a mano sin que el pipeline lo note).
function Get-ContentBounds($src) {
  $minX = $src.Width; $maxX = -1; $minY = $src.Height; $maxY = -1
  for ($y = 0; $y -lt $src.Height; $y++) {
    for ($x = 0; $x -lt $src.Width; $x++) {
      if ($src.GetPixel($x, $y).A -ge 128) {
        if ($x -lt $minX) { $minX = $x }
        if ($x -gt $maxX) { $maxX = $x }
        if ($y -lt $minY) { $minY = $y }
        if ($y -gt $maxY) { $maxY = $y }
      }
    }
  }
  if ($maxX -lt 0) { return @{ X = 0; Y = 0; W = $src.Width; H = $src.Height } } # todo transparente, no deberia pasar
  return @{ X = $minX; Y = $minY; W = ($maxX - $minX + 1); H = ($maxY - $minY + 1) }
}

function Convert-CardTo32($srcPath, $destPath) {
  $src = [System.Drawing.Bitmap]::FromFile($srcPath)
  $bounds = Get-ContentBounds $src
  # Offset UNICO para las dos direcciones: centra la caja de contenido
  # (no el archivo entero) en el canvas de 32x32. Si el contenido es
  # mas grande que 32px esto da un recorte al centro (mira mas adentro
  # de la fuente); si es mas chico, lo centra con relleno magenta a los
  # costados.
  $offsetX = [int]([math]::Round((32 - $bounds.W) / 2)) - $bounds.X
  $offsetY = [int]([math]::Round((32 - $bounds.H) / 2)) - $bounds.Y

  $dest = New-Object System.Drawing.Bitmap(32, 32)
  for ($y = 0; $y -lt 32; $y++) {
    for ($x = 0; $x -lt 32; $x++) {
      $sx = $x - $offsetX
      $sy = $y - $offsetY
      if ($sx -lt 0 -or $sx -ge $src.Width -or $sy -lt 0 -or $sy -ge $src.Height) {
        $dest.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(255, 255, 0, 255))
        continue
      }
      $p = $src.GetPixel($sx, $sy)
      if ($p.A -lt 128) {
        $dest.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(255, 255, 0, 255))
      } else {
        $dest.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(255, $p.R, $p.G, $p.B))
      }
    }
  }
  $src.Dispose()
  $dest.Save($destPath, [System.Drawing.Imaging.ImageFormat]::Png)
  $dest.Dispose()
}

# Mapeo de archivo -> (suit, value) descubierto revisando el pack:
# bloques de 10 por palo (oro=1..10, copa=11..20, espada=21..30, basto=31..40),
# posiciones 1-7 son valor 1-7, posiciones 8-10 son valor 10-11-12; los
# archivos "X-8"/"X-9" (X = 7,17,27,37) son el 8 y 9 de cada palo.
$suits = @("oro", "copa", "espada", "basto")
$order = @(1,2,3,4,5,6,7,10,11,12) # posiciones 1..10 dentro de cada bloque

# Arte dibujado a mano que reemplaza puntualmente al pack original --
# clave "suit_valor". Se van agregando ahi a medida que haya mas cartas
# nuevas (el resto sigue saliendo de free_deck como siempre).
$manualSrc = "C:\Users\Dex\Desktop\Resourses\Mazo"
$overrides = @{
  "oro_1" = (Join-Path $manualSrc "1-o-Pico.png")
  "oro_2" = (Join-Path $manualSrc "2-o-Pico.png")
  "oro_3" = (Join-Path $manualSrc "3-o-Pico.png")
  "oro_4" = (Join-Path $manualSrc "4-o-Pico.png")
  "oro_5" = (Join-Path $manualSrc "5-o-Pico.png")
  "oro_6" = (Join-Path $manualSrc "6-o-Pico.png")
  "oro_7" = (Join-Path $manualSrc "7-o-Pico.png")
  "oro_8" = (Join-Path $manualSrc "8-o-Pico.png")
  "oro_9" = (Join-Path $manualSrc "9-o-Pico-.png")
  "oro_10" = (Join-Path $manualSrc "10-o-Pico.png")
  "oro_11" = (Join-Path $manualSrc "11-o-Pico.png")
  "oro_12" = (Join-Path $manualSrc "12-o-Pico.png")
  "espada_1" = (Join-Path $manualSrc "a-e-Pico.png")
  "espada_2" = (Join-Path $manualSrc "2-e-Pico.png")
  "espada_3" = (Join-Path $manualSrc "3-e-Pico.png")
  "espada_4" = (Join-Path $manualSrc "4-e-Pico.png")
  "espada_5" = (Join-Path $manualSrc "5-e-Pico.png")
  "espada_6" = (Join-Path $manualSrc "6-e-Pico.png")
  "espada_7" = (Join-Path $manualSrc "7-e-Pico.png")
  "espada_8" = (Join-Path $manualSrc "8-e-Pico.png")
  "espada_9" = (Join-Path $manualSrc "9-e-Pico.png")
  "espada_10" = (Join-Path $manualSrc "10-e-pico.png")
  "espada_11" = (Join-Path $manualSrc "11-e-pico.png")
  "espada_12" = (Join-Path $manualSrc "12-e-pico.png")
  "basto_1" = (Join-Path $manualSrc "1-b-Pico.png")
  "basto_2" = (Join-Path $manualSrc "2-b-Pico.png")
  "basto_3" = (Join-Path $manualSrc "3-b-Pico.png")
  "basto_4" = (Join-Path $manualSrc "4-b-Pico.png")
  "basto_5" = (Join-Path $manualSrc "5-b-Pico.png")
  "basto_6" = (Join-Path $manualSrc "6-b-Pico.png")
  "basto_7" = (Join-Path $manualSrc "7-b-Pico.png")
  "basto_8" = (Join-Path $manualSrc "8-b-Pico.png")
  "basto_9" = (Join-Path $manualSrc "9-b-Pico.png")
  "basto_10" = (Join-Path $manualSrc "10-b-pico.png")
  "basto_11" = (Join-Path $manualSrc "11-b-pico.png")
  "basto_12" = (Join-Path $manualSrc "12-b-pico.png")
  "copa_1" = (Join-Path $manualSrc "1-c-Pico.png")
  "copa_2" = (Join-Path $manualSrc "2-c-Pico.png")
  "copa_3" = (Join-Path $manualSrc "3-c-Pico.png")
  "copa_4" = (Join-Path $manualSrc "4-c-Pico.png")
  "copa_5" = (Join-Path $manualSrc "5-c-Pico.png")
  "copa_6" = (Join-Path $manualSrc "6-c-Pico.png")
  "copa_7" = (Join-Path $manualSrc "7-c-Pico.png")
  "copa_8" = (Join-Path $manualSrc "8-c-Pico.png")
  "copa_9" = (Join-Path $manualSrc "9-c-Pico.png")
  "copa_10" = (Join-Path $manualSrc "10-c-pico.png")
  "copa_11" = (Join-Path $manualSrc "11-c-pico.png")
  "copa_12" = (Join-Path $manualSrc "12-c-pico.png")
}

function Get-CardSource($suitName, $value, $defaultPath) {
  $key = "${suitName}_${value}"
  if ($overrides.ContainsKey($key)) { return $overrides[$key] }
  return $defaultPath
}

$count = 0
for ($s = 0; $s -lt 4; $s++) {
  $suitName = $suits[$s]
  $blockStart = $s * 10

  for ($pos = 1; $pos -le 10; $pos++) {
    $value = $order[$pos - 1]
    $fileNum = $blockStart + $pos
    $srcFile = Get-CardSource $suitName $value (Join-Path $deckSrc "$fileNum.PNG")
    Convert-CardTo32 $srcFile (Join-Path $gfxOut "card_${suitName}_${value}.png")
    $count++
  }

  # Los extra "8" y "9" de este palo.
  $baseSeven = $blockStart + 7
  foreach ($extra in @(8, 9)) {
    $srcFile = Get-CardSource $suitName $extra (Join-Path $deckSrc "$baseSeven-$extra.PNG")
    Convert-CardTo32 $srcFile (Join-Path $gfxOut "card_${suitName}_${extra}.png")
    $count++
  }
}

Write-Output "Cartas convertidas: $count (esperado 48)"

# Dorso de carta: arte de verdad del usuario, ya puesto directo en
# gfx/card_back.png (32x32) -- el placeholder viejo (make-cardback.ps1)
# ya no se genera aca para no pisarlo en cada corrida.

# Fondos de gameplay seleccionables (10, ver make-bg-patterns.ps1 --
# ya vienen tenidos de rojo).
powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "make-bg-patterns.ps1") | Out-Null

# ---------- grit: convierte los PNG a datos C para la DS ----------
# Las 48 cartas + el dorso se procesan JUNTAS en una sola invocacion con
# paleta compartida (-pS): la DS solo tiene 16 bancos de paleta de
# sprites, no alcanzan 49 paletas propias una por carta. Cada carta
# igual genera su propio array de tiles (asi cada una se puede cargar
# en VRAM por separado segun haga falta), pero todas comparten los
# mismos indices de color.
$env:PATH = "D:\devkitPro\tools\bin;" + $env:PATH
$sourceOut = "D:\Proyectos\bazas-nds\source"
$grit = "D:\devkitPro\tools\bin\grit.exe"

# Por las dudas: no dejar .c/.h generados de una corrida anterior dando
# vueltas en gfx/.
Get-ChildItem -Path $gfxOut -Include "*.c", "*.h" -Recurse | Remove-Item -Force

# IMPORTANTE: grit arma la paleta compartida (-pS) en el ORDEN en que le
# llegan los archivos -- el primer color nuevo que ve ocupa el primer
# indice libre. El color transparente (magenta, -gT) tiene que caer
# justo en el indice 0 (el UNICO que la DS trata como invisible de
# verdad en hardware); si el primer archivo de la lista no tiene ni un
# pixel magenta, el magenta recien aparece mas adelante y termina en
# cualquier indice mas alto -- ahi deja de ser transparente y se ve
# como un color solido mas (esto paso: "card_back"/"card_basto_1" no
# tienen margen transparente y son los primeros en orden alfabetico).
# Se resuelve poniendo PRIMERO en la lista una carta que YA tenga
# magenta adentro (no importa cual, con que exista alcanza).
function Test-HasMagenta($path) {
  $img = [System.Drawing.Bitmap]::FromFile($path)
  $found = $false
  for ($y = 0; $y -lt $img.Height -and -not $found; $y++) {
    for ($x = 0; $x -lt $img.Width -and -not $found; $x++) {
      $p = $img.GetPixel($x, $y)
      if ($p.A -eq 255 -and $p.R -eq 255 -and $p.G -eq 0 -and $p.B -eq 255) { $found = $true }
    }
  }
  $img.Dispose()
  return $found
}

Push-Location $gfxOut
$cardFiles = Get-ChildItem -Filter "card_*.png" | ForEach-Object { $_.Name }
$magentaSeed = $cardFiles | Where-Object { Test-HasMagenta (Join-Path $gfxOut $_) } | Select-Object -First 1
if ($magentaSeed) {
  $cardFiles = @($magentaSeed) + ($cardFiles | Where-Object { $_ -ne $magentaSeed })
}
& $grit @cardFiles -ftc -gB8 -gt -gTFF00FF -pS -Ocards_shared -Scards_shared -m! 2>&1
Pop-Location

Move-Item (Join-Path $gfxOut "card_*.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "card_*.h") $sourceOut -Force
Move-Item (Join-Path $gfxOut "cards_shared.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "cards_shared.h") $sourceOut -Force
Write-Output "Cartas convertidas a C con grit (paleta compartida)."

# Fondos de gameplay (10, seleccionables desde Opciones): independientes
# de las cartas, chicos (2 colores), paleta COMPARTIDA entre los 10 --
# el tinte de rojo siempre da la misma paleta de salida sin importar el
# patron fuente (ver make-bg-patterns.ps1), asi que comparten un solo
# banco entre todos. -mp1 fuerza el mapa a usar el BANCO de paleta 1
# (no el 0) porque el banco 0 ya lo ocupa la fuente de la consola de
# texto - sin esto, cargar la paleta del fondo pisaria los colores del
# texto.
Push-Location $gfxOut
$bgPatternFiles = 0..9 | ForEach-Object { "bg_pattern_$_.png" }
& $grit @bgPatternFiles -ftc -gB4 -gt -m -mR4 -mp1 -pS -Obg_patterns_shared -Sbg_patterns_shared 2>&1
Pop-Location
Move-Item (Join-Path $gfxOut "bg_pattern_*.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "bg_pattern_*.h") $sourceOut -Force
Move-Item (Join-Path $gfxOut "bg_patterns_shared.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "bg_patterns_shared.h") $sourceOut -Force
Write-Output "Fondos de gameplay (10) convertidos a C con grit."

# Fondo de la pantalla de fin de mano (tabla dibujada a mano) -- MISMA
# configuracion que el patron animado (2 colores, mismo banco de
# paleta) para poder intercambiarlos en el mismo layer en tiempo real.
powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "make-score-bg.ps1") | Out-Null
Push-Location $gfxOut
& $grit "score_bg.png" -ftc -gB4 -gt -m -mR4 -mp1 -o score_bg 2>&1
Pop-Location
Move-Item (Join-Path $gfxOut "score_bg.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "score_bg.h") $sourceOut -Force
Write-Output "Fondo de puntaje convertido a C con grit."

# Pantalla de titulo -- remolino rojo/azul (inspirado en Balatro) SIN
# el logo (ver mas abajo, ahora es sprite aparte para poder panear el
# fondo sin mover el texto). 2 frames (title_bg_0/1.png), paleta
# compartida entre los dos (las mismas 5 bandas de color, solo cambia
# que pixel tiene cual -- -pS asegura que los dos frames usen los
# mismos indices de paleta, si no cada uno podria salir ordenado
# distinto y el intercambio en vivo mostraria colores cruzados).
powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "make-title-bg.ps1") | Out-Null
Push-Location $gfxOut
& $grit "title_bg_0.png" "title_bg_1.png" -ftc -gB4 -gt -pS -m -mR4 -mp1 -Otitle_bg_shared -Stitle_bg_shared 2>&1
Pop-Location
Move-Item (Join-Path $gfxOut "title_bg_0.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "title_bg_0.h") $sourceOut -Force
Move-Item (Join-Path $gfxOut "title_bg_1.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "title_bg_1.h") $sourceOut -Force
Move-Item (Join-Path $gfxOut "title_bg_shared.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "title_bg_shared.h") $sourceOut -Force
Write-Output "Fondo de titulo (2 frames) convertido a C con grit."

# Logo "BAZADO", 6 sprites de 64x64 -- uno POR LETRA (antes eran 4
# piezas de 256/4 sin relacion con las letras; el usuario corto cada
# letra a mano de title_logo.png, ver make-title-letters.ps1), posicion
# fija en pantalla -- independiente del paneo del remolino de atras.
powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "make-title-letters.ps1") | Out-Null
Push-Location $gfxOut
& $grit "title_letter_0.png" "title_letter_1.png" "title_letter_2.png" "title_letter_3.png" "title_letter_4.png" "title_letter_5.png" -ftc -gB4 -gt -gTFF00FF -pS -Otitle_letters_shared -Stitle_letters_shared -m! 2>&1
Pop-Location
Move-Item (Join-Path $gfxOut "title_letters_shared.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "title_letters_shared.h") $sourceOut -Force
for ($i = 0; $i -lt 6; $i++) {
  Move-Item (Join-Path $gfxOut "title_letter_$i.c") $sourceOut -Force
  Move-Item (Join-Path $gfxOut "title_letter_$i.h") $sourceOut -Force
}
Write-Output "Logo de titulo (6 letras) convertido a C con grit."

# Pantalla de fin de partida (arriba): SOLO texto, uno para victoria y
# otro para derrota -- se probo con un icono de usuario arriba tambien
# pero se saco del todo (ver make-outcome-screen.ps1), paleta
# compartida entre los dos.
powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "make-outcome-screen.ps1") | Out-Null
Push-Location $gfxOut
& $grit "win_text.png" "lose_text.png" -ftc -gB4 -gt -gTFF00FF -pS -Ooutcome_text_shared -Soutcome_text_shared -m! 2>&1
Pop-Location
Move-Item (Join-Path $gfxOut "outcome_text_shared.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "outcome_text_shared.h") $sourceOut -Force
Move-Item (Join-Path $gfxOut "win_text.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "win_text.h") $sourceOut -Force
Move-Item (Join-Path $gfxOut "lose_text.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "lose_text.h") $sourceOut -Force
Write-Output "Pantalla de fin de partida (texto ganar/perder) convertida a C con grit."

# Corona del ganador de la baza: chica (16x16), pocos colores, paleta
# propia (nada que ver con las cartas ni el fondo). -gTFF00FF para el
# mismo relleno magenta transparente que las cartas grandes.
powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "make-crown.ps1") | Out-Null
Push-Location $gfxOut
& $grit "crown.png" -ftc -gB4 -gt -gTFF00FF -m! -o crown 2>&1
Pop-Location
Move-Item (Join-Path $gfxOut "crown.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "crown.h") $sourceOut -Force
Write-Output "Corona convertida a C con grit."

# Flecha de turno, en el centro de la cruz -- 4 variantes YA rotadas
# como arte estatico (derecha/abajo/izquierda/arriba, ver
# make-arrow.ps1), paleta compartida entre las 4 (son la misma flecha,
# solo gira).
powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "make-arrow.ps1") | Out-Null
Push-Location $gfxOut
& $grit "arrow_right.png" "arrow_down.png" "arrow_left.png" "arrow_up.png" -ftc -gB4 -gt -gTFF00FF -pS -Oarrow_shared -Sarrow_shared -m! 2>&1
Pop-Location
Move-Item (Join-Path $gfxOut "arrow_right.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "arrow_right.h") $sourceOut -Force
Move-Item (Join-Path $gfxOut "arrow_down.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "arrow_down.h") $sourceOut -Force
Move-Item (Join-Path $gfxOut "arrow_left.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "arrow_left.h") $sourceOut -Force
Move-Item (Join-Path $gfxOut "arrow_up.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "arrow_up.h") $sourceOut -Force
Move-Item (Join-Path $gfxOut "arrow_shared.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "arrow_shared.h") $sourceOut -Force
Write-Output "Flechas (4 direcciones) convertidas a C con grit."

# Tanza (pantalla de abajo, mientras se arrastra una carta).
powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "make-slingshot.ps1") | Out-Null
Push-Location $gfxOut
& $grit "sling_band.png" -ftc -gB4 -gt -gTFF00FF -m! -o sling_band 2>&1
Pop-Location
Move-Item (Join-Path $gfxOut "sling_band.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "sling_band.h") $sourceOut -Force
Write-Output "Tanza convertida a C con grit."

# Particula de festejo del ancho.
powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "make-particle.ps1") | Out-Null
Push-Location $gfxOut
& $grit "particle.png" -ftc -gB4 -gt -gTFF00FF -m! -o particle 2>&1
Pop-Location
Move-Item (Join-Path $gfxOut "particle.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "particle.h") $sourceOut -Force
Write-Output "Particula convertida a C con grit."

# Marco verde (64x64) para la carta que va ganando la baza.
powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "make-leader-frame.ps1") | Out-Null
Push-Location $gfxOut
& $grit "leader_frame.png" -ftc -gB4 -gt -gTFF00FF -m! -o leader_frame 2>&1
Pop-Location
Move-Item (Join-Path $gfxOut "leader_frame.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "leader_frame.h") $sourceOut -Force
Write-Output "Marco de lider convertido a C con grit."

# Corazones de vida (fin de mano): lleno + roto, paleta compartida
# entre los dos (mismos colores de contorno/relleno se repiten).
powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "make-hearts.ps1") | Out-Null
Push-Location $gfxOut
& $grit "heart_full.png" "heart_broken.png" -ftc -gB4 -gt -gTFF00FF -pS -Ohearts_shared -Shearts_shared -m! 2>&1
Pop-Location
Move-Item (Join-Path $gfxOut "heart_*.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "heart_*.h") $sourceOut -Force
Move-Item (Join-Path $gfxOut "hearts_shared.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "hearts_shared.h") $sourceOut -Force
Write-Output "Corazones convertidos a C con grit."

# Iconos de esquina (prediccion/ganadas), arte del usuario -- paleta
# compartida entre los dos, mismo criterio que los corazones.
powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "make-corner-icons.ps1") | Out-Null
Push-Location $gfxOut
& $grit "icon_pred.png" "icon_won.png" -ftc -gB4 -gt -gTFF00FF -pS -Ocorner_icons_shared -Scorner_icons_shared -m! 2>&1
Pop-Location
Move-Item (Join-Path $gfxOut "icon_*.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "icon_*.h") $sourceOut -Force
Move-Item (Join-Path $gfxOut "corner_icons_shared.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "corner_icons_shared.h") $sourceOut -Force
Write-Output "Iconos de esquina convertidos a C con grit."

# Placeholder de avatar (fin de mano): cuadrado vacio, paleta propia.
powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "make-avatar.ps1") | Out-Null
Push-Location $gfxOut
& $grit "avatar_placeholder.png" -ftc -gB4 -gt -gTFF00FF -m! -o avatar_placeholder 2>&1
Pop-Location
Move-Item (Join-Path $gfxOut "avatar_placeholder.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "avatar_placeholder.h") $sourceOut -Force
Write-Output "Avatar convertido a C con grit."

# Botones cuadrados del selector de prediccion (7, uno por numero 0-6),
# paleta compartida entre ellos (los mismos colores de borde/relleno se
# repiten en los 7).
powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "make-buttons.ps1") | Out-Null
Push-Location $gfxOut
$buttonFiles = Get-ChildItem -Filter "button_*.png" | ForEach-Object { $_.Name }
& $grit @buttonFiles -ftc -gB4 -gt -gTFF00FF -pS -Obuttons_shared -Sbuttons_shared -m! 2>&1
Pop-Location
Move-Item (Join-Path $gfxOut "button_*.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "button_*.h") $sourceOut -Force
Move-Item (Join-Path $gfxOut "buttons_shared.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "buttons_shared.h") $sourceOut -Force
Write-Output "Botones convertidos a C con grit."

# Festejo del ancho: 4 frases posibles, cada una un banner de arte
# (mismo criterio que el logo de titulo) en vez de texto de consola.
powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "make-ancho-banner.ps1") | Out-Null
Push-Location $gfxOut
& $grit "ancho_banner_0.png" "ancho_banner_1.png" "ancho_banner_2.png" "ancho_banner_3.png" -ftc -gB4 -gt -gTFF00FF -pS -Oancho_banner_shared -Sancho_banner_shared -m! 2>&1
Pop-Location
Move-Item (Join-Path $gfxOut "ancho_banner_shared.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "ancho_banner_shared.h") $sourceOut -Force
Move-Item (Join-Path $gfxOut "ancho_banner_0.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "ancho_banner_0.h") $sourceOut -Force
Move-Item (Join-Path $gfxOut "ancho_banner_1.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "ancho_banner_1.h") $sourceOut -Force
Move-Item (Join-Path $gfxOut "ancho_banner_2.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "ancho_banner_2.h") $sourceOut -Force
Move-Item (Join-Path $gfxOut "ancho_banner_3.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "ancho_banner_3.h") $sourceOut -Force
Write-Output "Banners de ancho (4 frases) convertidos a C con grit."

# Boton "jugar/continuar" (title, fin de mano, fin de partida), 2
# frames -- sin apretar / apretado (paleta compartida entre los 2,
# -m! sprite). Lienzo 64x64 (antes 64x32) para que se vea mas grande
# en pantalla -- ver notas en make-play-button.ps1 y en main.c.
powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "make-play-button.ps1") | Out-Null
Push-Location $gfxOut
& $grit "play_button_0.png" "play_button_1.png" -ftc -gB4 -gt -gTFF00FF -pS -Oplay_button_shared -Splay_button_shared -m! 2>&1
Pop-Location
Move-Item (Join-Path $gfxOut "play_button_shared.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "play_button_shared.h") $sourceOut -Force
Move-Item (Join-Path $gfxOut "play_button_0.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "play_button_0.h") $sourceOut -Force
Move-Item (Join-Path $gfxOut "play_button_1.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "play_button_1.h") $sourceOut -Force
Write-Output "Boton jugar (2 frames) convertido a C con grit."

# Pantalla de la productora (arranque, antes que nada mas): 2 frames de
# BITMAP de pantalla completa (256x192, no sprites/tiles -- ver
# show_studio_splash en main.c), paleta compartida.
powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "make-studio-splash.ps1") | Out-Null
Push-Location $gfxOut
& $grit "studio_splash_0.png" "studio_splash_1.png" -ftc -gB8 -gb -pS -Ostudio_splash_shared -Sstudio_splash_shared 2>&1
Pop-Location
Move-Item (Join-Path $gfxOut "studio_splash_0.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "studio_splash_0.h") $sourceOut -Force
Move-Item (Join-Path $gfxOut "studio_splash_1.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "studio_splash_1.h") $sourceOut -Force
Move-Item (Join-Path $gfxOut "studio_splash_shared.c") $sourceOut -Force
Move-Item (Join-Path $gfxOut "studio_splash_shared.h") $sourceOut -Force
Write-Output "Pantalla de productora (2 frames bitmap) convertida a C con grit."

# Icono de 32x32 para el banner de la ROM (titulo en el menu de la DS
# real/TWiLight Menu++/etc.) -- se consume directo como .bmp desde el
# Makefile (GAME_ICON), no pasa por grit.
powershell -ExecutionPolicy Bypass -File (Join-Path $PSScriptRoot "make-icon.ps1") | Out-Null

# ---------- Genera la tabla de lookup (suit,value) -> tiles ----------
$suitOrder = @("espada", "basto", "oro", "copa") # mismo orden que el enum Suit en deck.h
$allValues = @(1,2,3,4,5,6,7,8,9,10,11,12)

$includes = New-Object System.Collections.Generic.List[string]
$rows = New-Object System.Collections.Generic.List[string]
foreach ($suit in $suitOrder) {
  $entries = New-Object System.Collections.Generic.List[string]
  foreach ($v in $allValues) {
    $sym = "card_${suit}_${v}"
    $includes.Add("#include `"$sym.h`"") | Out-Null
    $entries.Add("${sym}Tiles") | Out-Null
  }
  $rows.Add("  { " + ($entries -join ", ") + " }") | Out-Null
}
$includes.Add("#include `"card_back.h`"") | Out-Null

$cardGfxH = @"
// GENERADO por tools/prep-assets.ps1 -- no editar a mano.
#ifndef BAZAS_CARD_GFX_H
#define BAZAS_CARD_GFX_H

#include "deck.h"

const unsigned int* card_tiles_for(Suit suit, int value);
const unsigned int* card_back_tiles(void);

#endif
"@

$cardGfxC = @"
// GENERADO por tools/prep-assets.ps1 -- no editar a mano.
#include "card_gfx.h"
$($includes -join "`n")

static const unsigned int* const CARD_TILES[4][12] = {
$($rows -join ",`n")
};

const unsigned int* card_tiles_for(Suit suit, int value) {
  return CARD_TILES[suit][value - 1];
}

const unsigned int* card_back_tiles(void) {
  return card_backTiles;
}
"@

Set-Content -Path (Join-Path $sourceOut "card_gfx.h") -Value $cardGfxH -Encoding ASCII
Set-Content -Path (Join-Path $sourceOut "card_gfx.c") -Value $cardGfxC -Encoding ASCII
Write-Output "Tabla de lookup card_gfx.c/.h generada."

Write-Output "Listo. Graficos convertidos en $sourceOut"
