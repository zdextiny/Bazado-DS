# Festejo del ancho: las 4 frases posibles como arte (sprite), no texto
# de consola -- el texto de consola no se puede escalar (un caracter
# ocupa siempre un tile de 8x8) y ademas compite en el mismo nivel de
# prioridad que el marco de lider/la flecha (sprites de prioridad 0),
# asi que a veces terminaba tapado. Mismo estilo (relleno crema, borde
# oscuro) que title_logo.png / make-title-logo.ps1.
Add-Type -AssemblyName System.Drawing

$w = 64
$h = 32
$gfxOut = "D:\Proyectos\bazas-nds\gfx"
$phrases = @("ANCHO!", "GRANDE!", "BRUTAL!", "QUE CARTA!")

$outlineColor = [System.Drawing.Color]::FromArgb(40, 8, 16)
$fillColor = [System.Drawing.Color]::FromArgb(255, 238, 196)
$outlineBrush = New-Object System.Drawing.SolidBrush($outlineColor)
$fillBrush = New-Object System.Drawing.SolidBrush($fillColor)
$fmt = New-Object System.Drawing.StringFormat
$fmt.Alignment = [System.Drawing.StringAlignment]::Center
$fmt.LineAlignment = [System.Drawing.StringAlignment]::Center

for ($i = 0; $i -lt $phrases.Count; $i++) {
  $text = $phrases[$i]
  $img = New-Object System.Drawing.Bitmap($w, $h)
  $g = [System.Drawing.Graphics]::FromImage($img)
  $g.Clear([System.Drawing.Color]::FromArgb(255, 0, 255))
  $g.TextRenderingHint = [System.Drawing.Text.TextRenderingHint]::SingleBitPerPixelGridFit

  # Tamano de fuente mas grande que entre (probar de mayor a menor) --
  # las frases mas largas ("QUE CARTA!", 10 caracteres) necesitan una
  # fuente bastante mas chica que "ANCHO!" para entrar en 64px de ancho.
  $margin = 4
  $font = $null
  for ($size = 20; $size -ge 6; $size--) {
    $candidate = New-Object System.Drawing.Font("Arial", $size, [System.Drawing.FontStyle]::Bold)
    $measured = $g.MeasureString($text, $candidate)
    if ($measured.Width -le ($w - $margin) -and $measured.Height -le ($h - $margin)) {
      $font = $candidate
      break
    }
  }
  if (-not $font) { $font = New-Object System.Drawing.Font("Arial", 6, [System.Drawing.FontStyle]::Bold) }

  $cx = $w / 2
  $cy = $h / 2
  $offsets = @(
    @(-1,-1), @(0,-1), @(1,-1),
    @(-1, 0),           @(1, 0),
    @(-1, 1), @(0, 1), @(1, 1)
  )
  foreach ($o in $offsets) {
    $g.DrawString($text, $font, $outlineBrush, [System.Drawing.PointF]::new($cx + $o[0], $cy + $o[1]), $fmt)
  }
  $g.DrawString($text, $font, $fillBrush, [System.Drawing.PointF]::new($cx, $cy), $fmt)

  # Igual que title_logo: forzar opaco cualquier pixel que no sea
  # magenta puro (el anti-alias de GDI+ deja bordes semi-transparentes
  # pese al hint, y grit los tomaria como magenta-a-medias = huecos).
  for ($y = 0; $y -lt $h; $y++) {
    for ($x = 0; $x -lt $w; $x++) {
      $p = $img.GetPixel($x, $y)
      $isMagenta = ($p.R -eq 255 -and $p.G -eq 0 -and $p.B -eq 255)
      if (-not $isMagenta) {
        $img.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(255, $p.R, $p.G, $p.B))
      }
    }
  }
  $g.Dispose()
  $outPath = Join-Path $gfxOut "ancho_banner_$i.png"
  $img.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
  $img.Dispose()
  Write-Output "Banner '$text' generado en $outPath (fuente $($font.Size)pt)"
}