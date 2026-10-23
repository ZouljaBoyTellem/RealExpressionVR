#pragma once

namespace Settings
{
	// [OSC]
	inline bool          oscEnabled = true;
	inline std::string   oscBindAddress = "127.0.0.1";
	inline std::uint16_t oscPort = 9000;

	// [Face]
	inline std::uint32_t faceUpdateInterval = 10;  // frames between face updates

	// Reads Data/SKSE/Plugins/<PluginName>.ini. Missing keys keep the defaults above.
	void Load();
}
