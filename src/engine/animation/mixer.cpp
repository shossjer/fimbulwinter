
#include "mixer.hpp"

#include "Armature.hpp"

#include <core/container/Collection.hpp>
#include <core/debug.hpp>
#include <core/maths/Matrix.hpp>
#include <core/maths/Quaternion.hpp>
#include <core/maths/Vector.hpp>
#include <core/maths/algorithm.hpp>

#include <engine/Entity.hpp>
#include <engine/extras/Asset.hpp>

#include <utility/optional.hpp>

#include <cstdint>
#include <fstream>
#include <utility>
#include <vector>

namespace
{
	using Action = engine::animation::Armature::Action;
	using Armature = engine::animation::Armature;
	using Joint = engine::animation::Armature::Joint;
	using Mixer = unsigned int;

	core::container::UnorderedCollection
	<
		engine::extras::Asset,
		201,
		std::array<Armature, 101>,
		std::array<int, 1> // because
	>
	resources;

	struct Fade
	{
		float elapsed;
		float stepsize;

		Mixer from;
		Mixer to;

		Action::Frame compute()
		{

		}
	};
	struct Fadein
	{
		Mixer to;
	};
	struct Fadeout
	{
		Mixer from;
	};
	struct Playback
	{
		const Armature * armature;
		const Action * action;

		float elapsed;
		float stepsize;
		float length;
		int times_to_play;

		void update()
		{
			static int framei = 0;
			framei += 1;
			if (framei >= action->length) framei = 0;

			int rooti = 0;
			while (rooti < static_cast<int>(armature->njoints))
				rooti = update(*action, rooti, /*core::maths::Matrix4x4f::identity(), */framei);
		}
		int update(const Action & action, const int mei, /*const core::maths::Matrix4x4f & parent_matrix, */const int framei)
		{
			const auto pos = action.frames[framei].channels[mei].translation;
			const auto rot = action.frames[framei].channels[mei].rotation;
			const auto scale = action.frames[framei].channels[mei].scale;

			// const auto pose =
			// 	make_translation_matrix(pos) *
			// 	make_matrix(rot) *
			// 	make_scale_matrix(scale);

			// matrices[mei] = parent_matrix * pose;

			int childi = mei + 1;
			for (int i = 0; i < static_cast<int>(armature->joints[mei].nchildren); i++)
				childi = update(action, childi, /*matrices[mei], */framei);
			return childi;
		}
	};

	core::container::UnorderedCollection
	<
		Mixer,
		1001,
		std::array<Fade, 101>,
		std::array<Fadein, 101>,
		std::array<Fadeout, 101>,
		std::array<Playback, 101>
	>
	mixers;

	struct step_func
	{
		utility::optional<Mixer> operator () (Fade & fade)
		{
			fade.elapsed += fade.stepsize;
			if (fade.elapsed >= 1.f)
			{
				// this can be reduced to fade.to
				// return true;
				return utility::optional<Mixer>{fade.to};
			}
			mixers.call(fade.from, step_func{});
			mixers.call(fade.to, step_func{});
			// return false;
			return utility::optional<Mixer>{};
		}
		utility::optional<Mixer> operator () (Fadein & fadein)
		{
			mixers.call(fadein.to, step_func{});
			// return false;
			return utility::optional<Mixer>{};
		}
		utility::optional<Mixer> operator () (Fadeout & fadeout)
		{
			mixers.call(fadeout.from, step_func{});
			// return false;
			return utility::optional<Mixer>{};
		}
		utility::optional<Mixer> operator () (Playback & playback)
		{
			playback.elapsed += playback.stepsize;
			if (playback.elapsed >= playback.length)
			{
				if (playback.times_to_play <= 1)
				{
					// we are done with this one
					// return true;
					return utility::optional<Mixer>{}; // ??
				}
				playback.times_to_play--;
				playback.elapsed -= playback.length;
			}
			// return false;
			return utility::optional<Mixer>{};
		}
	};

	struct Character
	{
		const Armature * armature;

		Mixer mixer;
	};

	core::container::Collection
	<
		engine::Entity,
		201,
		std::array<Character, 101>,
		std::array<int, 1> // because
	>
	components;
}
