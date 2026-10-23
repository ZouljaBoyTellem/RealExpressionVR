#include "FaceUpdater.h"
#include "OscReceiver.h"
#include "Settings.h"

namespace
{
	void InitializeLogging()
	{
		auto path = logger::log_directory();
		if (!path) {
			SKSE::stl::report_and_fail("Unable to find the SKSE log directory."sv);
		}

		*path /= std::format("{}.log", SKSE::PluginDeclaration::GetSingleton()->GetName());

		auto log = std::make_shared<spdlog::logger>(
			"Global", std::make_shared<spdlog::sinks::basic_file_sink_mt>(path->string(), true));
		log->set_level(spdlog::level::info);
		log->flush_on(spdlog::level::info);
		log->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] %v");

		spdlog::set_default_logger(std::move(log));
	}

	void OnMessage(SKSE::MessagingInterface::Message* a_msg)
	{
		switch (a_msg->type) {
		case SKSE::MessagingInterface::kDataLoaded:
			logger::info("Data loaded.");
			break;
		default:
			break;
		}
	}
}

SKSEPluginLoad(const SKSE::LoadInterface* a_skse)
{
	SKSE::Init(a_skse);
	InitializeLogging();

	const auto* plugin = SKSE::PluginDeclaration::GetSingleton();
	logger::info("{} v{} loaded on {} (runtime {})",
		plugin->GetName(),
		plugin->GetVersion().string(),
		REL::Module::IsVR() ? "Skyrim VR"sv : "Skyrim SE/AE"sv,
		REL::Module::get().version().string());

	SKSE::GetMessagingInterface()->RegisterListener(OnMessage);

	Settings::Load();

	// A failure here (e.g. the port is taken) is logged but doesn't stop the plugin from loading.
	// Read tracker values with OscReceiver::GetSingleton()->GetValue("/avatar/parameters/...").
	if (Settings::oscEnabled) {
		OscReceiver::GetSingleton()->Start(Settings::oscBindAddress, Settings::oscPort);
		FaceUpdater::Install();
	}

	return true;
}
