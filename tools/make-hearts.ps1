# Corazones de "vida" para la pantalla de fin de mano (16x16 cada uno,
# paleta compartida entre los dos): uno lleno (vida que queda) y uno
# roto -- tachado con una X roja -- (vida perdida). Relleno magenta =
# transparente, mismo criterio que el resto de los iconos.
Add-Type -AssemblyName System.Drawing

function New-HeartCanvas() {
  $img = New-Object System.Drawing.Bitmap(16, 16)
  $g = [System.Drawing.Graphics]::FromImage($img)
  $g.Clear([System.Drawing.Color]::FromArgb(255, 0, 255))
  return @{ Img = $img; G = $g }
}

function Fill($g, $color, $x, $y, $w, $h) {
  $b = New-Object System.Drawing.SolidBrush($color)
  $g.FillRectangle($b, $x, $y, $w, $h)
  $b.Dispose()
}

$outline = [System.Drawing.Color]::FromArgb(90, 10, 15)
$red = [System.Drawing.Color]::FromArgb(220, 40, 55)
$gray = [System.Drawing.Color]::FromArgb(120, 110, 115)

# Forma de corazon con rectangulos (pixel art bien cuadrado), simetrica:
# dos "bultos" arriba, se juntan en una punta abajo.
function Draw-HeartShape($g, $fillColor, $outlineColor) {
  # contorno (un pixel mas grande por lado)
  Fill $g $outlineColor 2 2 4 3
  Fill $g $outlineColor 9 2 4 3
  Fill $g $outlineColor 1 4 14 4
  Fill $g $outlineColor 2 8 12 2
  Fill $g $outlineColor 3 10 10 2
  Fill $g $outlineColor 4 12 8 1
  Fill $g $outlineColor 5 13 6 1
  Fill $g $outlineColor 6 14 4 1
  Fill $g $outlineColor 7 15 2 1

  # relleno, un pixel adentro
  Fill $g $fillColor 3 3 3 2
  Fill $g $fillColor 10 3 3 2
  Fill $g $fillColor 2 5 12 3
  Fill $g $fillColor 3 8 10 2
  Fill $g $fillColor 4 10 8 2
  Fill $g $fillColor 5 12 6 1
  Fill $g $fillColor 6 13 4 1
  Fill $g $fillColor 7 14 2 1
}

# --- Corazon lleno (vida que queda) ---
$c1 = New-HeartCanvas
Draw-HeartShape $c1.G $red $outline
$shine = [System.Drawing.Color]::FromArgb(240, 130, 140)
Fill $c1.G $shine 3 4 2 1
$c1.G.Dispose()
$c1.Img.Save("D:\Proyectos\bazas-nds\gfx\heart_full.png", [System.Drawing.Imaging.ImageFormat]::Png)
$c1.Img.Dispose()

# --- Corazon roto (vida perdida): gris hueco + tachado con una X roja ---
$c2 = New-HeartCanvas
Draw-HeartShape $c2.G $gray $outline
$penX = New-Object System.Drawing.Pen($red, 2)
$c2.G.DrawLine($penX, 2, 2, 13, 13)
$c2.G.DrawLine($penX, 13, 2, 2, 13)
$penX.Dispose()
$c2.G.Dispose()
$c2.Img.Save("D:\Proyectos\bazas-nds\gfx\heart_broken.png", [System.Drawing.Imaging.ImageFormat]::Png)
$c2.Img.Dispose()

Write-Output "Corazones generados en D:\Proyectos\bazas-nds\gfx\"
