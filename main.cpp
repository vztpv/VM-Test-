

#include "mimalloc-new-delete.h"

#include <iostream>
#include <regex>
#include <map>
#include <queue>
#include <unordered_map>

#include "clau_parser.h"

#include "smart_ptr.h"

//#include "ArrayMap.h"


using namespace std::literals;



enum FUNC { TRUE = 0, FALSE, FUNC_QUERY, FUNC_ASSIGN, FUNC_GET, FUNC_FIND, FUNC_WHILE, FUNC_RETURN_VALUE, FUNC_IS_END, FUNC_NOT,
	FUNC_LOAD_DATA, FUNC_ENTER, FUNC_CALL, FUNC_NEXT, FUNC_RETURN, FUNC_COMP_RIGHT,
	FUNC_ADD, FUNC_GET_IDX, FUNC_GET_SIZE, FUNC_GET_NOW, FUNC_CLONE, FUNC_QUIT, FUNC_IF, FUNC_IS_ITEM,
	FUNC_IS_GROUP, FUNC_SET_IDX, FUNC_AND_ALL, FUNC_AND, FUNC_OR, FUNC_IS_QUOTED_STR, FUNC_COMP_LEFT, FUNC_SET_NAME, FUNC_GET_NAME, FUNC_GET_VALUE,
		FUNC_SET_VALUE, FUNC_REMOVE_QUOTED, CONSTANT, THEN, WHILE_END, IF_END, START_DIR, DIR, END_DIR, FUNC_PRINT, NONE,
	KEY, VALUE,  SIZE // chk?
  };
const char* func_to_str[FUNC::SIZE] = {
	"TRUE", "FALSE", "QUERY",
	"ASSIGN",
	"GET",
	"FIND", "WHILE", "RETURN_VALUE", "IS_END", "NOT", "LOAD_DATA", "ENTER", "CALL", "NEXT", "RETURN", "COMP_RIGHT",
	"ADD", "GET_IDX", "GET_SIZE", "GET_NOW", "CLONE", "QUIT", "IF", "IS_ITEM",
	"IS_GROUP", "SET_IDX", "AND_ALL", "AND", "OR", "IS_QUOTED_STR", "COMP_LEFT", "SET_NAME", "GET_NAME", "GET_VALUE",
	"SET_VALUE", "REMOVE_QUOTED", "CONSTANT", "THEN", "WHILE_END", "IF_END", "START_DIR", "DIR", "END_DIR", "PRINT", "NONE", "KEY", "VALUE"
};

class Workspace {
public:
	wiz::SmartPtr<clau_parser::Reader> reader;
public:
	Workspace(clau_parser::UserType* ut = nullptr) {
		if (reader) {
			*reader = clau_parser::Reader(ut);
		}
	}
};

class Token {
public:
	FUNC func = FUNC::NONE;
private:
	mutable long long int_val = 0;
	mutable long double float_val = 0;
	mutable std::string str_val;

	enum Type { INT = 1, FLOAT = 2, STRING = 4, FUNC = 8, USERTYPE = 16, WORKSPACE = 32, BOOL = 64, NONE = 128 };
	mutable Type type = Type::NONE;

public:
	wiz::SmartPtr<clau_parser::UserType> ut_val = nullptr;
	Workspace workspace;
	long long line = 0;
	
	Token(clau_parser::UserType* ut = nullptr) : workspace(ut) {
		//
	}

	const std::string& ToString() const {

		if (type & Type::STRING) {
			return str_val;
		}

		if (type & Type::INT) {
			str_val = std::to_string(int_val);
			type = static_cast<Type>(type | Type::STRING);
			return str_val;
		}

		if (type & Type::FLOAT) {
			str_val = std::to_string(float_val);
			type = static_cast<Type>(type | Type::STRING);
			return str_val;
		}

		// throw error?

		return str_val;
	}
	long long ToInt() const {
		if (type & Type::INT) {
			return int_val;
		}

		if (type & Type::FLOAT) {
			return float_val;
		}

		if (type & Type::STRING) {
			int_val = std::stoll(str_val);
			type = static_cast<Type>(type | Type::INT);
			return int_val;
		}

		return 0;
	}
	long double ToFloat() const {
		if (type & Type::FLOAT) {
			return float_val;
		}

		if (type & Type::INT) {
			return int_val;
		}

		if (type & Type::STRING) {
			float_val = std::stold(str_val);
			type = static_cast<Type>(type | Type::FLOAT);
			return float_val;
		}

		return 0;
	}
	bool ToBool() const {
		if (type & Type::BOOL) {
			return int_val == 1;
		}

		if (type & Type::INT) {
			type = static_cast<Type>(type | Type::BOOL);
			return int_val;
		}

		if (type & Type::FLOAT) {
			type = static_cast<Type>(type | Type::BOOL);
			return float_val;
		}

		return false;
	}
	void SetString(const std::string& str) {
		str_val = str;
		type = Type::STRING;
	}
	void SetInt(long long x) {
		int_val = x;
		type = Type::INT;
	}
	void SetFloat(long double x) {
		float_val = x;
		type = Type::FLOAT;
	}
	void SetWorkspace(wiz::SmartPtr<clau_parser::Reader> ptr) {
		workspace.reader = ptr;
		type = Type::WORKSPACE;
	}
	void SetBool(bool x) {
		int_val = x ? 1 : 0;
		type = Type::BOOL;
	}
	void SetFunc() {
		type = Type::FUNC;
	}

	bool IsInt() const {
		if (type & Type::INT) {
			return true;
		}

		if (type & Type::STRING) {

			int state = 0;
			const auto& str = str_val;

			for (int i = 0; i < str.size(); ++i) {
				switch (state)
				{
				case 0:
					if ('+' == str[i] || '-' == str[i]) {
						state = 0;
					}
					else if (str[i] >= '0' && str[i] <= '9')
					{
						state = 1;
					}
					else return false;
					break;
				case 1:
					if (str[i] >= '0' && str[i] <= '9') {
						state = 1;
					}
					else return false;
				}
			}
			if (1 == state) {
				//type = static_cast<Type>(type | Type::INT);
				//int_val = std::stoll(str_val);
				return true;
			}
		}

		return false;
	}
	bool IsFloat() const {
		if (type & Type::FLOAT) {
			return true;
		}
		
		if (type & Type::STRING) {
			int state = 0;
			const auto& str = str_val;

			for (int i = 0; i < str.size(); ++i) {
				switch (state)
				{
				case 0:
					if ('+' == str[i] || '-' == str[i]) {
						state = 0;
					}
					else if (str[i] >= '0' && str[i] <= '9')
					{
						state = 1;
					}
					else { return false; }
					break;
				case 1:
					if (str[i] >= '0' && str[i] <= '9') {
						state = 1;
					}
					else if (str[i] == '.') {
						state = 2;
					}
					else { return false; }
					break;
				case 2:
					if (str[i] >= '0' && str[i] <= '9') { state = 3; }
					else { return false; }
					break;
				case 3:
					if (str[i] >= '0' && str[i] <= '9') { state = 3; }
					else { return false; }
					break;
				}
			}
			if (state == 3) {
				//type = static_cast<Type>(type | Type::FLOAT);
				//float_val = std::stold(str_val);
				return true;
			}
		}

		return false;
	}
	bool IsString() const {
		return type & Type::STRING;
	}
	bool IsBool() const {
		return type & Type::BOOL;
	}
};


static std::vector<Token> FindValue(clau_parser::UserType* ut, const std::string& str)
{ // std::string ��� vector<std::string> ??
	int count = 0;
	int idx = -1;
	for (int i = str.size() - 1; i >= 0; --i) {
		if ('/' == str[i]) {
			if (count == 0) {
				idx = i;
			}
			count++;
		}
	}

	std::vector<Token> result;

	if (count == 1)
	{ 
		Token token;
		token.SetString({});
		return { token };
	}
	else {
		auto x = clau_parser::UserType::Find(ut, str.substr(0, idx + 1));

		if (x.first == false) { return result; }

		for (int i = 0; i < x.second.size(); ++i) {
			std::string itemName = str.substr(idx + 1);

			if (itemName._Starts_with("$it") && itemName.size() >= 4) {
				int itemIdx = std::stoi(itemName.substr(3));

				Token temp;
				temp.SetString(x.second[i]->GetItemList(itemIdx).Get(0));
				result.push_back(temp);
			}
			else {
				if (itemName == "_") {
					itemName = "";
				}
				auto temp = x.second[i]->GetItem(itemName);
				if (!temp.empty()) {
					for (int j = 0; j < temp.size(); ++j) {
						Token tkn;
						tkn.SetString(temp[j].Get(0));

						result.push_back(tkn);
					}
				}
			}
		}
	}
	return result;
}


struct Event {
	Workspace workspace;
	std::string id;
	std::vector<int> event_data;
	long long now = 0;
	std::vector<Token> return_value;
	long long return_value_now;
	wiz::SmartPtr<std::vector<Token>> input; // ?
	std::unordered_map<std::string, Token> parameter; // std::unordered_map
	std::unordered_map<std::string, Token> local; // std::unordered_map
};


