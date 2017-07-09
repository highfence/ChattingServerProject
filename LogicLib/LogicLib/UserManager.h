#pragma once
#include <unordered_map>
#include <deque>
#include <string>

namespace NCommon
{
	enum class ERROR_CODE : short;
}

using ERROR_CODE = NCommon::ERROR_CODE;

namespace LogicLib
{
	class User;

	class UserManager
	{
	public :
		UserManager();
		virtual ~UserManager();

		void Init(const int maxUserCount);

		ERROR_CODE AddUser(const int sessionIndex, const char * pszId);
		ERROR_CODE RemoveUser(const int sessionIndex);

		std::tuple<ERROR_CODE, User*> GetUser(const int sessionIndex);

	private :

		User * allocUserObjPoolIndex();
		void releaseUserObjPoolIndex(const int index);

		User * findUser(const int sessionIndex);
		User * findUser(const char * pszID);

	private :

		std::vector<User> _userObjPool;
		std::deque<int> _userObjPoolIndex;

		std::unordered_map<int, User*> _userSessionDic;
		// char* 는 key로 사용하지 못한다.
		std::unordered_map<const char*, User*> _userIdDic;
	};
}