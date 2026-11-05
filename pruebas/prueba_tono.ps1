# Prueba de linealidad: genera un tono de 1 kHz que baja 6 dB por paso,
# lo reproduce por el altavoz del ordenador y registra el nivel del ESP32.
# Uso: cerrar el Arduino IDE y ejecutar  .\prueba_tono.ps1 -Puerto COM6
param([string]$Puerto = 'COM6')

$wav = Join-Path $env:TEMP 'prueba_tono.wav'
Add-Type -TypeDefinition @'
using System; using System.IO;
public static class Tono { public static void Crear(string ruta) {
  int fs=48000; double[] niveles={double.NegativeInfinity,0,-6,-12,-18,-24,-30,double.NegativeInfinity}; double[] dur={3,4,4,4,4,4,4,3};
  int total=0; foreach(var d in dur) total+=(int)(d*fs);
  using(var w=new BinaryWriter(File.Create(ruta))){
    w.Write(System.Text.Encoding.ASCII.GetBytes("RIFF")); w.Write(36+total*2); w.Write(System.Text.Encoding.ASCII.GetBytes("WAVEfmt ")); w.Write(16); w.Write((short)1); w.Write((short)1); w.Write(fs); w.Write(fs*2); w.Write((short)2); w.Write((short)16); w.Write(System.Text.Encoding.ASCII.GetBytes("data")); w.Write(total*2);
    long n=0; for(int s=0;s<niveles.Length;s++){ double a=double.IsInfinity(niveles[s])?0:0.8*Math.Pow(10,niveles[s]/20); int m=(int)(dur[s]*fs); for(int i=0;i<m;i++,n++) w.Write((short)(32767*a*Math.Sin(2*Math.PI*1000*n/fs))); }
  } } }
'@
[Tono]::Crear($wav)

$sp = New-Object System.IO.Ports.SerialPort $Puerto,115200,'None',8,'One'
$sp.DtrEnable = $false; $sp.RtsEnable = $false
$pl = New-Object System.Media.SoundPlayer $wav; $pl.Load()
$filas = @()
try {
  $sp.Open(); $sp.DiscardInBuffer(); $pl.Play()
  $sw = [Diagnostics.Stopwatch]::StartNew(); $buf = ''
  while ($sw.ElapsedMilliseconds -lt 30000) {
    Start-Sleep -Milliseconds 50
    $buf += $sp.ReadExisting(); $partes = $buf -split "`n"; $buf = $partes[-1]
    foreach ($l in $partes[0..($partes.Count-2)]) {
      if ($l -match 'nivel_dBFS:(-?[\d.]+) leds:(\d+)') {
        $filas += [pscustomobject]@{ t = $sw.ElapsedMilliseconds/1000; db = [double]$matches[1]; leds = [int]$matches[2] }
      }
    }
  }
} finally { $pl.Stop(); if ($sp.IsOpen) { $sp.Close() } }

# Mediana de cada tramo, sin su primer segundo (transición del filtro)
$tramos = @(@('silencio',0,3),@('0 dB',3,7),@('-6 dB',7,11),@('-12 dB',11,15),@('-18 dB',15,19),@('-24 dB',19,23),@('-30 dB',23,27),@('silencio',27,30))
foreach ($g in $tramos) {
  $w = $filas | Where-Object { $_.t -ge ($g[1]+1) -and $_.t -lt $g[2] }
  if ($w) {
    $s = $w | Sort-Object db; $l = $w | Sort-Object leds
    "{0,-9} mediana {1,6:N1} dBFS  LEDs {2}" -f $g[0], $s[[int]($s.Count/2)].db, $l[[int]($l.Count/2)].leds
  }
}
