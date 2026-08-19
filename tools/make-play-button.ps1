# Boton "jugar/continuar" (title, fin de mano, fin de partida): ahora
# 2 frames nomas (antes 4) -- sin apretar / apretado, arte nuevo del
# usuario (PlayButton1/2.png, un triangulo blanco con un brillo
# rosa/naranja abajo que desaparece al apretar). Se agranda el lienzo
# nativo de 64x32 a 64x64 (el arte fuente ya es 64x61, casi cuadrado)
# para que al aplicarle el mismo 2x por afin de siempre en main.c de
# 128x64 pase a 128x128 -- bastante mas grande, ocupando gran parte de
# la pantalla de abajo, sin arriesgar recorte (sizeDouble reserva
# EXACTO el doble del nativo con 2x, ver notas en main.c).
Add-Type -AssemblyName System.Drawing

$src = "D:\PixelArt123\2026"
$gfxOut = "D:\Proyectos\bazas-nds\gfx"
$targetW = 64
$targetH = 64
$margin = 2

function Convert-PlayButtonFrame($srcPath, $destPath) {
  $orig = [System.Drawing.Bitmap]::FromFile($srcPath)

  $maxW = $targetW - 2 * $margin
  $maxH = $targetH - 2 * $margin
  $scale = [math]::Min($maxW / $orig.Width, $maxH / $orig.Height)
  $contentW = [int]([math]::Round($orig.Width * $scale))
  $contentH = [int]([math]::Round($orig.Height * $scale))
  $destX = [int](($targetW - $contentW) / 2)
  $destY = [int](($targetH - $contentH) / 2)

  $small = New-Object System.Drawing.Bitmap($targetW, $targetH)
  $g = [System.Drawing.Graphics]::FromImage($small)
  $g.Clear([System.Drawing.Color]::FromArgb(0, 0, 0, 0))
  $g.InterpolationMode = [System.Drawing.Drawing2D.InterpolationMode]::NearestNeighbor
  $g.PixelOffsetMode = [System.Drawing.Drawing2D.PixelOffsetMode]::Half
  $g.DrawImage($orig, $destX, $destY, $contentW, $contentH)
  $g.Dispose()
  $orig.Dispose()

  # Alpha bajo (incluye el margen recien agregado, que arranca 100%
  # transparente) -> magenta (mismo color llave -gTFF00FF del resto del
  # pipeline); el resto se deja tal cual (opaco).
  for ($y = 0; $y -lt $targetH; $y++) {
    for ($x = 0; $x -lt $targetW; $x++) {
      $p = $small.GetPixel($x, $y)
      if ($p.A -lt 128) {
        $small.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(255, 255, 0, 255))
      } else {
        $small.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(255, $p.R, $p.G, $p.B))
      }
    }
  }
  $small.Save($destPath, [System.Drawing.Imaging.ImageFormat]::Png)
  $small.Dispose()
}

Convert-PlayButtonFrame (Join-Path $src "PlayButton1.png") (Join-Path $gfxOut "play_button_0.png")
Convert-PlayButtonFrame (Join-Path $src "PlayButton2.png") (Join-Path $gfxOut "play_button_1.png")

# Chequeo de colores combinados (paleta COMPARTIDA entre los 2 frames,
# -pS -- tienen que entrar en 16 colores contando el magenta).
$allColors = New-Object 'System.Collections.Generic.HashSet[int]'
foreach ($i in 0..1) {
  $img = [System.Drawing.Bitmap]::FromFile((Join-Path $gfxOut "play_button_$i.png"))
  for ($y = 0; $y -lt $img.Height; $y++) {
    for ($x = 0; $x -lt $img.Width; $x++) {
      [void]$allColors.Add($img.GetPixel($x, $y).ToArgb())
    }
  }
  $img.Dispose()
}
Write-Output "Boton jugar: $($allColors.Count) colores combinados (los 2 frames juntos, incluye magenta)"
if ($allColors.Count -gt 16) {
  Write-Warning "Mas de 16 colores combinados -- grit va a fallar o recortar mal, hace falta reducir el arte fuente."
}
