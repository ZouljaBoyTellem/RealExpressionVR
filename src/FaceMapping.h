#pragma once

// Maps Quest Pro face tracking weights (sent by Steam Link as /sl/xrfb/facew/<Name>) onto Skyrim's
// facegen sliders. Each mapping takes the largest of its source values, multiplies it by a
// per-mapping scale from the ini, and clamps the result to [0, 1].
//
// Tracker values with no clear Skyrim equivalent are deliberately left unmapped; see the ini.
namespace FaceMapping
{
	struct Mapping
	{
		RE::BSFaceGenKeyframe::Type     type;
		std::uint32_t                   index;    // BSFaceGenKeyframeMultiple::Expression/Modifier/Phoneme
		std::string_view                name;     // ini keys are "f" + name and, for guesses, "bEnable" + name
		std::array<std::string_view, 4> sources;  // OSC addresses, empty entries are ignored
		bool                            guess{ false };  // uncertain match, can be switched off in the ini
		float                           scale{ 1.0f };
		bool                            enabled{ true };

		// Disabled or zero-scale mappings leave their slider to the game.
		[[nodiscard]] bool IsActive() const noexcept { return enabled && scale != 0.0f; }
	};

	[[nodiscard]] std::span<Mapping> GetMappings() noexcept;

	// ini section holding the scales for this slider type.
	[[nodiscard]] const char* GetSection(RE::BSFaceGenKeyframe::Type a_type) noexcept;

	// Current slider value for a_mapping from the latest OSC data, in [0, 1]. Thread-safe.
	[[nodiscard]] float Evaluate(const Mapping& a_mapping);
}
