
#include "collision.hpp"

#include <core/debug.hpp>
#include <core/maths/Matrix.hpp>
#include <core/maths/algorithm.hpp>
#include <engine/graphics/opengl.hpp>
#include <engine/graphics/opengl/Matrix.hpp>

#include <vector>

namespace
{
	int read_buffer = 0;
	int write_buffer = 1;

	engine::physics::Plane planes[3]; // ground, slope and wall
	engine::physics::Rectangle me;

	enum class State
	{
		INTERSECTION,
		COLLISION,
		EVERYTHING_IS_OKAY
	};

	struct Collision
	{
		engine::physics::Body & body1;
		engine::physics::Body & body2;

		core::maths::Vector2d point;
		core::maths::Vector2d normal;

		Collision(engine::physics::Body & body1,
		          engine::physics::Body & body2,
		          core::maths::Vector2d point,
		          core::maths::Vector2d normal) :
			body1(body1),
			body2(body2),
			point{point},
			normal{normal}
		{
		}
	};

	std::vector<Collision> collisions;

	void compute_forces()
	{
		// note: so far only `me` does this
		auto & nb = me.body.buffer[write_buffer];

		nb.force.set(0., -9.82 * me.body.mass);
		nb.torque = 0.;
	}
	void update_objects(const double dt)
	{
		// note: so far only `me` does this
		const auto & ob = me.body.buffer[read_buffer];
		auto & nb = me.body.buffer[write_buffer];

		// note: the error in these equations are too big, we need to use
		// something better; like velocity verlot
		nb.position = ob.position + dt * ob.velocity;
		nb.rotation = ob.rotation + dt * ob.angularv;
		nb.velocity = ob.velocity + dt / me.body.mass * ob.force;
		nb.angularv = ob.angularv + dt / me.body.mofi * ob.torque;
	}