class VM {
private:
	class UtInfo {
	public:
		clau_parser::UserType* global;
		clau_parser::UserType* ut;
		std::string dir;
		long long itCount = 0;
		long long utCount = 0;
		long long count = 0;
	public:
		UtInfo(clau_parser::UserType* global, clau_parser::UserType* ut, const std::string& dir, long long itCount = 0, long long utCount = 0)
			: global(global), ut(ut), itCount(itCount), utCount(utCount), count(0), dir(dir)
		{
			//
		}
	};

	// for $insert, $update, $delete
	inline bool EqualFunc(clau_parser::UserType* global, const clau_parser::ItemType<std::string>& x,
		clau_parser::ItemType<std::string> y, long long x_idx, const std::string& dir, VM* vm) {

		auto x_name = x.GetName();
		auto x_value = x.Get();

		bool use_not = false;
		if (y.Get()._Starts_with("!"sv)) {
			use_not = true;
			y.Set(0, y.Get().substr(1));
		}

		{
			std::string name = y.GetName();

			if (name._Starts_with("&"sv) && name.size() >= 2) {
				long long idx = std::stoll(name.substr(1));
				if (idx < 0 || idx >= global->GetItemListSize()) {
					return false;
				}

				if (y.Get() == "%any"sv) {
					return !use_not;
				}

				x_idx = idx;
				x_name = global->GetItemList(idx).GetName();
				x_value = global->GetItemList(idx).Get();
			}
		}

		if (y.Get() == "%any"sv) {
			return !use_not;
		}

		if (y.Get()._Starts_with("%event_"sv)) {
			std::string event_id = y.Get().substr(7);

			std::unordered_map<std::string, Token> param;

			Token token;
			token.SetString(x_name.empty() ? "EMPTY_NAME" : x_name);
			param["name"] = token; 
			token.SetString(x_value);
			param["value"] = token;
			token.SetBool(false);
			param["is_user_type"] = token;

			std::string result = vm->Run(event_id, global, param)[0].ToString();

			bool success = false;
			if (result == "TRUE"sv) {
				success = true;
			}

			if (use_not) {
				return !success;
			}
			return success;
		}

		if (use_not) {
			return x_value != y.Get();
		}
		return x_value == y.Get();
	}


	bool _InsertFunc(clau_parser::UserType* global, clau_parser::UserType* insert_ut, VM* vm) {
		std::queue<UtInfo> que;

		std::string dir = "/.";

		que.push(UtInfo(global, insert_ut, dir));

		while (!que.empty()) {
			UtInfo x = que.front();
			que.pop();

			// find non-@
			long long ut_count = 0;
			long long it_count = 0;
			long long count = 0;

			for (long long i = 0; i < x.ut->GetIListSize(); ++i) {
				if (x.ut->IsItemList(i) && x.ut->GetItemList(it_count).GetName().empty()
					&& !x.ut->GetItemList(it_count).Get()._Starts_with("@"sv)) {
					// chk exist all case of value?
					auto item = x.global->GetItemIdx("");
					// no exist -> return false;
					if (item.empty()) {
						// LOG....
						return false;
					}

					bool pass = false;
					for (long long j = 0; j < item.size(); ++j) {
						if (EqualFunc(x.global, x.global->GetItemList(item[j]), x.ut->GetItemList(it_count), item[j],
							x.dir, vm)) {
							pass = true;
							break;
						}
					}
					if (!pass) {
						return false;
					}
				}
				else if (x.ut->IsItemList(i) && !x.ut->GetItemList(it_count).GetName().empty() && !x.ut->GetItemList(it_count).GetName()._Starts_with("@"sv)) {
					// chk exist all case of value?
					auto item = x.global->GetItemIdx(x.ut->GetItemList(it_count).GetName());
					// no exist -> return false;
					if (item.empty()) {
						// LOG....
						return false;
					}

					bool pass = false;

					for (long long j = 0; j < item.size(); ++j) {
						if (EqualFunc(x.global, x.global->GetItemList(item[j]), x.ut->GetItemList(it_count), item[j],
							x.dir, vm)) {
							pass = true;
							break;
						}
					}
					if (!pass) {
						return false;
					}

				}
				else if (x.ut->IsUserTypeList(i) && !x.ut->GetUserTypeList(ut_count)->GetName()._Starts_with("@"sv)) {
					// chk all case exist of @.
					// no exist -> return false;
					if (x.ut->GetUserTypeList(ut_count)->GetName()._Starts_with("$"sv)) {
						ut_count++;
						count++;
						continue;
					}

					auto usertype = x.global->GetUserTypeItemIdx(x.ut->GetUserTypeList(ut_count)->GetName());

					if (usertype.empty()) {
						return false;
					}

					ut_count++;
					count++;

					for (long long j = 0; j < usertype.size(); ++j) {
						que.push(UtInfo(x.global->GetUserTypeList(usertype[j]), x.ut->GetUserTypeList(ut_count - 1),
							x.dir));
					}

					continue;
				}

				if (x.ut->IsItemList(i)) {
					it_count++;
				}
				else {
					ut_count++;
				}
				count++;
			}
		}

		return true;
	}

	bool _RemoveFunc(clau_parser::UserType* global, clau_parser::UserType* insert_ut, VM* vm) {
		std::queue<UtInfo> que;
		std::string dir = "/.";
		que.push(UtInfo(global, insert_ut, dir));

		while (!que.empty()) {
			UtInfo x = que.front();
			que.pop();

			// find non-@
			long long ut_count = 0;
			long long it_count = 0;
			long long count = 0;

			for (long long i = 0; i < x.ut->GetIListSize(); ++i) {
				if (x.ut->IsItemList(i) && x.ut->GetItemList(it_count).GetName().empty()
					&& !x.ut->GetItemList(it_count).Get()._Starts_with("@"sv)) {
					// chk exist all case of value?
					auto item = x.global->GetItemIdx("");
					// no exist -> return false;
					if (item.empty()) {
						// LOG....
						return false;
					}

					bool pass = false;
					for (long long j = 0; j < item.size(); ++j) {
						if (EqualFunc(x.global, x.global->GetItemList(item[j]), x.ut->GetItemList(it_count), item[j],
							x.dir, vm)) {
							pass = true;
							break;
						}
					}
					if (!pass) {
						return false;
					}
				}
				else if (x.ut->IsItemList(i) && !x.ut->GetItemList(it_count).GetName().empty() && !x.ut->GetItemList(it_count).GetName()._Starts_with("@"sv)) {
					// chk exist all case of value?
					auto item = x.global->GetItemIdx(x.ut->GetItemList(it_count).GetName());
					// no exist -> return false;
					if (item.empty()) {
						if (x.ut->GetItemList(it_count).Get() == "!%any") {
							return true;
						}
						// LOG....
						return false;
					}

					bool pass = false;

					for (long long j = 0; j < item.size(); ++j) {
						if (EqualFunc(x.global, x.global->GetItemList(item[j]), x.ut->GetItemList(it_count), item[j],
							x.dir, vm)) {
							pass = true;
							break;
						}
					}
					if (!pass) {
						return false;
					}

				}
				else if (x.ut->IsUserTypeList(i) && !x.ut->GetUserTypeList(ut_count)->GetName()._Starts_with("@"sv)) {
					// chk all case exist of @.
					// no exist -> return false;
					if (x.ut->GetUserTypeList(ut_count)->GetName()._Starts_with("$"sv)) {
						ut_count++;
						count++;
						continue;
					}

					auto usertype = x.global->GetUserTypeItemIdx(x.ut->GetUserTypeList(ut_count)->GetName());

					if (usertype.empty()) {
						return false;
					}

					ut_count++;
					count++;

					for (long long j = 0; j < usertype.size(); ++j) {
						que.push(UtInfo(x.global->GetUserTypeList(usertype[j]), x.ut->GetUserTypeList(ut_count - 1),
							x.dir + "/$ut" + std::to_string(usertype[j])));
					}

					continue;
				}
				else if (x.ut->IsUserTypeList(i) && x.ut->GetUserTypeList(ut_count)->GetName() == "@$"sv) {
					//
				}

				if (x.ut->IsItemList(i)) {
					it_count++;
				}
				else {
					ut_count++;
				}
				count++;
			}
		}

		return true;
	}


