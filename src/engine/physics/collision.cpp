
#include "collision.hpp"

#include <core/debug.hpp>
#include <core/maths/Matrix.hpp>
#include <core/maths/algorithm.hpp>
#include <engine/graphics/opengl.hpp>
#include <engine/graphics/opengl/Matrix.hpp>

#include <vector>

#define USE_FRICTION 1

namespace
{
	const double THREASHOLD = 0.1;

	int read_buffer = 0;
	int write_buffer = 1;

//	engine::physics::Plane planes[3]; // ground, slope and wall
//	engine::physics::Rectangle me;

	std::vector<engine::physics::Circle> circles;
	std::vector<engine::physics::Plane> planes;
	std::vector<engine::physics::Rectangle> rectangles;

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

	std::vector<Collision> collisionsPlane;
	std::vector<Collision> collisionCircles;

	void compute_forces(engine::physics::Body & body)
	{
		// note: so far only `me` does this
		auto & nb = body.buffer[write_buffer];

		nb.force.set(0., -9.82 * body.mass);
		nb.torque.set(0.);
	}
	void update_objects(engine::physics::Body & body, const double dt)
	{
		// note: so far only `me` does this
		const auto & ob = body.buffer[read_buffer];
		auto & nb = body.buffer[write_buffer];

		// note: the error in these equations are too big, we need to use
		// something better; like velocity verlot
		nb.position = ob.position + dt * ob.velocity;
		nb.rotation = ob.rotation + dt * ob.angularv;
		nb.velocity = ob.velocity + dt / body.mass * ob.force;
		nb.angularv = ob.angularv + dt / body.mofi * ob.torque;
	}

	//// distance Circle to Circle
	//double distance(const core::maths::Vector2d & callerCentre, const double callerRadie, const core::maths::Vector2d & targetCentre, const double targetRadie)
	//{

	//}

	//// distance Circle to Plane (also Circle vs Rectangle (called 1 - 2 times))
	//double distance(const core::maths::Vector2d & callerCentre, const double callerRadie, const core::maths::Vector2d & targetNormal)
	//{

	//}
	//// distance Rectangle to Plane (also Rectangle vs Rectangle (called 1 - 4 times))
	//double distance()
	//{

	//}


