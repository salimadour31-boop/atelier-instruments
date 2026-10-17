; Atelier Instruments - Inno Setup script (Windows x64)
#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif
[Setup]
AppId={{6F1C7E52-3A1B-4C8E-9A51-0A7E11E70001}
AppName=Atelier Instruments
AppVersion={#AppVersion}
AppPublisher=Atelier Instruments (open source)
AppPublisherURL=https://github.com/salimadour31-boop/atelier-instruments
DefaultDirName={commoncf64}\VST3
DisableDirPage=yes
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputBaseFilename=AtelierInstruments-{#AppVersion}-Windows-x64-Setup
Compression=lzma2/max
SolidCompression=yes
LicenseFile=..\LICENSE
UninstallDisplayName=Atelier Instruments (VST3)
PrivilegesRequired=admin
WizardStyle=modern

[Languages]
Name: "fr"; MessagesFile: "compiler:Languages\French.isl"
Name: "en"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full"; Description: "Tous les instruments"
Name: "custom"; Description: "Personnalise"; Flags: iscustom

[Components]
Name: "piano"; Description: "Atelier Piano (piano a queue)"; Types: full custom
Name: "batterie"; Description: "Atelier Batterie (batterie acoustique GM)"; Types: full custom
Name: "cordes"; Description: "Atelier Cordes (ensemble de cordes)"; Types: full custom

[Files]
Source: "..\dist\win\Atelier Piano.vst3\*"; DestDir: "{commoncf64}\VST3\Atelier Piano.vst3"; Components: piano; Flags: recursesubdirs ignoreversion
Source: "..\dist\win\Atelier Batterie.vst3\*"; DestDir: "{commoncf64}\VST3\Atelier Batterie.vst3"; Components: batterie; Flags: recursesubdirs ignoreversion
Source: "..\dist\win\Atelier Cordes.vst3\*"; DestDir: "{commoncf64}\VST3\Atelier Cordes.vst3"; Components: cordes; Flags: recursesubdirs ignoreversion

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Atelier Piano.vst3"
Type: filesandordirs; Name: "{commoncf64}\VST3\Atelier Batterie.vst3"
Type: filesandordirs; Name: "{commoncf64}\VST3\Atelier Cordes.vst3"