	bool _UpdateFunc(clau_parser::UserType* global, clau_parser::UserType* insert_ut, VM* vm) {
		std::queue<UtInfo> que;
		std::string dir = "/.";
		que.push(UtInfo(global, insert_ut, dir));

		while (!que.empty()) {
			UtInfo x = que.front();
			que.pop();


			// find non-@
			long long ut_count = 0;
			long long it_count = 0;
			long long count = 0;

			for (long long i = 0; i < x.ut->GetIListSize(); ++i) {
				if (x.ut->IsItemList(i) && x.ut->GetItemList(it_count).GetName().empty()
					&& !x.ut->GetItemList(it_count).Get()._Starts_with("@"sv)) {
					// chk exist all case of value?
					auto item = x.global->GetItemIdx("");
					// no exist -> return false;
					if (item.empty()) {
						// LOG....
						return false;
					}

					bool pass = false;
					for (long long j = 0; j < item.size(); ++j) {
						if (EqualFunc(x.global, x.global->GetItemList(item[j]), x.ut->GetItemList(it_count), item[j],
							x.dir, vm)) {
							pass = true;
							break;
						}
					}
					if (!pass) {
						return false;
					}
				}
				else if (x.ut->IsItemList(i) && !x.ut->GetItemList(it_count).GetName().empty() && !x.ut->GetItemList(it_count).GetName()._Starts_with("@"sv)) {
					// chk exist all case of value?
					auto item = x.global->GetItemIdx(x.ut->GetItemList(it_count).GetName());
					// no exist -> return false;
					if (item.empty()) {
						// LOG....
						return false;
					}

					bool pass = false;

					for (long long j = 0; j < item.size(); ++j) {
						if (EqualFunc(x.global, x.global->GetItemList(item[j]), x.ut->GetItemList(it_count), item[j],
							dir, vm)) {
							pass = true;
							break;
						}
					}
					if (!pass) {
						return false;
					}

				}
				else if (x.ut->IsUserTypeList(i) && !x.ut->GetUserTypeList(ut_count)->GetName()._Starts_with("@"sv)) {
					// chk all case exist of @.
					// no exist -> return false;
					if (x.ut->GetUserTypeList(ut_count)->GetName()._Starts_with("$"sv)) {
						ut_count++;
						count++;
						continue;
					}

					auto usertype = x.global->GetUserTypeItemIdx(x.ut->GetUserTypeList(ut_count)->GetName());

					if (usertype.empty()) {
						return false;
					}

					ut_count++;
					count++;

					for (long long j = 0; j < usertype.size(); ++j) {
						que.push(UtInfo(x.global->GetUserTypeList(usertype[j]), x.ut->GetUserTypeList(ut_count - 1),
							dir + "$ut" + std::to_string(usertype[j])));
					}

					continue;
				}

				if (x.ut->IsItemList(i)) {
					it_count++;
				}
				else {
					ut_count++;
				}
				count++;
			}
		}

		return true;
	}

	// starts with '@' -> insert target
	// else -> condition target.
	bool InsertFunc(clau_parser::UserType* global, clau_parser::UserType* insert_ut, VM* vm) {
		if (!_InsertFunc(global, insert_ut, vm)) {
			return false;
		}
		std::string dir = "/.";
		std::queue<UtInfo> que;
		// insert
		que.push(UtInfo(global, insert_ut, dir));

		while (!que.empty()) {
			UtInfo x = que.front();
			que.pop();

			// find non-@
			long long ut_count = 0;
			long long it_count = 0;
			long long count = 0;

			//chk only @  ! - todo
			for (long long i = 0; i < x.ut->GetIListSize(); ++i) {
				if (x.ut->IsItemList(i) && x.ut->GetItemList(it_count).GetName().empty()
					&& x.ut->GetItemList(it_count).Get()._Starts_with("@"sv)) {

					if (x.ut->GetItemList(it_count).Get()._Starts_with("@%event_"sv)) {
						std::string event_id = x.ut->GetItemList(it_count).Get().substr(8);

						std::unordered_map<std::string, Token> param;
						Token token;
						token.SetString("EMPTY_STRING");
						param["name"] = token;
						token.SetBool(true);
						param["is_user_type"] = token;

						std::string result = vm->Run(event_id, global, param)[0].ToString();

						x.global->AddItem("", result);
					}
					else {
						x.global->AddItem("", x.ut->GetItemList(it_count).Get().substr(1));
					}

					it_count++;
				}
				else if (x.ut->IsItemList(i) && x.ut->GetItemList(it_count).GetName()._Starts_with("@"sv)) {
					if (x.ut->GetItemList(it_count).Get()._Starts_with("%event_"sv)) {
						std::string event_id = x.ut->GetItemList(it_count).Get().substr(7);

						std::unordered_map<std::string, Token> param;
						Token token;
						token.SetString(x.ut->GetItemList(it_count).GetName());
						param["name"] = token;
						token.SetBool(true);
						param["is_user_type"] = token;

						std::string result = vm->Run(event_id, global, param)[0].ToString();

						x.global->AddItem(
							x.ut->GetItemList(it_count).GetName().substr(1),
							result);
					}
					else {
						x.global->AddItem(
							x.ut->GetItemList(it_count).GetName().substr(1),
							x.ut->GetItemList(it_count).Get());
					}
					it_count++;
				}
				else if (x.ut->IsUserTypeList(i) && x.ut->GetUserTypeList(ut_count)->GetName()._Starts_with("@"sv)) {
					x.global->LinkUserType(x.ut->GetUserTypeList(ut_count));
					x.ut->GetUserTypeList(ut_count)->SetName(x.ut->GetUserTypeList(ut_count)->GetName().substr(1));
					x.ut->GetUserTypeList(ut_count) = nullptr;

					x.ut->RemoveUserTypeList(ut_count);
					count--;
					i--;
				}
				else if (x.ut->IsUserTypeList(i) && !x.ut->GetUserTypeList(ut_count)->GetName()._Starts_with("@"sv)) {
					if (x.ut->GetUserTypeList(ut_count)->GetName()._Starts_with("$"sv)) {
						auto temp = x.ut->GetUserTypeList(ut_count)->GetName();
						auto name = temp.substr(1);

						if (name.empty()) {

							for (long long j = 0; j < x.global->GetUserTypeListSize(); ++j) {
								if (_InsertFunc(x.global->GetUserTypeList(j), x.ut->GetUserTypeList(ut_count), vm)) {
									que.push(UtInfo(x.global->GetUserTypeList(j), x.ut->GetUserTypeList(ut_count)
										, x.dir + "/$ut" + std::to_string(j)
									));
								}
							}
						}
						else {

							auto usertype = x.global->GetUserTypeItemIdx(name);

							for (long long j = 0; j < usertype.size(); ++j) {
								if (_InsertFunc(x.global->GetUserTypeList(usertype[j]), x.ut->GetUserTypeList(ut_count), vm)) {
									que.push(UtInfo(x.global->GetUserTypeList(usertype[j]), x.ut->GetUserTypeList(ut_count),
										x.dir + "/$ut" + std::to_string(usertype[j])));
								}
							}
						}
					}
					else {
						auto usertype = x.global->GetUserTypeItemIdx(x.ut->GetUserTypeList(ut_count)->GetName());

						for (long long j = 0; j < usertype.size(); ++j) {
							//if (_InsertFunc(x.global->GetUserTypeList(usertype[j]), x.ut->GetUserTypeList(ut_count), eventUT)) {
							que.push(UtInfo(x.global->GetUserTypeList(usertype[j]), x.ut->GetUserTypeList(ut_count),
								x.dir + "/$ut" + std::to_string(usertype[j])));
							//}
						}
					}
					ut_count++;
				}
				else if (x.ut->IsUserTypeList(i)) {
					ut_count++;
				}
				else {
					it_count++;
				}

				count++;
			}
		}

		return true;
	}

