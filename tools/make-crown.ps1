# Corona: arte de verdad del usuario (Mazo/Corona.png, 16x9 con alpha
# real), no mas la version dibujada a mano por acá. Se centra (segun
# el contenido realmente opaco, no el tamaño del archivo -- mismo
# criterio que Convert-CardTo32 en prep-assets.ps1) en un lienzo de
# 16x16, y de ahi se escala 2x con nearest-neighbor (pixel perfecto,
# sin ningun suavizado) a 32x32 -- mas grande que la version vieja de
# 16x16 y del tamaño justo para ir centrada arriba de la carta que
# gana la ronda.
Add-Type -AssemblyName System.Drawing

$srcPath = "C:\Users\Dex\Desktop\Resourses\Mazo\Corona.png"
$gfxOut = "D:\Proyectos\bazas-nds\gfx"

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
  return @{ X = $minX; Y = $minY; W = ($maxX - $minX + 1); H = ($maxY - $minY + 1) }
}

$src = [System.Drawing.Bitmap]::FromFile($srcPath)
$bounds = Get-ContentBounds $src

$native = 16
$offsetX = [int]([math]::Round(($native - $bounds.W) / 2)) - $bounds.X
$offsetY = [int]([math]::Round(($native - $bounds.H) / 2)) - $bounds.Y

$canvas = New-Object System.Drawing.Bitmap($native, $native)
for ($y = 0; $y -lt $native; $y++) {
  for ($x = 0; $x -lt $native; $x++) {
    $sx = $x - $offsetX
    $sy = $y - $offsetY
    if ($sx -lt 0 -or $sx -ge $src.Width -or $sy -lt 0 -or $sy -ge $src.Height) {
      $canvas.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(255, 255, 0, 255))
      continue
    }
    $p = $src.GetPixel($sx, $sy)
    if ($p.A -lt 128) {
      $canvas.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(255, 255, 0, 255))
    } else {
      $canvas.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(255, $p.R, $p.G, $p.B))
    }
  }
}
$src.Dispose()

$outSize = 32
$out = New-Object System.Drawing.Bitmap($outSize, $outSize)
for ($y = 0; $y -lt $outSize; $y++) {
  $sy = [math]::Floor($y * $native / $outSize)
  for ($x = 0; $x -lt $outSize; $x++) {
    $sx = [math]::Floor($x * $native / $outSize)
    $out.SetPixel($x, $y, $canvas.GetPixel($sx, $sy))
  }
}
$canvas.Dispose()

$outPath = Join-Path $gfxOut "crown.png"
$out.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
$out.Dispose()
Write-Output "Corona (arte del usuario) convertida y generada en $outPath"
