# Pantalla de fin de partida (arriba): SOLO el texto ("Ganaste"/
# "Perdiste", misma tipografia que el logo de BAZADO), sin icono (se
# probo con un icono de usuario arriba del texto -- pixel_art_small --
# pero se decidio sacarlo del todo y dejar nada mas que el texto,
# agrandado, centrado, igual que ya se hacia en la derrota).
#
# Arte del usuario, con alpha real, compuesto sobre el mismo fondo
# magenta que usa el resto del pipeline (mismo criterio que
# make-corner-icons.ps1: >=128 de alpha se toma como opaco, si no se
# vuelve transparente/magenta). El texto fuente (36x10/36x11) queda
# minusculo si se centra tal cual en el lienzo -- se agranda primero
# nearest-neighbor (sin suavizar, pixel art) hasta llenar casi todo el
# lienzo, ANTES de que el juego le aplique su propio 2x por afin
# arriba de esto (ver OUTCOME_TEXT_* en main.c).
Add-Type -AssemblyName System.Drawing

$gfxOut = "D:\Proyectos\bazas-nds\gfx"
$textCanvasW = 64
$textCanvasH = 32 # no existe un tamano de sprite de la DS de 64x16 -- el mas chico que
                   # entra un ancho de 64 es 64x32 (el mismo que ya usa el boton de jugar)
$textFitW = 60 # 2px de margen por lado dentro del lienzo
$textFitH = 28

function New-FittedBitmap($srcPath, $canvasW, $canvasH, $fitW, $fitH) {
  $src = [System.Drawing.Bitmap]::FromFile($srcPath)
  $dest = New-Object System.Drawing.Bitmap($canvasW, $canvasH)
  $magenta = [System.Drawing.Color]::FromArgb(255, 255, 0, 255)
  for ($y = 0; $y -lt $canvasH; $y++) {
    for ($x = 0; $x -lt $canvasW; $x++) {
      $dest.SetPixel($x, $y, $magenta)
    }
  }

  # Escala preservando proporcion para entrar en fitW x fitH, nearest-neighbor.
  $scale = [math]::Min([double]$fitW / $src.Width, [double]$fitH / $src.Height)
  $scaledW = [int][math]::Round($src.Width * $scale)
  $scaledH = [int][math]::Round($src.Height * $scale)
  $offX = [int](($canvasW - $scaledW) / 2)
  $offY = [int](($canvasH - $scaledH) / 2)

  for ($y = 0; $y -lt $scaledH; $y++) {
    $sy = [int][math]::Floor($y / $scale)
    if ($sy -ge $src.Height) { $sy = $src.Height - 1 }
    for ($x = 0; $x -lt $scaledW; $x++) {
      $sx = [int][math]::Floor($x / $scale)
      if ($sx -ge $src.Width) { $sx = $src.Width - 1 }
      $p = $src.GetPixel($sx, $sy)
      if ($p.A -ge 128) {
        $dest.SetPixel($offX + $x, $offY + $y, [System.Drawing.Color]::FromArgb(255, $p.R, $p.G, $p.B))
      }
    }
  }
  $src.Dispose()
  return $dest
}

$winText = New-FittedBitmap "D:\PixelArt123\2026\Ganaste.png" $textCanvasW $textCanvasH $textFitW $textFitH
$winText.Save((Join-Path $gfxOut "win_text.png"), [System.Drawing.Imaging.ImageFormat]::Png)
$winText.Dispose()
$loseText = New-FittedBitmap "D:\PixelArt123\2026\Perdiste.png" $textCanvasW $textCanvasH $textFitW $textFitH
$loseText.Save((Join-Path $gfxOut "lose_text.png"), [System.Drawing.Imaging.ImageFormat]::Png)
$loseText.Dispose()

Write-Output "Pantalla de fin de partida (texto ganar/perder) generada en $gfxOut"
