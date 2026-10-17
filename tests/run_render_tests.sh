#!/usr/bin/env bash
# Offline render of a few notes per instrument; atelier_render fails on silence or clipping.
set -euo pipefail
R="$1"; OUT="${2:-renders}"; mkdir -p "$OUT"
"$R" Instruments/Piano/piano.sfz "$OUT/piano.wav" 48,40,0,1.5 60,90,0.5,1.5 64,100,1,1.5 67,127,1.5,2 36,127,2,2 84,70,2.5,1
"$R" Instruments/Batterie/batterie.sfz "$OUT/batterie.wav" 36,120,0,0.1 42,90,0,0.1 42,70,0.25,0.1 38,127,0.5,0.1 42,90,0.5,0.1 46,100,0.75,0.1 36,110,1,0.1 44,80,1,0.1 38,120,1.5,0.1 49,127,2,0.1 36,127,2,0.1 48,100,2.5,0.1 45,100,2.75,0.1 41,110,3,0.1 51,90,3.25,0.1 53,90,3.5,0.1
"$R" Instruments/Cordes/cordes.sfz "$OUT/cordes.wav" 48,90,0,6 55,90,0,6 64,90,0,6 72,110,0,6 40,100,6.5,3
echo "Tous les rendus sont OK"
