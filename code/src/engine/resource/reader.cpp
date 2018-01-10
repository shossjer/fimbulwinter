
#include "reader.hpp"

#include <core/async/Thread.hpp>
#include <core/container/CircleQueue.hpp>
#include <core/sync/Event.hpp>

#include <engine/debug.hpp>

#include <utility/string.hpp>
#include <utility/variant.hpp>

#include <fstream>

namespace
{
	bool file_exists(const std::string & filename)
	{
		std::ifstream stream(filename, std::ifstream::binary);
		return !!stream;
	}

	bool check_if_json(const std::string & name)
	{
		return file_exists(utility::to_string("res/", name, ".json"));
	}
	bool check_if_msh(const std::string & name)
	{
		return file_exists(utility::to_string("res/", name, ".msh"));
	}

	void read_count(std::ifstream & stream, uint16_t & count)
	{
		stream.read(reinterpret_cast<char *>(& count), sizeof(uint16_t));
	}
	void read_length(std::ifstream & stream, int32_t & length)
	{
		stream.read(reinterpret_cast<char *>(& length), sizeof(int32_t));
	}
	void read_matrix(std::ifstream & stream, core::maths::Matrix4x4f & matrix)
	{
		core::maths::Matrix4x4f::array_type buffer;
		stream.read(reinterpret_cast<char *>(buffer), sizeof(buffer));
		matrix.set_aligned(buffer);
	}
	void read_quaternion(std::ifstream & stream, core::maths::Quaternionf & quat)
	{
		core::maths::Quaternionf::array_type buffer;
		stream.read(reinterpret_cast<char *>(buffer), sizeof(buffer));
		quat.set_aligned(buffer);
	}
	template <std::size_t N>
	void read_string(std::ifstream & stream, char (& buffer)[N])
	{
		uint16_t len; // excluding null character
		stream.read(reinterpret_cast<char *>(& len), sizeof(uint16_t));
		debug_assert(len < N);

		stream.read(buffer, len);
		buffer[len] = '\0';
	}
	void read_string(std::ifstream & stream, std::string & str)
	{
		char buffer[64]; // arbitrary
		read_string(stream, buffer);
		str = buffer;
	}
	void read_vector(std::ifstream & stream, float (&buffer)[3])
	{
		stream.read(reinterpret_cast<char *>(buffer), sizeof(buffer));
	}
	void read_vector(std::ifstream & stream, core::maths::Vector3f & vec)
	{
		core::maths::Vector3f::array_type buffer;
		stream.read(reinterpret_cast<char *>(buffer), sizeof(buffer));
		vec.set_aligned(buffer);
	}

	void read_meshes(std::ifstream & ifile, std::vector<engine::resource::reader::Level::Mesh> & meshes)
	{
		uint16_t nmeshes;
		read_count(ifile, nmeshes);

		meshes.resize(nmeshes);
		for (auto & mesh : meshes)
		{
			read_string(ifile, mesh.name);
			read_matrix(ifile, mesh.matrix);

			// color
			read_vector(ifile, mesh.color);

			std::vector<float> vertexes;
			uint16_t nvertices;
			read_count(ifile, nvertices);

			mesh.vertices.resize<float>(nvertices * 3);
			ifile.read(mesh.vertices.data(), mesh.vertices.size());

			uint16_t nnormals;
			read_count(ifile, nnormals);
			debug_assert(nvertices == nnormals);
			mesh.normals.resize<float>(nnormals * 3);
			ifile.read(mesh.normals.data(), mesh.normals.size());

			uint16_t ntriangles;
			read_count(ifile, ntriangles);
			mesh.triangles.resize<uint16_t>(ntriangles * 3);
			ifile.read(mesh.triangles.data(), mesh.triangles.size());
		}
	}

	void read_placeholders(std::ifstream & ifile, std::vector<engine::resource::reader::Level::Placeholder> & placeholders)
	{
		uint16_t nplaceholders;
		read_count(ifile, nplaceholders);

		placeholders.resize(nplaceholders);
		for (auto & placeholder : placeholders)
		{
			read_string(ifile, placeholder.name);
			read_vector(ifile, placeholder.translation);
			read_quaternion(ifile, placeholder.rotation);
			read_vector(ifile, placeholder.scale);
		}
	}

	void read_level(std::ifstream & ifile, engine::resource::reader::Level & level)
	{
		read_meshes(ifile, level.meshes);

		read_placeholders(ifile, level.placeholders);
	}

	template <class T, std::size_t N>
	void read_buffer(std::ifstream & stream, core::container::Buffer & buffer)
	{
		uint16_t count;
		read_count(stream, count);

		buffer.resize<T>(N * count);
		stream.read(buffer.data(), buffer.size());
	}

	void read_weights(std::ifstream & ifile, engine::model::mesh_t & mesh)
	{
		uint16_t weights;
		read_count(ifile, weights);

		mesh.weights.resize(weights);
		for (auto && weight : mesh.weights)
		{
			uint16_t ngroups;
			read_count(ifile, ngroups);

			debug_assert(ngroups > 0);

			// read the first weight value
			read_count(ifile, weight.index);
			ifile.read(reinterpret_cast<char *>(&weight.value), sizeof(float));

			// skip remaining ones if any for now
			for (auto i = 1; i < ngroups; i++)
			{
				uint16_t unused;
				read_count(ifile, unused);

				float value;
				ifile.read(reinterpret_cast<char *>(&value), sizeof(float));
			}
		}
	}

