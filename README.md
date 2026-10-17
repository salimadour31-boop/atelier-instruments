# Atelier Instruments

**Instruments virtuels libres et gratuits au son acoustique, pour FL Studio et tous les logiciels VST3 / AU.**
Windows x64 (VST3) et macOS universel Intel + Apple Silicon (VST3 + Audio Unit).

| Plugin | Instrument | Échantillons (source, licence) |
|---|---|---|
| **Atelier Piano** | Piano à queue de concert (88 touches, 4 couches de vélocité, pédale de sustain CC64) | Salamander Grand Piano V3 – Alexander Holm – **CC-BY 3.0** |
| **Atelier Batterie** | Batterie acoustique, mapping **General MIDI**, mixeur par élément, round-robin, charleston « choke » | Virtuosity Drums (micros « mid ») – Versilian Studios / sfzinstruments – **CC0 1.0** |
| **Atelier Cordes** | Ensemble de cordes (violoncelles, altos, violons) en sustain vibrato bouclé, 2 couches de vélocité | VSCO 2 Community Edition – Versilian Studios – **CC0 1.0** |

👉 **Téléchargement : [Releases](https://github.com/salimadour31-boop/atelier-instruments/releases/latest)**

---

## Français

### Ce que fait le moteur
Chaque plugin embarque le même moteur d'échantillonneur maison (style SFZ) :
- couches de vélocité, **round-robin** (`seq_length` / `seq_position`), groupes d'étouffement (`group` / `off_by`, ex. charleston ouvert coupé par le fermé) ;
- interpolation Hermite 4 points, transposition et **accord** (±12 demi-tons) ;
- enveloppe **ADSR** (piano et cordes), boucles de sustain avec fondu enchaîné pré-calculé (cordes) ;
- pédale de sustain (CC64), 96 voix de polyphonie avec vol de voix intelligent ;
- **réverbération** intégrée (quantité + taille de salle), volume, sensibilité à la vélocité ;
- batterie : **mixeur par élément** (grosse caisse, caisse claire, charleston, toms, cymbales/overheads) et 16 pads cliquables ;
- interface soignée (thème bois sombre), clavier cliquable, vu-mètre, état de chargement.

Les échantillons sont livrés **dans le plugin** (dossier `Contents/Resources` du bundle) : il suffit de copier le plugin, rien d'autre à installer.

### Installation – Windows (FL Studio)
1. Téléchargez `AtelierInstruments-0.1.0-Windows-x64-Setup.exe` et lancez-le (il installe dans `C:\Program Files\Common Files\VST3`).
   *Ou* décompressez `AtelierInstruments-0.1.0-Windows-x64-VST3.zip` et copiez les dossiers `Atelier Piano.vst3`, `Atelier Batterie.vst3`, `Atelier Cordes.vst3` dans
   `C:\Program Files\Common Files\VST3`.
   > Windows SmartScreen peut afficher « Windows a protégé votre ordinateur » (installateur non signé) : cliquez sur **Informations complémentaires → Exécuter quand même**.
2. Dans FL Studio : **Options → Manage plugins** (Gestionnaire de plugins) → **Find installed plugins / Find plugins** (rescan).
3. Cochez l'étoile ★ à côté des trois « Atelier … » pour les ajouter à la liste, puis ajoutez-les dans le **Channel Rack** (*Add → Atelier Piano*).

### Installation – macOS (Intel ou Apple Silicon, ex. iMac Intel sous Ventura)
1. Téléchargez `AtelierInstruments-0.1.0-macOS-universal.pkg` et ouvrez-le
   (installe dans `/Library/Audio/Plug-Ins/VST3` et `/Library/Audio/Plug-Ins/Components`).
   *Ou* décompressez le `.zip` et copiez :
   - les `.vst3` dans `~/Library/Audio/Plug-Ins/VST3`
   - les `.component` dans `~/Library/Audio/Plug-Ins/Components`
   (dans le Finder : **Aller → Aller au dossier…** puis collez le chemin).
2. **Gatekeeper** – les plugins ne sont pas signés par Apple (projet gratuit). Si macOS bloque le paquet ou si FL Studio ne voit pas le plugin :
   - pour le `.pkg` : **clic droit → Ouvrir → Ouvrir** (ou *Réglages Système → Confidentialité et sécurité → Ouvrir quand même*) ;
   - pour les plugins, ouvrez le **Terminal** et tapez :
     ```bash
     xattr -cr ~/Library/Audio/Plug-Ins/VST3/Atelier*.vst3 ~/Library/Audio/Plug-Ins/Components/Atelier*.component
     # si vous avez utilisé le .pkg :
     sudo xattr -cr /Library/Audio/Plug-Ins/VST3/Atelier*.vst3 /Library/Audio/Plug-Ins/Components/Atelier*.component
     ```
3. Dans FL Studio Mac : **Options → Manage plugins → Find plugins**, puis ajoutez « Atelier … » au Channel Rack (VST3 ou AU).

### Utilisation rapide
- **Atelier Batterie** suit le mapping General MIDI (comme les batteries VST que les utilisateurs de FL connaissent) :
  35/36 grosse caisse · 37 cross-stick · 38 caisse claire · 40 rimshot · 42 charleston fermé · 44 charleston pied · 46 charleston ouvert ·
  41/43 toms basse · 45/47/48/50 toms · 49/57 crash · 51/59 ride · 53 cloche de ride · 55 splash.
  Dans FL Studio, ouvrez le **Piano roll** : les noms de notes GM correspondent.
- **Atelier Piano** : utilisez la pédale de sustain (CC64) ; *Relâchement* règle la durée après relâchement des touches.
- **Atelier Cordes** : *Attaque* lente (0,3–1 s) pour des nappes, rapide pour des notes plus franches.

### Limites honnêtes (version 0.1.0)
- Ce n'est **pas** un équivalent de Kontakt / Battery / bibliothèques commerciales : pas de lecture depuis le disque (DFD), échantillons chargés en mémoire (≈ 150 Mo pour le piano, ≈ 60 Mo batterie, ≈ 120 Mo cordes en RAM).
- Piano : 4 couches de vélocité sur les 16 de Salamander, notes échantillonnées toutes les tierces mineures, pas de résonance de cordes ni de bruits de marteau/pédale, queues raccourcies (3,5 à 13 s).
- Batterie : une seule perspective de micro (stéréo « mid »), pas de micros séparés kick/snare/overheads/room ; 2 toms réels (les 6 toms GM sont obtenus par transposition) ; pas de sorties multiples.
- Cordes : une seule articulation (sustain vibrato), boucles automatiques (de petits artefacts peuvent s'entendre sur certaines notes), pas de legato ni de pizzicato.
- Pas de format CLAP ni AAX pour l'instant ; plugins non signés (voir Gatekeeper / SmartScreen).
- Guitare acoustique : pas encore incluse (pas de source CC0 de qualité suffisante trouvée pour cette version).

### Compiler soi-même
```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release        # télécharge JUCE 8 automatiquement
cmake --build build --config Release
bash tests/run_render_tests.sh build/atelier_render_artefacts/Release/atelier_render renders
```
Les échantillons sont déjà dans `Instruments/`. Pour les régénérer depuis les sources : `tools/fetch_sources.sh` puis `python3 tools/prepare_samples.py ../ai-src Instruments`.
La CI GitHub Actions compile Windows, macOS (universel) et Linux, lance **pluginval** (VST3/AU), **auval** (macOS) et des rendus audio hors-ligne qui échouent si le son est muet ou sature.

### Licences
- **Code** : **GNU AGPL v3** (fichier `LICENSE`). Le projet utilise le framework **JUCE 8**, distribué sous AGPLv3 ou licence commerciale ; nous avons choisi l'AGPLv3, donc tout le code du projet l'est aussi (une licence MIT n'était pas possible avec JUCE). JUCE a été retenu pour sa prise en charge fiable de VST3 **et** Audio Unit sur Windows/macOS universel.
- **Échantillons** : voir [`ATTRIBUTION.md`](ATTRIBUTION.md) – CC-BY 3.0 (piano, attribution obligatoire) et CC0 (batterie, cordes). Les noms Kontakt, Battery, MT Power Drum Kit, Real Guitar, etc. sont des marques de leurs propriétaires ; ce projet n'utilise **aucun** code, échantillon ni élément de marque de ces produits.
- « VST » est une marque de Steinberg Media Technologies GmbH.

---

## English summary
Atelier Instruments is a free, open-source set of three sample-based instrument plugins (**Atelier Piano**, **Atelier Batterie** – a General-MIDI acoustic drum kit with a per-piece mixer –, and **Atelier Cordes** – a string ensemble) built on a custom SFZ-style engine (velocity layers, round robin, choke groups, ADSR, looping, sustain pedal, reverb). Formats: VST3 for Windows x64, VST3 + AU universal (Intel + Apple Silicon) for macOS; downloads are on the Releases page (Windows installer `.exe` / zip, macOS `.pkg` / zip – unsigned, see the Gatekeeper notes above). Code is **AGPLv3** (because of JUCE); samples are CC-BY 3.0 (Salamander Grand Piano, Alexander Holm) and CC0 (Virtuosity Drums, VSCO 2 CE). It is a first milestone, not a Kontakt-class library: in-memory playback, reduced velocity layers, one mic position for drums, one articulation for strings.

## Agzul s Teqbaylit
Atelier Instruments d allalen n uẓawan ilelliyen, baṭel, s yimesli yemmalen ɣer wid n tidet: **Atelier Piano** (apyanu), **Atelier Batterie** (tabaṭrit s umsefsay i yal aferdis, General MIDI) d **Atelier Cordes** (tiɣenjawin / izlan s yiɣunan). Teddun deg FL Studio d yiseɣzanen-nniḍen VST3 / AU, ɣef Windows d macOS (Intel d Apple Silicon). Sider-iten seg usebter *Releases*, syin ssers-iten deg ukaram VST3 (Windows: `C:\Program Files\Common Files\VST3`), sakin nadi-ten deg *Plugin Manager* n FL Studio. Tangalt s turagt AGPLv3; imesliyen s turagt CC0 d CC-BY 3.0 (wali `ATTRIBUTION.md`).
