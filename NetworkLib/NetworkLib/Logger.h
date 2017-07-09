#pragma once
#include <stdio.h>
#include <stdarg.h>

namespace NetworkLib
{
	const int logStringMaxLength = 256;

	enum class LOG_TYPE : short
	{
		L_TRACE = 1,
		L_DEBUG = 2,
		L_WARN = 3,
		L_ERROR = 4,
		L_INFO = 5
	};

	class Logger
	{
	public:
		Logger() {};
		virtual ~Logger() {};

		virtual void Write(const LOG_TYPE nType, const char* pFormat, ...)
		{
			char szText[logStringMaxLength];

			va_list args;
			va_start(args, pFormat);
			vsprintf_s(szText, logStringMaxLength, pFormat, args);
			va_end(args);

			switch (nType)
			{
			case LOG_TYPE::L_INFO :
				info(szText);	
				break;
			case LOG_TYPE::L_ERROR:
				error(szText);
				break;
			case LOG_TYPE::L_WARN:
				warn(szText);
				break;
			case LOG_TYPE::L_DEBUG:
				debug(szText);
				break;
			case LOG_TYPE::L_TRACE:
				info(szText);
				break;
			default:
				break;
			}
		}

	private :
		virtual void error(const char* pText) = 0;
		virtual void warn(const char* pText) = 0;
		virtual void debug(const char* pText) = 0;
		virtual void trace(const char* pText) = 0;
		virtual void info(const char* pText) = 0;
	};
}

