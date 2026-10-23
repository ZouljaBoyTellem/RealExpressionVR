#include "FaceMapping.h"

#include "OscReceiver.h"

namespace FaceMapping
{
	namespace
	{
		using Type = RE::BSFaceGenKeyframe::Type;
		using KF = RE::BSFaceGenKeyframeMultiple;

		// Left/right follow the tracker's convention (the wearer's own left), which should match
		// the character's left in Skyrim. Swap the sources here if a mirror shows otherwise.
		std::array mappings{
			// Eyes and brows
			Mapping{ Type::Modifier, KF::BlinkLeft, "BlinkLeft", { "/sl/xrfb/facew/EyesClosedL" } },
			Mapping{ Type::Modifier, KF::BlinkRight, "BlinkRight", { "/sl/xrfb/facew/EyesClosedR" } },
			Mapping{ Type::Modifier, KF::BrowDownLeft, "BrowDownLeft", { "/sl/xrfb/facew/BrowLowererL" } },
			Mapping{ Type::Modifier, KF::BrowDownRight, "BrowDownRight", { "/sl/xrfb/facew/BrowLowererR" } },
			Mapping{ Type::Modifier, KF::BrowUpLeft, "BrowUpLeft", { "/sl/xrfb/facew/InnerBrowRaiserL", "/sl/xrfb/facew/OuterBrowRaiserL" } },
			Mapping{ Type::Modifier, KF::BrowUpRight, "BrowUpRight", { "/sl/xrfb/facew/InnerBrowRaiserR", "/sl/xrfb/facew/OuterBrowRaiserR" } },
			Mapping{ Type::Modifier, KF::LookDown, "LookDown", { "/sl/xrfb/facew/EyesLookDownL", "/sl/xrfb/facew/EyesLookDownR" } },
			Mapping{ Type::Modifier, KF::LookLeft, "LookLeft", { "/sl/xrfb/facew/EyesLookLeftL", "/sl/xrfb/facew/EyesLookLeftR" } },
			Mapping{ Type::Modifier, KF::LookRight, "LookRight", { "/sl/xrfb/facew/EyesLookRightL", "/sl/xrfb/facew/EyesLookRightR" } },
			Mapping{ Type::Modifier, KF::LookUp, "LookUp", { "/sl/xrfb/facew/EyesLookUpL", "/sl/xrfb/facew/EyesLookUpR" } },
			Mapping{ Type::Modifier, KF::SquintLeft, "SquintLeft", { "/sl/xrfb/facew/LidTightenerL" } },
			Mapping{ Type::Modifier, KF::SquintRight, "SquintRight", { "/sl/xrfb/facew/LidTightenerR" } },

			// Mouth. Steam Link sends the tongue values under both "Tongue" and the misspelled
			// "Tounge", so both are read.
			Mapping{ Type::Phoneme, KF::Aah, "Aah", { "/sl/xrfb/facew/JawDrop" } },
			Mapping{ Type::Phoneme, KF::BMP, "BMP", { "/sl/xrfb/facew/LipPressorL", "/sl/xrfb/facew/LipPressorR" } },
			Mapping{ Type::Phoneme, KF::OohQ, "OohQ", { "/sl/xrfb/facew/LipPuckerL", "/sl/xrfb/facew/LipPuckerR" } },
			Mapping{ Type::Phoneme, KF::DST, "DST", { "/sl/xrfb/facew/TongueTipAlveolar", "/sl/xrfb/facew/ToungeTipAlveolar" } },
			Mapping{ Type::Phoneme, KF::Th, "Th", { "/sl/xrfb/facew/TongueTipInterdental", "/sl/xrfb/facew/ToungeTipInterdental" } },
			Mapping{ Type::Phoneme, KF::K, "K", { "/sl/xrfb/facew/BackDorsalVelar" } },

			// Guesses: no tracker value clearly matches these, so each can be disabled in the ini.
			// BigAah and N share their source with Aah and DST, so both sliders move together.
			Mapping{ Type::Phoneme, KF::BigAah, "BigAah", { "/sl/xrfb/facew/JawDrop" }, true },
			Mapping{ Type::Phoneme, KF::ChJSh, "ChJSh", { "/sl/xrfb/facew/FrontDorsalPalate" }, true },
			Mapping{ Type::Phoneme, KF::Eee, "Eee", { "/sl/xrfb/facew/LipStretcherL", "/sl/xrfb/facew/LipStretcherR" }, true },
			Mapping{ Type::Phoneme, KF::FV, "FV", { "/sl/xrfb/facew/LipSuckLB", "/sl/xrfb/facew/LipSuckRB" }, true },
			Mapping{ Type::Phoneme, KF::N, "N", { "/sl/xrfb/facew/TongueTipAlveolar", "/sl/xrfb/facew/ToungeTipAlveolar" }, true },
			Mapping{ Type::Phoneme, KF::Oh, "Oh", { "/sl/xrfb/facew/LipFunnelerLB", "/sl/xrfb/facew/LipFunnelerLT", "/sl/xrfb/facew/LipFunnelerRB", "/sl/xrfb/facew/LipFunnelerRT" }, true },
			Mapping{ Type::Phoneme, KF::W, "W", { "/sl/xrfb/facew/LipPuckerL", "/sl/xrfb/facew/LipPuckerR" }, true },

			// Moods change the whole face, and only the strongest one is shown. Skyrim has no slider
			// for a smile, frown or nose wrinkle, so each drives the mood it's most associated with.
			Mapping{ Type::Expression, KF::MoodHappy, "MoodHappy", { "/sl/xrfb/facew/LipCornerPullerL", "/sl/xrfb/facew/LipCornerPullerR" }, true },
			Mapping{ Type::Expression, KF::MoodSad, "MoodSad", { "/sl/xrfb/facew/LipCornerDepressorL", "/sl/xrfb/facew/LipCornerDepressorR" }, true },
			Mapping{ Type::Expression, KF::MoodDisgusted, "MoodDisgusted", { "/sl/xrfb/facew/NoseWrinklerL", "/sl/xrfb/facew/NoseWrinklerR" }, true },
		};
	}

	std::span<Mapping> GetMappings() noexcept
	{
		return mappings;
	}

	const char* GetSection(RE::BSFaceGenKeyframe::Type a_type) noexcept
	{
		switch (a_type) {
		case Type::Expression:
			return "Expression";
		case Type::Modifier:
			return "Modifier";
		case Type::Phoneme:
			return "Phoneme";
		default:
			return "Custom";
		}
	}

	float Evaluate(const Mapping& a_mapping)
	{
		const auto* receiver = OscReceiver::GetSingleton();

		float value = 0.0f;
		for (const auto source : a_mapping.sources) {
			if (source.empty()) {
				continue;
			}
			if (const auto received = receiver->GetValue(source)) {
				value = std::max(value, *received);
			}
		}

		return std::clamp(value * a_mapping.scale, 0.0f, 1.0f);
	}
}
