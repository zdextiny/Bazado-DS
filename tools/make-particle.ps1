# Particula chiquita (8x8) para el festejo del ancho (espada/basto) --
# un destello de 4 puntas, dorado con centro claro. Varias de estas se
# tiran alrededor de la carta cuando sale un ancho (ver
# celebrate_ancho en main.c).
Add-Type -AssemblyName System.Drawing

$img = New-Object System.Drawing.Bitmap(8, 8)
$g = [System.Drawing.Graphics]::FromImage($img)
$g.Clear([System.Drawing.Color]::FromArgb(255, 0, 255))
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None

$gold = [System.Drawing.Color]::FromArgb(255, 205, 60)
$bright = [System.Drawing.Color]::FromArgb(255, 250, 210)

function Px($g, $color, $x, $y) {
  $b = New-Object System.Drawing.SolidBrush($color)
  $g.FillRectangle($b, $x, $y, 1, 1)
  $b.Dispose()
}

# 4 puntas (forma de "+" con esquinas recortadas), centro mas claro.
$goldPts = @(3,1, 4,1, 1,3, 1,4, 6,3, 6,4, 3,6, 4,6)
for ($i = 0; $i -lt $goldPts.Count; $i += 2) {
  Px $g $gold $goldPts[$i] $goldPts[$i + 1]
}
Px $g $bright 3 3
Px $g $bright 4 3
Px $g $bright 3 4
Px $g $bright 4 4

$g.Dispose()
$outPath = "D:\Proyectos\bazas-nds\gfx\particle.png"
$img.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
$img.Dispose()
Write-Output "Particula generada en $outPath"
