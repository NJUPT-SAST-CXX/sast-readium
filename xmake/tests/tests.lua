-- xmake/tests/tests.lua
-- Comprehensive Test Support split from root

includes("../modules/qt.lua")
includes("../modules/dependencies.lua")

-- Include test runner
includes("test_runner.lua")

if has_config("enable_tests") then

    target("TestUtilities")
        set_kind("static")
        add_files("$(projectdir)/tests/TestUtilities.cpp")
        add_headerfiles("$(projectdir)/tests/TestUtilities.h")
        add_packages("pkgconfig::poppler-qt6", "spdlog")

        local qt_base = find_qt_installation()
        if qt_base then
            if is_plat("windows") then
                add_includedirs(qt_base .. "/include/qt6")
                add_includedirs(qt_base .. "/include/qt6/QtCore")
                add_includedirs(qt_base .. "/include/qt6/QtGui")
                add_includedirs(qt_base .. "/include/qt6/QtWidgets")
                add_includedirs(qt_base .. "/include/qt6/QtTest")
                add_linkdirs(qt_base .. "/lib")
                add_links("Qt6Core", "Qt6Gui", "Qt6Widgets", "Qt6Test")
            elseif is_plat("linux") then
                if os.isdir(qt_base .. "/include/qt6") then
                    add_includedirs(qt_base .. "/include/qt6")
                    add_includedirs(qt_base .. "/include/qt6/QtCore")
                    add_includedirs(qt_base .. "/include/qt6/QtGui")
                    add_includedirs(qt_base .. "/include/qt6/QtWidgets")
                    add_includedirs(qt_base .. "/include/qt6/QtTest")
                else
                    add_includedirs(qt_base .. "/include/QtCore")
                    add_includedirs(qt_base .. "/include/QtGui")
                    add_includedirs(qt_base .. "/include/QtWidgets")
                    add_includedirs(qt_base .. "/include/QtTest")
                end
                add_linkdirs(qt_base .. "/lib")
                add_links("Qt6Core", "Qt6Gui", "Qt6Widgets", "Qt6Test")
            elseif is_plat("macosx") then
                if os.isdir(qt_base .. "/include/qt6") then
                    add_includedirs(qt_base .. "/include/qt6")
                    add_includedirs(qt_base .. "/include/qt6/QtCore")
                    add_includedirs(qt_base .. "/include/qt6/QtGui")
                    add_includedirs(qt_base .. "/include/qt6/QtWidgets")
                    add_includedirs(qt_base .. "/include/qt6/QtTest")
                else
                    add_includedirs(qt_base .. "/include/QtCore")
                    add_includedirs(qt_base .. "/include/QtGui")
                    add_includedirs(qt_base .. "/include/QtWidgets")
                    add_includedirs(qt_base .. "/include/QtTest")
                end
                add_linkdirs(qt_base .. "/lib")
                add_links("Qt6Core", "Qt6Gui", "Qt6Widgets", "Qt6Test")
            end
            add_defines("QT_CORE_LIB", "QT_GUI_LIB", "QT_WIDGETS_LIB", "QT_TEST_LIB")
        end

        add_includedirs("$(projectdir)", "$(projectdir)/app", "$(projectdir)/tests")
    target_end()

    function create_test_target(name, sources, timeout, opts)
        target(name)
            set_kind("binary")
            set_default(false)
            add_deps("TestUtilities")
            add_files(sources)
            add_packages("pkgconfig::poppler-qt6", "spdlog")

            local qt_base = find_qt_installation()
            if qt_base then
                if is_plat("windows") then
                    add_includedirs(qt_base .. "/include/qt6")
                    add_includedirs(qt_base .. "/include/qt6/QtCore")
                    add_includedirs(qt_base .. "/include/qt6/QtGui")
                    add_includedirs(qt_base .. "/include/qt6/QtWidgets")
                    add_includedirs(qt_base .. "/include/qt6/QtTest")
                    add_linkdirs(qt_base .. "/lib")
                    add_links("Qt6Core", "Qt6Gui", "Qt6Widgets", "Qt6Test")
                elseif is_plat("linux") then
                    if os.isdir(qt_base .. "/include/qt6") then
                        add_includedirs(qt_base .. "/include/qt6")
                        add_includedirs(qt_base .. "/include/qt6/QtCore")
                        add_includedirs(qt_base .. "/include/qt6/QtGui")
                        add_includedirs(qt_base .. "/include/qt6/QtWidgets")
                        add_includedirs(qt_base .. "/include/qt6/QtTest")
                    else
                        add_includedirs(qt_base .. "/include/QtCore")
                        add_includedirs(qt_base .. "/include/QtGui")
                        add_includedirs(qt_base .. "/include/QtWidgets")
                        add_includedirs(qt_base .. "/include/QtTest")
                    end
                    add_linkdirs(qt_base .. "/lib")
                    add_links("Qt6Core", "Qt6Gui", "Qt6Widgets", "Qt6Test")
                elseif is_plat("macosx") then
                    if os.isdir(qt_base .. "/include/qt6") then
                        add_includedirs(qt_base .. "/include/qt6")
                        add_includedirs(qt_base .. "/include/qt6/QtCore")
                        add_includedirs(qt_base .. "/include/qt6/QtGui")
                        add_includedirs(qt_base .. "/include/qt6/QtWidgets")
                        add_includedirs(qt_base .. "/include/qt6/QtTest")
                    else
                        add_includedirs(qt_base .. "/include/QtCore")
                        add_includedirs(qt_base .. "/include/QtGui")
                        add_includedirs(qt_base .. "/include/QtWidgets")
                        add_includedirs(qt_base .. "/include/QtTest")
                    end
                    add_linkdirs(qt_base .. "/lib")
                    add_links("Qt6Core", "Qt6Gui", "Qt6Widgets", "Qt6Test")
                end
                add_defines("QT_CORE_LIB", "QT_GUI_LIB", "QT_WIDGETS_LIB", "QT_TEST_LIB")
            end

            if has_config("enable_qgraphics_pdf") then
                add_defines("ENABLE_QGRAPHICS_PDF_SUPPORT")
            end

            add_includedirs("$(projectdir)", "$(projectdir)/app", "$(projectdir)/tests")
            add_deps("app_lib")

            if is_plat("linux") then
                add_syslinks("pthread", "dl")
            elseif is_plat("macosx") then
                add_frameworks("CoreFoundation", "CoreServices")
            end

            -- Test executable size optimizations
            if is_mode("debug") then
                -- Use minimal debug info for tests to reduce executable size (50-70% reduction)
                if has_config("split_debug_info") then
                    add_cxxflags("-g1", {tools = {"gcc", "clang"}})
                end
            elseif is_mode("release") then
                -- Strip test executables in release mode
                if has_config("strip_binaries") then
                    add_ldflags("-s", {tools = {"gcc", "clang"}})
                end
            end

            -- Reduce GNU ld memory footprint when linking large test binaries
            if is_plat("windows") then
                add_ldflags("-Wl,--no-keep-memory", "-Wl,--reduce-memory-overheads", {tools = {"gcc"}})
            end

            if timeout then
                add_defines("TEST_TIMEOUT=" .. timeout)
            end

            if opts and opts.packages then
                add_packages(table.unpack(opts.packages))
            end
        target_end()
    end

    -- Create test targets (same as original list)
    create_test_target("test_smoke", "$(projectdir)/tests/test_smoke.cpp", 30)

    create_test_target("test_cache_manager", "$(projectdir)/tests/cache/test_cache_manager.cpp $(projectdir)/tests/cache/CacheTestHelpers.cpp", 300)
    create_test_target("test_pdf_cache_manager", "$(projectdir)/tests/cache/test_pdf_cache_manager.cpp $(projectdir)/tests/cache/CacheTestHelpers.cpp", 300)
    create_test_target("test_page_text_cache", "$(projectdir)/tests/cache/test_page_text_cache.cpp $(projectdir)/tests/cache/CacheTestHelpers.cpp", 300)
    create_test_target("test_search_result_cache", "$(projectdir)/tests/cache/test_search_result_cache.cpp $(projectdir)/tests/cache/CacheTestHelpers.cpp", 300)

    create_test_target("test_command_manager", "$(projectdir)/tests/command/test_command_manager.cpp", 300)
    create_test_target("test_document_commands", "$(projectdir)/tests/command/test_document_commands.cpp", 300)
    create_test_target("test_navigation_commands", "$(projectdir)/tests/command/test_navigation_commands.cpp", 300)
    create_test_target("test_initialization_command", "$(projectdir)/tests/command/test_initialization_command.cpp", 300)

    create_test_target("test_service_locator", "$(projectdir)/tests/controller/test_service_locator.cpp $(projectdir)/tests/controller/ControllerTestMocks.cpp", 300)
    create_test_target("test_state_manager", "$(projectdir)/tests/controller/test_state_manager.cpp $(projectdir)/tests/controller/ControllerTestMocks.cpp", 300)
    create_test_target("test_state_manager_comprehensive", "$(projectdir)/tests/controller/test_state_manager_comprehensive.cpp $(projectdir)/tests/controller/ControllerTestMocks.cpp", 300)
    create_test_target("test_event_bus", "$(projectdir)/tests/controller/test_event_bus.cpp $(projectdir)/tests/controller/ControllerTestMocks.cpp", 300)
    create_test_target("test_application_controller", "$(projectdir)/tests/controller/test_application_controller.cpp $(projectdir)/tests/controller/ControllerTestMocks.cpp", 300)
    create_test_target("test_document_controller", "$(projectdir)/tests/controller/test_document_controller.cpp", 300)
    create_test_target("test_page_controller", "$(projectdir)/tests/controller/test_page_controller.cpp", 300)
    create_test_target("test_configuration_manager", "$(projectdir)/tests/controller/test_configuration_manager.cpp $(projectdir)/tests/controller/ControllerTestMocks.cpp", 300)
    create_test_target("test_action_map", "$(projectdir)/tests/controller/test_action_map.cpp $(projectdir)/tests/controller/ControllerTestMocks.cpp", 300)

    create_test_target("test_model_factory", "$(projectdir)/tests/factory/test_model_factory.cpp", 300)
    create_test_target("test_command_prototype_registry", "$(projectdir)/tests/factory/test_command_prototype_registry.cpp", 300)

    create_test_target("test_application_startup", "$(projectdir)/tests/integration/test_application_startup.cpp", 900)
    create_test_target("test_search_integration", "$(projectdir)/tests/integration/test_search_integration.cpp", 600)
    create_test_target("test_rendering_mode_switch", "$(projectdir)/tests/integration/test_rendering_mode_switch.cpp", 600)
    create_test_target("test_qgraphics_pdf", "$(projectdir)/tests/integration/test_qgraphics_pdf.cpp", 300)

    create_test_target("test_logging_comprehensive", "$(projectdir)/tests/logging/test_logging_comprehensive.cpp", 300)

    if has_package("gtest") then
    create_test_target("test_windows_path", "$(projectdir)/tests/managers/test_windows_path.cpp", 300, {packages = {"gtest"}})
    elseif not has_config("quiet") then
        cprint("${yellow}Google Test not found, skipping test_windows_path${clear}")
    end

    create_test_target("test_search_model", "$(projectdir)/tests/model/test_search_model.cpp", 300)

    create_test_target("test_rendering_performance", "$(projectdir)/tests/performance/test_rendering_performance.cpp", 600)
    if has_config("enable_qgraphics_pdf") then
    create_test_target("test_qgraphics_rendering_performance", "$(projectdir)/tests/performance/test_qgraphics_rendering_performance.cpp", 600)
    elseif not has_config("quiet") then
        cprint("${dim}QGraphics PDF support disabled, skipping test_qgraphics_rendering_performance${clear}")
    end
    create_test_target("test_search_optimizations", "$(projectdir)/tests/performance/test_search_optimizations.cpp", 600)

    create_test_target("test_real_pdf_documents", "$(projectdir)/tests/real_world/test_real_pdf_documents.cpp", 900)

    create_test_target("test_search_engine", "$(projectdir)/tests/search/test_search_engine.cpp", 300)
    create_test_target("test_search_configuration", "$(projectdir)/tests/search/test_search_configuration.cpp", 300)
    create_test_target("test_search_executor", "$(projectdir)/tests/search/test_search_executor.cpp", 300)
    create_test_target("test_search_features", "$(projectdir)/tests/search/test_search_features.cpp", 300)
    create_test_target("test_search_metrics", "$(projectdir)/tests/search/test_search_metrics.cpp", 300)
    create_test_target("test_search_performance", "$(projectdir)/tests/search/test_search_performance.cpp", 300)
    create_test_target("test_search_validator", "$(projectdir)/tests/search/test_search_validator.cpp", 300)
    create_test_target("test_text_extractor", "$(projectdir)/tests/search/test_text_extractor.cpp", 300)
    create_test_target("test_background_processor", "$(projectdir)/tests/search/test_background_processor.cpp", 300)
    create_test_target("test_incremental_search_manager", "$(projectdir)/tests/search/test_incremental_search_manager.cpp", 300)
    create_test_target("test_memory_aware_search_results", "$(projectdir)/tests/search/test_memory_aware_search_results.cpp", 300)
    create_test_target("test_memory_manager", "$(projectdir)/tests/search/test_memory_manager.cpp", 300)
    create_test_target("test_memory_manager_stubs", "$(projectdir)/tests/search/test_memory_manager_stubs.cpp", 300)
    create_test_target("test_multi_search_engine", "$(projectdir)/tests/search/test_multi_search_engine.cpp", 300)
    create_test_target("test_smart_eviction_policy", "$(projectdir)/tests/search/test_smart_eviction_policy.cpp", 300)
    create_test_target("test_search_error_recovery", "$(projectdir)/tests/search/test_search_error_recovery.cpp", 300)
    create_test_target("test_search_thread_safety", "$(projectdir)/tests/search/test_search_thread_safety.cpp", 600)

    create_test_target("test_debug_log_panel_integration", "$(projectdir)/tests/ui/test_debug_log_panel_integration.cpp", 600)
    create_test_target("test_tool_bar_integration", "$(projectdir)/tests/ui/test_tool_bar_integration.cpp", 300)
    create_test_target("test_thumbnail_generator_integration", "$(projectdir)/tests/ui/test_thumbnail_generator_integration.cpp", 600)
    create_test_target("test_document_comparison_integration", "$(projectdir)/tests/ui/test_document_comparison_integration.cpp", 600)
    create_test_target("test_document_metadata_dialog_integration", "$(projectdir)/tests/ui/test_document_metadata_dialog_integration.cpp", 300)
    create_test_target("test_menu_bar_integration", "$(projectdir)/tests/ui/test_menu_bar_integration.cpp", 300)
    create_test_target("test_pdf_animations_integration", "$(projectdir)/tests/ui/test_pdf_animations_integration.cpp", 300)
    create_test_target("test_pdf_outline_widget_integration", "$(projectdir)/tests/ui/test_pdf_outline_widget_integration.cpp", 300)
    create_test_target("test_pdf_viewer_integration", "$(projectdir)/tests/ui/test_pdf_viewer_integration.cpp", 300)
    create_test_target("test_right_side_bar_integration", "$(projectdir)/tests/ui/test_right_side_bar_integration.cpp", 300)
    create_test_target("test_search_widget_integration", "$(projectdir)/tests/ui/test_search_widget_integration.cpp", 300)
    create_test_target("test_side_bar_integration", "$(projectdir)/tests/ui/test_side_bar_integration.cpp", 300)
    create_test_target("test_status_bar_integration", "$(projectdir)/tests/ui/test_status_bar_integration.cpp", 300)
    create_test_target("test_thumbnail_context_menu_integration", "$(projectdir)/tests/ui/test_thumbnail_context_menu_integration.cpp", 300)
    create_test_target("test_thumbnail_list_view_integration", "$(projectdir)/tests/ui/test_thumbnail_list_view_integration.cpp", 300)
    create_test_target("test_thumbnail_widget_integration", "$(projectdir)/tests/ui/test_thumbnail_widget_integration.cpp", 300)
    create_test_target("test_view_widget_integration", "$(projectdir)/tests/ui/test_view_widget_integration.cpp", 300)
    create_test_target("test_welcome_screen_manager_integration", "$(projectdir)/tests/ui/test_welcome_screen_manager_integration.cpp", 300)
    create_test_target("test_qgraphics_components", "$(projectdir)/tests/ui/test_qgraphics_components.cpp", 300)

    create_test_target("test_document_analyzer", "$(projectdir)/tests/utils/test_document_analyzer.cpp", 300)
    create_test_target("test_error_handling", "$(projectdir)/tests/utils/test_error_handling.cpp", 300)
    create_test_target("test_error_recovery", "$(projectdir)/tests/utils/test_error_recovery.cpp", 300)
    create_test_target("test_pdf_utilities", "$(projectdir)/tests/utils/test_pdf_utilities.cpp", 300)
    create_test_target("test_pdf_optimizations", "$(projectdir)/tests/utils/test_pdf_optimizations.cpp", 300)
    if has_package("gtest") then
        create_test_target("test_thread_safety", "$(projectdir)/tests/utils/test_thread_safety.cpp", 600, {packages = {"gtest"}})
    elseif not has_config("quiet") then
        cprint("${yellow}Google Test not found, skipping test_thread_safety${clear}")
    end

    if not has_config("quiet") then
        cprint("${green}Comprehensive test support enabled with 73+ test targets${clear}")
    end
end
