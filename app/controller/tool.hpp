#pragma once

enum ActionMap {
    openFile,
    openFolder,
    save,
    saveAs,
    // 标签页相关操作
    newTab,
    closeTab,
    closeCurrentTab,
    closeAllTabs,
    nextTab,
    prevTab,
    switchToTab,
    // 侧边栏相关操作
    toggleSideBar,
    showSideBar,
    hideSideBar,
    // 查看模式相关操作
    setSinglePageMode,
    setContinuousScrollMode,
    setTwoPagesMode,
    setBookViewMode,
    // 页面导航操作
    firstPage,
    previousPage,
    nextPage,
    lastPage,
    goToPage,
    // 缩放操作
    zoomIn,
    zoomOut,
    fitToWidth,
    fitToPage,
    fitToHeight,
    // 旋转操作
    rotateLeft,
    rotateRight,
    // 主题操作
    toggleTheme,
    // 搜索操作
    showSearch,
    hideSearch,
    toggleSearch,
    findNext,
    findPrevious,
    clearSearch,
    // 文档信息操作
    showDocumentMetadata,
    // 设置操作
    showSettings,
    // 最近文件操作
    openRecentFile,
    clearRecentFiles,
    // 从合并分支添加的操作
    saveFile,
    closeFile,
    fullScreen,
    // 文档操作
    exportFile,
    printFile,
    reloadFile,
    // 应用程序操作
    quit,
    showHelp
};
