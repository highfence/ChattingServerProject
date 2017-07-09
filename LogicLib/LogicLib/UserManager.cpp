#include "../../Common/ErrorCode.h"
#include "User.h"
#include "UserManager.h"

namespace LogicLib
{
	UserManager::UserManager()
	{

	}

	UserManager::~UserManager()
	{

	}

	void UserManager::Init(const int maxUserCount)
	{
		for (int i = 0; i < maxUserCount; ++i)
		{
			User user;
			user.Init(static_cast<short>(i));

			_userObjPool.push_back(user);
			_userObjPoolIndex.push_back(i);
		}
	}

	ERROR_CODE UserManager::AddUser(const int sessionIndex, const char * pszId)
	{
		if (findUser(pszId) != nullptr)
		{
			return ERROR_CODE::USER_MGR_ID_DUPLICATION;
		}

		auto pUser = allocUserObjPoolIndex();
		if (pUser == nullptr)
		{
			return ERROR_CODE::USER_MGR_MAX_USER_COUNT;
		}

		pUser->Set(sessionIndex, pszId);

		_userSessionDic.insert({ sessionIndex, pUser });
		_userIdDic.insert({ pszId, pUser });

		return ERROR_CODE::NONE;
	}

	ERROR_CODE UserManager::RemoveUser(const int sessionIndex)
	{
		auto pUser = findUser(sessionIndex);

		if (pUser == nullptr)
		{
			return ERROR_CODE::USER_MGR_REMOVE_INVALID_SESSION;
		}

		auto index = pUser->GetIndex();
		auto pszId = pUser->GetId();

		_userSessionDic.erase(sessionIndex);
		_userIdDic.erase(pszId.c_str());
		releaseUserObjPoolIndex(index);

		return ERROR_CODE::NONE;
	}

	std::tuple<ERROR_CODE, User*> UserManager::GetUser(const int sessionIndex)
	{
		auto pUser = findUser(sessionIndex);

		if (pUser == nullptr)
		{
			return { ERROR_CODE::USER_MGR_INVALID_SESSION_INDEX, nullptr };
		}

		if (pUser->IsConfirm() == false)
		{
			return { ERROR_CODE::USER_MGR_NOT_CONFIRM_USER, nullptr };
		}

		return { ERROR_CODE::NONE, pUser };
	}

	User * UserManager::allocUserObjPoolIndex()
	{
		if (_userObjPoolIndex.empty())
		{
			return nullptr;
		}

		int index = _userObjPoolIndex.front();
		_userObjPoolIndex.pop_front();
		return &_userObjPool[index];
	}

	void UserManager::releaseUserObjPoolIndex(const int index)
	{
		_userObjPoolIndex.push_back(index);
		_userObjPool[index].Clear();
	}

	User * UserManager::findUser(const int sessionIndex)
	{
		auto findIter = _userSessionDic.find(sessionIndex);

		if (findIter == _userSessionDic.end())
		{
			return nullptr;
		}

		return findIter->second;
	}

	User * UserManager::findUser(const char * pszID)
	{
		auto findIter = _userIdDic.find(pszID);

		if (findIter == _userIdDic.end())
		{
			return nullptr;
		}

		return findIter->second;
	}
}
