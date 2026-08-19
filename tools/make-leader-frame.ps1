# Marco verde para resaltar la carta que va ganando la baza en este
# momento -- igual espiritu que .card-leader de la version web. En vez
# de un cuadrado generico, la forma sale de la silueta REAL de una
# carta de frente (mismo contorno achaflanado en las esquinas que
# comparten las 48 -- el back NO sirve de referencia, esa arte es
# opaca de punta a punta, sin el margen transparente de las de
# frente), asi el marco queda pegado al borde de verdad de la carta en
# vez de flotar como un cuadrado mas grande que no calza con las
# esquinas cortadas.
#
# El arte de las cartas NO tiene margen transparente arriba/abajo (el
# contenido llega hasta el borde del lienzo de 32x32 en esas dos
# direcciones, solo a los costados hay aire) -- por eso el anillo no
# puede crecer siempre hacia AFUERA (no habria donde en arriba/abajo).
# En cambio se calcula un anillo que abraza el borde real de la carta
# de los dos lados a la vez (un poco para afuera donde hay lugar, un
# poco para adentro donde no), asi queda parejo en las 4 direcciones.
Add-Type -AssemblyName System.Drawing

$nativeSize = 32
$outSize = 64
$cardRef = "D:\Proyectos\bazas-nds\gfx\card_oro_1.png" # cualquier carta de frente sirve: todas comparten silueta
$out = "D:\Proyectos\bazas-nds\gfx\leader_frame.png"

$src = [System.Drawing.Bitmap]::FromFile($cardRef)

$mask = New-Object 'bool[,]' $nativeSize, $nativeSize
for ($y = 0; $y -lt $nativeSize; $y++) {
  for ($x = 0; $x -lt $nativeSize; $x++) {
    $p = $src.GetPixel($x, $y)
    $isMagenta = ($p.R -eq 255 -and $p.G -eq 0 -and $p.B -eq 255)
    $mask[$y, $x] = -not $isMagenta
  }
}
$src.Dispose()

function Dilate($mask, $radius, $size) {
  $result = New-Object 'bool[,]' $size, $size
  for ($y = 0; $y -lt $size; $y++) {
    for ($x = 0; $x -lt $size; $x++) {
      if ($mask[$y, $x]) { $result[$y, $x] = $true; continue }
      $hit = $false
      for ($dy = -$radius; $dy -le $radius -and -not $hit; $dy++) {
        $ny = $y + $dy
        if ($ny -lt 0 -or $ny -ge $size) { continue }
        for ($dx = -$radius; $dx -le $radius; $dx++) {
          $nx = $x + $dx
          if ($nx -lt 0 -or $nx -ge $size) { continue }
          if ($mask[$ny, $nx]) { $hit = $true; break }
        }
      }
      $result[$y, $x] = $hit
    }
  }
  # OJO: "return $result" a secas desenrolla el array multidimensional
  # a traves del pipeline (PowerShell lo aplana solo) y se pierde toda
  # la forma 2D -- la coma evita ese desenrollado.
  return ,$result
}

# Erosion: lo opuesto a Dilate -- un pixel sigue "adentro" solo si TODOS
# sus vecinos en el radio tambien lo estan. Los vecinos que caen afuera
# del lienzo cuentan como "afuera" (no como "adentro"), a proposito:
# asi el borde del lienzo se trata igual que un borde real de la carta
# (que es exactamente lo que pasa arriba/abajo, sin margen transparente
# ahi) y la erosion tambien se come esas filas/columnas.
function Erode($mask, $radius, $size) {
  $result = New-Object 'bool[,]' $size, $size
  for ($y = 0; $y -lt $size; $y++) {
    for ($x = 0; $x -lt $size; $x++) {
      if (-not $mask[$y, $x]) { continue }
      $allIn = $true
      for ($dy = -$radius; $dy -le $radius -and $allIn; $dy++) {
        $ny = $y + $dy
        if ($ny -lt 0 -or $ny -ge $size) { $allIn = $false; break }
        for ($dx = -$radius; $dx -le $radius; $dx++) {
          $nx = $x + $dx
          if ($nx -lt 0 -or $nx -ge $size -or -not $mask[$ny, $nx]) { $allIn = $false; break }
        }
      }
      $result[$y, $x] = $allIn
    }
  }
  return ,$result
}

# Anillo de ~2px de grosor (escala nativa) que abraza el borde real de
# la carta: crece 1px hacia afuera Y se come 1px hacia adentro, asi
# queda parejo en las 4 direcciones sin importar si de ese lado hay
# margen transparente (costados) o no (arriba/abajo).
$outer = Dilate $mask 1 $nativeSize
$inner = Erode $mask 1 $nativeSize

$img = New-Object System.Drawing.Bitmap($outSize, $outSize)
$green = [System.Drawing.Color]::FromArgb(255, 70, 230, 120)
$magenta = [System.Drawing.Color]::FromArgb(255, 255, 0, 255)

for ($y = 0; $y -lt $outSize; $y++) {
  $sy = [math]::Floor($y / ($outSize / $nativeSize))
  for ($x = 0; $x -lt $outSize; $x++) {
    $sx = [math]::Floor($x / ($outSize / $nativeSize))
    $ring = $outer[$sy, $sx] -and (-not $inner[$sy, $sx])
    if ($ring) {
      $img.SetPixel($x, $y, $green)
    } else {
      $img.SetPixel($x, $y, $magenta)
    }
  }
}

$img.Save($out, [System.Drawing.Imaging.ImageFormat]::Png)
$img.Dispose()
Write-Output "Marco de lider generado en $out (siguiendo la silueta real de la carta)"
