#define BOOST_BIND_GLOBAL_PLACEHOLDERS

#include "Application.h"
#include <dxgidebug.h>

#include <boost/algorithm/string.hpp>
#include <boost/program_options.hpp>

std::ofstream gLogsFile;


void DXGICheckMemory() {
    ComPtr<IDXGIDebug> debugInterface = nullptr;
    ThrowIfFailed(DXGIGetDebugInterface1(0, IID_PPV_ARGS(&debugInterface)));

    debugInterface->ReportLiveObjects(DXGI_DEBUG_ALL, DXGI_DEBUG_RLO_FLAGS::DXGI_DEBUG_RLO_ALL);
    debugInterface.Reset();
}

OblivionMode GetApplicationModeFromString(const std::string& textInput) {
    if (boost::iequals("debug", textInput)) {
        return OblivionMode::Debug;
    }
    return OblivionMode::None;
}

std::optional<OblivionInitialization> ParseCommandLine(int argc, const char* argv[]) {
    try {
        using namespace boost::program_options;

        OblivionInitialization initStructure{};

        options_description genericOptions{ "Generic options" };
        genericOptions.add_options()
            ("help,h", "Help screen")
            ("version,v", "Print version string")
            ;

        options_description configOptions{ "Configuration" };
        configOptions.add_options()
            ("num-samples", value<unsigned int>(&initStructure.numSamples)->default_value(4))
            ("app-mode,a", value<std::string>()->default_value("debug"), "Application mode: debug")
            ("config-file", value<std::string>(&initStructure.configFile)->default_value("config.json"),
                            "Configuration file. If the specified file can not be opened, default options will be used")
            ("max-seconds-per-frame,s", value<float>(&initStructure.maxSecondsPerFrame)->default_value(FLT_MAX))
            ;

        options_description hiddenOptions{ "Hidden options" };
        hiddenOptions.add_options()
            ("input-files", value<std::vector<std::string>>(&initStructure.inputFiles), "Input files")
            ;

        options_description cmdlineOptions;
        cmdlineOptions.add(genericOptions).add(configOptions).add(hiddenOptions);

        options_description configFileOptions;
        configFileOptions.add(configOptions).add(hiddenOptions);

        options_description visibleOptions("Allowed options");
        visibleOptions.add(genericOptions).add(configOptions);

        positional_options_description inputFilesOption;
        inputFilesOption.add("input-files", -1);

        variables_map vm;
        store(command_line_parser(argc, argv).options(cmdlineOptions).positional(inputFilesOption).run(), vm);
        notify(vm);

        if (vm.count("help")) {
            Oblivion::DebugPrintLine("Usage: PathTracer.exe [options]");
            Oblivion::DebugPrintLine(visibleOptions);
            return std::nullopt;
        } else if (vm.count("version")) {
            Oblivion::DebugPrintLine("Current version: ", APP_VERSION);
            return std::nullopt;
        } else {
            Oblivion::DebugPrintLine("Number of samples: ", initStructure.numSamples);
            Oblivion::DebugPrintLine("Application mode: ", vm["app-mode"].as<std::string>());
            Oblivion::DebugPrintLine("Output file: ", initStructure.outputFile);
            Oblivion::DebugPrintLine("Input files ", initStructure.inputFiles);
            Oblivion::DebugPrintLine("Config file: ", initStructure.configFile);
            initStructure.applicationMode = GetApplicationModeFromString(vm["app-mode"].as<std::string>());
            if (initStructure.applicationMode == OblivionMode::None) {
                Oblivion::DebugPrintLine("Unable to parse application mode. Defaulting to Debug");
                initStructure.applicationMode = OblivionMode::None;
            }
            return initStructure;
        }
    } catch (const std::exception& e) {
        Oblivion::DebugPrintLine(e.what());
        return std::nullopt;
    }
}

int main(int argc, const char* argv[]) {

#ifdef LOG_TO_FILE
    gLogsFile.open("OblivionLogs.txt");
    EVALUATESHOW(gLogsFile.is_open(), "Unable to open OblivionLogs.txt for writing");
#endif

    auto initStructure = ParseCommandLine(argc, argv);
    if (!initStructure.has_value()) {
        return 0;
    }

    HINSTANCE hCurrentInstance = GetModuleHandle(NULL);
    TRY_PRINT_ERROR(Application::Get(hCurrentInstance, *initStructure)->Run());
    Application::Reset();
    TRY_PRINT_ERROR(DXGICheckMemory());

    return 0;

}
