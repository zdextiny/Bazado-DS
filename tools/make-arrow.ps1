# Flecha (16x16, pixel art simple) para marcar de quien es el turno --
# ahora en el centro de la cruz, apuntando al asiento correspondiente.
#
# Antes esto rotaba en TIEMPO REAL con oamRotateScale (una sola matriz
# afin) -- el usuario reporto que solo se veia apuntar izquierda/
# derecha, nunca arriba/abajo. En vez de perseguir el bug de la matriz
# afin (angulo, unidades, sizeDouble...), se generan las 4 variantes ya
# rotadas COMO ARTE ESTATICO (Bitmap.RotateFlip, rotacion exacta pixel
# a pixel, sin matrices ni redondeo) y el juego elige cual mostrar segun
# a quien le toca -- mismo criterio ya probado que icon_pred/icon_won.
Add-Type -AssemblyName System.Drawing

$size = 16
$img = New-Object System.Drawing.Bitmap($size, $size)
$g = [System.Drawing.Graphics]::FromImage($img)
$g.Clear([System.Drawing.Color]::FromArgb(255, 0, 255))

$outline = [System.Drawing.Color]::FromArgb(20, 60, 20)
$fill = [System.Drawing.Color]::FromArgb(80, 220, 100)

$outlinePts = @(
  (New-Object System.Drawing.Point 1,2),
  (New-Object System.Drawing.Point 13,8),
  (New-Object System.Drawing.Point 1,14)
)
$fillPts = @(
  (New-Object System.Drawing.Point 3,4),
  (New-Object System.Drawing.Point 11,8),
  (New-Object System.Drawing.Point 3,12)
)

$outlineBrush = New-Object System.Drawing.SolidBrush($outline)
$g.FillPolygon($outlineBrush, $outlinePts)
$outlineBrush.Dispose()

$fillBrush = New-Object System.Drawing.SolidBrush($fill)
$g.FillPolygon($fillBrush, $fillPts)
$fillBrush.Dispose()
$g.Dispose()

$gfxOut = "D:\Proyectos\bazas-nds\gfx"
$img.Save((Join-Path $gfxOut "arrow_right.png"), [System.Drawing.Imaging.ImageFormat]::Png)

$down = $img.Clone()
$down.RotateFlip([System.Drawing.RotateFlipType]::Rotate90FlipNone)
$down.Save((Join-Path $gfxOut "arrow_down.png"), [System.Drawing.Imaging.ImageFormat]::Png)
$down.Dispose()

$left = $img.Clone()
$left.RotateFlip([System.Drawing.RotateFlipType]::Rotate180FlipNone)
$left.Save((Join-Path $gfxOut "arrow_left.png"), [System.Drawing.Imaging.ImageFormat]::Png)
$left.Dispose()

$up = $img.Clone()
$up.RotateFlip([System.Drawing.RotateFlipType]::Rotate270FlipNone)
$up.Save((Join-Path $gfxOut "arrow_up.png"), [System.Drawing.Imaging.ImageFormat]::Png)
$up.Dispose()

$img.Dispose()
Write-Output "Flechas (4 direcciones) generadas en $gfxOut"
