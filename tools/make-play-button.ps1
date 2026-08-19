# Boton "jugar/continuar" animado (title, fin de mano, fin de partida):
# 4 frames del usuario (Play1..4.png -- idle, apretado, rebote,
# vuelta a idle), reescalados de 224x112 a 64x32 con NEAREST NEIGHBOR
# -- el arte fuente ya es completamente plano (Play1/3/4 tienen 5
# colores, Play2 solo 2, sin degrade ni antialiasing), asi que un
# resize con blending de por medio inventaria colores nuevos de la
# nada; nearest solo re-muestrea los que ya existen.
#
# OJO: escalar 224x112 a 64x32 JUSTO (edge-to-edge, sin margen) perdia
# detalle de las esquinas redondeadas y de la sombra rosa/roja de abajo
# -- quedaban recortadas contra el borde del sprite. Ahora se deja un
# margen fijo (MARGIN px a cada lado) y el contenido se escala un poco
# MAS CHICO que el canvas entero para entrar completo con aire
# alrededor, centrado -- mismo criterio que Convert-CardTo32 en
# prep-assets.ps1 (centrar la caja de contenido, no estirarla al borde).
Add-Type -AssemblyName System.Drawing

$src = "C:\Users\Dex\Desktop\Resourses\Mazo"
$gfxOut = "D:\Proyectos\bazas-nds\gfx"
$targetW = 64
$targetH = 32
$margin = 3

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

Convert-PlayButtonFrame (Join-Path $src "Play1.png") (Join-Path $gfxOut "play_button_0.png")
Convert-PlayButtonFrame (Join-Path $src "Play2.png") (Join-Path $gfxOut "play_button_1.png")
Convert-PlayButtonFrame (Join-Path $src "Play3.png") (Join-Path $gfxOut "play_button_2.png")
Convert-PlayButtonFrame (Join-Path $src "Play4.png") (Join-Path $gfxOut "play_button_3.png")

# Chequeo de colores combinados (paleta COMPARTIDA entre los 4 frames,
# -pS -- tienen que entrar los 4 en 16 colores contando el magenta).
$allColors = New-Object 'System.Collections.Generic.HashSet[int]'
foreach ($i in 0..3) {
  $img = [System.Drawing.Bitmap]::FromFile((Join-Path $gfxOut "play_button_$i.png"))
  for ($y = 0; $y -lt $img.Height; $y++) {
    for ($x = 0; $x -lt $img.Width; $x++) {
      [void]$allColors.Add($img.GetPixel($x, $y).ToArgb())
    }
  }
  $img.Dispose()
}
Write-Output "Boton jugar: $($allColors.Count) colores combinados (los 4 frames juntos, incluye magenta)"
if ($allColors.Count -gt 16) {
  Write-Warning "Mas de 16 colores combinados -- grit va a fallar o recortar mal, hace falta reducir el arte fuente."
}
