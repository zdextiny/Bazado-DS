# Placeholder de foto de perfil (16x16): un cuadrado vacio con borde,
# para cuando haya retratos de personaje de verdad -- por ahora solo
# marca el lugar. Relleno magenta = transparente.
Add-Type -AssemblyName System.Drawing

$size = 16
$img = New-Object System.Drawing.Bitmap($size, $size)
$g = [System.Drawing.Graphics]::FromImage($img)
$g.Clear([System.Drawing.Color]::FromArgb(255, 0, 255))

$fill = [System.Drawing.Color]::FromArgb(235, 225, 210)
$border = [System.Drawing.Color]::FromArgb(90, 70, 50)

$b = New-Object System.Drawing.SolidBrush($fill)
$g.FillRectangle($b, 0, 0, $size, $size)
$b.Dispose()

$pen = New-Object System.Drawing.Pen($border, 2)
$g.DrawRectangle($pen, 1, 1, $size - 2, $size - 2)
$pen.Dispose()

$g.Dispose()
$img.Save("D:\Proyectos\bazas-nds\gfx\avatar_placeholder.png", [System.Drawing.Imaging.ImageFormat]::Png)
$img.Dispose()
Write-Output "Placeholder de avatar generado en D:\Proyectos\bazas-nds\gfx\avatar_placeholder.png"
