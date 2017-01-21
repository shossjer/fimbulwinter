
#include "level_placeholder.hpp"

#include <gameplay/characters.hpp>

#include <engine/Entity.hpp>
#include <engine/animation/mixer.hpp>
#include <engine/graphics/renderer.hpp>
#include <engine/physics/physics.hpp>

#include <utility/json.hpp>

#include <fstream>
#include <sstream>

namespace
{
	using namespace gameplay::level;

	using Entity = engine::Entity;

	using Vector3f = core::maths::Vector3f;
	using Quaternionf = core::maths::Quaternionf;
	using Matrix4x4f = core::maths::Matrix4x4f;

	using turret_t = gameplay::characters::turret_t;

	std::vector<std::string> split(const std::string & name)
	{
		std::vector<std::string> parts;

		std::stringstream ss;
		ss.str(name);
		std::string item;
		while (std::getline(ss, item, '.'))
		{
			parts.push_back(item);
		}

		return parts;
	}

	Vector3f parse_pos(const json jtransform)
	{
		const json jpos = jtransform["pos"];
		return Vector3f {jpos[0], jpos[1], jpos[2]};
	}

	Quaternionf parse_quat(const json jtransform)
	{
		const json jpos = jtransform["quat"];
		return Quaternionf {jpos[0], jpos[1], jpos[2], jpos[3]};
	}

	void load_shapes(std::vector<engine::physics::ShapeData> & shapes, const json & jshapes)
	{
		for (const json & jshape : jshapes)
		{
			const std::string type = jshape["type"];

			if (type=="box")
			{
				const json & jscale = jshape["scale"];
				const json & jtransform = jshape["transform"];

				const auto pos = parse_pos(jtransform);
				const auto quat = parse_quat(jtransform);

				shapes.push_back(
					engine::physics::ShapeData {
						engine::physics::ShapeData::Type::MESH,
						engine::physics::Material::STONE,
						0.2f,
						pos,
						quat,
						engine::physics::ShapeData::Geometry {
							engine::physics::ShapeData::Geometry::Box {jscale[0], jscale[1], jscale[2]}}});
			}
			else
			if (type=="mesh")
			{
				const json & jmesh = jshape["mesh"];

				std::vector<float> points;
				points.reserve(jmesh.size()*3);

				for (const json & jvert:jmesh)
				{
					points.push_back(jvert[0]);
					points.push_back(jvert[1]);
					points.push_back(jvert[2]);
				}

				shapes.push_back(engine::physics::ShapeData {
					engine::physics::ShapeData::Type::MESH,
					engine::physics::Material::STONE,
					0.2f,
					Vector3f {0.f, 0.f, 0.f},
					Quaternionf {1.f, 0.f, 0.f, 0.f},
					engine::physics::ShapeData::Geometry {engine::physics::ShapeData::Geometry::Mesh {points}}});
			}
		}
	}

	turret_t::pivot_t parse_pivot(const json jpivot)
	{
		return turret_t::pivot_t {parse_pos(jpivot), parse_quat(jpivot)};
	}

	void load_turret(const placeholder_t & placeholder)
	{
		const json jcontent = json::parse(std::ifstream("turret.json"));

		// for now add all shapes to the same actor...
		std::vector<engine::physics::ShapeData> shapes;

		turret_t turret;

		turret.id = Entity::create();
		// read platform
		{
			const json jgroup = jcontent["platform"];

			load_shapes(shapes, jgroup["shapes"]);
		}
		// read head
		{
			const json jgroup = jcontent["head"];

			load_shapes(shapes, jgroup["shapes"]);

			turret.head = turret_t::head_t {Entity::create(), parse_pivot(jgroup["pivot"])};
		}
		// read barrel
		{
			const json jgroup = jcontent["barrel"];

			load_shapes(shapes, jgroup["shapes"]);

			turret.barrel = turret_t::barrel_t {Entity::create(), parse_pivot(jgroup["pivot"])};
			turret.projectile = parse_pivot(jgroup["projectile"]);
		}


		// use the same shapes for renderer for now.
		for (const auto shape : shapes)
		{
			auto id = Entity::create();

			const engine::physics::ShapeData::Geometry::Box & b = shape.geometry.box;

			const Matrix4x4f matrix =
				make_translation_matrix(placeholder.pos + shape.pos) *
				make_matrix(shape.rot);

			engine::graphics::data::CuboidC data = {
				matrix,
				b.w*2,
				b.h*2,
				b.d*2,
				0xffffffff
			};
			engine::graphics::renderer::add(id, data);
		}
	}
}

namespace gameplay
{
namespace level
{
	void load(const placeholder_t & placeholder)
	{
		std::vector<std::string> parts = split(placeholder.name);

		if (parts.size()<=1)
		{
			debug_printline(0xffffffff, "Warning: Placeholder does not have a valid name: ", placeholder.name);
			return;
		}

		if (parts[1]=="turret")
		{
			load_turret(placeholder);
		}
		else
		{
			debug_printline(0xffffffff, "Warning: Placeholder is unknown: ", placeholder.name);
		}
	}
}
}