	bool RemoveFunc(clau_parser::UserType* global, clau_parser::UserType* insert_ut, VM* vm) {
		if (!_RemoveFunc(global, insert_ut, vm)) {
			return false;
		}
		std::string dir = "/.";
		std::queue<UtInfo> que;
		// insert
		que.push(UtInfo(global, insert_ut, dir));

		while (!que.empty()) {
			UtInfo x = que.front();
			que.pop();

			// find non-@
			long long ut_count = x.ut->GetUserTypeListSize() - 1;
			long long it_count = x.ut->GetItemListSize() - 1;
			long long count = x.ut->GetIListSize() - 1;

			//chk only @  ! - todo
			for (long long i = x.ut->GetIListSize() - 1; i >= 0; --i) {
				if (x.ut->IsItemList(i) && x.ut->GetItemList(it_count).GetName().empty()
					&& x.ut->GetItemList(it_count).Get()._Starts_with("@"sv)) {

					if (x.ut->GetItemList(it_count).Get()._Starts_with("@%event_"sv)) {
						std::string event_id = x.ut->GetItemList(it_count).Get().substr(8);

						auto temp = x.global->GetItemIdx("");

						for (long long j = 0; j < temp.size(); ++j) {
							std::unordered_map<std::string, Token> param;

							Token token;
							token.SetString("EMPTY_STRING");
							param["name"] = token;
							token.SetString(x.global->GetItemList(temp[j]).Get());
							param["value"] = token;
							token.SetBool(false);
							param["is_user_type"] = token;

							std::string result = vm->Run(event_id, global, param)[0].ToString();


							if (result == "TRUE"sv) { //x.ut->GetItemList(it_count).Get().substr(1)) {
								x.global->RemoveItemList(temp[j]);
							}
						}
					}
					else {
						x.global->RemoveItemList("", x.ut->GetItemList(it_count).Get().substr(1));
					}
					it_count--;
					//x.global->AddItemType(clau_parser::ItemType<std::string>("", x.ut->GetItemList(it_count).Get().substr(1)));
				}
				else if (x.ut->IsItemList(i) && x.ut->GetItemList(it_count).GetName()._Starts_with("@"sv)) {
					//x.global->AddItemType(clau_parser::ItemType<std::string>(
					//	x.ut->GetItemList(it_count).GetName().substr(1),
					//	x.ut->GetItemList(it_count).Get()));
					if (x.ut->GetItemList(it_count).Get()._Starts_with("%event_"sv)) {
						std::string event_id = x.ut->GetItemList(it_count).Get().substr(7);

						auto name = x.ut->GetItemList(it_count).GetName().substr(1);
						auto temp = x.global->GetItemIdx(name);

						if (name._Starts_with("&"sv) && name.size() >= 2) {
							long long idx = std::stoll(name.substr(1));

							if (idx < 0 || idx >= x.global->GetItemListSize()) { // .size()) {
								return false;
							}
							auto valName = x.ut->GetItemList(it_count).Get();

							std::unordered_map<std::string, Token> param;
							Token token;
							token.SetString(name);
							param["name"] = token;
							token.SetString(x.global->GetItemList(idx).Get());
							param["value"] = token;
							token.SetBool(false);
							param["is_user_type"] = token;

							std::string result = vm->Run(event_id, global, param)[0].ToString();


							if (result == "TRUE"sv) {
								x.global->RemoveItemList(idx);
							}
						}
						else {
							for (long long j = 0; j < temp.size(); ++j) {
								std::unordered_map<std::string, Token> param;
								Token token;
								token.SetString(name);
								param["name"] = token;
								token.SetString(x.global->GetItemList(temp[j]).Get());
								param["value"] = token;
								token.SetBool(false);
								param["is_user_type"] = token;

								std::string result = vm->Run(event_id, global, param)[0].ToString();


								if (result == "TRUE"sv) {
									x.global->RemoveItemList(temp[j]);
								}
							}
						}
					}
					else {
						x.global->RemoveItemList(x.ut->GetItemList(it_count).GetName().substr(1), x.ut->GetItemList(it_count).Get());
					}

					it_count--;
				}
				else if (x.ut->IsUserTypeList(i) && x.ut->GetUserTypeList(ut_count)->GetName()._Starts_with( "@"sv)) {
					if (x.ut->GetUserTypeList(ut_count)->GetName() == "@$"sv) {
						for (long long j = x.global->GetUserTypeListSize() - 1; j >= 0; --j) {
							if (_RemoveFunc(x.global->GetUserTypeList(j), x.ut->GetUserTypeList(ut_count), vm)) {
								delete[] x.global->GetUserTypeList(j);
								x.global->GetUserTypeList(j) = nullptr;
								x.global->RemoveUserTypeList(j);
							}
						}
					}
					else {
						auto usertype = x.global->GetUserTypeItemIdx(x.ut->GetUserTypeList(ut_count)->GetName().substr(1));

						for (long long j = usertype.size() - 1; j >= 0; --j) {
							if (_RemoveFunc(x.global->GetUserTypeList(usertype[j]), x.ut->GetUserTypeList(ut_count), vm)) {
								x.global->RemoveUserTypeList(usertype[j]);
							}
						}
					}
					ut_count--;
				}
				else if (x.ut->IsUserTypeList(i) && false == x.ut->GetUserTypeList(ut_count)->GetName()._Starts_with("@"sv)) {
					if (x.ut->GetUserTypeList(ut_count)->GetName()._Starts_with("$"sv)) {
						auto temp = x.ut->GetUserTypeList(ut_count)->GetName();
						auto name = temp.substr(1);

						if (name.empty()) {

							for (long long j = 0; j < x.global->GetUserTypeListSize(); ++j) {
								if (_InsertFunc(x.global->GetUserTypeList(j), x.ut->GetUserTypeList(ut_count), vm)) {
									que.push(UtInfo(x.global->GetUserTypeList(j), x.ut->GetUserTypeList(ut_count)
										, x.dir + "/$ut" + std::to_string(j)
									));
								}
							}
						}
						else {
							auto usertype = x.global->GetUserTypeItemIdx(name);

							for (long long j = 0; j < usertype.size(); ++j) {
								if (_InsertFunc(x.global->GetUserTypeList(usertype[j]), x.ut->GetUserTypeList(ut_count), vm)) {
									que.push(UtInfo(x.global->GetUserTypeList(usertype[j]), x.ut->GetUserTypeList(ut_count),
										x.dir + "/$ut" + std::to_string(usertype[j])));
								}
							}
						}
					}
					else {
						auto usertype = x.global->GetUserTypeItemIdx(x.ut->GetUserTypeList(ut_count)->GetName());

						for (long long j = 0; j < usertype.size(); ++j) {
							//if (_RemoveFunc(x.global->GetUserTypeList(usertype[j]), x.ut->GetUserTypeList(ut_count), eventUT)) {
							que.push(UtInfo(x.global->GetUserTypeList(usertype[j]), x.ut->GetUserTypeList(ut_count),
								x.dir + "/$ut" + std::to_string(usertype[j])));
							//}
						}
					}

					ut_count--;
				}
				else if (x.ut->IsUserTypeList(i)) {
					ut_count--;
				}
				else if (x.ut->IsItemList(i)) {
					it_count--;
				}

				count--;
			}
		}

		return true;
	}

