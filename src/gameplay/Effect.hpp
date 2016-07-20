
namespace gameplay
{
namespace effects
{
	class Effect
	{
	public:
		/**
		 *	
		 */
		virtual bool finished() = 0;
		/**
		 *
		 */
		virtual void update() = 0;
	};
}
}
