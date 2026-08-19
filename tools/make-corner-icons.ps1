# Iconos de "prediccion" (objetivo) y "bazas ganadas" (trofeo) para las
# esquinas de puntaje, reemplazando las letras sueltas "P"/"G" -- arte
# del usuario (Mazo/Objetivo.png, Mazo/Ganadas.png), 9x9 con alpha real.
# Se AGRANDAN a 16x16 (mismo tamano que el resto de los iconos de
# estado del juego -- corona, flecha, avatar) en vez de achicarse: 9x9
# es demasiado chico para un sprite de la DS (no hay tamano de OAM que
# calce) y, a diferencia de reducir, agrandar con nearest-neighbor
# nunca pierde ningun pixel fuente (cada uno se pisa varias veces, no
# se salta ninguno) -- se probo reducir a 8x8 primero y se comia el
# detalle fino (el punto del centro del objetivo, las asas del trofeo).
Add-Type -AssemblyName System.Drawing

$srcDir = "C:\Users\Dex\Desktop\Resourses\Mazo"
$gfxOut = "D:\Proyectos\bazas-nds\gfx"
$outSize = 16

function Convert-IconTo16x16($srcPath, $destPath) {
  $src = [System.Drawing.Bitmap]::FromFile($srcPath)
  $srcSize = $src.Width # 9x9, cuadrado

  $dest = New-Object System.Drawing.Bitmap($outSize, $outSize)
  for ($y = 0; $y -lt $outSize; $y++) {
    $sy = [math]::Floor($y * $srcSize / $outSize)
    for ($x = 0; $x -lt $outSize; $x++) {
      $sx = [math]::Floor($x * $srcSize / $outSize)
      $p = $src.GetPixel($sx, $sy)
      if ($p.A -ge 128) {
        $dest.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(255, $p.R, $p.G, $p.B))
      } else {
        $dest.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(255, 255, 0, 255))
      }
    }
  }
  $src.Dispose()
  $dest.Save($destPath, [System.Drawing.Imaging.ImageFormat]::Png)
  $dest.Dispose()
}

Convert-IconTo16x16 (Join-Path $srcDir "Objetivo.png") (Join-Path $gfxOut "icon_pred.png")
Convert-IconTo16x16 (Join-Path $srcDir "Ganadas.png") (Join-Path $gfxOut "icon_won.png")
Write-Output "Iconos de esquina (prediccion/ganadas) generados en $gfxOut"