	bool UpdateFunc(clau_parser::UserType* global, clau_parser::UserType* insert_ut, VM* vm) {
		if (!_UpdateFunc(global, insert_ut, vm)) {
			return false;
		}
		std::string dir = "/.";
		std::queue<UtInfo> que;
		// insert
		que.push(UtInfo(global, insert_ut, dir));

		while (!que.empty()) {
			UtInfo x = que.front();
			que.pop();

			// find non-@
			long long ut_count = 0;
			long long it_count = 0;
			long long count = 0;

			//chk only @  ! - todo
			for (long long i = 0; i < x.ut->GetIListSize(); ++i) {
				if (x.ut->IsItemList(i) && x.ut->GetItemList(it_count).GetName().empty()
					&& x.ut->GetItemList(it_count).Get()._Starts_with( "@"sv)) {
					// think @&0 = 3 # 0 <- index, 3 <- value.
					//x.global->GetItemList(0).Set(0, x.ut->GetItemList(it_count).Get());
				}
				else if (x.ut->IsItemList(i) && x.ut->GetItemList(it_count).GetName()._Starts_with( "@"sv)) {
					if (x.ut->GetItemList(it_count).Get()._Starts_with( "%event_"sv)) {
						std::string event_id = x.ut->GetItemList(it_count).Get().substr(7);

						std::string name = x.ut->GetItemList(it_count).GetName().substr(1);
						auto position = x.global->GetItemIdx(name);

						{
							std::string name = x.ut->GetItemList(it_count).GetName().substr(1);
							if (name._Starts_with("&"sv) && name.size() >= 2) {
								long long idx = std::stoll(name.substr(1));
								if (idx < 0 || idx >= x.global->GetItemListSize()) {
									return false; // error
								}
								else {
									position.clear();
									position.push_back(idx);
								}
							}

							for (long long j = 0; j < position.size(); ++j) {
								std::unordered_map<std::string, Token> param;
								Token token;
								token.SetString(x.ut->GetItemList(it_count).GetName().substr(1));
								param["name"] = token;
								token.SetString(x.global->GetItemList(position[j]).Get());
								param["value"] = token;
								token.SetBool(false);
								param["is_user_type"] = token;

								std::string result = vm->Run(event_id, global, param)[0].ToString();

								x.global->GetItemList(position[j]).Set(0, result);

							}
						}
					}
					else {
						std::string name = x.ut->GetItemList(it_count).GetName().substr(1);
						if (name._Starts_with("&"sv) && name.size() >= 2) {
							long long idx = std::stoll(name.substr(1));
							if (idx < 0 || idx >= x.global->GetItemListSize()) {
								return false;
							}
							auto value = x.ut->GetItemList(it_count).Get();
							x.global->GetItemList(idx).Set(0, value);
						}
						else {
							x.global->SetItem(std::string(x.ut->GetItemList(it_count).GetName().substr(1)),
								x.ut->GetItemList(it_count).Get());
						}
					}
				}
				else if (x.ut->IsUserTypeList(i) && !x.ut->GetUserTypeList(ut_count)->GetName()._Starts_with( "@"sv)) {
					if (x.ut->GetUserTypeList(ut_count)->GetName()._Starts_with("$"sv)) {
						auto temp = x.ut->GetUserTypeList(ut_count)->GetName();
						auto name = temp.substr(1);

						if (name.empty()) {

							for (long long j = 0; j < x.global->GetUserTypeListSize(); ++j) {
								if (_InsertFunc(x.global->GetUserTypeList(j), x.ut->GetUserTypeList(ut_count), vm)) {
									que.push(UtInfo(x.global->GetUserTypeList(j), x.ut->GetUserTypeList(ut_count)
										, x.dir + "/$ut" + std::to_string(j)
									));
								}
							}
						}
						else {

							auto usertype = x.global->GetUserTypeItemIdx(name);

							for (long long j = 0; j < usertype.size(); ++j) {
								if (_InsertFunc(x.global->GetUserTypeList(usertype[j]), x.ut->GetUserTypeList(ut_count), vm)) {
									que.push(UtInfo(x.global->GetUserTypeList(usertype[j]), x.ut->GetUserTypeList(ut_count),
										x.dir + "/$ut" + std::to_string(usertype[j])));
								}
							}
						}
					}
					else {
						auto usertype = x.global->GetUserTypeItemIdx(x.ut->GetUserTypeList(ut_count)->GetName());

						for (long long j = 0; j < usertype.size(); ++j) {
							//if (_UpdateFunc(x.global->GetUserTypeList(usertype[j]), x.ut->GetUserTypeList(ut_count), eventUT)) {
							que.push(UtInfo(x.global->GetUserTypeList(usertype[j]), x.ut->GetUserTypeList(ut_count),
								x.dir + "/$ut" + std::to_string(usertype[j])));
							//	}
						}
					}
				}

				if (x.ut->IsItemList(i)) {
					it_count++;
				}
				else {
					ut_count++;
				}
				count++;
			}
		}

		return true;
	}
	std::pair<bool, std::vector<clau_parser::UserType*> > _Find(clau_parser::UserType* global, const std::string& _position) /// option, option_offset
	{
		std::string position = _position;

		if (!position.empty() && position[0] == '@') { position.erase(position.begin()); }

		std::vector<std::string> x;


		//wiz::Out << "string view is " << pos_sv << " ";
		std::vector<std::string> tokens = clau_parser::tokenize(position, '/');

		for (size_t i = 0; i < tokens.size(); ++i) {
			std::string temp = tokens[i];

			//wiz::Out << tokens[i] << " ";

			if (temp == ".") {
				continue;
			}

			if (x.empty()) {
				x.push_back(temp);
			}
			else if (x.back() != ".." && temp == "..") {
				x.pop_back();
			}
			else {
				x.push_back(temp);
			}
		}

		return __Find(std::move(x), global);
	}
private:
	// find userType! not itemList!,
	// this has bug
	/// /../x ok   ( /../../y ok)
	/// /x/../x/../x no
	std::pair<bool, std::vector< clau_parser::UserType*> > __Find(std::vector<std::string>&& tokens, clau_parser::UserType* global) /// option, option_offset
	{
		std::vector<clau_parser::UserType* > temp;
		int start = 0;

		if (tokens.empty()) { temp.push_back(global); return{ true, temp }; }
		if (tokens.size() == 1 && tokens[0] == ".") { temp.push_back(global); return{ true, temp }; }
		//if (position.size() == 1 && position[0] == "/./") { temp.push_back(global); return{ true, temp }; } // chk..
		//if (position.size() == 1 && position[0] == "/.") { temp.push_back(global); return{ true, temp }; }
		//if (position.size() == 1 && position[0] == "/") { temp.push_back(global); return{ true, temp }; }

		if (tokens.size() > 1 && tokens[0] == ".")
		{
			start = 1;
			//position = String::substring(position, 3);
		}

		std::list<std::pair<clau_parser::UserType*, int >> utDeck;
		std::pair<clau_parser::UserType*, int> utTemp;
		utTemp.first = global;
		utTemp.second = 0;
		std::vector<std::string> strVec;

		//wiz::Out << "position is " << position << "\t";
		for (int i = start; i < tokens.size(); ++i) {
			std::string strTemp = tokens[i];

			//wiz::Out << strTemp << " ";

			if (strTemp == "root" && i == 0) {
			}
			else {
				strVec.push_back(strTemp);
			}

			if ((strVec.size() >= 1) && (" " == strVec[strVec.size() - 1])) /// chk!!
			{
				strVec[strVec.size() - 1] = "";
			}
			else if ((strVec.size() >= 1) && ("_" == strVec[strVec.size() - 1]))
			{
				strVec[strVec.size() - 1] = "";
			}
		}

		// it has bug!
		{
			int count = 0;

			for (int i = 0; i < strVec.size(); ++i) {
				if (strVec[i] == "..") {
					count++;
				}
				else {
					break;
				}
			}

			std::reverse(strVec.begin(), strVec.end());

			for (int i = 0; i < count; ++i) {
				if (utTemp.first == nullptr) {
					return{ false, std::vector< clau_parser::UserType* >() };
				}
				utTemp.first = utTemp.first->GetParent();
				strVec.pop_back();
			}
			std::reverse(strVec.begin(), strVec.end());
		}
		//wiz::Out << "\n";

		utDeck.push_front(utTemp);

		bool exist = false;
		while (false == utDeck.empty()) {
			utTemp = utDeck.front();
			utDeck.pop_front();

			if (utTemp.second < strVec.size() && strVec[utTemp.second] == "$")
			{
				for (int j = utTemp.first->GetUserTypeListSize() - 1; j >= 0; --j) {
					clau_parser::UserType* x = utTemp.first->GetUserTypeList(j);
					utDeck.push_front(std::make_pair(x, utTemp.second + 1));
				}
			}
			else if (utTemp.second < strVec.size() && strVec[utTemp.second]._Starts_with("$.")) /// $."abc"
			{
				std::string rex_str = strVec[utTemp.second].substr(3, strVec[utTemp.second].size() - 4);
				std::regex rgx(rex_str.data());

				for (int j = utTemp.first->GetUserTypeListSize() - 1; j >= 0; --j) {
					if (std::regex_match((utTemp.first->GetUserTypeList(j)->GetName()), rgx)) {
						clau_parser::UserType* x = utTemp.first->GetUserTypeList(j);
						utDeck.push_front(std::make_pair(x, utTemp.second + 1));
					}
				}
			}
			else if (utTemp.second < strVec.size() &&
				(utTemp.first->GetUserTypeItem(strVec[utTemp.second]).empty() == false))
			{
				auto  x = utTemp.first->GetUserTypeItem(strVec[utTemp.second]);
				for (int j = x.size() - 1; j >= 0; --j) {
					utDeck.push_front(std::make_pair(x[j], utTemp.second + 1));
				}
			}

			if (utTemp.second == strVec.size()) {
				exist = true;
				temp.push_back(utTemp.first);
			}
		}
		if (false == exist) {
			return{ false, std::vector<clau_parser::UserType*>() };
		}
		return{ true, temp };
	}