	State check_for_collisions()
	{
		collisionCircles.clear();
		collisionsPlane.clear();

		State state = State::EVERYTHING_IS_OKAY;

		// check all circles
		//for (auto & circle : circles)
		for(unsigned int i = 0; i < circles.size(); i++)
		{
			// note: so far we only check if `me` has collided with anything...
			const auto & nb1 = circles[i].body.buffer[write_buffer];

			// 
			for (unsigned int j = i + 1; j < circles.size(); j++)
			{
				const auto distanceVector = circles[j].body.buffer[write_buffer].position - circles[i].body.buffer[write_buffer].position;
				const auto distanceSquare = square(distanceVector);

				const auto radie = circles[i].radie + circles[j].radie;

				if (distanceSquare < (radie*radie - THREASHOLD)) // intersection
					return State::INTERSECTION;

				if (distanceSquare < (radie*radie + THREASHOLD)) // collision
				{
					const auto & nb2 = circles[j].body.buffer[write_buffer];

					const auto relvelVec = nb2.velocity - nb1.velocity;
					const auto relvel = dot(relvelVec, distanceVector).get();


					if (relvel < 0.) // moving into
					{
					//	const auto rd = rn < 0. ? r : core::maths::Vector2d{ 0., 0. }-r; // direction of left/right

						const auto deltaN = distanceVector / std::sqrt(distanceSquare);

						collisionCircles.emplace_back(circles[i].body, circles[j].body, nb1.position + deltaN*circles[i].radie, core::maths::Vector2d{ 0., 0. }-deltaN);

						state = State::COLLISION;
					}
				}
			}

			// note: ...and that anything is any plane
			for (auto && plane : planes)
			{
				const auto r = plane.plane.normal() * (-circles[i].radie);

				// compute the effective radius
				const auto rn = dot(r, plane.plane.normal());
				const auto reff = abs(rn);
				// distance = ax + by + cz + d
				const auto dist = distance(plane.plane, nb1.position) - reff;

				if (dist < -THREASHOLD) // intersection
					return State::INTERSECTION;
				
				if (dist < THREASHOLD) // collision
				{
					const auto rd_ = rn < 0. ? r : -r; // direction of left/right

					const auto Position = nb1.position + rd_;

					const auto CMToCornerPerp = get_perp(Position - nb1.position);

					const auto Velocity = nb1.velocity + nb1.angularv * CMToCornerPerp;

					const auto relvel = dot(Velocity, plane.plane.normal());
					// debug_printline(0xffffffff, "relvel=", relvel);

					if (relvel < 0.) // moving into
					{
						{
							const auto rd = rn < 0. ? r : -r; // direction of left/right

							collisionsPlane.emplace_back(circles[i].body, plane.body,
							                        nb1.position + rd,
							                        plane.plane.normal());
						}

						state = State::COLLISION;
					}
				}
			}
		}

		// check all rectangles
		for (auto & rectangle : rectangles)
		{
			// note: so far we only check if `me` has collided with anything...
			const auto & nb = rectangle.body.buffer[write_buffer];

			const auto rotation = core::maths::Matrix2x2d::rotation(core::maths::radiand{nb.rotation.get()});
			const auto r = rotation * core::maths::Vector2d{ rectangle.width / 2., 0. };
			const auto s = rotation * core::maths::Vector2d{ 0., rectangle.height / 2. };

			// note: ...and that anything is any plane
			for (auto && plane : planes)
			{
				// compute the effective radius
				const auto rn = dot(r, plane.plane.normal());
				const auto sn = dot(s, plane.plane.normal());
				const auto reff = abs(rn) + abs(sn);
				// distance = ax + by + cz + d
				const auto dist = distance(plane.plane, nb.position) - reff;

				const auto collision_boundary = .1;
				if (dist < -collision_boundary) // intersection
				{
					return State::INTERSECTION;
				}
				if (dist < collision_boundary) // collision
				{
					const auto rd_ = rn < 0. ? r : -r; // direction of left/right
					const auto sd_ = sn < 0. ? s : -s; // direction of bottom/top

					const auto Position = nb.position + rd_ + sd_;

					const auto CMToCornerPerp = get_perp(Position - nb.position);

					const auto Velocity = nb.velocity + nb.angularv * CMToCornerPerp;

					const auto relvel = dot(Velocity, plane.plane.normal());
					// debug_printline(0xffffffff, "relvel=", relvel);

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
							const auto rd = rn < 0. ? r : -r; // direction of left/right
							const auto sd = sn < 0. ? s : -s; // direction of bottom/top


							collisionsPlane.emplace_back(rectangle.body, plane.body,
							                        nb.position + rd + sd,
							                        plane.plane.normal());
						}
						state = State::COLLISION;
					}
				}
			}
		}
		return state;
	}
	void resolve_collisions()
	{
	//	debug_printline(0xffffffff, "#collisions=", collisions.size());
		const auto e = 0.7; // 0 <= e <= 1
		const auto u = 0.4; // 0 <= u <= 1

		for (auto && collision : collisionsPlane)
		{
			auto &nb = collision.body1.buffer[write_buffer];
			const auto CMToCornerPerp = get_perp(collision.point - nb.position);

			
			// note: this code only handles collisions with immovable objects

			const auto Velocity = nb.velocity + nb.angularv * CMToCornerPerp;

			const auto ImpulseNumerator = -(1. + e) * dot(Velocity, collision.normal);

	        const auto perpN = dot(CMToCornerPerp, collision.normal);

			const auto ImpulseDenominator = (1. / collision.body1.mass) + (1. / collision.body1.mofi) * perpN * perpN;

			const auto Impulse = ImpulseNumerator / ImpulseDenominator;

		//	debug_printline(0xffffffff, "impulse=", Impulse);

			nb.velocity += Impulse * (1. / collision.body1.mass) * collision.normal;
			nb.angularv += Impulse * (1. / collision.body1.mofi) * perpN;

#if USE_FRICTION
			
			// friction

			core::maths::Vector2d tangentN;	// two ways to determine it
			{
				//// Has issue with Circles when they are moving along surfaces, bouncing is fine
				//// calculate the tangent of collision normal (direction is important)
				//const auto tangent = Velocity - dot(Velocity, collision.normal) * collision.normal;

				//// debug
				//const auto debugLength = length(tangent);

				//if (debugLength == 0.)	// this keeps happening with Circle, NEVER with rectangle!
				//{
				//	debug_printline(0xffffffff, "no friction vector");
				//	continue;
				//}

				//// http://gamedevelopment.tutsplus.com/tutorials/how-to-create-a-custom-2d-physics-engine-friction-scene-and-jump-table--gamedev-7756
				//tangentN = normalize(tangent);
			}
			{
				core::maths::Vector2d::array_type pos;
				collision.normal.get(pos);

				// alternative to above
				if (cross2(Velocity, collision.normal) < 0.)
					tangentN = core::maths::Vector2d(-pos[1], pos[0]);
				else
					tangentN = core::maths::Vector2d(pos[1], -pos[0]);
			}
					
			// 
			const auto perpTan = dot(CMToCornerPerp, tangentN).get();

			// Solve for magnitude to apply along the friction vector
			const auto frictionImpNumerator = -dot(Velocity, tangentN).get();
			const auto frictionImpDenominator = (1. / collision.body1.mass) + 
												(1. / collision.body1.mofi) * perpTan * perpTan;
			
			const auto frictionImpulse = frictionImpNumerator / frictionImpDenominator;
			const auto frictionLimit = Impulse*u;
			
			if (std::abs(frictionImpulse) < frictionLimit)
			{
				debug_printline(0xffffffff, "friction NOT limited");
				nb.velocity += frictionImpulse * (1. / collision.body1.mass) * tangentN;
				nb.angularv += frictionImpulse * (1. / collision.body1.mofi) * perpTan;
			}
			else
			{
				debug_printline(0xffffffff, "frition LIMITED");
				nb.velocity -= frictionLimit * (1. / collision.body1.mass) * tangentN;
				nb.angularv -= frictionLimit * (1. / collision.body1.mofi) * perpTan;
			}

			// test - make sure movement is sufficient
			{
				const auto lvel = nb.velocity + nb.angularv * CMToCornerPerp;

				const auto relvel = dot(lvel, collision.normal).get();

				if (relvel < 0.) // moving into
				{
					debug_printline(0xffffffff, "After impulse comp. still moving towards each others!");

				//	const auto compensate = relvel*collision.normal;
				//	nb.velocity -= compensate;
				//	nb.angularv -= relvel;
				}
			}
#endif
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

		for (auto && collision : collisionCircles)
		{
			debug_printline(0xffffffff, "non single collision");

			auto &nb1 = collision.body1.buffer[write_buffer];
			auto &nb2 = collision.body2.buffer[write_buffer];

			const auto CMToCornerPerp1 = get_perp(collision.point - nb1.position);
			const auto CMToCornerPerp2 = get_perp(collision.point - nb2.position);

			// note: this code only handles collisions with immovable objects

			const auto Velocity1 = nb1.velocity + nb1.angularv * CMToCornerPerp1;
			const auto Velocity2 = nb2.velocity + nb2.angularv * CMToCornerPerp2;
			const auto velBoth = Velocity2 - Velocity1;

			const auto ImpulseNumerator = -(1. + e) * dot(velBoth, collision.normal).get();

			const auto perpN1 = dot(CMToCornerPerp1, collision.normal).get();
			const auto perpN2 = dot(CMToCornerPerp2, collision.normal).get();

			const auto mass = (1. / collision.body1.mass) + (1. / collision.body2.mass);
		
			const auto ImpulseDenominator = mass +	(1. / collision.body1.mofi) * perpN1 * perpN1 + 
													(1. / collision.body2.mofi) * perpN2 * perpN2;

			const auto Impulse = ImpulseNumerator / ImpulseDenominator;

			//	debug_printline(0xffffffff, "impulse=", Impulse);

			nb1.velocity -= Impulse * (1. / collision.body1.mass) * collision.normal;
			nb1.angularv -= Impulse * (1. / collision.body1.mofi) * perpN1;
			nb2.velocity += Impulse * (1. / collision.body2.mass) * collision.normal;
			nb2.angularv += Impulse * (1. / collision.body2.mofi) * perpN2;

#if USE_FRICTION
			//// friction - somehow make it work for two objects

			//core::maths::Vector2d tangentN1;
			//core::maths::Vector2d tangentN2;

			//core::maths::Vector2d::array_type collisionNormal;
			//collision.normal.get(collisionNormal);

			//if (cross2(Velocity1, collision.normal) < 0.)
			//	tangentN1 = core::maths::Vector2d(-collisionNormal[1], collisionNormal[0]);
			//else
			//	tangentN1 = core::maths::Vector2d(collisionNormal[1], -collisionNormal[0]);

			//if (cross2(Velocity2, collision.normal) < 0.)
			//	tangentN2 = core::maths::Vector2d(-collisionNormal[1], collisionNormal[0]);
			//else
			//	tangentN2 = core::maths::Vector2d(collisionNormal[1], -collisionNormal[0]);

			//// 
			//const auto perpTan1 = dot(CMToCornerPerp1, tangentN1).get();
			//const auto perpTan2 = dot(CMToCornerPerp2, tangentN2).get();

			//// Solve for magnitude to apply along the friction vector
			//const auto frictionImpNumerator = -dot(velBoth, tangentN1).get();
			//const auto frictionImpDenominator = (1. / collision.body1.mass) +
			//	(1. / collision.body1.mofi) * perpTan * perpTan;

			//const auto frictionImpulse = frictionImpNumerator / frictionImpDenominator;
			//const auto frictionLimit = Impulse*u;

			//if (std::abs(frictionImpulse) < frictionLimit)
			//{
			//	debug_printline(0xffffffff, "friction NOT limited");
			//	nb1.velocity += frictionImpulse * (1. / collision.body1.mass) * tangentN;
			//	nb1.angularv += frictionImpulse * (1. / collision.body1.mofi) * perpTan;
			//}
			//else
			//{
			//	debug_printline(0xffffffff, "frition LIMITED");
			//	nb.velocity -= frictionLimit * (1. / collision.body1.mass) * tangentN;
			//	nb.angularv -= frictionLimit * (1. / collision.body1.mofi) * perpTan;
			//}
#endif
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
			planes.emplace_back();
			planes.emplace_back();
			planes.emplace_back();

			planes[0].plane = core::maths::Plane2d{0., 1., 0.};
			planes[0].plane.normalize();

			planes[0].body.mass = std::numeric_limits<double>::infinity();
			planes[0].body.mofi = std::numeric_limits<double>::infinity();
			// slope
			planes[1].plane = core::maths::Plane2d{1., 2., 0.};
			planes[1].plane.normalize();

			planes[1].body.mass = std::numeric_limits<double>::infinity();
			planes[1].body.mofi = std::numeric_limits<double>::infinity();
			// wall
			planes[2].plane = core::maths::Plane2d{-1., 0., 60.};
			planes[2].plane.normalize();

			planes[2].body.mass = std::numeric_limits<double>::infinity();
			planes[2].body.mofi = std::numeric_limits<double>::infinity();

			const double density = 3.;


			//rectangles.push_back(engine::physics::Rectangle());
			//{
			//	// me
			//	rectangles.back().width = 20.;
			//	rectangles.back().height = 10.;

			//	rectangles.back().body.mass = density * rectangles.back().width * rectangles.back().height;
			//	rectangles.back().body.mofi = (rectangles.back().body.mass / 12.) * (square(rectangles.back().width) + square(rectangles.back().height));

			//	rectangles.back().body.buffer[read_buffer].position.set(20., 57.);
			//	rectangles.back().body.buffer[read_buffer].rotation = 0.1; // radians
			//	rectangles.back().body.buffer[read_buffer].velocity.set(0., 0.);
			//	rectangles.back().body.buffer[read_buffer].angularv = 0.1; // radians/s
			//}

			//rectangles.push_back(engine::physics::Rectangle());
			//{
			//	// you
			//	rectangles.back().width = 10.;
			//	rectangles.back().height = 5.;

			//	rectangles.back().body.mass = density * rectangles.back().width * rectangles.back().height;
			//	rectangles.back().body.mofi = (rectangles.back().body.mass / 12.) * (square(rectangles.back().width) + square(rectangles.back().height));

			//	rectangles.back().body.buffer[read_buffer].position.set(-20., 57.);
			//	rectangles.back().body.buffer[read_buffer].rotation = 0.1; // radians
			//	rectangles.back().body.buffer[read_buffer].velocity.set(0., 0.);
			//	rectangles.back().body.buffer[read_buffer].angularv = 0.1; // radians/s
			//}
			// note: no need to set force and torque since they will be
			// initialized in `compute_forces`
			
			circles.push_back(engine::physics::Circle());
			{
				// bob
				circles.back().radie = 10.;
				circles.back().calc_body(density);

				circles.back().body.buffer[read_buffer].position.set(-50., 50.);
				circles.back().body.buffer[read_buffer].rotation = 0.; // radians
				circles.back().body.buffer[read_buffer].velocity.set(0., 0.);
				circles.back().body.buffer[read_buffer].angularv = 0.0; // radians/s
			}

			circles.push_back(engine::physics::Circle());
			{
				circles.back().radie = 10.;
				circles.back().calc_body(density);

				circles.back().body.buffer[read_buffer].position.set(-20., 40.);
				circles.back().body.buffer[read_buffer].rotation = 0.; // radians
				circles.back().body.buffer[read_buffer].velocity.set(0., 0.);
				circles.back().body.buffer[read_buffer].angularv = 0.0; // radians/s
			}


			circles.push_back(engine::physics::Circle());
			{
				circles.back().radie = 5.;
				circles.back().calc_body(density);

				circles.back().body.buffer[read_buffer].position.set(20., 40.);
				circles.back().body.buffer[read_buffer].rotation = 0.; // radians
				circles.back().body.buffer[read_buffer].velocity.set(0., 0.);
				circles.back().body.buffer[read_buffer].angularv = 0.0; // radians/s
			}


			circles.push_back(engine::physics::Circle());
			{
				circles.back().radie = 5.;
				circles.back().calc_body(density);

				circles.back().body.buffer[read_buffer].position.set(20., 100.);
				circles.back().body.buffer[read_buffer].rotation = 0.; // radians
				circles.back().body.buffer[read_buffer].velocity.set(0., 0.);
				circles.back().body.buffer[read_buffer].angularv = 0.0; // radians/s
			}
			circles.push_back(engine::physics::Circle());
			{
				circles.back().radie = 5.;
				circles.back().calc_body(density);

				circles.back().body.buffer[read_buffer].position.set(-50., 80.);
				circles.back().body.buffer[read_buffer].rotation = 0.; // radians
				circles.back().body.buffer[read_buffer].velocity.set(0., 0.);
				circles.back().body.buffer[read_buffer].angularv = 0.0; // radians/s
			}
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

			for (const auto & plane : planes)
			{
				glLineWidth(4.f);
				glBegin(GL_LINES);
				glColor3ub(255, 0, 255);

				const auto c = plane.plane.normal() * (-plane.plane.offset());
				const auto perp = get_perp(plane.plane.normal());

				core::maths::Vector2d::array_type start;
				(c + perp * 1000).get(start);
				core::maths::Vector2d::array_type end;
				(c - perp * 1000).get(end);

				glVertex3f(start[0], start[1], 0.f);
				glVertex3f(end[0], end[1], 0.f);

				glEnd();
			}


			for (const auto & rectangle : rectangles)
			{
				glPushMatrix();
				{
					// me
					glLineWidth(4.f);
					core::maths::Vector2d::array_type pos;
					rectangle.body.buffer[read_buffer].position.get(pos);
					core::maths::Matrix4x4f mat;
					mat = core::maths::Matrix4x4f::translation(float(pos[0]), float(pos[1]), 0.f);
					mat *= core::maths::Matrix4x4f::rotation(core::maths::radianf{float(rectangle.body.buffer[read_buffer].rotation.get())}, 0.f, 0.f, 1.f);

					glMultMatrix(mat);

					glBegin(GL_LINE_LOOP);
					glColor3ub(0, 255, 0);
					glVertex3f(-rectangle.width / 2.f, -rectangle.height / 2.f, 0.f);
					glVertex3f(-rectangle.width / 2.f, +rectangle.height / 2.f, 0.f);
					glVertex3f(+rectangle.width / 2.f, +rectangle.height / 2.f, 0.f);
					glVertex3f(+rectangle.width / 2.f, -rectangle.height / 2.f, 0.f);
					glEnd();
				}
				glPopMatrix();
			}

			{
				const std::size_t SEGMENTS = 12;
				float theta = 2 * 3.1415926f / float(SEGMENTS);
				float c = cosf(theta);//precalculate the sine and cosine
				float s = sinf(theta);
				float t;

				for (const auto & circle : circles)
				{
					glPushMatrix();
					{
						core::maths::Vector2d::array_type pos;
						circle.body.buffer[read_buffer].position.get(pos);
						core::maths::Matrix4x4f mat;
						mat = core::maths::Matrix4x4f::translation(float(pos[0]), float(pos[1]), 0.f);
						mat *= core::maths::Matrix4x4f::rotation(core::maths::radianf{float(circle.body.buffer[read_buffer].rotation.get())}, 0.f, 0.f, 1.f);

						glMultMatrix(mat);

						float x = circle.radie;//we start at angle = 0 
						float y = 0;

						glBegin(GL_LINE_LOOP);
						glColor3ub(0, 255, 255);
						for (std::size_t i = 0; i < SEGMENTS; i++)
						{
							glVertex2f(x, y);//output vertex 

							//apply the rotation matrix
							t = x;
							x = c * x - s * y;
							y = s * t + c * y;
						}
						glEnd();
					}
					glPopMatrix();
				}
			}
		}

		void simulate(const double dt)
		{
			using std::swap;

			double begint = 0.;
			double endt = dt;

			while (begint < dt)
			{
				debug_assert(endt - begint > 0.00000001);

				for (auto & val : rectangles)
				{
					compute_forces(val.body);
					update_objects(val.body, endt - begint);
				}

				for (auto & val : circles)
				{
					compute_forces(val.body);
					update_objects(val.body, endt - begint);
				}

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
