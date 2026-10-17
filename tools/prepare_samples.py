#!/usr/bin/env python3
"""Prepare the sample sets shipped with Atelier Instruments.

Downloads nothing by itself: run tools/fetch_sources.sh first (sparse git
clones of the CC0 / CC-BY source libraries), then this script converts a
subset to OGG/FLAC and writes our own SFZ mappings.

Sources (licences verified at the source repositories):
  * Salamander Grand Piano V3 - Alexander Holm - CC-BY 3.0
  * Virtuosity Drums - Versilian Studios / sfzinstruments - CC0 1.0
  * VSCO 2 Community Edition (strings) - Versilian Studios - CC0 1.0
"""
import os, re, subprocess, sys, glob
import numpy as np, soundfile as sf

SRC = sys.argv[1] if len(sys.argv) > 1 else "../ai-src"
OUT = sys.argv[2] if len(sys.argv) > 2 else "Instruments"
NOTES = {"C":0,"C#":1,"D":2,"D#":3,"E":4,"F":5,"F#":6,"G":7,"G#":8,"A":9,"A#":10,"B":11}

def note_to_midi(name, octave_shift=0):
    m = re.match(r"([A-G]#?)(-?\d)", name)
    return 12*(int(m.group(2))+1) + NOTES[m.group(1)] + 12*octave_shift

def load(path):
    x, sr = sf.read(path, dtype="float32", always_2d=True)
    if x.shape[1] == 1: x = np.repeat(x, 2, axis=1)
    return x[:, :2], sr

def trim(x, sr, max_s, fade_s=0.4):
    # drop leading silence below -60 dB (keeps the attack intact)
    env = np.abs(x).max(axis=1)
    thr = 10**(-60/20) * max(env.max(), 1e-9)
    start = int(np.argmax(env > thr))
    start = max(0, start - int(0.002*sr))
    x = x[start:start+int(max_s*sr)].copy()
    # trailing silence
    env = np.abs(x).max(axis=1)
    idx = np.where(env > 10**(-70/20))[0]
    if len(idx): x = x[:idx[-1]+1]
    n = min(len(x), int(fade_s*sr))
    if n > 0: x[-n:] *= np.linspace(1, 0, n)[:, None]**2
    return x

def write(x, sr, dst, fmt):
    os.makedirs(os.path.dirname(dst), exist_ok=True)
    if fmt == "flac":
        sf.write(dst, x, sr, subtype="PCM_16"); return
    tmp = dst + ".wav"; sf.write(tmp, x, sr, subtype="PCM_16")
    subprocess.run(["ffmpeg","-v","error","-y","-i",tmp,"-c:a","libvorbis","-q:a","5",dst], check=True)
    os.remove(tmp)

# ------------------------------------------------------------------ piano
def piano():
    root = os.path.join(OUT, "Piano"); lines = [
      "// Atelier Piano - mapping (c) Atelier Instruments, AGPLv3",
      "// Samples: Salamander Grand Piano V3 by Alexander Holm, CC-BY 3.0 (subset, 4 of 16 velocity layers)",
      "<global> amp_veltrack=45 ampeg_release=0.5 bus=0", ""]
    layers = [(4,1,40),(8,41,76),(12,77,104),(15,105,127)]
    files = glob.glob(os.path.join(SRC,"SalamanderGrandPiano/Samples/*v15.flac"))
    keys = sorted(note_to_midi(re.match(r"([A-G]#?\d)v", os.path.basename(f)).group(1)) for f in files)
    names = {note_to_midi(re.match(r"([A-G]#?\d)v", os.path.basename(f)).group(1)): re.match(r"([A-G]#?\d)v", os.path.basename(f)).group(1) for f in files}
    for i,k in enumerate(keys):
        lo = 21 if i == 0 else (keys[i-1]+k)//2 + 1
        hi = 108 if i == len(keys)-1 else (k+keys[i+1])//2
        maxlen = max(3.5, 13 - (k-21)*0.12)
        for v,lv,hv in layers:
            src = os.path.join(SRC, f"SalamanderGrandPiano/Samples/{names[k]}v{v}.flac")
            x, sr = load(src); x = trim(x, sr, maxlen, 0.8)
            fn = f"samples/{names[k].replace('#','s')}_v{v}.ogg"
            write(x, sr, os.path.join(root, fn), "ogg")
            lines.append(f"<region> sample={fn} lokey={lo} hikey={hi} pitch_keycenter={k} lovel={lv} hivel={hv}")
    open(os.path.join(root,"piano.sfz"),"w").write("\n".join(lines)+"\n")

