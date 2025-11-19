-- xmake/tests/test_runner.lua
-- Test runner utility for managing and executing tests

-- Test categories for better organization
local test_categories = {
    smoke = {"test_smoke"},
    cache = {
        "test_cache_manager", "test_pdf_cache_manager",
        "test_page_text_cache", "test_search_result_cache"
    },
    command = {
        "test_command_manager", "test_document_commands",
        "test_navigation_commands", "test_initialization_command"
    },
    controller = {
        "test_service_locator", "test_state_manager", "test_state_manager_comprehensive",
        "test_event_bus", "test_application_controller", "test_document_controller",
        "test_page_controller", "test_configuration_manager", "test_action_map"
    },
    factory = {
        "test_model_factory", "test_command_prototype_registry"
    },
    integration = {
        "test_application_startup", "test_search_integration",
        "test_rendering_mode_switch", "test_qgraphics_pdf"
    },
    logging = {"test_logging_comprehensive"},
    managers = {"test_windows_path"},
    model = {"test_search_model"},
    performance = {
        "test_rendering_performance", "test_qgraphics_rendering_performance",
        "test_search_optimizations"
    },
    real_world = {"test_real_pdf_documents"},
    search = {
        "test_search_engine", "test_search_configuration", "test_search_executor",
        "test_search_features", "test_search_metrics", "test_search_performance",
        "test_search_validator", "test_text_extractor", "test_background_processor",
        "test_incremental_search_manager", "test_memory_aware_search_results",
        "test_memory_manager", "test_memory_manager_stubs", "test_multi_search_engine",
        "test_smart_eviction_policy", "test_search_error_recovery", "test_search_thread_safety"
    },
    ui = {
        "test_debug_log_panel_integration", "test_tool_bar_integration",
        "test_thumbnail_generator_integration", "test_document_comparison_integration",
        "test_document_metadata_dialog_integration", "test_menu_bar_integration",
        "test_pdf_animations_integration", "test_pdf_outline_widget_integration",
        "test_pdf_viewer_integration", "test_right_side_bar_integration",
        "test_search_widget_integration", "test_side_bar_integration",
        "test_status_bar_integration", "test_thumbnail_context_menu_integration",
        "test_thumbnail_list_view_integration", "test_thumbnail_widget_integration",
        "test_view_widget_integration", "test_welcome_screen_manager_integration",
        "test_qgraphics_components"
    },
    utils = {
        "test_document_analyzer", "test_error_handling", "test_error_recovery",
        "test_pdf_utilities", "test_pdf_optimizations", "test_thread_safety"
    }
}

-- Function to run tests by category
function run_test_category(category)
    local tests = test_categories[category]
    if not tests then
        cprint("${red}Unknown test category: %s${clear}", category)
        return false
    end

    cprint("${bright}Running %s tests...${clear}", category)
    local success_count = 0
    local total_count = #tests

    for _, test_name in ipairs(tests) do
        cprint("${cyan}Running %s...${clear}", test_name)
        local result = os.execv("xmake", {"run", test_name})
        if result == 0 then
            cprint("${green}✓ %s passed${clear}", test_name)
            success_count = success_count + 1
        else
            cprint("${red}✗ %s failed${clear}", test_name)
        end
    end

    cprint("${bright}%s tests: %d/%d passed${clear}", category, success_count, total_count)
    return success_count == total_count
end

-- Function to run all tests
function run_all_tests()
    cprint("${bright}Running all test categories...${clear}")
    local total_success = 0
    local total_categories = 0

    for category, _ in pairs(test_categories) do
        total_categories = total_categories + 1
        if run_test_category(category) then
            total_success = total_success + 1
        end
        cprint("") -- Empty line for readability
    end

    cprint("${bright}Test Summary: %d/%d categories passed${clear}", total_success, total_categories)
    return total_success == total_categories
end

-- Function to run specific tests
function run_specific_tests(test_names)
    cprint("${bright}Running specific tests...${clear}")
    local success_count = 0
    local total_count = #test_names

    for _, test_name in ipairs(test_names) do
        cprint("${cyan}Running %s...${clear}", test_name)
        local result = os.execv("xmake", {"run", test_name})
        if result == 0 then
            cprint("${green}✓ %s passed${clear}", test_name)
            success_count = success_count + 1
        else
            cprint("${red}✗ %s failed${clear}", test_name)
        end
    end

    cprint("${bright}Specific tests: %d/%d passed${clear}", success_count, total_count)
    return success_count == total_count
end

-- Function to list available test categories
function list_test_categories()
    cprint("${bright}Available test categories:${clear}")
    for category, tests in pairs(test_categories) do
        cprint("  ${green}%s${clear} (%d tests)", category, #tests)
    end
end

-- Function to list tests in a category
function list_tests_in_category(category)
    local tests = test_categories[category]
    if not tests then
        cprint("${red}Unknown test category: %s${clear}", category)
        return
    end

    cprint("${bright}Tests in %s category:${clear}", category)
    for _, test_name in ipairs(tests) do
        cprint("  ${cyan}%s${clear}", test_name)
    end
end

-- Add test runner task
task("test")
    set_menu({
        usage = "xmake test [options] [category|test_names...]",
        description = "Run tests by category or specific test names",
        options = {
            {'c', "category", "k", nil, "Run tests by category"},
            {'l', "list", "k", nil, "List available categories or tests in category"},
            {'a', "all", "k", nil, "Run all tests"},
            {nil, "specific", "vs", nil, "Run specific tests by name"}
        }
    })

    on_run(function ()
        import("core.base.option")

        if option.get("list") then
            if option.get("category") then
                list_tests_in_category(option.get("category"))
            else
                list_test_categories()
            end
            return
        end

        if option.get("all") then
            if not run_all_tests() then
                os.exit(1)
            end
            return
        end

        if option.get("category") then
            if not run_test_category(option.get("category")) then
                os.exit(1)
            end
            return
        end

        if option.get("specific") then
            if not run_specific_tests(option.get("specific")) then
                os.exit(1)
            end
            return
        end

        -- Default: run smoke tests
        if not run_test_category("smoke") then
            os.exit(1)
        end
    end)
task_end()