	State check_for_collisions()
	{
		collisions.clear();
		State state = State::EVERYTHING_IS_OKAY;

		// note: so far we only check if `me` has collided with anything...
		const auto & nb = me.body.buffer[write_buffer];

		const auto rotation = core::maths::Matrix2x2d::rotation(core::maths::make_radian(nb.rotation));
		const auto r = rotation * core::maths::Vector2d{me.width / 2., 0.};
		const auto s = rotation * core::maths::Vector2d{0., me.height / 2.};

		// note: ...and that anything is any plane
		for (auto && plane : planes)
		{
			// compute the effective radius
			const auto rn = dot(r, plane.normal).get(); // dot returns Scalar<T>
			const auto sn = dot(s, plane.normal).get();
			const auto reff = std::abs(rn) + std::abs(sn);
			// distance = ax + by + cz + d
			const auto distance = dot(nb.position, plane.normal).get() + (plane.d - reff); // dot returns Scalar<T>
			// debug_printline(0xffffffff, distance);

			const auto collision_boundary = .1;
			if (distance < -collision_boundary) // intersection
			{
				return State::INTERSECTION;
			}
			if (distance < collision_boundary) // collision
			{
				const auto rd_ = rn < 0. ? r : core::maths::Vector2d{0., 0.}-r; // direction of left/right
				const auto sd_ = sn < 0. ? s : core::maths::Vector2d{0., 0.}-s; // direction of bottom/top

				const auto Position = nb.position + rd_ + sd_;

				const auto CMToCornerPerp = get_perp(Position - nb.position);

				const auto Velocity = nb.velocity + nb.angularv * CMToCornerPerp;

				const auto relvel = dot(Velocity, plane.normal).get();
				debug_printline(0xffffffff, "relvel=", relvel);

				if (relvel < 0.) // moving into
				{
					// note: we need to do something like this in order to
					// detect edge-collisions and not only vertex collisions
					// const auto edge_boundary = 0.0001;
					// if (std::abs(rn) < edge_boundary) // bottom/top edge collision
					// {
					// 	const auto sd = sn < 0. ? sn : -sn; // direction of bottom/top

					// 	collisions.emplace_back(me.body, ground.body,
					// 	                        nb.position + sd * s - r,
					// 	                        nb.position + sd * s + r,
					// 	                        ground.normal);
					// }
					// else if (std::abs(sn) < edge_boundary) // left/right edge collision
					// {
					// 	const auto rd = rn < 0. ? rn : -rn; // direction of left/right

					// 	collisions.emplace_back(me.body, ground.body,
					// 	                        nb.position + rd * r - s,
					// 	                        nb.position + rd * r + s,
					// 	                        ground.normal);
					// }
					// else // vertex collision
					{
						const auto rd = rn < 0. ? r : core::maths::Vector2d{0., 0.}-r; // direction of left/right
						const auto sd = sn < 0. ? s : core::maths::Vector2d{0., 0.}-s; // direction of bottom/top

						collisions.emplace_back(me.body, plane.body,
						                        nb.position + rd + sd,
						                        plane.normal);
					}
					state = State::COLLISION;
				}
			}
		}
		return state;
	}
	template <typename T>
	auto square(T && thing) -> decltype(thing * thing)
	{
		return thing * thing;
	}
	void resolve_collisions()
	{
		debug_printline(0xffffffff, "#collisions=", collisions.size());
		const auto e = 0.7; // 0 <= e <= 1

		for (auto && collision : collisions)
		{
			// note: this code only handles collisions with immovable objects
			auto &nb = collision.body1.buffer[write_buffer];

			const auto Position = collision.point;

			const auto CMToCornerPerp = get_perp(Position - nb.position);

			const auto Velocity = nb.velocity + nb.angularv * CMToCornerPerp;

			const auto ImpulseNumerator = -(1. + e) * dot(Velocity, collision.normal).get();

			const auto PerpDot = dot(CMToCornerPerp, collision.normal).get();

			const auto ImpulseDenominator = (1. / collision.body1.mass) + (1. / collision.body1.mofi) * PerpDot * PerpDot;

			const auto Impulse = ImpulseNumerator / ImpulseDenominator;
			debug_printline(0xffffffff, "impulse=", Impulse);

			nb.velocity += Impulse * (1. / collision.body1.mass) * collision.normal;

			nb.angularv += Impulse * (1. / collision.body1.mofi) * PerpDot;

			// note: something like this is what we should do instead
			// const auto body1_collision_point = collision.point - collision.body1.buffer[write_buffer].position;
			// const auto body2_collision_point = collision.point - collision.body2.buffer[write_buffer].position;

			// // TODO compute collision.relvel
			// const auto point_perp = get_perp(body1_collision_point, collision.normal);
			// const auto relvel = collision.body1.buffer[write_buffer].velocity + collision.body1.buffer[write_buffer].angularv * point_perp;

			// const auto body1_perp_dot = dot(get_perp(body1_collision_point, collision.normal), collision.normal).get(); // dot returns Scalar<T>
			// const auto body2_perp_dot = dot(get_perp(body2_collision_point, collision.normal), collision.normal).get();

			// const auto enumerator =
			// 	-(1. + e) * dot(relvel, collision.normal).get(); // dot returns Scalar<T>
			// const auto denominator =
			// 	(1. / collision.body1.mass + 1. / collision.body2.mass) +
			// 	(square(body1_perp_dot) / collision.body1.mofi) +
			// 	(square(body2_perp_dot) / collision.body2.mofi);
			// const auto impulse = enumerator / denominator;

			// debug_printline(0xffffffff, impulse);

			// collision.body1.buffer[write_buffer].velocity += impulse / collision.body1.mass * collision.normal;
			// collision.body1.buffer[write_buffer].angularv += impulse / collision.body1.mofi * body2_perp_dot;
			// collision.body2.buffer[write_buffer].velocity -= impulse / collision.body2.mass * collision.normal;
			// collision.body2.buffer[write_buffer].angularv -= impulse / collision.body2.mofi * body2_perp_dot;
		}
	}
}

