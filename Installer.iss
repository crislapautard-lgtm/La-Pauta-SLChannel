#define AppName "La Pauta SL Channel"
#define AppVersion "1.0.0"
#define Artefacts "build\LaPautaSLChannel_artefacts\Release"

[Setup]
AppId={{7C3E5A21-9B4D-4F6A-8E12-5D0A3B9C41F7}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=La Pauta
DefaultDirName={autopf}\La Pauta\SL Channel
DefaultGroupName=La Pauta
DisableProgramGroupPage=yes
OutputDir=installer_output
OutputBaseFilename=LaPautaSLChannel-Setup-{#AppVersion}
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
PrivilegesRequired=admin
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
UninstallDisplayName={#AppName}

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"

[Types]
Name: "full"; Description: "Instalacion completa (VST3 + Standalone)"
Name: "custom"; Description: "Personalizada"; Flags: iscustom

[Components]
Name: "vst3"; Description: "Plugin VST3"; Types: full custom
Name: "standalone"; Description: "Aplicacion Standalone"; Types: full custom

[Files]
Source: "{#Artefacts}\VST3\{#AppName}.vst3\*"; DestDir: "{commoncf64}\VST3\{#AppName}.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#Artefacts}\Standalone\{#AppName}.exe"; DestDir: "{app}"; Components: standalone; Flags: ignoreversion

[Icons]
Name: "{group}\{#AppName}"; Filename: "{app}\{#AppName}.exe"; Components: standalone
Name: "{group}\Desinstalar {#AppName}"; Filename: "{uninstallexe}"
