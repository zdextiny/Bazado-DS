# Logo "BAZADO" letra por letra (6 sprites, uno por letra -- antes eran
# 4 piezas de 64x64 sin relacion con las letras en si). El usuario
# corto cada letra a mano de title_logo.png y las dejo en gfx/ (B.png,
# A.png, Z.png, a-2.png -- la segunda A, D.png, O.png), todas con el
# mismo fondo magenta que ya usa el resto del pipeline.
#
# Cada letra ya mide 64 de alto pero un ANCHO propio (31-37px, no
# encajan en ningun tamano "alto" de sprite de la DS que sea mas
# angosto que 64) -- se pegan contra el borde IZQUIERDO de un lienzo
# cuadrado de 64x64 (relleno magenta) para poder usar SpriteSize_64x64
# como el resto del juego, sin offset que despues haya que descontar en
# main.c: el borde izquierdo del sprite es directamente el borde
# izquierdo de la letra. El ANCHO REAL de cada una (medido a mano,
# hardcodeado en main.c) es lo que se usa para calcular donde empieza
# la letra siguiente -- los sprites se superponen de sobra (64 de ancho
# cada uno, la letra ocupa bastante menos), no pasa nada porque el
# resto es magenta/transparente.
Add-Type -AssemblyName System.Drawing

$gfxOut = "D:\Proyectos\bazas-nds\gfx"
$letters = @("B", "A", "Z", "a-2", "D", "O") # orden de BAZADO
$canvas = 64

for ($i = 0; $i -lt $letters.Count; $i++) {
  $srcPath = Join-Path $gfxOut "$($letters[$i]).png"
  $src = [System.Drawing.Bitmap]::FromFile($srcPath)

  $dest = New-Object System.Drawing.Bitmap($canvas, $canvas)
  $g = [System.Drawing.Graphics]::FromImage($dest)
  $magentaBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, 255, 0, 255))
  $g.FillRectangle($magentaBrush, 0, 0, $canvas, $canvas)
  $magentaBrush.Dispose()
  $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
  # Pegada contra el borde izquierdo (x=0) -- alto ya es 64, sin offset
  # vertical tampoco.
  $g.DrawImageUnscaled($src, 0, 0)
  $g.Dispose()
  $src.Dispose()

  $outPath = Join-Path $gfxOut "title_letter_$i.png"
  $dest.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
  $dest.Dispose()
  Write-Output "title_letter_$i.png <- $($letters[$i]).png"
}