namespace engine
{
	namespace physics
	{
		void init()
		{
			// note: the drawing of the planes are hard coded and do not depend
			// on the values written here
			// ground
			planes[0].normal.set(0., 1.);
			planes[0].d = -dot(planes[0].normal, core::maths::Vector2d{0., 0.}).get(); // dot returns Scalar<T>

			planes[0].body.mass = std::numeric_limits<double>::infinity();
			planes[0].body.mofi = std::numeric_limits<double>::infinity();
			// slope
			planes[1].normal = normalize(core::maths::Vector2d{1., 2.});
			planes[1].d = -dot(planes[1].normal, core::maths::Vector2d{0., 0.}).get(); // dot returns Scalar<T>

			planes[1].body.mass = std::numeric_limits<double>::infinity();
			planes[1].body.mofi = std::numeric_limits<double>::infinity();
			// wall
			planes[2].normal.set(-1., 0.);
			planes[2].d = -dot(planes[2].normal, core::maths::Vector2d{50., 0.}).get(); // dot returns Scalar<T>

			planes[2].body.mass = std::numeric_limits<double>::infinity();
			planes[2].body.mofi = std::numeric_limits<double>::infinity();

			// me
			me.width = 20.;
			me.height = 10.;

			const double density = 3.;
			me.body.mass = density * me.width * me.height;
			me.body.mofi = (me.body.mass / 12.) * (square(me.width) + square(me.height));

			me.body.buffer[read_buffer].position.set(-37., 57.);
			me.body.buffer[read_buffer].rotation = 0.1; // radians
			me.body.buffer[read_buffer].velocity.set(0., 0.);
			me.body.buffer[read_buffer].angularv = 0.1; // radians/s
			// note: no need to set force and torque since they will be
			// initialized in `compute_forces`
		}

		// note: this function should not exist; everything should be rendered
		// directly in the renderer
		void draw()
		{
			glLineWidth(1.f);
			glBegin(GL_LINES);
			glColor3ub(191, 191, 191);
			for (int xi = -100; xi <= +100; xi += 10)
			{
				glVertex3f(float(xi),   0.f, 0.f);
				glVertex3f(float(xi), 100.f, 0.f);
			}
			for (int yi = 0; yi <= +100; yi += 10)
			{
				glVertex3f(-100.f, float(yi), 0.f);
				glVertex3f(+100.f, float(yi), 0.f);
			}
			glEnd();

			// ground
			glLineWidth(4.f);
			glBegin(GL_LINES);
			glColor3ub(255, 0, 255);
			glVertex3f(-1000.f, 0.f, 0.f);
			glVertex3f(+1000.f, 0.f, 0.f);
			glEnd();
			// slope
			glLineWidth(4.f);
			glBegin(GL_LINES);
			glColor3ub(255, 0, 255);
			glVertex3f(-1000.f, +500.f, 0.f);
			glVertex3f(+1000.f, -500.f, 0.f);
			glEnd();
			// wall
			glLineWidth(4.f);
			glBegin(GL_LINES);
			glColor3ub(255, 0, 255);
			glVertex3f(50.f, -1000.f, 0.f);
			glVertex3f(50.f, +1000.f, 0.f);
			glEnd();

			// me
			glLineWidth(4.f);
			core::maths::Vector2d::array_type pos;
			me.body.buffer[read_buffer].position.get(pos);
			core::maths::Matrix4x4f mat;
			mat  = core::maths::Matrix4x4f::translation(float(pos[0]), float(pos[1]), 0.f);
			mat *= core::maths::Matrix4x4f::rotation(core::maths::make_radian(float(me.body.buffer[read_buffer].rotation)), 0.f, 0.f, 1.f);
			glPushMatrix();
			glMultMatrix(mat);
			glBegin(GL_LINE_LOOP);
			glColor3ub(0, 255, 0);
			glVertex3f(- me.width / 2.f, - me.height / 2.f, 0.f);
			glVertex3f(- me.width / 2.f, + me.height / 2.f, 0.f);
			glVertex3f(+ me.width / 2.f, + me.height / 2.f, 0.f);
			glVertex3f(+ me.width / 2.f, - me.height / 2.f, 0.f);
			glEnd();
			glPopMatrix();
		}

		void simulate(const double dt)
		{
			using std::swap;

			double begint = 0.;
			double endt = dt;

			while (begint < dt)
			{
				debug_assert(endt - begint > 0.00000001);

				compute_forces();
				update_objects(endt - begint);

				const State state = check_for_collisions();

				if (state == State::INTERSECTION)
				{
					debug_printline(0xffffffff, "INTERSECTION");
					endt = (endt - begint) / 2. + begint;
					continue;
				}
				if (state == State::COLLISION)
				{
					debug_printline(0xffffffff, "COLLISION");
					// note: this is a dummy variable
					int kjhs = 0;
					do
					{
						resolve_collisions();
						kjhs++;
					}
					while (check_for_collisions() == State::COLLISION &&
					       kjhs < 100);
					debug_assert(kjhs < 100);
				}

				begint = endt;
				endt = dt;

				swap(read_buffer, write_buffer);
			}
		}
	}
}
