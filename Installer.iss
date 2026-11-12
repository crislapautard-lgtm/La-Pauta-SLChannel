; Instalador de Windows para La Pauta SL Channel (Inno Setup 6)
; Compilar con: ISCC.exe installer.iss  (o con build_installer.bat)

#define AppName "La Pauta SL Channel"
#define AppVersion "1.0.0"
#define Artefacts "build\LaPautaSLChannel_artefacts\Release"

[Setup]
AppId={{B3F1C2A4-5D7E-4A8B-9C10-LAPAUTASL0001}
AppName={#AppName}
AppVersion={#AppVersion}
AppPublisher=La Pauta
DefaultDirName={autopf}\La Pauta\SL Channel
DefaultGroupName=La Pauta
OutputDir=installer_output
OutputBaseFilename=LaPautaSLChannel-Setup-{#AppVersion}
