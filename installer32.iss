; Script generated by the Inno Setup Script Wizard.
; SEE THE DOCUMENTATION FOR DETAILS ON CREATING INNO SETUP SCRIPT FILES!

#define APP_VERSION "5.2.9"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{07AF81F8-4484-4672-90B4-6262A94E8E1B}
AppName={cm:AppNameBits}
AppVersion={#APP_VERSION}
AppVerName={cm:AppNameBits} ver.{#APP_VERSION}
VersionInfoVersion={#APP_VERSION}
AppPublisher={cm:Author}
AppPublisherURL=https://katahiromz.fc2.page/
AppSupportURL=https://katahiromz.fc2.page/
AppUpdatesURL=https://katahiromz.fc2.page/
DefaultDirName={pf}\XWordGiver32
DefaultGroupName={cm:AppNameBits}
OutputDir=.
OutputBaseFilename=XWordGiver-{#APP_VERSION}-x86-setup
Compression=lzma
SolidCompression=yes
UninstallDisplayIcon={app}\XWordGiver32.exe
SetupIconFile=res\Icon_1.ico
LicenseFile=LICENSE.txt
ChangesAssociations=yes

[Languages]
Name: "en"; MessagesFile: "compiler:Default.isl"
Name: "ja"; MessagesFile: "compiler:Languages\Japanese.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "XWordGiver32.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "ReadMe-ENG.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "ReadMe-JPN.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "TechNote.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "HISTORY.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "DICT\BasicDict-ENG-Freedom.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\BasicDict-ENG-Modesty.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\BasicDict-ENG2JPN-Modesty.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\BasicDict-ENG2JPN-Freedom.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\BasicDict-FRA-Freedom.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\BasicDict-JPN-Kana-Freedom.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\BasicDict-JPN-Kana-Modesty.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\BasicDict-JPN-Kanji-Freedom.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\BasicDict-RUS-Freedom.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\SubDict-JPN-Kana-Kotowaza.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\SubDict-JPN-Kana-4KanjiWords.tsv"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\tags-ENG.xls"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "DICT\tags-JPN.xls"; DestDir: "{app}\DICT"; Flags: ignoreversion
Source: "BLOCK\club.emf"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "BLOCK\diamond.emf"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "BLOCK\heart.emf"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "BLOCK\ink-save.gif"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "BLOCK\mario-block-1.emf"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "BLOCK\mario-block-2.emf"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "BLOCK\mario-block-3.emf"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "BLOCK\perfect-black.bmp"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "BLOCK\sakura.jpg"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "BLOCK\spade.emf"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "BLOCK\star.emf"; DestDir: "{app}\BLOCK"; Flags: ignoreversion
Source: "SOUND\Success.wav"; DestDir: "{app}\SOUND"; Flags: ignoreversion
Source: "SOUND\Canceled.wav"; DestDir: "{app}\SOUND"; Flags: ignoreversion
Source: "SOUND\Failed.wav"; DestDir: "{app}\SOUND"; Flags: ignoreversion
Source: "LICENSE.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "PAT.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "POLICY.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "THEME.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "dict_analyze\dict_analyze.exe"; DestDir: "{app}"; Flags: ignoreversion
Source: "Policy-ENG.txt"; DestDir: "{app}"; Flags: ignoreversion
Source: "Policy-JPN.txt"; DestDir: "{app}"; Flags: ignoreversion
; NOTE: Don't use "Flags: ignoreversion" on any shared system files

[Icons]
Name: "{group}\{cm:AppNameBits}"; Filename: "{app}\XWordGiver32.exe"
Name: "{group}\ReadMe-ENG.txt"; Filename: "{app}\ReadMe-ENG.txt"
Name: "{group}\ReadMe-JPN.txt"; Filename: "{app}\ReadMe-JPN.txt"
Name: "{group}\TechNote.txt"; Filename: "{app}\TechNote.txt"
Name: "{group}\HISTORY.txt"; Filename: "{app}\HISTORY.txt"
Name: "{group}\{cm:AuthorsHomepage}"; Filename: "http://katahiromz.web.fc2.com/"
Name: "{group}\{cm:UninstallProgram,{cm:AppName}}"; Filename: "{uninstallexe}"
Name: "{group}\DICT"; Filename: "{app}\DICT"
Name: "{group}\BLOCK"; Filename: "{app}\BLOCK"
Name: "{group}\LICENSE.txt"; Filename: "{app}\LICENSE.txt"
Name: "{commondesktop}\{cm:AppNameBits}"; Filename: "{app}\XWordGiver32.exe"; Tasks: desktopicon

[Registry]
Root: HKCU; Subkey: "Software\Katayama Hirofumi MZ\XWord32"; Flags: uninsdeletekey
Root: HKCR; Subkey: ".xwj"; ValueType: string; ValueName: ""; ValueData: "XWordGiver.XwjFile"; Flags: uninsdeletevalue; Tasks: association_xwj
Root: HKCR; Subkey: "XWordGiver.XwjFile"; ValueType: string; ValueName: ""; ValueData: "{cm:XwjDataFile}"; Flags: uninsdeletekey; Tasks: association_xwj
Root: HKCR; Subkey: "XWordGiver.XwjFile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\XWordGiver32.exe,3"; Tasks: association_xwj
Root: HKCR; Subkey: "XWordGiver.XwjFile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\XWordGiver32.exe"" ""%1"""; Tasks: association_xwj
Root: HKCR; Subkey: ".xd"; ValueType: string; ValueName: ""; ValueData: "XWordGiver.XdFile"; Flags: uninsdeletevalue; Tasks: association_xd
Root: HKCR; Subkey: "XWordGiver.XdFile"; ValueType: string; ValueName: ""; ValueData: "{cm:XdDataFile}"; Flags: uninsdeletekey; Tasks: association_xd
Root: HKCR; Subkey: "XWordGiver.XdFile\DefaultIcon"; ValueType: string; ValueName: ""; ValueData: "{app}\XWordGiver32.exe,3"; Tasks: association_xd
Root: HKCR; Subkey: "XWordGiver.XdFile\shell\open\command"; ValueType: string; ValueName: ""; ValueData: """{app}\XWordGiver32.exe"" ""%1"""; Tasks: association_xd

[Tasks] 
Name: association_xwj; Description: "{cm:AssociateXWJFiles}"
Name: association_xd; Description: "{cm:AssociateXdFiles}"

[CustomMessages]
AppName=XWordGiver
AppNameBits=XWordGiver (x86)
Author=Katayama Hirofumi MZ
AssociateXWJFiles=Associate XWJ files
AssociateXdFiles=Associate XD files
XwjDataFile=XWordGiver XWJ file
XdDataFile=XWordGiver Xd file
AuthorsHomepage=Author's homepage
ja.AppName=クロスワード ギバー
ja.AppNameBits=クロスワード ギバー (32ビット)
ja.Author=片山博文MZ
ja.AssociateXWJFiles=XWJファイルを関連付ける
ja.AssociateXdFiles=XDファイルを関連付ける
ja.XwjDataFile=XWordGiver XWJ ファイル
ja.XdDataFile=XWordGiver XD ファイル
ja.AuthorsHomepage=作者のホームページ

[Run]
Filename: "{app}\XWordGiver32.exe"; Description: "{cm:LaunchProgram,{cm:AppName}}"; Flags: nowait postinstall skipifsilent
