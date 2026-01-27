; NSIS Installer Script for sast-readium
; USE MODERN UI
!include "MUI2.nsh"

; 允许通过命令行 -D 参数覆盖
!ifndef RELEASE_BUILD_DIR
  !define RELEASE_BUILD_DIR "..\build\Release\app"
!endif

!ifndef OUT_DIR
  !define OUT_DIR "."
!endif

!searchparse /file "${RELEASE_BUILD_DIR}\config.h" `#define PROJECT_NAME "` PRODUCT_NAME `"`
!searchparse /file "${RELEASE_BUILD_DIR}\config.h" `#define APP_NAME "` PRODUCT_NAME_FULL `"`
!searchparse /file "${RELEASE_BUILD_DIR}\config.h" `#define PROJECT_VER "` PRODUCT_VERSION `"`

Name "${PRODUCT_NAME_FULL} ${PRODUCT_VERSION}"
OutFile "${OUT_DIR}\${PRODUCT_NAME}_Installer_${PRODUCT_VERSION}.exe"

; 默认安装目录
InstallDir "$PROGRAMFILES\${PRODUCT_NAME}"
InstallDirRegKey HKLM "Software\${PRODUCT_NAME}" "InstallDir"

; 请求管理员权限
RequestExecutionLevel admin

!define MUI_ICON "..\assets\images\icon.ico"
!define MUI_UNICON "..\assets\images\icon.ico"

; INSTALL PAGES
!insertmacro MUI_PAGE_LICENSE "LICENSE"
!insertmacro MUI_PAGE_COMPONENTS
!insertmacro MUI_PAGE_DIRECTORY
!insertmacro MUI_PAGE_INSTFILES
!insertmacro MUI_PAGE_FINISH

!insertmacro MUI_UNPAGE_CONFIRM
!insertmacro MUI_UNPAGE_INSTFILES

; LANG
!insertmacro MUI_LANGUAGE "SimpChinese"

; INSTALL SECTIONS

Section "${PRODUCT_NAME_FULL}"

SetOutPath $INSTDIR
File "${RELEASE_BUILD_DIR}\${PRODUCT_NAME}.exe"
File "${RELEASE_BUILD_DIR}\*.dll"
File "${RELEASE_BUILD_DIR}\*.qm"

SetOutPath $INSTDIR\iconengines
File /nonfatal /a /r "${RELEASE_BUILD_DIR}\iconengines\"

SetOutPath $INSTDIR\imageformats
File /nonfatal /a /r "${RELEASE_BUILD_DIR}\imageformats\"

SetOutPath $INSTDIR\platforms
File /nonfatal /a /r "${RELEASE_BUILD_DIR}\platforms\"

;SetOutPath $INSTDIR\plugins
;File /nonfatal /a /r "${RELEASE_BUILD_DIR}\plugins\"

SetOutPath $INSTDIR\styles
File /nonfatal /a /r "${RELEASE_BUILD_DIR}\styles\"


;SetOutPath $INSTDIR\configs
;File /nonfatal /a /r "..\configs\"

SetOutPath $INSTDIR
CreateShortCut "$DESKTOP\${PRODUCT_NAME_FULL}.lnk" $INSTDIR\${PRODUCT_NAME}.exe

; 写入卸载程序
WriteUninstaller "$INSTDIR\Uninstall.exe"

; 写入注册表（用于控制面板卸载）
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "DisplayName" "${PRODUCT_NAME_FULL}"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "UninstallString" "$INSTDIR\Uninstall.exe"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "DisplayIcon" "$INSTDIR\${PRODUCT_NAME}.exe"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "DisplayVersion" "${PRODUCT_VERSION}"
WriteRegStr HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}" "Publisher" "NJUPT-SAST"
WriteRegStr HKLM "Software\${PRODUCT_NAME}" "InstallDir" "$INSTDIR"

SectionEnd

; 卸载 Section
Section "Uninstall"

; 删除快捷方式
Delete "$DESKTOP\${PRODUCT_NAME_FULL}.lnk"

; 删除安装的文件
RMDir /r "$INSTDIR\iconengines"
RMDir /r "$INSTDIR\imageformats"
RMDir /r "$INSTDIR\platforms"
RMDir /r "$INSTDIR\styles"
Delete "$INSTDIR\*.exe"
Delete "$INSTDIR\*.dll"
Delete "$INSTDIR\*.qm"
Delete "$INSTDIR\Uninstall.exe"
RMDir "$INSTDIR"

; 删除注册表项
DeleteRegKey HKLM "Software\Microsoft\Windows\CurrentVersion\Uninstall\${PRODUCT_NAME}"
DeleteRegKey HKLM "Software\${PRODUCT_NAME}"

SectionEnd