	std::vector<Token> Find(clau_parser::UserType* ut, std::string str) {
		std::vector<Token> result;
		auto uts = _Find(ut, str);
		
		if (!uts.first) {
			return result;
		}

		for (long long i = 0; i < uts.second.size(); ++i) {
			Token _token;
			_token.SetWorkspace(new clau_parser::Reader(uts.second[i]));
			result.push_back(_token);
		}
		
		return result;
	}
public:

	
	std::vector<Token> Run(const std::string& id, clau_parser::UserType* global, 
					std::unordered_map<std::string, Token> parameter = std::unordered_map<std::string, Token>()) {
		Event main = _event_list[id];
		main.parameter = parameter;
		std::vector<Token> token_stack;
		std::vector<Event> _stack;

		_stack.push_back(main);
		int count = 0;
		std::string dir = "";

		std::vector<Token> return_value;

		while (!_stack.empty()) {
			auto& x = _stack.back();
			count++;

			//std::cout << func_to_str[x.event_data[x.now]] << "\n";
			
			switch (x.event_data[x.now]) {
			case FUNC::TRUE:
			{
				Token token;
				token.SetBool(true);

				token_stack.push_back(token);
			}
				break;
			case FUNC::FALSE:
			{
				Token token;
				token.SetBool(false);

				token_stack.push_back(token);
			}
				break;
			case FUNC::FUNC_QUERY:
			{
				x.now++;

				auto ut = (*x.input)[x.event_data[x.now]];
				auto dir = token_stack.back(); token_stack.pop_back();

				auto workspace = this->Find(global, dir.ToString())[0].workspace;
				

				for (int i = 1; i < ut.ut_val->GetUserTypeListSize(); ++i) {
					if (ut.ut_val->GetUserTypeList(i)->GetName() == "$insert"sv) {
						this->InsertFunc(workspace.reader->GetUT(), ut.ut_val->GetUserTypeList(i), this);
					}
					else if (ut.ut_val->GetUserTypeList(i)->GetName() == "$update"sv) {
						this->UpdateFunc(workspace.reader->GetUT(), ut.ut_val->GetUserTypeList(i), this);
					}
					else if (ut.ut_val->GetUserTypeList(i)->GetName() == "$delete"sv) {
						this->RemoveFunc(workspace.reader->GetUT(), ut.ut_val->GetUserTypeList(i), this);
					}
				}
			}
			break;
			case FUNC::FUNC_GET:
			{
				auto token = token_stack.back();
				token_stack.pop_back();

				if (token.ToString()._Starts_with("$local."sv)) {

					token_stack.push_back(x.local[token.ToString().substr(7)]);
				}
				
				else {
					auto value = FindValue(global, token.ToString()); // ToString?

					token_stack.push_back(value[0]);
				}
			}

			break;
			case FUNC::FUNC_ASSIGN:
			{
				auto value = token_stack.back();
				token_stack.pop_back();

				auto name = token_stack.back();
				token_stack.pop_back();

				if (name.ToString()._Starts_with("$local."sv)) {
					x.local[name.ToString().substr(7)] = value;
				}
				else if (name.ToString()._Starts_with("$parameter."sv)) {
					x.parameter[name.ToString().substr(11)] = value;
				}
				else {
					// todo
				}
			}
			break;
			case FUNC::START_DIR:
				count = 0;
				dir = "/";
				break;
			case FUNC::DIR:
				////std::cout << "DIR chk" << token_stack.back().ToString() << "\n";
			{
				auto str = token_stack.back().ToString();

				if (str._Starts_with("$parameter."sv)) {
					str = str.substr(11);

					Token token = x.parameter[str];
					dir += token.ToString();
				}
				else if (str._Starts_with("$local."sv)) {
					str = str.substr(7);

					Token token = x.local[str];
					dir += token.ToString();
				}
				else {
					dir += token_stack.back().ToString(); // ToString
				}

				token_stack.pop_back();
				dir += "/";
			}

			break;
			case FUNC::END_DIR:
			{
				Token token;
				
				if (dir.back() == '/') {
					token.SetString(dir.substr(0, dir.size() - 1));
				}
				else {
					token.SetString(dir);
				}
				token_stack.push_back(token);

				dir = "";
				count = 0;
			}
			break;
			case FUNC::FUNC_REMOVE_QUOTED:
			{
				auto str =token_stack.back().ToString();

				if (str.size() >= 2) {
					str = str.substr(1, str.size() - 2);
				}
				token_stack.pop_back();

				Token temp;
				temp.SetString(str);
				token_stack.push_back(temp);
			}
				break;
			case FUNC::FUNC_IS_QUOTED_STR:
			{
				auto str = token_stack.back().ToString();
				bool chk = str.size() >= 2 && (str)[0] == str.back() && str.back() == '\"';

				token_stack.pop_back();

				Token temp;
				
				temp.SetBool(chk);

				token_stack.push_back(temp);
				break;
			}
			case FUNC::FUNC_RETURN:
				x.now++;
				
				//std::cout << "return .... \n";)
				if (_stack.size() == 1) {
					std::vector<Token> temp;
					int count = x.event_data[x.now];

					for(int i=0; i < count; ++i) {
						temp.push_back(token_stack.back());
						token_stack.pop_back();
					}
					for (int i = 0; i < temp.size(); ++i) {
						return_value.push_back(temp[i]);
					}
				}
				_stack.pop_back();
				break;
			case FUNC::CONSTANT:
				x.now++;

				{
					const auto& value = (*x.input)[x.event_data[x.now]];
					//std::cout << value.ToString() << "\n";

					if (value.IsString()) {
						if (value.ToString()._Starts_with("$parameter."sv)) {
							auto param = value.ToString().substr(11);

							token_stack.push_back(x.parameter[param]);

							x.now++;
							continue;
						}
					}

					{ 
						token_stack.push_back(value);
					}
				}

				break;
			case FUNC::FUNC_ADD:
			{
				auto a = token_stack.back();
				token_stack.pop_back();
				auto b = token_stack.back();
				token_stack.pop_back();

				{
					Token token;

					if (a.IsFloat() && b.IsFloat()) {
						token.SetFloat(a.ToFloat() + b.ToFloat());
					}
					else if(a.IsInt() && b.IsInt()) {
						token.SetInt(a.ToInt() + b.ToInt());
					}

					token_stack.push_back(token);
				}
			}
			break;
			case FUNC::FUNC_CALL:
				x.now++;

				{
					auto count = x.event_data[x.now];

					x.now++;

					Event e;

					for (int i = 0; i < count; ++i) {
						auto value = token_stack.back();
						token_stack.pop_back();
						auto name = token_stack.back();
						token_stack.pop_back();

						if (name.ToString() == "id"sv) {
							e.id = value.ToString();

							//		//std::cout << e.id << "\n";

							e.event_data = _event_list[value.ToString()].event_data;
							e.input = _event_list[value.ToString()].input;
							e.now = 0;
							e.return_value_now = 0;

							break;
						}
						
						e.parameter[name.ToString()] = std::move(value); // name.ToString()
					}

					////std::cout << "call " << e.id << "\n";
					_stack.push_back(std::move(e));
				}

				continue;

				break;

				// do not compare bools
			case FUNC::FUNC_COMP_LEFT:
				// Compare!
			{
				auto a = token_stack.back();
				token_stack.pop_back();
				auto b = token_stack.back();
				token_stack.pop_back();

				{
					Token token;

					if (a.IsFloat() && b.IsFloat()) {
						token.SetBool(a.ToFloat() > b.ToFloat());
					}
					else if (a.IsInt() && b.IsInt()) {
						token.SetBool(a.ToInt() > b.ToInt());
					}
					else if (a.IsString() && b.IsString()) {
						token.SetBool(a.ToString() > b.ToString());
					}

					token_stack.push_back(token);
				}
			}
			break;
			case FUNC::FUNC_COMP_RIGHT:
			{
				auto b = token_stack.back();
				token_stack.pop_back();
				auto a = token_stack.back();
				token_stack.pop_back();

				{
					Token token;

					if (a.IsFloat() && b.IsFloat()) {
						token.SetBool(a.ToFloat() < b.ToFloat());
					}
					else if (a.IsInt() && b.IsInt()) {
						token.SetBool(a.ToInt() < b.ToInt());
					}
					else if (a.IsString() && b.IsString()) {
						token.SetBool(a.ToString() < b.ToString());
					}
					
					token_stack.push_back(token);
				}
			}
			break;
			case FUNC::FUNC_FIND:
			{
				auto a = token_stack.back();
				token_stack.pop_back();

				x.return_value = Find(global, a.ToString());
				x.return_value_now = 0;
			}
			break;
			case FUNC::FUNC_RETURN_VALUE:
			{
				token_stack.push_back(x.return_value[x.return_value_now]);
			}
			break;
			case FUNC::FUNC_NEXT:
				x.return_value_now++;
				break;
			case FUNC::FUNC_LOAD_DATA:
			{
				std::string fileName =token_stack.back().ToString();
				fileName = fileName.substr(1, fileName.size() - 2);
				token_stack.pop_back();


				clau_parser::UserType* dir = token_stack.back().workspace.reader->GetUT();
				token_stack.pop_back();

				// fileName = substr... - todo!
				clau_parser::LoadData::LoadDataFromFile(fileName, *dir, 0, 0);
			}
			break;

			case FUNC::FUNC_ENTER:
				token_stack.back().workspace.reader->Enter();
				token_stack.pop_back();
				break;
			case FUNC::FUNC_QUIT:
				token_stack.back().workspace.reader->Quit();
				token_stack.pop_back();
				break;
			case FUNC::FUNC_SET_NAME:
			{ 
				auto name = token_stack.back().ToString();
				
				token_stack.pop_back();

				token_stack.back().workspace.reader->SetKey(name);
				
				token_stack.pop_back();
			}
			break;
			case FUNC::FUNC_SET_VALUE:
			{
				auto value = token_stack.back().ToString();
				
				token_stack.pop_back();

				token_stack.back().workspace.reader->SetData(value);
				
				token_stack.pop_back();
			}
			break;
			case FUNC::FUNC_GET_NAME:
			{
				Token token;
				token.SetString(token_stack.back().workspace.reader->GetKey());

				token_stack.pop_back();
				token_stack.push_back(token);
			}
			break;
			case FUNC::FUNC_GET_VALUE:
			{
				Token token;
				token.SetString(token_stack.back().workspace.reader->GetData());

				token_stack.pop_back();
				token_stack.push_back(token);
			}
			break;
			case FUNC::FUNC_GET_IDX:
			{
				Token token;
				token.SetInt(token_stack.back().workspace.reader->GetIdx());

				token_stack.pop_back();
				token_stack.push_back(token);
			}
			break;

			case FUNC::FUNC_SET_IDX:
			{
				//auto a = std::stoll(x.input[x.event_data[x.now]].ToString());

				auto a = token_stack.back().ToInt();
				token_stack.pop_back();

				auto space = token_stack.back().workspace;
				token_stack.pop_back();

				space.reader->SetIndex(a);
			}
			break;

			case FUNC::FUNC_WHILE:
			{
				//std::cout << "WHILE.... \n";
			}
			break;

			case FUNC::FUNC_AND_ALL:
			{
				x.now++;
				auto count = x.event_data[x.now];
				bool result = true;

				for (int i = 0; i < count; i += 1) {
					bool b = token_stack.back().ToBool();
					result = result && b;
					token_stack.pop_back();
				}

				Token temp;
				temp.SetBool(result);

				token_stack.push_back(temp);
			}
			break;
			case FUNC::FUNC_AND: 
			{
				bool result = true;

				for (int i = 0; i < 2; i += 1) {
					bool b = token_stack.back().ToBool();
					result = result && b;
					token_stack.pop_back();
				}

				Token temp;
				temp.SetBool(result);

				token_stack.push_back(temp);
			}
						
			break;
			case FUNC::FUNC_OR:
			{
				bool result = true;

				for (int i = 0; i < 2; i += 1) {
					bool b = token_stack.back().ToBool();
					result = result || b;
					token_stack.pop_back();
				}

				Token temp;

				temp.SetBool(result);

				token_stack.push_back(temp);
			}
			break;
			case FUNC::FUNC_GET_NOW:
			{
				auto space = token_stack.back().workspace;
				token_stack.pop_back();

				Token temp;
				temp.SetWorkspace(space.reader);
				

				token_stack.push_back(temp);

			}
				break;

			case FUNC::THEN:
			{
				auto param = token_stack.back().ToBool(); // bool
				token_stack.pop_back();

				if (param) {
					x.now++;
				}
				else {
					x.now++;

					x.now = x.event_data[x.now];

					x.now--;
				}
			}
			break;
			case FUNC::WHILE_END:
			{
				x.now++;
				x.now = x.event_data[x.now];

				//std::cout << "chk .. " << func_to_str[x.event_data[x.now]] << "\n";

				x.now--;
			}
			break;
			case FUNC::FUNC_IF:
			{
				//
			}
			break;

			case FUNC::IF_END:
			{
				//
			}
			break;
			case FUNC::FUNC_IS_END:
			{
				Token token;

				token.SetInt(x.return_value_now >= x.return_value.size());

				token_stack.push_back(token);
			}
			break;
			case FUNC::FUNC_NOT:
			{
				auto a = token_stack.back();
				token_stack.pop_back();

				a.SetBool(!a.ToBool());

				token_stack.push_back(a);
			}
			break;
			case FUNC::FUNC_IS_GROUP:
			{
				Token token;
				
				token.SetBool(token_stack.back().workspace.reader->IsGroup());

				token_stack.pop_back();

				token_stack.push_back(token);
			}
			break;
			case FUNC::FUNC_IS_ITEM:
			{
				Token token;
				
				token.SetBool(!token_stack.back().workspace.reader->IsGroup());

				token_stack.pop_back();

				token_stack.push_back(token);
			}
			break;
			case FUNC::FUNC_GET_SIZE:
			{
				Token token;
				token.SetString(std::to_string(token_stack.back().workspace.reader->Length()));
				token_stack.pop_back();

				token_stack.push_back(token);

				break;
			}
			case FUNC::FUNC_CLONE:
			{
				auto a = token_stack.back().workspace;
				token_stack.pop_back();

				Token b;
				b.workspace.reader = wiz::SmartPtr<clau_parser::Reader>(new clau_parser::Reader(*a.reader));
				token_stack.push_back(b);
			}
				break;

			case FUNC::FUNC_PRINT:
			{
				x.now++;

				std::vector<Token> vec;
				
				if (x.event_data[x.now] > 0) {
					vec.reserve(x.event_data[x.now]);
				}
				
				for (int i = 0; i < x.event_data[x.now]; ++i) {
					vec.push_back(token_stack.back());
					token_stack.pop_back();
				}
				for (int i = vec.size() - 1; i >= 0; --i) {
					if (vec[i].ToString() == "\\n"sv) {
						std::cout << "\n";
					}
					else {
						std::cout << vec[i].ToString();
					}
				}
			}
				break;
			default:
				//std::cout << "error \n";
				break;;

			}
			x.now++;
		}

		return return_value;
	}

