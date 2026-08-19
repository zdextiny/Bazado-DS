# Genera 7 sprites de "boton cuadrado" (16x16), uno por numero (0-6),
# para el selector de prediccion — relleno + borde + el numero dibujado
# adentro, bien nitido (sin antialiasing, para que se vea pixel art y no
# borroso a este tamano tan chico).
Add-Type -AssemblyName System.Drawing

$size = 16
$outDir = "D:\Proyectos\bazas-nds\gfx"
New-Item -ItemType Directory -Path $outDir -Force | Out-Null

$fill = [System.Drawing.Color]::FromArgb(60, 40, 110)      # violeta oscuro, tono del fondo/UI
$fillLight = [System.Drawing.Color]::FromArgb(90, 60, 160) # highlight arriba-izq (efecto biselado)
$border = [System.Drawing.Color]::FromArgb(230, 200, 90)   # borde dorado
$digitColor = [System.Drawing.Color]::White

for ($n = 0; $n -le 6; $n++) {
  $img = New-Object System.Drawing.Bitmap($size, $size)
  $g = [System.Drawing.Graphics]::FromImage($img)
  $g.Clear([System.Drawing.Color]::FromArgb(255, 0, 255)) # magenta = transparente

  $borderBrush = New-Object System.Drawing.SolidBrush($border)
  $g.FillRectangle($borderBrush, 0, 0, $size, $size)
  $borderBrush.Dispose()

  $fillBrush = New-Object System.Drawing.SolidBrush($fill)
  $g.FillRectangle($fillBrush, 1, 1, $size - 2, $size - 2)
  $fillBrush.Dispose()

  # Biselado simple: una franja mas clara arriba y a la izquierda.
  $lightBrush = New-Object System.Drawing.SolidBrush($fillLight)
  $g.FillRectangle($lightBrush, 1, 1, $size - 2, 2)
  $g.FillRectangle($lightBrush, 1, 1, 2, $size - 2)
  $lightBrush.Dispose()

  $g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::SingleBitPerPixelGridFit
  $g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
  $font = New-Object System.Drawing.Font("Consolas", 10, [System.Drawing.FontStyle]::Bold)
  $textBrush = New-Object System.Drawing.SolidBrush($digitColor)
  $text = "$n"
  $textSize = $g.MeasureString($text, $font)
  $tx = [int](($size - $textSize.Width) / 2)
  $ty = [int](($size - $textSize.Height) / 2) - 1
  $g.DrawString($text, $font, $textBrush, $tx, $ty)
  $font.Dispose()
  $textBrush.Dispose()

  $g.Dispose()
  $img.Save((Join-Path $outDir "button_$n.png"), [System.Drawing.Imaging.ImageFormat]::Png)
  $img.Dispose()
}

Write-Output "7 botones generados en $outDir"
