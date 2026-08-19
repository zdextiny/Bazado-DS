# Pantalla de titulo: remolino rojo/azul con domain warping de verdad
# (fbm + desplazamiento de coordenadas, la MISMA tecnica que el
# prototipo de shader WebGL que se armo para probar el look -- ver el
# shader del artifact). Sale mucho mas organico/tipo-tinta que un
# plasma de puras sinusoides.
#
# El logo "BAZADO" NO se dibuja aca -- ver make-title-logo.ps1. Estuvo
# horneado directo en este mismo lienzo en una version anterior, pero
# eso significaba que paneaba junto con el fondo (el usuario lo dijo
# clarito: "deja el texto de bazado quieto"). Ahora el logo es un
# sprite aparte con posicion FIJA en pantalla; este archivo solo genera
# el remolino de atras, que si puede panear/ciclar libremente.
#
# La DS no tiene shaders -- el "movimiento" sale de generar varios
# FRAMES estaticos (mismo campo de ruido, con el tiempo del shader
# congelado en un valor distinto cada uno) y ciclarlos lento por
# codigo (ver show_title_background/title_bg_tick en main.c).
#
# El computo pesado (ruido fbm, ~25 evaluaciones por pixel) se hace en
# C# compilado al vuelo (Add-Type), no en un loop de PowerShell
# interpretado -- a pixel por pixel puro-PS esto tardaria minutos.
Add-Type -AssemblyName System.Drawing

$csharp = @'
using System;
using System.Drawing;

public static class SwirlGen {
  static double Hash(double x, double y) {
    double v = Math.Sin(x * 127.1 + y * 311.7) * 43758.5453;
    return v - Math.Floor(v);
  }

  static double VNoise(double x, double y) {
    double ix = Math.Floor(x), iy = Math.Floor(y);
    double fx = x - ix, fy = y - iy;
    double a = Hash(ix, iy), b = Hash(ix + 1, iy), c = Hash(ix, iy + 1), d = Hash(ix + 1, iy + 1);
    double ux = fx * fx * (3 - 2 * fx), uy = fy * fy * (3 - 2 * fy);
    return a + (b - a) * ux + (c - a) * uy * (1 - ux) + (d - b) * ux * uy;
  }

  static double Fbm(double x, double y) {
    double v = 0, amp = 0.55, px = x, py = y;
    for (int i = 0; i < 5; i++) {
      v += amp * VNoise(px, py);
      double nx = 1.6 * px - 1.2 * py;
      double ny = 1.2 * px + 1.6 * py;
      px = nx; py = ny;
      amp *= 0.5;
    }
    return v;
  }

  static void Warp(double x, double y, double t, double warpAmt, out double rx, out double ry) {
    double qx = Fbm(x, y);
    double qy = Fbm(x + 5.2, y + 1.3);
    rx = Fbm(x + warpAmt * qx + 1.7 + 0.15 * t, y + warpAmt * qy + 9.2 + 0.15 * t);
    ry = Fbm(x + warpAmt * qx + 8.3 + 0.126 * t, y + warpAmt * qy + 2.8 + 0.126 * t);
  }

  static double Smoothstep(double e0, double e1, double x) {
    double t = Math.Max(0, Math.Min(1, (x - e0) / (e1 - e0)));
    return t * t * (3 - 2 * t);
  }

  static Color LerpColor(Color a, Color b, double t) {
    t = Math.Max(0, Math.Min(1, t));
    return Color.FromArgb(
      (int)(a.R + (b.R - a.R) * t),
      (int)(a.G + (b.G - a.G) * t),
      (int)(a.B + (b.B - a.B) * t));
  }

  // A PROPOSITO sin interpolar entre paradas (nada de LerpColor aca):
  // un gradiente suave de verdad da cientos de colores distintos (se
  // probo -- 600 por frame), muy por encima del limite de 16 colores
  // de una capa 4bpp de la DS. Bandas DURAS, en cambio, dejan SOLO 5
  // colores fijos sin importar la forma del remolino -- ademas hace
  // que aparezcan muchos tiles de 8x8 repetidos (areas planas del
  // mismo color), asi el tileset entra comodo en VRAM.
  static Color Stops5(double t, Color c0, Color c1, Color c2, Color c3, Color c4) {
    t = Math.Max(0, Math.Min(1, t));
    int i = (int)(t * 5.0);
    if (i > 4) i = 4;
    switch (i) {
      case 0: return c0;
      case 1: return c1;
      case 2: return c2;
      case 3: return c3;
      default: return c4;
    }
  }

  public static Bitmap GenerateFrame(int size, double t, double warpAmt) {
    Bitmap bmp = new Bitmap(size, size);
    Color c0 = Color.FromArgb(8, 6, 20);
    Color c1 = Color.FromArgb(20, 24, 74);
    Color c2 = Color.FromArgb(40, 70, 168);
    Color c3 = Color.FromArgb(196, 42, 74);
    Color c4 = Color.FromArgb(110, 16, 40);
    for (int y = 0; y < size; y++) {
      double uy = (y - size / 2.0) / size;
      for (int x = 0; x < size; x++) {
        double ux = (x - size / 2.0) / size;
        double px = ux * 1.7;
        double py = uy * 1.7;
        double rx, ry;
        Warp(px, py, t, warpAmt, out rx, out ry);
        double v = Fbm(px + 1.8 * rx, py + 1.8 * ry);
        v = Smoothstep(0.08, 0.92, v);
        bmp.SetPixel(x, y, Stops5(v, c0, c1, c2, c3, c4));
      }
    }
    return bmp;
  }
}
'@
Add-Type -TypeDefinition $csharp -ReferencedAssemblies System.Drawing

$size = 256
$gfxOut = "D:\Proyectos\bazas-nds\gfx"

# 2 frames nomas (no 3): cada frame agrega su propio set de tiles a la
# VRAM (el campo de ruido no se repite como el patron viejo, asi que
# se reduce poco) -- 2 ya alcanza para que se note que "respira" al
# ciclar, sin arriesgarse a quedarse sin banco de tiles.
$frameTimes = @(0.0, 6.0)
$warpAmt = 2.2

for ($i = 0; $i -lt $frameTimes.Count; $i++) {
  $bmp = [SwirlGen]::GenerateFrame($size, $frameTimes[$i], $warpAmt)
  $outPath = Join-Path $gfxOut "title_bg_$i.png"
  $bmp.Save($outPath, [System.Drawing.Imaging.ImageFormat]::Png)
  $bmp.Dispose()
  Write-Output "Frame $i del fondo de titulo generado en $outPath"
}