# ------------------------------------------------------------------ drums
def drums():
    root = os.path.join(OUT, "Batterie"); d = os.path.join(SRC, "virtuosity_drums/Samples/mid")
    lines = ["// Atelier Batterie - General MIDI mapping (c) Atelier Instruments, AGPLv3",
             "// Samples: Virtuosity Drums (mid mics) by Versilian Studios, CC0 1.0",
             "// bus: 0 kick, 1 snare, 2 hi-hat, 3 toms, 4 cymbals",
             "<global> loop_mode=one_shot amp_veltrack=55 ampeg_release=0.08", ""]
    done = {}
    def piece(prefix, maxlen, max_bands=8):
        if prefix in done: return done[prefix]
        files = sorted(glob.glob(os.path.join(d, "*", f"mid_{prefix}_vl*.*")))
        parsed = []
        for f in files:
            m = re.search(r"_vl(\d+)(?:_rr(\d+))?\.", f); parsed.append((int(m.group(1)), int(m.group(2) or 1), f))
        vls = sorted({p[0] for p in parsed}); has_rr = any(p[1] > 1 for p in parsed)
        if has_rr: bands = [[v] for v in vls]
        else:
            per = max(1, -(-len(vls)//max_bands)); bands = [vls[i:i+per] for i in range(0, len(vls), per)]
        out = []
        for bi, band in enumerate(bands):
            lv = 1 + (127*bi)//len(bands); hv = (127*(bi+1))//len(bands)
            items = [p for p in parsed if p[0] in band]
            for si,(vl,rr,f) in enumerate(sorted(items)):
                x, sr = load(f); x = trim(x, sr, maxlen, min(0.3, maxlen/4))
                fn = f"samples/{os.path.splitext(os.path.basename(f))[0].replace('mid_','')}.ogg"
                write(x, sr, os.path.join(root, fn), "ogg")
                out.append((fn, lv, hv, len(items), si+1))
        done[prefix] = out; return out
    gm = [ # key, prefix, maxlen, bus, transpose, extra
      (35,"kick_snon",1.6,0,-1,""),(36,"kick_snon",1.6,0,0,""),
      (37,"snare_crossstick",1.0,1,0,""),(38,"snare_center",2.2,1,0,""),(40,"snare_rimshot",2.2,1,0,""),
      (42,"hh_closed",1.2,2,0,"group=2"),(44,"hh_pedal",1.5,2,0,"group=2"),(46,"hh_open",4.0,2,0,"group=1 off_by=2"),
      (41,"ltom_center",3.0,3,-5,""),(43,"ltom_center",3.0,3,-3,""),(45,"ltom_center",3.0,3,0,""),
      (47,"htom_center",3.0,3,-2,""),(48,"htom_center",3.0,3,0,""),(50,"htom_center",3.0,3,2,""),
      (49,"crash_crash",6.0,4,0,""),(57,"crash_crash",6.0,4,1,""),(55,"flatride_crash",4.0,4,0,""),
      (51,"ride_ride",5.0,4,0,""),(59,"ride_ride",5.0,4,-1,""),(53,"ride_bell",5.0,4,0,""),
    ]
    for key,prefix,maxlen,bus,tr,extra in gm:
        lines.append(f"// key {key}: {prefix}")
        for fn,lv,hv,n,pos in piece(prefix,maxlen):
            seq = f" seq_length={n} seq_position={pos}" if n > 1 else ""
            lines.append(f"<region> sample={fn} key={key} pitch_keycenter={key-tr} lovel={lv} hivel={hv}{seq} bus={bus} {extra}".rstrip())
    open(os.path.join(root,"batterie.sfz"),"w").write("\n".join(lines)+"\n")

# ------------------------------------------------------------------ strings
def bake_loop(x, sr):
    n = len(x); start = int(n*0.30); end = min(int(n*0.85), start + int(4.0*sr))
    xf = int(0.35*sr); xf = min(xf, start)
    w = np.sin(np.linspace(0, np.pi/2, xf))[:, None]
    y = x[:end].copy()
    y[end-xf:end] = x[end-xf:end]*np.cos(np.linspace(0,np.pi/2,xf))[:,None] + x[start-xf:start]*w
    return y, start, end

def strings():
    root = os.path.join(OUT, "Cordes"); base = os.path.join(SRC, "vsco/Strings")
    lines = ["// Atelier Cordes - string ensemble mapping (c) Atelier Instruments, AGPLv3",
             "// Samples: VSCO 2 Community Edition by Versilian Studios, CC0 1.0 (sustain vibrato, looped)",
             "<global> loop_mode=loop_continuous amp_veltrack=60 ampeg_attack=0.12 ampeg_release=0.6", ""]
    sets = [("Cello Section/susvib", r"susvib_([A-G]#?\d)_v(\d)_1\.wav", 24, 54),
            ("Viola Section/susvib", r"ViolaEns_susvib_([A-G]#?\d)_v(\d)_1\.wav", 55, 66),
            ("Violin Section/susVib", r"VlnEns_susVib_([A-G]#?\d)_v(\d)\.wav", 67, 103)]
    for sub, rx, kmin, kmax in sets:
        found = {}
        for f in glob.glob(os.path.join(base, sub, "*.wav")):
            m = re.match(rx, os.path.basename(f))
            if m: found.setdefault(note_to_midi(m.group(1), 1), {})[int(m.group(2))] = f
        keys = sorted(k for k in found if kmin-6 <= k <= kmax+6)
        inrange = [k for k in keys if kmin <= k <= kmax] or keys
        # choose the samples whose centre lies in this section's range, plus edges
        sel = sorted(set(inrange + [min(keys, key=lambda k: abs(k-kmin)), min(keys, key=lambda k: abs(k-kmax))]))
        for i,k in enumerate(sel):
            lo = kmin if i == 0 else (sel[i-1]+k)//2 + 1
            hi = kmax if i == len(sel)-1 else (k+sel[i+1])//2
            lo = max(lo, kmin); hi = min(hi, kmax)
            if lo > hi: continue
            vels = sorted(found[k])
            for j,v in enumerate(vels):
                lv, hv = (1,80) if j == 0 and len(vels) > 1 else ((81,127) if len(vels) > 1 else (1,127))
                x, sr = load(found[k][v]); x = trim(x, sr, 30, 0.0)
                y, ls, le = bake_loop(x, sr)
                fn = f"samples/{sub.split()[0].lower()}_{k}_v{v}.flac"
                write(y, sr, os.path.join(root, fn), "flac")
                lines.append(f"<region> sample={fn} lokey={lo} hikey={hi} pitch_keycenter={k} lovel={lv} hivel={hv} loop_start={ls} loop_end={le-1}")
    open(os.path.join(root,"cordes.sfz"),"w").write("\n".join(lines)+"\n")

if __name__ == "__main__":
    which = sys.argv[3:] or ["piano","drums","strings"]
    for w in which: globals()[w](); print("done", w)
