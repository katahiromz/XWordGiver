; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{CDECA641-CFBB-43E0-A075-13802703E7EC}
AppName={cm:AppNameBits}
AppVerName={cm:AppNameBits} ver.4.7.8
AppPublisher={cm:Author}
AppPublisherURL=http://katahiromz.web.fc2.com/
AppSupportURL=http://katahiromz.web.fc2.com/
AppUpdatesURL=http://katahiromz.web.fc2.com/
DefaultDirName={pf}\XWordGiver64
DefaultGroupName={cm:AppNameBits}
OutputDir=.
OutputBaseFilename=XWordGiver-x64-4.7.8-setup
Compression=lzma
SolidCompression=yes
UninstallDisplayIcon={app}\XWordGiver64.exe
SetupIconFile=res\Icon_1.ico
LicenseFile=LICENSE.txt
ChangesAssociations=yes
ArchitecturesInstallIn64BitMode=x64

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "ja"; MessagesFile: "compiler:Languages\Japanese.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "XWordGiver64.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "ReadMe-ENG.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "ReadMe-JPN.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "TechNote.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "HISTORY.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "DICT\BasicDict-ENG-Freedom.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\BasicDict-ENG2JPN-Modesty.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\BasicDict-ENG2JPN-Freedom.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\BasicDict-JPN-Kana-Freedom.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\BasicDict-JPN-Kana-Modesty.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\BasicDict-JPN-Kanji-Freedom.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\BasicDict-JPN-Kotowaza.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\BasicDict-JPN-Kana-4KanjiWords.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\BasicDict-RUS-Freedom.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
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
Source: "Rules-ENG.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "Rules-JPN.txt"; DestDir: "{app}"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\{cm:AppNameBits}"; Filename: "{app}\XWordGiver64.exe"
Name: "{group}\ReadMe-ENG.txt"; Filename: "{app}\ReadMe-ENG.txt"
Name: "{group}\ReadMe-JPN.txt"; Filename: "{app}\ReadMe-JPN.txt"
Name: "{group}\TechNote.txt"; Filename: "{app}\TechNote.txt"
Name: "{group}\HISTORY.txt"; Filename: "{app}\HISTORY.txt"
Name: "{group}\{cm:AuthorsHomepage}"; Filename: "http://katahiromz.web.fc2.com/"
Name: "{group}\{cm:UninstallProgram,{cm:AppName}}"; Filename: "{uninstallexe}"
Name: "{group}\DICT"; Filename: "{app}\DICT"
Name: "{group}\BLOCK"; Filename: "{app}\BLOCK"
Name: "{group}\LICENSE.txt"; Filename: "{app}\LICENSE.txt"
Name: "{commondesktop}\{cm:AppNameBits}"; Filename: "{app}\XWordGiver64.exe"; Tasks: desktopicon

[Registry]
Root: HKCU; Subkey: "Software\Katayama Hirofumi MZ\XWord64"; Flags: uninsdeletekey
Root: HKCR; Subkey: ".xwj"; ValueType: string; ValueName: ""; ValueData: "XWordGiver.JsonFile"; Flags: uninsdeletevalue; Tasks: association
Root: HKCR; Subkey: "XWordGiver.JsonFile"; ValueType: string; ValueName: ""; ValueData: "{cm:CrosswordJSONData}"; Flags: uninsdeletekey; Tasks: association
Root: HKCR; Subkey: "XWordGiver.JsonFile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\XWordGiver64.exe,3"; Tasks: association
Root: HKCR; Subkey: "XWordGiver.JsonFile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\XWordGiver64.exe"" ""%1"""; Tasks: association

[Tasks] 
Name: association; Description: "{cm:AssociateXWJFiles}"

[CustomMessages]
AppName=XWordGiver
AppNameBits=XWordGiver (x64)
Author=Katayama Hirofumi MZ
AssociateXWJFiles=Associate *.xwj files
CrosswordJSONData=Crossword JSON data
AuthorsHomepage=Author's homepage
ja.AppName=クロスワード ギバー
ja.AppNameBits=クロスワード ギバー (64ビット)
ja.Author=片山博文MZ
ja.AssociateXWJFiles=*.xwjファイルを関連付ける
ja.CrosswordJSONData=クロスワード JSON データ
ja.AuthorsHomepage=作者のホームページ

[Run]
Filename: "{app}\XWordGiver64.exe"; Description: "{cm:LaunchProgram,{cm:AppName}}"; Flags: nowait postinstall skipifsilent
