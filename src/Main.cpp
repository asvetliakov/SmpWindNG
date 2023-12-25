#include <stddef.h>

#include "Config.h"
#include "Wind.h"

using namespace RE::BSScript;
using namespace SKSE;
using namespace SKSE::log;
using namespace SKSE::stl;

class VMClassRegistry;

namespace wind {
    constexpr unsigned long VERSION_MAJOR{2};
    constexpr unsigned long VERSION_MINOR{1};
    constexpr unsigned long VERSION_PATCH{0};
    constexpr unsigned long pluginVersion =
        (VERSION_MAJOR & 0xFF) << 24 | (VERSION_MINOR & 0xFF) << 16 | (VERSION_PATCH & 0xFF) << 8;

    Config g_config;
    Config g_configDefault;
    Wind g_wind;

    inline static hdt::PluginInterface::Version interfaceMin{1, 0, 0};
    inline static hdt::PluginInterface::Version interfaceMax{2, 0, 0};

    inline static hdt::PluginInterface::Version bulletMin{hdt::PluginInterface::BULLET_VERSION};
    inline static hdt::PluginInterface::Version bulletMax{hdt::PluginInterface::BULLET_VERSION.major + 1, 0, 0};

    bool registerFunctions(IVirtualMachine* vm);
    void onConnect(hdt::PluginInterface* smp);

}  // namespace wind

void wind::onConnect(hdt::PluginInterface* smp) {
    PWSTR wpath;
    HRESULT res = SHGetKnownFolderPath(FOLDERID_Documents, 0, NULL, &wpath);
    if (SUCCEEDED(res)) {
        std::filesystem::path documents(wpath, std::filesystem::path::native_format);
        std::filesystem::path configPath(documents / "My Games\\Skyrim Special Edition\\SKSE\\SMP Wind.ini");

        log::info("Loading config file {}...", configPath.string().c_str());

        if (wind::g_config.load(configPath)) {
            log::info("Config file loaded");
        } else {
            log::info("WARNING: Failed to load config file. Settings will not be saved");
        }
    } else {
        log::info("WARNING: Documents folder not found. Settings will not be saved");
    }

    wind::g_wind.init(wind::g_config);

    smp->addListener(&wind::g_wind);

    log::info("Initialisation complete.\n");
}

