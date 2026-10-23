#include "FaceUpdater.h"

#include "FaceMapping.h"
#include "OscReceiver.h"
#include "Settings.h"

namespace FaceUpdater
{
	namespace
	{
		using Type = RE::BSFaceGenKeyframe::Type;

		std::uint32_t frameCounter = 0;  // only touched on the game thread

		RE::BSFaceGenKeyframeMultiple* GetKeyframe(RE::BSFaceGenAnimationData& a_faceData, Type a_type)
		{
			switch (a_type) {
			case Type::Modifier:
				return &a_faceData.modifierKeyFrame;
			case Type::Phoneme:
				return &a_faceData.phenomeKeyFrame;
			default:
				return nullptr;
			}
		}

		void Apply(RE::Actor* a_actor)
		{
			// Until the tracker has sent something, leave the face to the game.
			if (OscReceiver::GetSingleton()->GetPacketCount() == 0) {
				return;
			}

			auto* faceData = a_actor->GetFaceGenAnimationData();
			if (!faceData) {
				return;
			}

			// Skyrim shows one mood at a time, so the strongest mapped expression wins.
			std::optional<std::pair<std::uint32_t, float>> mood;
			{
				RE::BSSpinLockGuard guard(faceData->lock);
				for (const auto& mapping : FaceMapping::GetMappings()) {
					if (!mapping.IsActive()) {
						continue;  // leave this slider to the game
					}

					const float value = FaceMapping::Evaluate(mapping);
					if (mapping.type == Type::Expression) {
						if (!mood || value > mood->second) {
							mood = { mapping.index, value };
						}
						continue;
					}

					auto* keyframe = GetKeyframe(*faceData, mapping.type);
					if (keyframe && keyframe->values && mapping.index < keyframe->count) {
						keyframe->SetValue(mapping.index, value);
					}
				}
			}

			if (mood) {
				// Same sequence as MFGFix: clear the override flag so SetExpressionOverride applies,
				// then set it so the game's own moods (combat, dialogue) don't replace ours.
				faceData->exprOverride = false;
				faceData->SetExpressionOverride(mood->first, mood->second);
				faceData->exprOverride = true;
			}
		}

		struct PlayerUpdate
		{
			static void thunk(RE::PlayerCharacter* a_player, float a_delta)
			{
				func(a_player, a_delta);

				if (++frameCounter >= Settings::faceUpdateInterval) {
					frameCounter = 0;
					Apply(a_player);
				}
			}

			static inline REL::Relocation<decltype(thunk)> func;
		};
	}

	void Install()
	{
		// Actor::Update is vtable slot 0xAD on SE/AE and 0xAF on VR (see Actor::Update in CommonLib).
		const auto index = REL::Relocate<std::size_t>(0xAD, 0xAD, 0xAF);

		REL::Relocation<std::uintptr_t> vtable{ RE::VTABLE_PlayerCharacter[0] };
		PlayerUpdate::func = vtable.write_vfunc(index, PlayerUpdate::thunk);

		logger::info("Hooked PlayerCharacter::Update, applying face tracking every {} frame(s).", Settings::faceUpdateInterval);
	}
}
