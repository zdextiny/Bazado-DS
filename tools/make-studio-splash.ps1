# Pantalla de la productora (Dextiny Production) al arrancar el juego,
# antes que nada mas -- ver show_studio_splash en main.c. Arte del
# usuario (141x100) centrado en un lienzo de pantalla completa
# (256x192), relleno con el mismo gris oscuro del fondo del logo (se
# toma directo de la esquina de la imagen) para que no se note la
# costura entre el arte y el resto de la pantalla.
Add-Type -AssemblyName System.Drawing

$src = "D:\PixelArt123\2026"
$gfxOut = "D:\Proyectos\bazas-nds\gfx"
$canvasW = 256
$canvasH = 192

function Convert-SplashFrame($srcPath, $destPath) {
  $orig = [System.Drawing.Bitmap]::FromFile($srcPath)
  $fill = $orig.GetPixel(0, 0) # gris oscuro del fondo del logo

  $canvas = New-Object System.Drawing.Bitmap($canvasW, $canvasH)
  $g = [System.Drawing.Graphics]::FromImage($canvas)
  $fillBrush = New-Object System.Drawing.SolidBrush([System.Drawing.Color]::FromArgb(255, $fill.R, $fill.G, $fill.B))
  $g.FillRectangle($fillBrush, 0, 0, $canvasW, $canvasH)
  $fillBrush.Dispose()

  $destX = [int](($canvasW - $orig.Width) / 2)
  $destY = [int](($canvasH - $orig.Height) / 2)
  $g.DrawImage($orig, $destX, $destY, $orig.Width, $orig.Height)
  $g.Dispose()
  $orig.Dispose()

  $canvas.Save($destPath, [System.Drawing.Imaging.ImageFormat]::Png)
  $canvas.Dispose()
}

Convert-SplashFrame (Join-Path $src "DextinyProductions1.png") (Join-Path $gfxOut "studio_splash_0.png")
Convert-SplashFrame (Join-Path $src "DextinyProductions2.png") (Join-Path $gfxOut "studio_splash_1.png")
Write-Output "Frames de la pantalla de productora generados (256x192)."
