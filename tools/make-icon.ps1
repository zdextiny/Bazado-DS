# Icono de 32x32 para el banner de la ROM (titulo en el menu de la
# DS/3DS, TWiLight Menu++, etc.) -- el Makefile nunca definia
# GAME_ICON, asi que ndstool armaba el banner con un archivo vacio.
# Probablemente la causa real de "An error has occurred" al abrirlo
# via nds-bootstrap en la 3DS (TWiLight Menu++ lee el banner para su
# lista de juegos; un banner invalido/vacio lo puede hacer fallar,
# aunque en un emulador como melonDS -- que no lo necesita para nada --
# nunca se hubiera notado).
Add-Type -AssemblyName System.Drawing

$bg = [System.Drawing.Color]::FromArgb(30, 26, 56) # mismo azul oscuro que el panel de fin de mano
$img = New-Object System.Drawing.Bitmap(32, 32)
$g = [System.Drawing.Graphics]::FromImage($img)
$g.Clear($bg)
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None

# El as de espada (arte del usuario, a-e-Pico.png -- la carta mas
# fuerte del mazo, buen icono representativo) es 22x32: ya mide 32 de
# alto igual que el icono entero, asi que entra sin escalar (pixel a
# pixel, sin perder nitidez) y solo hace falta centrarlo horizontal.
$cardSrc = [System.Drawing.Bitmap]::FromFile("C:\Users\Dex\Desktop\Resourses\Mazo\a-e-Pico.png")
$destX = [int]((32 - $cardSrc.Width) / 2)

for ($y = 0; $y -lt $cardSrc.Height; $y++) {
  for ($x = 0; $x -lt $cardSrc.Width; $x++) {
    $p = $cardSrc.GetPixel($x, $y)
    if ($p.A -ge 128) {
      $img.SetPixel($destX + $x, $y, [System.Drawing.Color]::FromArgb($p.R, $p.G, $p.B))
    }
  }
}
$cardSrc.Dispose()

$g.Dispose()

# ndstool pide un BMP CON PALETA (indexado) -- Bitmap.Save de GDI+ por
# defecto escribe RGB de 24 bits sin paleta ("Bitmap must have a
# palette." fue el error). Se arma a mano un bitmap de 8bpp indexado:
# se junta la lista de colores realmente usados (son pocos, arte
# pixel-art simple) y se escribe un indice por pixel via LockBits.
$colors = New-Object 'System.Collections.Generic.List[System.Drawing.Color]'
$indexOf = @{}
$indexed = New-Object 'int[,]' 32, 32
for ($y = 0; $y -lt 32; $y++) {
  for ($x = 0; $x -lt 32; $x++) {
    $c = $img.GetPixel($x, $y)
    $key = $c.ToArgb()
    if (-not $indexOf.ContainsKey($key)) {
      $indexOf[$key] = $colors.Count
      $colors.Add($c)
    }
    $indexed[$y, $x] = $indexOf[$key]
  }
}
$img.Dispose()

# El formato de icono de la DS es de 4bpp (16 colores) -- si el arte
# fuente llegara a tener mas que eso, avisa fuerte en vez de dejar que
# ndstool lo trunque/corrompa en silencio.
if ($colors.Count -gt 16) {
  Write-Warning "El icono tiene $($colors.Count) colores (max 16 para un icono de DS) -- hace falta reducir el arte fuente."
}

$paletted = New-Object System.Drawing.Bitmap(32, 32, [System.Drawing.Imaging.PixelFormat]::Format8bppIndexed)
$pal = $paletted.Palette
for ($i = 0; $i -lt $colors.Count; $i++) { $pal.Entries[$i] = $colors[$i] }
$paletted.Palette = $pal

$rect = New-Object System.Drawing.Rectangle(0, 0, 32, 32)
$bmpData = $paletted.LockBits($rect, [System.Drawing.Imaging.ImageLockMode]::WriteOnly, [System.Drawing.Imaging.PixelFormat]::Format8bppIndexed)
$rowBytes = New-Object byte[] ($bmpData.Stride)
for ($y = 0; $y -lt 32; $y++) {
  for ($x = 0; $x -lt 32; $x++) { $rowBytes[$x] = [byte]$indexed[$y, $x] }
  [System.Runtime.InteropServices.Marshal]::Copy($rowBytes, 0, [IntPtr]::Add($bmpData.Scan0, $y * $bmpData.Stride), $bmpData.Stride)
}
$paletted.UnlockBits($bmpData)

$outPath = "D:\Proyectos\bazas-nds\gfx\icon.bmp"
$paletted.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Bmp)
$paletted.Dispose()
Write-Output "Icono generado en $outPath ($($colors.Count) colores)"
