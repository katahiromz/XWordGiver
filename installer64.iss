; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{07AF81F8-4484-4672-90B4-6262A94E8E1C}
AppName=クロスワード ギバー (64ビット版)
AppVerName=クロスワード ギバー (64ビット版) ver.4.2
AppPublisher=片山博文MZ
AppPublisherURL=http://katahiromz.web.fc2.com/
AppSupportURL=http://katahiromz.web.fc2.com/
AppUpdatesURL=http://katahiromz.web.fc2.com/
DefaultDirName={pf64}\XWordGiver64
DefaultGroupName=クロスワード ギバー (64ビット版)
OutputDir=.
OutputBaseFilename=xword64-4.2-setup
Compression=lzma
SolidCompression=yes
ArchitecturesAllowed=x64
UninstallDisplayIcon={app}\xword64.exe
SetupIconFile=res\Icon_1.ico
LicenseFile=LICENSE.txt

[Languages]
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "xword64.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "ReadMe.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "TechNote.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "HISTORY.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "DICT\基本辞書データ（カナ、謹慎）.dic"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\基本辞書データ（カナ、自由）.dic"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\基本辞書データ（英和、謹慎）.dic"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\基本辞書データ（英和、自由）.dic"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\基本辞書データ（漢字、自由）.dic"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\基本辞書データ（ロシア、自由）.dic"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "点対称パターン.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\クロスワード ギバー (64ビット版)"; Filename: "{app}\xword64.exe"
Name: "{group}\ReadMe.txt"; Filename: "{app}\ReadMe.txt"
Name: "{group}\TechNote.txt"; Filename: "{app}\TechNote.txt"
Name: "{group}\HISTORY.txt"; Filename: "{app}\HISTORY.txt"
Name: "{group}\作者のホームページ"; Filename: "http://katahiromz.web.fc2.com/"
Name: "{group}\{cm:UninstallProgram,クロスワード ギバー}"; Filename: "{uninstallexe}"
Name: "{group}\点対称パターン.txt"; Filename: "{app}\点対称パターン.txt"
Name: "{group}\DICT"; Filename: "{app}\DICT"
Name: "{group}\LICENSE.txt"; Filename: "{app}\LICENSE.txt"
Name: "{commondesktop}\クロスワード ギバー (64ビット版)"; Filename: "{app}\xword64.exe"; Tasks: desktopicon

[Run]
Filename: "{app}\xword64.exe"; Description: "{cm:LaunchProgram,クロスワード ギバー}"; Flags: nowait postinstall skipifsilent