namespace {
    /**
     * Setup logging.
     *
     * <p>
     * Logging is important to track issues. CommonLibSSE bundles functionality for spdlog, a common C++ logging
     * framework. Here we initialize it, using values from the configuration file. This includes support for a debug
     * logger that shows output in your IDE when it has a debugger attached to Skyrim, as well as a file logger which
     * writes data to the standard SKSE logging directory at <code>Documents/My Games/Skyrim Special Edition/SKSE</code>
     * (or <code>Skyrim VR</code> if you are using VR).
     * </p>
     */
    void InitializeLogging() {
        auto path = log_directory();
        if (!path) {
            report_and_fail("Unable to lookup SKSE logs directory.");
        }
        *path /= PluginDeclaration::GetSingleton()->GetName();
        *path += L".log";

        std::shared_ptr<spdlog::logger> log;
        if (IsDebuggerPresent()) {
            log = std::make_shared<spdlog::logger>("Global", std::make_shared<spdlog::sinks::msvc_sink_mt>());
        } else {
            log = std::make_shared<spdlog::logger>(
                "Global", std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
        }
        log->set_level(spdlog::level::level_enum::info);
        log->flush_on(spdlog::level::level_enum::trace);

        spdlog::set_default_logger(std::move(log));
        spdlog::set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%n] [%l] [%t] [%s:%#] %v");
    }

    /**
     * Initialize the SKSE cosave system for our plugin.
     *
     * <p>
     * SKSE comes with a feature called a <em>cosave</em>, an additional save file kept alongside the original Skyrim
     * save file. SKSE plugins can write their own data to this file, and load it again when the save game is loaded,
     * allowing them to keep custom data along with a player's save. Each plugin must have a unique ID, which is four
     * characters long (similar to the record names used by forms in ESP files). Note however this is little-endian, so
     * technically the 'SMPL' here ends up as 'LPMS' in the save file, unless we use a byte order swap.
     * </p>
     *
     * <p>
     * There can only be one serialization callback for save, revert (called on new game and before a load), and load
     * for the entire plugin.
     * </p>
     */
    // void InitializeSerialization() {
    //     log::trace("Initializing cosave serialization...");
    //     auto* serde = GetSerializationInterface();
    //     serde->SetUniqueID(_byteswap_ulong('SMPL'));
    //     serde->SetSaveCallback(Sample::Manager::OnGameSaved);
    //     serde->SetRevertCallback(Sample::Manager::OnRevert);
    //     serde->SetLoadCallback(Sample::Manager::OnGameLoaded);
    //     log::trace("Cosave serialization initialized.");
    // }

    /**
     * Initialize our Papyrus extensions.
     *
     * <p>
     * A common use of SKSE is to add new Papyrus functions. You can call a registration callback to do this. This
     * callback will not necessarily be called immediately, if the Papyrus VM has not been initialized yet (in that case
     * it's execution is delayed until the VM is available).
     * </p>
     *
     * <p>
     * You can call the <code>Register</code> function as many times as you want and at any time you want to register
     * additional functions.
     * </p>
     */
    void InitializePapyrus() {
        log::trace("Initializing Papyrus binding...");
        if (GetPapyrusInterface()->Register(wind::registerFunctions)) {
            log::debug("Papyrus functions bound.");
        } else {
            stl::report_and_fail("Failure to register Papyrus bindings.");
        }
    }

    /**
     * Initialize the trampoline space for function hooks.
     *
     * <p>
     * Function hooks are one of the most powerful features available to SKSE developers, allowing you to replace
     * functions with your own, or replace a function call with a call to another function. However, to do this, we
     * need a code snippet that replicates the first instructions of the original code we overwrote, in order to be
     * able to call back to the original control flow with all the same functionality.
     * </p>
     *
     * <p>
     * CommonLibSSE comes with functionality to allocate trampoline space, including a common singleton space we can
     * access from anywhere. While this is not necessarily the most advanced use of trampolines and hooks, this will
     * suffice for our demo project.
     * </p>
     */
    // void InitializeHooking() {
    //     log::trace("Initializing trampoline...");
    //     auto& trampoline = GetTrampoline();
    //     trampoline.create(64);
    //     log::trace("Trampoline initialized.");

    //     Hooks::Initialize(trampoline);
    // }

    /**
     * Register to listen for messages.
     *
     * <p>
     * SKSE has a messaging system to allow for loosely coupled messaging. This means you don't need to know about or
     * link with a message sender to receive their messages. SKSE itself will send messages for common Skyrim lifecycle
     * events, such as when SKSE plugins are done loading, or when all ESP plugins are loaded.
     * </p>
     *
     * <p>
     * Here we register a listener for SKSE itself (because we have not specified a message source). Plugins can send
     * their own messages that other plugins can listen for as well, although that is not demonstrated in this example
     * and is not common.
     * </p>
     *
     * <p>
     * The data included in the message is provided as only a void pointer. It's type depends entirely on the type of
     * message, and some messages have no data (<code>dataLen</code> will be zero).
     * </p>
     */
    void InitializeMessaging() {
        if (!GetMessagingInterface()->RegisterListener([](MessagingInterface::Message* message) {
                switch (message->type) {
                    // Skyrim lifecycle events.
                    case MessagingInterface::kPostLoad:  // Called after all plugins have finished running
                                                         // SKSEPlugin_Load. It is now safe to do multithreaded
                                                         // operations, or operations against other plugins.
                        GetMessagingInterface()->RegisterListener("hdtSMP64", [](MessagingInterface::Message* msg) {
                            if (msg && msg->type == hdt::PluginInterface::MSG_STARTUP && msg->data) {
                                auto smp = reinterpret_cast<hdt::PluginInterface*>(msg->data);

                                auto&& info = smp->getVersionInfo();

                                if (info.interfaceVersion.major >= wind::interfaceMin.major &&
                                    info.interfaceVersion.major < wind::interfaceMax.major) {
                                    if (info.bulletVersion.major >= wind::bulletMin.major &&
                                        info.bulletVersion.major < wind::bulletMax.major) {
                                        log::info("Connection established");
                                        wind::onConnect(smp);
                                    } else {
                                        log::error("Incompatible Bullet version.");
                                    }
                                } else {
                                    log::error("Incompatible HDT-SMP interface.");
                                }
                            }
                        });
                        break;
                    case MessagingInterface::kPostPostLoad:  // Called after all kPostLoad message handlers have run.
                    case MessagingInterface::kInputLoaded:   // Called when all game data has been found.
                        break;
                    case MessagingInterface::kDataLoaded:  // All ESM/ESL/ESP plugins have loaded, main menu is now
                                                           // active.
                        // It is now safe to access form data.

                        break;

                    // Skyrim game events.
                    case MessagingInterface::kNewGame:      // Player starts a new game from main menu.
                    case MessagingInterface::kPreLoadGame:  // Player selected a game to load, but it hasn't loaded yet.
                                                            // Data will be the name of the loaded save.
                    case MessagingInterface::kPostLoadGame:  // Player's selected save game has finished loading.
                                                             // Data will be a boolean indicating whether the load was
                                                             // successful.
                    case MessagingInterface::kSaveGame:      // The player has saved a game.
                                                             // Data will be the save name.
                    case MessagingInterface::kDeleteGame:  // The player deleted a saved game from within the load menu.
                        break;
                }
            })) {
            stl::report_and_fail("Unable to register message listener.");
        }
    }
}  // namespace

/**
 * This if the main callback for initializing your SKSE plugin, called just before Skyrim runs its main function.
 *
 * <p>
 * This is your main entry point to your plugin, where you should initialize everything you need. Many things can't be
 * done yet here, since Skyrim has not initialized and the Windows loader lock is not released (so don't do any
 * multithreading). But you can register to listen for messages for later stages of Skyrim startup to perform such
 * tasks.
 * </p>
 */
SKSEPluginLoad(const LoadInterface* skse) {
    InitializeLogging();

    auto* plugin = PluginDeclaration::GetSingleton();
    auto version = plugin->GetVersion();
    log::info("{} {} is loading...", plugin->GetName(), version);

    Init(skse);
    InitializePapyrus();
    InitializeMessaging();

    log::info("{} has finished loading.", plugin->GetName());
    return true;
}
