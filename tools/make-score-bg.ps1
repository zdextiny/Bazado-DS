# Fondo de la pantalla de fin de mano (arriba): ya no "dibujado a
# mano" -- ahora un panel de color solido por jugador (esquinas
# achaflanadas, bisel simple), un color de asiento fijo por jugador
# (mismo criterio que SEAT_COLORS de la version web: el asiento 0
# siempre es el mismo color aunque cambie quien se sienta ahi). Fondo
# general azul oscuro para que los paneles resalten.
Add-Type -AssemblyName System.Drawing

$w = 256
$h = 256
$bgColor = [System.Drawing.Color]::FromArgb(30, 26, 56)

$seatColors = @(
  [System.Drawing.Color]::FromArgb(46, 196, 214),  # cyan -- asiento 0 (vos)
  [System.Drawing.Color]::FromArgb(255, 138, 61),  # naranja -- asiento 1
  [System.Drawing.Color]::FromArgb(168, 107, 255), # violeta -- asiento 2
  [System.Drawing.Color]::FromArgb(79, 214, 122)   # verde -- asiento 3
)

$img = New-Object System.Drawing.Bitmap($w, $h)
$g = [System.Drawing.Graphics]::FromImage($img)
$bgBrush = New-Object System.Drawing.SolidBrush($bgColor)
$g.FillRectangle($bgBrush, 0, 0, $w, $h)
$bgBrush.Dispose()

function Fill($g, $color, $x, $y, $w, $h) {
  if ($w -le 0 -or $h -le 0) { return }
  $b = New-Object System.Drawing.SolidBrush($color)
  $g.FillRectangle($b, $x, $y, $w, $h)
  $b.Dispose()
}

function Lighten($color, $amt) {
  $r = [Math]::Min(255, $color.R + $amt)
  $gg = [Math]::Min(255, $color.G + $amt)
  $b = [Math]::Min(255, $color.B + $amt)
  return [System.Drawing.Color]::FromArgb($r, $gg, $b)
}
function Darken($color, $amt) {
  $r = [Math]::Max(0, $color.R - $amt)
  $gg = [Math]::Max(0, $color.G - $amt)
  $b = [Math]::Max(0, $color.B - $amt)
  return [System.Drawing.Color]::FromArgb($r, $gg, $b)
}

$panelTops = @(8, 56, 104, 152)
$panelX = 12
$panelW = 232
$panelH = 40
$chamfer = 4

for ($i = 0; $i -lt 4; $i++) {
  $y = $panelTops[$i]
  $color = $seatColors[$i]
  $light = Lighten $color 35
  $dark = Darken $color 45

  Fill $g $color $panelX $y $panelW $panelH
  # bisel: linea clara arriba, linea oscura abajo (misma idea que los
  # botones de prediccion).
  Fill $g $light $panelX $y $panelW 2
  Fill $g $dark $panelX ($y + $panelH - 2) $panelW 2

  # esquinas achaflanadas: se "come" un triangulito de cada esquina con
  # el color de fondo, para una version pixel-art de bordes redondeados.
  for ($c = 0; $c -lt $chamfer; $c++) {
    $cw = $chamfer - $c
    Fill $g $bgColor $panelX ($y + $c) $cw 1
    Fill $g $bgColor ($panelX + $panelW - $cw) ($y + $c) $cw 1
    Fill $g $bgColor $panelX ($y + $panelH - 1 - $c) $cw 1
    Fill $g $bgColor ($panelX + $panelW - $cw) ($y + $panelH - 1 - $c) $cw 1
  }
}

$g.Dispose()
$outPath = "D:\Proyectos\bazas-nds\gfx\score_bg.png"
$img.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
$img.Dispose()
Write-Output "Fondo de puntaje generado en $outPath"