	void Register(Event e) {
		_event_list.insert(std::make_pair(e.id, e));
	}

private:
	std::unordered_map<std::string, Event> _event_list;
};


void _MakeByteCode(clau_parser::UserType* ut, Event* e) {
	long long it_count = 0, ut_count = 0;

	for (long long i = 0; i < ut->GetIListSize(); ++i) {
		if (ut->IsItemList(i)) {
			if (!ut->GetItemList(it_count).GetName().empty()) {
				if (ut->GetItemList(it_count).GetName() == "id"sv && ut->GetName() == "Event"sv) {
					it_count++;
					continue;
				}
			
				{
					Token token(ut);
					token.SetString(ut->GetItemList(it_count).GetName());

					e->input->push_back(token);
				}
				
				e->event_data.push_back(FUNC::CONSTANT);
				e->event_data.push_back(e->input->size() - 1);

				{
					auto a = ut->GetItemList(it_count).Get();

					if (a._Starts_with("/")) {

						e->event_data.push_back(FUNC::START_DIR);
						
						auto tokens = clau_parser::tokenize(a, '/');

						for (int i = 0; i < tokens.size(); ++i) {
							if (tokens[i]._Starts_with("$"sv) && !tokens[i]._Starts_with("$parameter."sv) && !tokens[i]._Starts_with("$local."sv)) {
								clau_parser::UserType new_ut;

								new_ut.AddUserTypeItem(clau_parser::UserType(tokens[i]));

								_MakeByteCode(&new_ut, e);
							}
							else {
								Token token(ut);
								token.SetString(tokens[i]);

								e->input->push_back(token);
								e->event_data.push_back(FUNC::CONSTANT); 
								e->event_data.push_back(e->input->size() - 1); 
								e->event_data.push_back(FUNC::DIR); 
							}
						}

						e->event_data.push_back(FUNC::END_DIR); 
					}
					else if (a._Starts_with("@")) {
						auto tokens = clau_parser::tokenize(a, '@');

						for (int i = 0; i < tokens.size(); ++i) {
							if (tokens[i]._Starts_with("$"sv) && !tokens[i]._Starts_with("$parameter."sv) && !tokens[i]._Starts_with("$local."sv)) {
								clau_parser::UserType new_ut;

								new_ut.AddUserTypeItem(clau_parser::UserType(tokens[i]));

								_MakeByteCode(&new_ut, e);
								break;
							}
							else {
								Token token(ut);
								token.SetString(tokens[i]);

								e->input->push_back(token);
								e->event_data.push_back(FUNC::CONSTANT); 
								e->event_data.push_back(e->input->size() - 1); 
							}
						}
					}
					else {
						Token token;
						
						token.SetString(ut->GetItemList(it_count).Get());

						e->input->push_back(token);

						e->event_data.push_back(FUNC::CONSTANT); 
						e->event_data.push_back(e->input->size() - 1); 
					}
				}
			}

			// $while, $if
			else if (ut->GetItemList(it_count).GetName().empty()) {
				if (ut->GetItemList(it_count).Get() == "$while"sv) {
					e->event_data.push_back(FUNC::FUNC_WHILE); 
					int idx = e->event_data.size() - 1;
					int then_idx = 0;

					_MakeByteCode(ut->GetUserTypeList(ut_count), e);

					ut_count++; ++i;

					e->event_data.push_back(FUNC::THEN); 
					e->event_data.push_back(0); 
					then_idx = e->event_data.size() - 1;
					{
						//Event _e;
						//int count2 = _MakeByteCode(ut->GetUserTypeList(ut_count), &_e);
						//count2;
						//e->event_data.push_back(0); 
					}

					_MakeByteCode(ut->GetUserTypeList(ut_count), e);

					ut_count++; ++i;

					e->event_data.push_back(FUNC::WHILE_END); 
					e->event_data.push_back(idx); 

					e->event_data[then_idx] = e->event_data.size(); //
				}
				else if (ut->GetItemList(it_count).Get() == "$if"sv) {
					e->event_data.push_back(FUNC::FUNC_IF); 
					int idx = 0;

					_MakeByteCode(ut->GetUserTypeList(ut_count), e);
					ut_count++; ++i;

					e->event_data.push_back(FUNC::THEN); 
					e->event_data.push_back(0); 
					idx = e->event_data.size() - 1;

					_MakeByteCode(ut->GetUserTypeList(ut_count), e);

					ut_count++; ++i;

					e->event_data.push_back(FUNC::IF_END);
					e->event_data[idx] = e->event_data.size(); 
				}
				else if (ut->GetItemList(it_count).Get() == "TRUE"sv) {
					e->event_data.push_back(FUNC::TRUE); 
				}
				else if (ut->GetItemList(it_count).Get() == "FALSE"sv) {
					e->event_data.push_back(FUNC::FALSE); 
				}
				else  {
					auto a = ut->GetItemList(it_count).Get();

					if (a._Starts_with("/")) {

						e->event_data.push_back(FUNC::START_DIR);
						
						auto tokens = clau_parser::tokenize(a, '/');

						for (int i = 0; i < tokens.size(); ++i) {
							if (tokens[i]._Starts_with("$"sv) && !tokens[i]._Starts_with("$parameter."sv) && !tokens[i]._Starts_with("$local."sv)) {
								clau_parser::UserType new_ut;

								new_ut.AddUserTypeItem(clau_parser::UserType(tokens[i]));

								_MakeByteCode(&new_ut, e);
								break;
							}
							else {
								Token token(ut);
								
								token.SetString(tokens[i]);

								e->input->push_back(token);
								e->event_data.push_back(FUNC::CONSTANT); 
								e->event_data.push_back(e->input->size() - 1); 
								e->event_data.push_back(FUNC::DIR); 
							}
						}

						e->event_data.push_back(FUNC::END_DIR); 
					}
					else if (a._Starts_with("@")) {
						auto tokens = clau_parser::tokenize(a, '@');

						for (int i = 0; i < tokens.size(); ++i) {
							if (tokens[i]._Starts_with("$"sv) && !tokens[i]._Starts_with("$parameter."sv) && !tokens[i]._Starts_with("$local."sv)) {
								clau_parser::UserType new_ut;

								new_ut.AddUserTypeItem(clau_parser::UserType(tokens[i]));

								_MakeByteCode(&new_ut, e);
							}
							else {
								Token token;

								token.SetString(tokens[i]);

								e->input->push_back(token);
								e->event_data.push_back(FUNC::CONSTANT); 
								e->event_data.push_back(e->input->size() - 1); 
							}
						}
					}
					else {
						Token token;

						token.SetString(ut->GetItemList(it_count).Get());

						e->input->push_back(token);

						e->event_data.push_back(FUNC::CONSTANT); 
						e->event_data.push_back(e->input->size() - 1); 
					}
				}
			}
			
			it_count++;
		}
		else {
			std::string name = ut->GetUserTypeList(ut_count)->GetName();
			bool call_flag = false;

			if (name == "$call"sv) {
				call_flag = true;
			}
			
			if (name != "$query"sv) {
				_MakeByteCode(ut->GetUserTypeList(ut_count), e);
			}

			if (!ut->GetUserTypeList(ut_count)->GetName().empty()) {
				if (name._Starts_with("$"sv)) {
					Token token;

					token.SetFunc(); // | Token::Type::UserType

					if (name == "$query"sv) {
						_MakeByteCode(ut->GetUserTypeList(ut_count)->GetUserTypeList(0), e);

						token.func = FUNC::FUNC_QUERY;

						e->event_data.push_back(FUNC::FUNC_QUERY);

						{
							Token temp;
							temp.ut_val = wiz::SmartPtr<clau_parser::UserType>(new clau_parser::UserType(*ut->GetUserTypeList(ut_count)));
							
							e->input->push_back(temp);
							e->event_data.push_back(e->input->size() - 1);
						}
					}
					else if (name == "$clone"sv) {
						token.func = FUNC::FUNC_CLONE;

						e->event_data.push_back(FUNC::FUNC_CLONE);
					}
					else if (name == "$call"sv) {
						token.func = FUNC::FUNC_CALL;

						e->event_data.push_back(FUNC::FUNC_CALL);
						e->event_data.push_back(ut->GetUserTypeList(ut_count)->GetItemListSize());

					}
					else if (name == "$set_idx"sv) {
						token.func = FUNC::FUNC_SET_IDX;

						e->event_data.push_back(FUNC::FUNC_SET_IDX);
					}
					else if (name == "$add"sv) {
						token.func = FUNC::FUNC_ADD;

						e->event_data.push_back(FUNC::FUNC_ADD);
					}
					else if (name == "$get"sv) {
						token.func = FUNC::FUNC_GET;

						e->event_data.push_back(FUNC::FUNC_GET);
					}
					else if (name == "$get_name"sv) {
						token.func = FUNC::FUNC_GET_NAME;

						e->event_data.push_back(FUNC::FUNC_GET_NAME);
					}
					else if (name == "$find"sv) {
						token.func = FUNC::FUNC_FIND;

						e->event_data.push_back(FUNC::FUNC_FIND);
					}
					else if (name == "$NOT"sv) {
						token.func = FUNC::FUNC_NOT;

						e->event_data.push_back(FUNC::FUNC_NOT);
					}
					else if (name == "$is_end"sv) {
						token.func = FUNC::FUNC_IS_END;

						e->event_data.push_back(FUNC::FUNC_IS_END);
					}
					else if (name == "$load_data"sv) {
						token.func = FUNC::FUNC_LOAD_DATA;

						e->event_data.push_back(FUNC::FUNC_LOAD_DATA);
					}
					else if (name == "$next"sv) {
						token.func = FUNC::FUNC_NEXT;

						e->event_data.push_back(FUNC::FUNC_NEXT);
					}
					else if (name == "$enter"sv) {
						token.func = FUNC::FUNC_ENTER;

						e->event_data.push_back(FUNC::FUNC_ENTER);
					}
					else if (name == "$quit"sv) {
						token.func = FUNC::FUNC_QUIT;

						e->event_data.push_back(FUNC::FUNC_QUIT);
					}
					else if (name == "$parameter"sv) {
						for (int i = 0; i < ut->GetUserTypeList(ut_count)->GetItemListSize(); ++i) {

							auto name = (*e->input)[e->event_data.back()].ToString();
							e->event_data.pop_back(); // name
							e->event_data.pop_back(); // CONSTATNT

							e->parameter[name] = Token();
						}
					}
					else if (name == "$local"sv) {
						for (int i = 0; i < ut->GetUserTypeList(ut_count)->GetItemListSize(); ++i) {

							auto name = (*e->input)[e->event_data.back()].ToString();
							e->event_data.pop_back(); // name
							e->event_data.pop_back(); // CONSTANT

							e->local[name] = Token();
						}
					}
					else if (name == "$assign"sv) {
						token.func = FUNC::FUNC_ASSIGN;

						e->event_data.push_back(FUNC::FUNC_ASSIGN);
					}
					else if (name == "$COMP<"sv) {
						token.func = FUNC::FUNC_COMP_RIGHT;

						e->event_data.push_back(FUNC::FUNC_COMP_RIGHT);
					}
					else if (name == "$COMP>"sv) {
						token.func = FUNC::FUNC_COMP_LEFT;

						e->event_data.push_back(FUNC::FUNC_COMP_LEFT);
					}
					else if (name == "$AND_ALL"sv) {
						token.func = FUNC::FUNC_AND_ALL;

						e->event_data.push_back(FUNC::FUNC_AND_ALL);
						e->event_data.push_back(ut->GetUserTypeList(ut_count)->GetIListSize());
					}
					else if (name == "$AND"sv) {
						token.func = FUNC::FUNC_AND;

						e->event_data.push_back(FUNC::FUNC_AND);
					}
					else if (name == "$OR"sv) {
						token.func = FUNC::FUNC_OR;

						e->event_data.push_back(FUNC::FUNC_OR);
					}
					else if (name == "$get_size"sv) {
						token.func = FUNC::FUNC_GET_SIZE;

						e->event_data.push_back(FUNC::FUNC_GET_SIZE);
					}
					else if (name == "$get_idx"sv) {
						token.func = FUNC::FUNC_GET_IDX;

						e->event_data.push_back(FUNC::FUNC_GET_IDX);
					}
					else if (name == "$return"sv) {
						token.func = FUNC::FUNC_RETURN;

						e->event_data.push_back(FUNC::FUNC_RETURN);

						e->event_data.push_back(ut->GetUserTypeList(ut_count)->GetIListSize());
					}
					else if (name == "$return_value"sv) {
						token.func = FUNC::FUNC_RETURN_VALUE;

						e->event_data.push_back(FUNC::FUNC_RETURN_VALUE); 
					}
					else if (name == "$set_name"sv) {
						token.func = FUNC::FUNC_SET_NAME;

						e->event_data.push_back(FUNC::FUNC_SET_NAME); 
					}
					else if (name == "$get_value"sv) {
						token.func = FUNC::FUNC_GET_VALUE;

						e->event_data.push_back(FUNC::FUNC_GET_VALUE); 
					}
					else if (name == "$set_value"sv) {
						token.func = FUNC::FUNC_SET_VALUE;

						e->event_data.push_back(FUNC::FUNC_SET_VALUE); 
					}
					else if (name == "$is_item"sv) {
						token.func = FUNC::FUNC_IS_ITEM;

						e->event_data.push_back(FUNC::FUNC_IS_ITEM); 
					}
					else if (name == "$is_group"sv) {
						token.func = FUNC::FUNC_IS_GROUP;

						e->event_data.push_back(FUNC::FUNC_IS_GROUP); 
					}
					else if (name == "$is_quoted_str"sv) {
						token.func = FUNC::FUNC_IS_QUOTED_STR;

						e->event_data.push_back(FUNC::FUNC_IS_QUOTED_STR); 
					}
					else if (name == "$remove_quoted"sv) {
						token.func = FUNC::FUNC_REMOVE_QUOTED;

						e->event_data.push_back(FUNC::FUNC_REMOVE_QUOTED); 
					}
					else if (name == "$get_now"sv) {

						token.func = FUNC::FUNC_GET_NOW;

						e->event_data.push_back(FUNC::FUNC_GET_NOW); 
					}
					else if (name == "$print"sv) {	
						token.func = FUNC::FUNC_PRINT;

						e->event_data.push_back(FUNC::FUNC_PRINT); 
						e->event_data.push_back(ut->GetUserTypeList(ut_count)->GetIListSize());
					}

					// todo - add processing. errors..

					e->input->push_back(token);
				}
				else {
					Token token;
					
					token.SetString(std::move(name));
					
					e->input->push_back(token);
					e->event_data.push_back(FUNC::CONSTANT); 
					e->event_data.push_back(e->input->size() - 1); 
				}
			}

			ut_count++;
		}
	}
}

