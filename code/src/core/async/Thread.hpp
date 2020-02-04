
#ifndef CORE_ASYNC_THREAD_HPP
#define CORE_ASYNC_THREAD_HPP

namespace core
{
	namespace async
	{
		class Thread
		{
		private:
			void * data_;

		public:
			~Thread();
			Thread();
			Thread(void (* callback)());
			Thread(Thread && other);
			Thread & operator = (Thread && other);

		public:
			bool valid() const;

			void join();
		};
	}
}

#endif /* ENGINE_ASYNC_THREAD_HPP */
