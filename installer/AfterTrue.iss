; Inno Setup script for AFTER TRUE. Built by tools/package-windows.ps1
#ifndef AppVersion
  #define AppVersion "0.1.0"
#endif
#ifndef SourceDir
  #define SourceDir "..\dist\AfterTrue"
#endif
#ifndef OutDir
  #define OutDir "..\dist"
#endif

[Setup]
AppId={{6F0B3C5E-2D9A-4B7E-8C41-AF7E7A0C2B11}
AppName=AFTER TRUE
AppVersion={#AppVersion}
AppPublisher=AFTER TRUE
DefaultDirName={autopf}\AFTER TRUE
DefaultGroupName=AFTER TRUE
DisableProgramGroupPage=yes
OutputDir={#OutDir}
OutputBaseFilename=AFTER-TRUE-Installer-{#AppVersion}
SetupIconFile=..\resources\icon.ico
UninstallDisplayIcon={app}\AfterTrue.exe
Compression=lzma2
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequiredOverridesAllowed=dialog
ChangesAssociations=yes

[Tasks]
Name: "desktopicon"; Description: "Create a &desktop shortcut"; GroupDescription: "Shortcuts:"
Name: "assoc"; Description: "Open .atrue project files with AFTER TRUE"; GroupDescription: "File types:"

[Files]
Source: "{#SourceDir}\*"; DestDir: "{app}"; Flags: recursesubdirs createallsubdirs ignoreversion

[Icons]
Name: "{autoprograms}\AFTER TRUE"; Filename: "{app}\AfterTrue.exe"
Name: "{autodesktop}\AFTER TRUE"; Filename: "{app}\AfterTrue.exe"; Tasks: desktopicon

[Registry]
Root: HKA; Subkey: "Software\Classes\.atrue"; ValueType: string; ValueData: "AfterTrue.Project"; Flags: uninsdeletevalue; Tasks: assoc
Root: HKA; Subkey: "Software\Classes\AfterTrue.Project"; ValueType: string; ValueData: "AFTER TRUE Project"; Flags: uninsdeletekey; Tasks: assoc
Root: HKA; Subkey: "Software\Classes\AfterTrue.Project\DefaultIcon"; ValueType: string; ValueData: "{app}\AfterTrue.exe,0"; Tasks: assoc
Root: HKA; Subkey: "Software\Classes\AfterTrue.Project\shell\open\command"; ValueType: string; ValueData: """{app}\AfterTrue.exe"" ""%1"""; Tasks: assoc

[Run]
Filename: "{app}\AfterTrue.exe"; Description: "Launch AFTER TRUE"; Flags: nowait postinstall skipifsilent
