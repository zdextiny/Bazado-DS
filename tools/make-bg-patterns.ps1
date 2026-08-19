# Fondos de patron seleccionables para el gameplay (10, uno por archivo
# en la carpeta "Fondos infinitos" del usuario) -- antes habia un unico
# patron hardcodeado (pattern_078.png, en la carpeta padre); ahora se
# puede elegir entre estos 10 desde Opciones (ver run_options_menu en
# main.c). Mismo tinte de rojo que se usaba antes (blanco -> rojo
# calido, negro se queda oscuro con un toque de rojo) para que pinten
# igual que el resto del arte del juego -- los 10 quedan con la MISMA
# paleta de 2 colores de salida sin importar el patron fuente, asi que
# comparten un solo banco de paleta (-pS en el grit de prep-assets.ps1).
Add-Type -AssemblyName System.Drawing

$bgSrcDir = "C:\Users\Dex\Desktop\Resourses\Fondo infinito\Fondos infinitos"
$gfxOut = "D:\Proyectos\bazas-nds\gfx"

$files = Get-ChildItem -Path $bgSrcDir -Filter "*.png" | Sort-Object Name
if ($files.Count -ne 10) {
  Write-Output "OJO: se esperaban 10 patrones en $bgSrcDir, se encontraron $($files.Count) -- BG_PATTERN_COUNT en main.c tiene que coincidir."
}

for ($i = 0; $i -lt $files.Count; $i++) {
  $bg = [System.Drawing.Bitmap]::FromFile($files[$i].FullName)
  $bgOut = New-Object System.Drawing.Bitmap($bg.Width, $bg.Height)
  for ($y = 0; $y -lt $bg.Height; $y++) {
    for ($x = 0; $x -lt $bg.Width; $x++) {
      $px = $bg.GetPixel($x, $y)
      if ($px.R -gt 128) {
        $bgOut.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(200, 40, 40))
      } else {
        $bgOut.SetPixel($x, $y, [System.Drawing.Color]::FromArgb(40, 8, 8))
      }
    }
  }
  $bg.Dispose()
  $outPath = Join-Path $gfxOut "bg_pattern_$i.png"
  $bgOut.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
  $bgOut.Dispose()
  Write-Output "bg_pattern_$i.png <- $($files[$i].Name)"
}
