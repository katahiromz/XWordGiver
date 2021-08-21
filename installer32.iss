; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{07AF81F8-4484-4672-90B4-6262A94E8E1B}
AppName=クロスワード ギバー (32ビット版)
AppVerName=クロスワード ギバー (32ビット版) ver.4.7.4
AppPublisher=片山博文MZ
AppPublisherURL=http://katahiromz.web.fc2.com/
AppSupportURL=http://katahiromz.web.fc2.com/
AppUpdatesURL=http://katahiromz.web.fc2.com/
DefaultDirName={pf}\XWordGiver32
DefaultGroupName=クロスワード ギバー (32ビット版)
OutputDir=.
OutputBaseFilename=xword32-4.7.4-setup
Compression=lzma
SolidCompression=yes
UninstallDisplayIcon={app}\xword32.exe
SetupIconFile=res\Icon_1.ico
LicenseFile=LICENSE.txt
ChangesAssociations=yes

[Languages]
Name: "japanese"; MessagesFile: "compiler:Languages\Japanese.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "xword32.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "ReadMe.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "TechNote.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "HISTORY.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "DICT\基本辞書データ（カナ、謹慎）.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\基本辞書データ（カナ、自由）.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\基本辞書データ（英和、謹慎）.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\基本辞書データ（英和、自由）.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\基本辞書データ（漢字、自由）.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\基本辞書データ（ロシア、自由）.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\★二重マス用四字熟語カナ辞書.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\★二重マス用ことわざカナ辞書.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "BLOCK\circle.emf"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "BLOCK\club.emf"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "BLOCK\diamond.emf"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "BLOCK\heart.emf"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "BLOCK\ink-save-1.bmp"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "BLOCK\ink-save-2.emf"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "BLOCK\mario-block-1.emf"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "BLOCK\mario-block-2.emf"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "BLOCK\mario-block-3.emf"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "BLOCK\perfect-black.bmp"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "BLOCK\sakura.bmp"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "BLOCK\spade.emf"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "BLOCK\star.bmp"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "BLOCK\star.emf"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "xword.py"; DestDir: "{app}"; Flags: ignoreversion
Source: "pat\data.json"; DestDir: "{app}\pat"; Flags: ignoreversion
Source: "dict_analyze\dict_analyze.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "黒マスルールの説明.txt"; DestDir: "{app}"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\クロスワード ギバー (32ビット版)"; Filename: "{app}\xword32.exe"
Name: "{group}\ReadMe.txt"; Filename: "{app}\ReadMe.txt"
Name: "{group}\TechNote.txt"; Filename: "{app}\TechNote.txt"
Name: "{group}\HISTORY.txt"; Filename: "{app}\HISTORY.txt"
Name: "{group}\作者のホームページ"; Filename: "http://katahiromz.web.fc2.com/"
Name: "{group}\{cm:UninstallProgram,クロスワード ギバー}"; Filename: "{uninstallexe}"
Name: "{group}\DICT"; Filename: "{app}\DICT"
Name: "{group}\BLOCK"; Filename: "{app}\BLOCK"
Name: "{group}\LICENSE.txt"; Filename: "{app}\LICENSE.txt"
Name: "{commondesktop}\クロスワード ギバー (32ビット版)"; Filename: "{app}\xword32.exe"; Tasks: desktopicon

[Registry]
Root: HKCU; Subkey: "Software\Katayama Hirofumi MZ\XWord32"; Flags: uninsdeletekey
Root: HKCR; Subkey: ".xwj"; ValueType: string; ValueName: ""; ValueData: "XWordGiver.JsonFile"; Flags: uninsdeletevalue; Tasks: association
Root: HKCR; Subkey: "XWordGiver.JsonFile"; ValueType: string; ValueName: ""; ValueData: "クロスワードJSONデータ"; Flags: uninsdeletekey; Tasks: association
Root: HKCR; Subkey: "XWordGiver.JsonFile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\xword32.exe,3"; Tasks: association
Root: HKCR; Subkey: "XWordGiver.JsonFile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\xword32.exe"" ""%1"""; Tasks: association

[Tasks] 
Name: association; Description: "*.xwjファイルを関連付ける"

[Run]
Filename: "{app}\xword32.exe"; Description: "{cm:LaunchProgram,クロスワード ギバー}"; Flags: nowait postinstall skipifsilent
