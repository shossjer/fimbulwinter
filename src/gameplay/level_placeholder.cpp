
#include "level_placeholder.hpp"

#include <core/maths/algorithm.hpp>

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
	using namespace gameplay;
	using namespace gameplay::level;

	using Entity = engine::Entity;

	using transform_t = ::engine::transform_t;

	struct beam_t
	{
		engine::Entity id;
		engine::Entity jointId;
	};

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
						engine::physics::ShapeData::Type::BOX,
						engine::physics::Material::STONE,
						0.1f,
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

				shapes.push_back(
					engine::physics::ShapeData {
						engine::physics::ShapeData::Type::MESH,
						engine::physics::Material::STONE,
						0.2f,
						Vector3f {0.f, 0.f, 0.f},
						Quaternionf {1.f, 0.f, 0.f, 0.f},
						engine::physics::ShapeData::Geometry {
							engine::physics::ShapeData::Geometry::Mesh {points}}});
			}
		}
	}

	transform_t parse_pivot(const json jpivot)
	{
		return transform_t {parse_pos(jpivot), parse_quat(jpivot)};
	}

	void load_turret(const placeholder_t & placeholder)
	{
		const json jcontent = json::parse(std::ifstream("res/turret.json"));

		asset::turret_t turret;

		turret.id = Entity::create();
		turret.transform = transform_t {placeholder.pos, placeholder.quat};

		// platform
		{
			const json jgroup = jcontent["platform"];

			std::vector<engine::physics::ShapeData> shapes;

			load_shapes(shapes, jgroup["shapes"]);

			// create physics object
			engine::physics::post_create(
				turret.id,
				engine::physics::ActorData(
					engine::physics::ActorData::Type::STATIC,
					engine::physics::ActorData::Behaviour::DEFAULT,
					placeholder.pos, // note: do not add pivot pos
					placeholder.quat * parse_quat(jgroup["pivot"]),
					shapes));

			// TODO: create renderer object
		}
		// head
		{
			const json jgroup = jcontent["head"];

			std::vector<engine::physics::ShapeData> shapes;

			load_shapes(shapes, jgroup["shapes"]);

			turret.head = asset::turret_t::head_t {
				Entity::create(),
				Entity::create(),
				parse_pivot(jgroup["pivot"])};

			// create physics object
			engine::physics::post_create(
				turret.head.id,
				engine::physics::ActorData(
					engine::physics::ActorData::Type::DYNAMIC,
					engine::physics::ActorData::Behaviour::DEFAULT,
					Vector3f{0.f, 0.f, 0.f}, // note: anything is fine?
					turret.head.pivot.quat,
					shapes));

			// TODO: create renderer object

		}
		// barrel
		{
			const json jgroup = jcontent["barrel"];

			std::vector<engine::physics::ShapeData> shapes;

			load_shapes(shapes, jgroup["shapes"]);

			turret.barrel = asset::turret_t::barrel_t {
				Entity::create(),
				Entity::create(),
				parse_pivot(jgroup["pivot"])};

			turret.projectile = parse_pivot(jgroup["projectile"]);
			turret.projectile.quat *= placeholder.quat;
			turret.projectile.pos += turret.head.pivot.pos;

			// create physics object
			engine::physics::post_create(
				turret.barrel.id,
				engine::physics::ActorData(
					engine::physics::ActorData::Type::DYNAMIC,
					engine::physics::ActorData::Behaviour::DEFAULT,
					Vector3f{0.f, 0.f, 0.f}, // note: anything is fine?
					turret.barrel.pivot.quat,
					shapes));

			// TODO: create renderer object
		}

		// connect the platform with the head
		engine::physics::post_joint(
			engine::physics::joint_t {
				turret.head.jointId,
				engine::physics::joint_t::Type::HINGE,
				turret.id,
				turret.head.id,
				transform_t{
					turret.head.pivot.pos,
					turret.head.pivot.quat * core::maths::rotation_of(Vector3f{0.f, 0.f, 1.f})},
				transform_t{
					Vector3f{0.f, 0.f, 0.f},
					core::maths::rotation_of(Vector3f{0.f, 0.f, 1.f})}});

		// connect the head with the barrel
		engine::physics::post_joint(
			engine::physics::joint_t {
				turret.head.jointId,
				engine::physics::joint_t::Type::FIXED,
				turret.head.id,
				turret.barrel.id,
				transform_t{
					turret.barrel.pivot.pos,
					turret.barrel.pivot.quat * core::maths::rotation_of(Vector3f{0.f, 0.f, 1.f})},
				transform_t{
					Vector3f{0.f, 0.f, 0.f},
					core::maths::rotation_of(Vector3f{0.f, 0.f, 1.f})}});

		// add the main turret entity to characters.
		// this object contains all the sub-objects
		//gameplay::characters::post_add_turret(turret);
	}

	void load_beam(const placeholder_t & placeholder)
	{
		//  load file with additional data of the "beam" object
		const json jcontent = json::parse(std::ifstream("res/beam.json"));

		beam_t beam;

		beam.id = Entity::create();
		beam.jointId = Entity::create();

		const auto jb = jcontent[placeholder.name];
		const float driveSpeed = jb["driveSpeed"];
		const float forceMax = jb["forceMax"];

		// create the beam object
		{
			std::vector<engine::physics::ShapeData> shapes;

			core::maths::Vector3f::array_type b;
			placeholder.scale.get_aligned(b);

			shapes.emplace_back(
				engine::physics::ShapeData {
					engine::physics::ShapeData::Type::BOX,
					engine::physics::Material::SUPER_RUBBER,
					0.5f,
					Vector3f {0.f, 0.f, 0.f},
					Quaternionf {1.f, 0.f, 0.f, 0.f},
					engine::physics::ShapeData::Geometry {
						engine::physics::ShapeData::Geometry::Box {
							b[0], b[1], b[2]}}});

			// create physics object
			engine::physics::post_create(
				beam.id,
				engine::physics::ActorData(
					engine::physics::ActorData::Type::DYNAMIC,
					engine::physics::ActorData::Behaviour::DEFAULT,
					Vector3f{0.f, 0.f, 0.f},
					placeholder.quat,
					shapes));

			// TODO: create renderer object
		}

		// make it an rotatable platform with a joint
		engine::physics::post_joint(
			engine::physics::joint_t {
				beam.jointId,
				engine::physics::joint_t::Type::HINGE,
				engine::Entity::INVALID,
				beam.id,
				transform_t{
					placeholder.pos,
					core::maths::rotation_of(Vector3f{0.f, 0.f, 1.f})
				},
				transform_t{
					Vector3f{0.f, 0.f, 0.f},
					core::maths::rotation_of(rotate(Vector3f{0.f, 0.f, 1.f}, inverse(placeholder.quat)))},
				driveSpeed,
				forceMax});
	}

	struct model_t
	{
		struct part_t
		{
			struct render_t
			{
				struct shape_t
				{
					std::string type;
					engine::graphics::data::Color color;
					core::container::Buffer vertices;
					core::container::Buffer indices;
					core::container::Buffer normals;
				};

				std::vector<shape_t> shapes;
			};

			std::string name;
			render_t render;
		};

		std::vector<part_t> parts;

		const part_t & part(const std::string & name) const
		{
			for(const auto i : this->parts)
			{
				if (i.name==name) return i;
			}

			debug_unreachable();
		}
	};

	model_t load_model_data(const json & jcontent)
	{
		model_t model {};

		const auto & jparts = jcontent["parts"];

		for(const auto & jpart : jparts)
		{
			const std::string name = jpart["name"];
			model_t::part_t::render_t render {};
			{
				const json & jrender = jpart["render"];
				const json & jshapes = jrender["shapes"];

				for (const json & jshape:jshapes)
				{
					model_t::part_t::render_t::shape_t shape;

					shape.type = jshape["type"];

					const json & jcolor = jshape["color"];
					const uint32_t r = ((float) jcolor["r"])*255;
					const uint32_t g = ((float) jcolor["g"])*255;
					const uint32_t b = ((float) jcolor["b"])*255;
					shape.color = r + (g<<8) + (b<<16) + (0xff<<24);

					const json & jindices = jshape["indices"];
					const json & jnormals = jshape["normals"];
					const json & jvertices = jshape["vertices"];
					shape.indices.resize<unsigned int>(jindices.size());
					std::copy(jindices.begin(), jindices.end(), shape.indices.data_as<unsigned int>());

					shape.normals.resize<float>(jnormals.size());
					std::copy(jnormals.begin(), jnormals.end(), shape.normals.data_as<float>());

					shape.vertices.resize<float>(jvertices.size());
					std::copy(jvertices.begin(), jvertices.end(), shape.vertices.data_as<float>());

					render.shapes.push_back(shape);
				}
			}

			model.parts.emplace_back(model_t::part_t {name, render});
		}

		return model;
	}

	void load_droid(const placeholder_t & placeholder)
	{
		//  load model data
		const model_t model = load_model_data(json::parse(std::ifstream("res/droid.json")));

		debug_printline(0xffffffff, "Droid parts: %d", model.parts.size());
		for (auto part:model.parts)
		{
			debug_printline(0xffffffff, "part: %s", part.name);

			for (auto shape:part.render.shapes)
			{
				engine::Entity id = engine::Entity::create();

				engine::graphics::renderer::add(id, engine::graphics::data::MeshC {
					shape.vertices, // vertices
					shape.indices, // triangles
					shape.normals, // normals
					shape.color}); // color

				const core::maths::Matrix4x4f matrix =
					make_translation_matrix(core::maths::Vector3f {15.f, 2.f, 0.f}) *
					make_matrix(core::maths::Quaternionf {1.f, 0.f, 0.f, 0.f});

				engine::graphics::renderer::update(id, engine::graphics::data::ModelviewMatrix {matrix});
			}
		}
	}
}

namespace gameplay
{
namespace level
{
	void load(const placeholder_t & placeholder, const std::string & type)
	{
		if (type == "turret")
		{
			load_turret(placeholder);
		}
		else
		if (type == "beam")
		{
			load_beam(placeholder);
		}
		else
		if (type=="droid")
		{
			load_droid(placeholder);
		}
		else
		{
			debug_printline(0xffffffff, "Warning: Placeholder is unknown: ", placeholder.name);
		}
	}
}
}
