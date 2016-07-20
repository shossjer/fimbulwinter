
namespace gameplay
{
namespace effects
{
	typedef unsigned int Id;

	void update();

	enum class Type
	{
		PLAYER_GRAVITY
	};

	Id create(const Type type, const Id callerId);

	void remove(const Id id);
}
}