	void read_mesh(std::ifstream & ifile, engine::model::mesh_t & mesh)
	{
		std::string name;
		read_string(ifile, name);
		debug_printline(engine::resource_channel, "mesh name: ", name);

		read_matrix(ifile, mesh.matrix);

		read_buffer<float, 3>(ifile, mesh.xyz);
		debug_printline(engine::resource_channel, "mesh vertices: ", mesh.xyz.count() / 3);

		read_buffer<float, 2>(ifile, mesh.uv);
		debug_printline(engine::resource_channel, "mesh uv's: ", mesh.uv.count() / 2);

		read_buffer<float, 3>(ifile, mesh.normals);
		debug_printline(engine::resource_channel, "mesh normals: ", mesh.normals.count() / 3);

		read_weights(ifile, mesh);
		debug_printline(engine::resource_channel, "mesh weights: ", mesh.weights.size());

		read_buffer<unsigned int, 3>(ifile, mesh.triangles);
		debug_printline(engine::resource_channel, "mesh triangles: ", mesh.triangles.count() / 3);
	}

	void read_json(std::ifstream & ifile, json & j)
	{
		j = json::parse(ifile);
	}
}

namespace
{
	struct MessageReadData
	{
		std::string name;
		void (* callback)(std::string name, engine::resource::reader::Data && data);
	};
	struct MessageReadLevel
	{
		std::string name;
		void (* callback)(std::string name, engine::resource::reader::Level && data);
	};
	struct MessageReadPlaceholder
	{
		std::string name;
		void (* callback)(std::string name, engine::resource::reader::Placeholder && data);
	};
	using Message = utility::variant
	<
		MessageReadData,
		MessageReadLevel,
		MessageReadPlaceholder
	>;

	core::container::CircleQueueSRMW<Message, 100> queue_messages;
}

namespace
{
	void process_messages()
	{
		Message message;
		while (queue_messages.try_pop(message))
		{
			struct ProcessMessage
			{
				void operator () (MessageReadData && x)
				{
					if (check_if_json(x.name))
					{
						std::ifstream ifile("res/" + x.name + ".json");
						debug_assert(ifile);

						json j;
						read_json(ifile, j);
						x.callback(std::move(x.name), engine::resource::reader::Data(std::move(j)));
					}
					else
					{
						debug_fail();
					}
				}
				void operator () (MessageReadLevel && x)
				{
					engine::resource::reader::Level level;
					{
						std::ifstream ifile(utility::to_string("res/", x.name, ".lvl"), std::ifstream::binary);
						debug_assert(ifile);

						read_level(ifile, level);
					}
					x.callback(std::move(x.name), std::move(level));
				}
				void operator () (MessageReadPlaceholder && x)
				{
					if (check_if_msh(x.name))
					{
						std::ifstream ifile(utility::to_string("res/", x.name, ".msh"), std::ifstream::binary);
						debug_assert(ifile);

						engine::model::mesh_t mesh;
						read_mesh(ifile, mesh);
						x.callback(std::move(x.name), engine::resource::reader::Placeholder(std::move(mesh)));
					}
					else if (check_if_json(x.name))
					{
						std::ifstream ifile("res/" + x.name + ".json");
						debug_assert(ifile);

						json j;
						read_json(ifile, j);
						x.callback(std::move(x.name), engine::resource::reader::Placeholder(std::move(j)));
					}
					else
					{
						debug_fail();
					}
				}
			};
			visit(ProcessMessage{}, std::move(message));
		}
	}

	core::async::Thread renderThread;
	volatile bool active = false;
	core::sync::Event<true> event;

	void run()
	{
		event.wait();
		event.reset();

		while (active)
		{
			process_messages();

			event.wait();
			event.reset();
		}
	}

	template <typename MessageType, typename ...Ps>
	void post_message(Ps && ...ps)
	{
		const auto res = queue_messages.try_emplace(utility::in_place_type<MessageType>, std::forward<Ps>(ps)...);
		debug_assert(res);
		if (res)
		{
			event.set();
		}
	}
}

namespace engine
{
	namespace resource
	{
		namespace reader
		{
			void create()
			{
				debug_assert(!active);

				active = true;
				renderThread = core::async::Thread{ run };
			}

			void destroy()
			{
				debug_assert(active);

				active = false;
				event.set();

				renderThread.join();
			}

			void post_read_data(std::string name, void (* callback)(std::string name, Data && data))
			{
				post_message<MessageReadData>(std::move(name), callback);
			}
			void post_read_level(std::string name, void (* callback)(std::string name, Level && data))
			{
				post_message<MessageReadLevel>(std::move(name), callback);
			}
			void post_read_placeholder(std::string name, void (* callback)(std::string name, Placeholder && data))
			{
				post_message<MessageReadPlaceholder>(std::move(name), callback);
			}
		}
	}
}