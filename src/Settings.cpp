#include "Settings.h"

#include "FaceMapping.h"

#include <SimpleIni.h>

namespace Settings
{
	void Load()
	{
		const auto path = std::format("Data/SKSE/Plugins/{}.ini", SKSE::PluginDeclaration::GetSingleton()->GetName());

		CSimpleIniA ini;
		ini.SetUnicode();
		if (ini.LoadFile(path.c_str()) < 0) {
			logger::warn("Could not read {}, using default settings.", path);
		}

		oscEnabled = ini.GetBoolValue("OSC", "bEnabled", oscEnabled);
		oscBindAddress = ini.GetValue("OSC", "sBindAddress", oscBindAddress.c_str());

		const long port = ini.GetLongValue("OSC", "iPort", oscPort);
		if (port < 1 || port > 65535) {
			logger::warn("[OSC] iPort={} is out of range, using {}.", port, oscPort);
		} else {
			oscPort = static_cast<std::uint16_t>(port);
		}

		logger::info("[OSC] bEnabled={} sBindAddress={} iPort={}", oscEnabled, oscBindAddress, oscPort);

		const long interval = ini.GetLongValue("Face", "iUpdateInterval", faceUpdateInterval);
		if (interval < 1) {
			logger::warn("[Face] iUpdateInterval={} must be at least 1, using {}.", interval, faceUpdateInterval);
		} else {
			faceUpdateInterval = static_cast<std::uint32_t>(interval);
		}
		logger::info("[Face] iUpdateInterval={}", faceUpdateInterval);

		for (auto& mapping : FaceMapping::GetMappings()) {
			const auto  section = FaceMapping::GetSection(mapping.type);
			const auto  key = std::format("f{}", mapping.name);
			const float scale = static_cast<float>(ini.GetDoubleValue(section, key.c_str(), mapping.scale));
			if (scale < 0.0f) {
				logger::warn("[{}] {}={} is negative, using {}.", section, key, scale, mapping.scale);
			} else {
				mapping.scale = scale;
			}
			logger::info("[{}] {}={}", section, key, mapping.scale);

			if (mapping.guess) {
				const auto enableKey = std::format("bEnable{}", mapping.name);
				mapping.enabled = ini.GetBoolValue(section, enableKey.c_str(), mapping.enabled);
				logger::info("[{}] {}={}", section, enableKey, mapping.enabled);
			}
		}
	}
}
