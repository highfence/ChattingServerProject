#include <algorithm>
#include "../Common/Packet.h"
#include "../Common/ErrorCode.h"
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
			user.Init((short)i);

			_userObjPool.push_back(user);
			_userObjPoolIndex.push_back(i);
		}
	}

	User* UserManager::allocUserObjPoolIndex()
	{
		if (_userObjPoolIndex.empty()) {
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

	ERROR_CODE UserManager::AddUser(const int sessionIndex, const char* pszID)
	{
		if (findUser(pszID) != nullptr) {
			return ERROR_CODE::USER_MGR_ID_DUPLICATION;
		}

		auto pUser = allocUserObjPoolIndex();
		if (pUser == nullptr) {
			return ERROR_CODE::USER_MGR_MAX_USER_COUNT;
		}

		pUser->Set(sessionIndex, pszID);
		
		_userSessionDic.insert({ sessionIndex, pUser });
		_userIDDic.insert({ pszID, pUser });

		return ERROR_CODE::NONE;
	}

	ERROR_CODE UserManager::RemoveUser(const int sessionIndex)
	{
		auto pUser = findUser(sessionIndex);

		if (pUser == nullptr) {
			return ERROR_CODE::USER_MGR_REMOVE_INVALID_SESSION;
		}

		auto index = pUser->GetIndex();
		auto pszID = pUser->GetID();

		_userSessionDic.erase(sessionIndex);
		_userIDDic.erase(pszID.c_str());
		releaseUserObjPoolIndex(index);

		return ERROR_CODE::NONE;
	}

	std::tuple<ERROR_CODE, User*> UserManager::GetUser(const int sessionIndex)
	{
		auto pUser = findUser(sessionIndex);

		if (pUser == nullptr) {
			return { ERROR_CODE::USER_MGR_INVALID_SESSION_INDEX, nullptr };
		}

		if (pUser->IsConfirm() == false) {
			return{ ERROR_CODE::USER_MGR_NOT_CONFIRM_USER, nullptr };
		}

		return{ ERROR_CODE::NONE, pUser };
	}

	User* UserManager::findUser(const int sessionIndex)
	{
		auto findIter = _userSessionDic.find(sessionIndex);
		
		if (findIter == _userSessionDic.end()) {
			return nullptr;
		}
		
		//auto pUser = (User*)&findIter->second;
		return (User*)findIter->second;
	}

	User* UserManager::findUser(const char* pszID)
	{
		auto findIter = _userIDDic.find(pszID);

		if (findIter == _userIDDic.end()) {
			return nullptr;
		}

		return (User*)findIter->second;
	}
}