void Debug(const Event& e) {
	for (int i = 0; i < e.event_data.size(); ++i) {
		if (e.event_data[i] < FUNC::SIZE) {
			//std::cout << func_to_str[e.event_data[i]] << " ";
		}
		else {
			//std::cout << e.event_data[i] << " \n";
		}
	}
}
// need to exception processing.
Event MakeByteCode(clau_parser::UserType* ut) {
	Event e;
	e.input = wiz::SmartPtr<std::vector<Token>>(new std::vector<Token>());

	_MakeByteCode(ut, &e);

	
	e.event_data.push_back(FUNC::FUNC_RETURN);
	e.id = ut->GetItem("id")[0].Get();

	Debug(e);

	return e;
}


int main(void)
{
	VM vm;
	int start = clock();
	clau_parser::UserType global;
	clau_parser::LoadData::LoadDataFromFile("C:\\Users\\vztpv\\source\\repos\\ClauScriptPlusPlus\\ClauScriptPlusPlus\\test.txt",
		global, 1, 0); // DO NOT change!

	auto arr = global.GetUserTypeIdx("Event");
	

	for (auto x : arr) {
		//std::cout << x << " ";
		//std::cout << global.GetUserTypeList(x)->ToString() << "\n";
		vm.Register(MakeByteCode(global.GetUserTypeList(x)));
	}
	for (int i = arr.size() - 1; i >= 0; --i) {
		global.RemoveUserTypeList(arr[i]);
	}

	vm.Run("main", &global);

	int last = clock();
		std::cout << last - start << "ms\n";
	
	clau_parser::LoadData::Save(global, "output.eu4");

	return 0;
}

