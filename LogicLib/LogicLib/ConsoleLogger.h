#pragma once
#include <mutex>

#pragma warning( push )
#pragma warning( disable : 4244 )
#include "../../Common/conmanip.h"
using namespace conmanip;
#pragma warning( pop )

#include "../../NetworkLib/NetworkLib/Logger.h"

namespace LogicLib
{
	class ConsoleLogger : public NetworkLib::Logger
	{
	public :
		ConsoleLogger()
		{
			console_out_context ctxout;
			_conout = ctxout;
		}

		virtual ~ConsoleLogger() {};

	protected :
		virtual void error(const char* pText) override
		{
			std::lock_guard<std::mutex> guard(_lock);
			std::cout << settextcolor(console_text_colors::red) << "[ERROR] | " << pText << std::endl;
		}

		virtual void warn(const char* pText) override
		{
			std::lock_guard<std::mutex> guard(_lock);
			std::cout << settextcolor(console_text_colors::yellow) << "[WARN] | " << pText << std::endl;
		}

		virtual void debug(const char* pText) override
		{
			std::lock_guard<std::mutex> guard(_lock);
			std::cout << settextcolor(console_text_colors::light_white) << "[DEBUG] | " << pText << std::endl;
		}

		virtual void trace(const char* pText) override
		{
			std::lock_guard<std::mutex> guard(_lock);
			std::cout << settextcolor(console_text_colors::light_white) << "[TRACE] | " << pText << std::endl;
		}

		virtual void info(const char* pText) override
		{
			std::lock_guard<std::mutex> guard(_lock);
			std::cout << settextcolor(console_text_colors::green) << "[INFO] | " << pText << std::endl;
		}

	protected :
		console_out _conout;
		std::mutex _lock;

	};
}