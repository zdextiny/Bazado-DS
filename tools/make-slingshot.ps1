# "Tanza" (linea/piolin) que aparece SOLO mientras se arrastra una
# carta -- el usuario pidio explicitamente que NO sea una hondera de
# caricatura (se probo con una horqueta de madera en V y no gustaba),
# sino algo mas sutil: un solo hilo tenso desde un punto fijo abajo de
# la pantalla hasta la carta. Un solo punto/cuenta chica (sling_band),
# reutilizado en cadena para armar el hilo entero -- no hay arte de
# horqueta en esta version.
Add-Type -AssemblyName System.Drawing

$gfxOut = "D:\Proyectos\bazas-nds\gfx"

$band = New-Object System.Drawing.Bitmap(8, 8)
$g = [System.Drawing.Graphics]::FromImage($band)
$g.Clear([System.Drawing.Color]::FromArgb(255, 0, 255))
$g.SmoothingMode = [System.Drawing.Drawing2D.SmoothingMode]::None
$brush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(224, 224, 220))
$g.FillRectangle($brush, 3, 3, 2, 2)
$brush.Dispose()
$g.Dispose()
$band.Save((Join-Path $gfxOut "sling_band.png"), [System.Drawing.Imaging.ImageFormat]::Png)
$band.Dispose()

Write-Output "Tanza generada en $gfxOut (sling_band.png)"
