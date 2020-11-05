
#define _CRT_SECURE_NO_WARNINGS

#ifdef _DEBUG
#include <vld.h>
#endif


#include "ClauText.h" 
#include <string>
#include <algorithm>
//
#include <Windows.h>
//
#include <wx/wx.h>

#include <wx/defs.h>
//
#include <wx/artprov.h>
#include <wx/xrc/xmlres.h>
#include <wx/string.h>
#include <wx/bitmap.h>
#include <wx/image.h>
#include <wx/icon.h>
#include <wx/menu.h>
#include <wx/gdicmn.h>
#include <wx/font.h>
#include <wx/colour.h>
#include <wx/settings.h>
#include <wx/button.h>
#include <wx/textctrl.h>
#include <wx/sizer.h>
#include <wx/dataview.h>
#include <wx/stc/stc.h>
#include <wx/frame.h>




class UtInfo {
public:
	wiz::load_data::UserType* global = nullptr;
	wiz::load_data::UserType* ut;
	std::string dir;
	long long itCount = 0;
	long long utCount = 0;
	long long count = 0;
public:
	UtInfo(wiz::load_data::UserType*  global, wiz::load_data::UserType* ut, const std::string& dir, long long itCount = 0, long long utCount = 0)
		: global(global), ut(ut), itCount(itCount), utCount(utCount), count(0), dir(dir)
	{
		//
	}
};

// for @insert, @update, @delete
inline bool EqualFunc(wiz::load_data::UserType* global, wiz::load_data::UserType* eventUT, const wiz::load_data::ItemType<WIZ_STRING_TYPE>& x,
	const wiz::load_data::ItemType<WIZ_STRING_TYPE>& y, long long x_idx, const std::string& dir) {

	if (y.Get() == "%any"sv) {
		return true;
	}

	if (wiz::String::startsWith(y.Get().ToString(), "%event_"sv)) {
		std::string event_id = y.Get().ToString().substr(7);

		wiz::ClauText clautext;
		wiz::ExecuteData executeData;
		executeData.pEvents = eventUT;
		wiz::Option option;

		std::string statements;

			// do not use NONE!(in user?)
		statements += " Event = { id = NONE  ";
		statements += " $call = { id = " + event_id + " ";
		statements += " name = " + x.GetName().ToString() + " ";
		statements += " value = " + y.Get().ToString() + " ";
		statements += " is_user_type = FALSE ";
		statements += " real_dir =  " + wiz::load_data::LoadData::GetRealDir(dir, global) + " ";
		statements += " relative_dir = " + dir + " ";
		statements += " idx = " + std::to_string(x_idx) + " "; // removal?
		statements += " } ";

		statements += " $return = { $return_value = { } }  ";
		statements += " } ";
		std::string result = clautext.execute_module(" Main = { $call = { id = NONE  } } " + statements, global, executeData, option, 1);
		
		if (result == "TRUE"sv) {
			return true;
		}
		return false;
	}
	return x.Get() == y.Get();
}


bool _InsertFunc(wiz::load_data::UserType* global, wiz::load_data::UserType* insert_ut, wiz::load_data::UserType* eventUT) {
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
			if (x.ut->IsItemList(i) && x.ut->GetItemList(it_count).GetName().ToString().empty()
				&& !wiz::String::startsWith(x.ut->GetItemList(it_count).Get().ToString(), "@"sv)) {
				// chk exist all case of value?
				auto item = x.global->GetItemIdx("");
				// no exist -> return false;
				if (item.empty()) {
					// LOG....
					return false;
				}

				bool pass = false;
				for (long long j = 0; j < item.size(); ++j) {
					if (EqualFunc(x.global, eventUT, x.global->GetItemList(item[j]), x.ut->GetItemList(it_count), item[j], 
						x.dir)) {
						pass = true;
						break;
					}
				}
				if (!pass) {
					return false;
				}
			}
			else if (x.ut->IsItemList(i) && !x.ut->GetItemList(it_count).GetName().ToString().empty() && !wiz::String::startsWith(x.ut->GetItemList(it_count).GetName().ToString(), "@"sv)) {
				// chk exist all case of value?
				auto item = x.global->GetItemIdx(x.ut->GetItemList(it_count).GetName().ToString());
				// no exist -> return false;
				if (item.empty()) {
					// LOG....
					return false;
				}

				bool pass = false;

				for (long long j = 0; j < item.size(); ++j) {
					if (EqualFunc(x.global, eventUT, x.global->GetItemList(item[j]), x.ut->GetItemList(it_count), item[j], 
						x.dir )) {
						pass = true;
						break;
					}
				}
				if (!pass) {
					return false;
				}

			}
			else if (x.ut->IsUserTypeList(i) && !wiz::String::startsWith(x.ut->GetUserTypeList(ut_count)->GetName().ToString(), "@"sv)) {
				// chk all case exist of @.
				// no exist -> return false;
				if (x.ut->GetUserTypeList(ut_count)->GetName() == "$"sv) {
					ut_count++;
					count++;
					continue;
				}

				auto usertype = x.global->GetUserTypeItemIdx(x.ut->GetUserTypeList(ut_count)->GetName().ToString());

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

bool _RemoveFunc(wiz::load_data::UserType* global, wiz::load_data::UserType* insert_ut, wiz::load_data::UserType* eventUT) {
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
			if (x.ut->IsItemList(i) && x.ut->GetItemList(it_count).GetName().ToString().empty()
				&& !wiz::String::startsWith(x.ut->GetItemList(it_count).Get().ToString(), "@"sv)) {
				// chk exist all case of value?
				auto item = x.global->GetItemIdx("");
				// no exist -> return false;
				if (item.empty()) {
					// LOG....
					return false;
				}

				bool pass = false;
				for (long long j = 0; j < item.size(); ++j) {
					if (EqualFunc(x.global, eventUT, x.global->GetItemList(item[j]), x.ut->GetItemList(it_count), item[j],
						x.dir )) {
						pass = true;
						break;
					}
				}
				if (!pass) {
					return false;
				}
			}
			else if (x.ut->IsItemList(i) && !x.ut->GetItemList(it_count).GetName().ToString().empty() && !wiz::String::startsWith(x.ut->GetItemList(it_count).GetName().ToString(), "@"sv)) {
				// chk exist all case of value?
				auto item = x.global->GetItemIdx(x.ut->GetItemList(it_count).GetName().ToString());
				// no exist -> return false;
				if (item.empty()) {
					// LOG....
					return false;
				}

				bool pass = false;

				for (long long j = 0; j < item.size(); ++j) {
					if (EqualFunc(x.global, eventUT, x.global->GetItemList(item[j]), x.ut->GetItemList(it_count), item[j],
						x.dir )) {
						pass = true;
						break;
					}
				}
				if (!pass) {
					return false;
				}

			}
			else if (x.ut->IsUserTypeList(i) && !wiz::String::startsWith(x.ut->GetUserTypeList(ut_count)->GetName().ToString(), "@"sv)) {
				// chk all case exist of @.
				// no exist -> return false;
				if (x.ut->GetUserTypeList(ut_count)->GetName() == "$"sv) {
					ut_count++;
					count++;
					continue;
				}

				auto usertype = x.global->GetUserTypeItemIdx(x.ut->GetUserTypeList(ut_count)->GetName().ToString());

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


bool _UpdateFunc(wiz::load_data::UserType* global, wiz::load_data::UserType* insert_ut, wiz::load_data::UserType* eventUT) {
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
			if (x.ut->IsItemList(i) && x.ut->GetItemList(it_count).GetName().ToString().empty()
				&& !wiz::String::startsWith(x.ut->GetItemList(it_count).Get().ToString(), "@"sv)) {
				// chk exist all case of value?
				auto item = x.global->GetItemIdx("");
				// no exist -> return false;
				if (item.empty()) {
					// LOG....
					return false;
				}

				bool pass = false;
				for (long long j = 0; j < item.size(); ++j) {
					if (EqualFunc(x.global, eventUT, x.global->GetItemList(item[j]), x.ut->GetItemList(it_count), item[j], 
						x.dir )) {
						pass = true;
						break;
					}
				}
				if (!pass) {
					return false;
				}
			}
			else if (x.ut->IsItemList(i) && !x.ut->GetItemList(it_count).GetName().ToString().empty() && !wiz::String::startsWith(x.ut->GetItemList(it_count).GetName().ToString(), "@"sv)) {
				// chk exist all case of value?
				auto item = x.global->GetItemIdx(x.ut->GetItemList(it_count).GetName().ToString());
				// no exist -> return false;
				if (item.empty()) {
					// LOG....
					return false;
				}

				bool pass = false;

				for (long long j = 0; j < item.size(); ++j) {
					if (EqualFunc(x.global, eventUT, x.global->GetItemList(item[j]), x.ut->GetItemList(it_count), item[j],
						dir)) {
						pass = true;
						break;
					}
				}
				if (!pass) {
					return false;
				}

			}
			else if (x.ut->IsUserTypeList(i) && !wiz::String::startsWith(x.ut->GetUserTypeList(ut_count)->GetName().ToString(), "@"sv)) {
				// chk all case exist of @.
				// no exist -> return false;
				if (x.ut->GetUserTypeList(ut_count)->GetName() == "$"sv) {
					ut_count++;
					count++;
					continue;
				}

				auto usertype = x.global->GetUserTypeItemIdx(x.ut->GetUserTypeList(ut_count)->GetName().ToString());

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
bool InsertFunc(wiz::load_data::UserType* global, wiz::load_data::UserType* insert_ut, wiz::load_data::UserType* eventUT) {
	if (!_InsertFunc(global, insert_ut, eventUT)) {
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
			if (x.ut->IsItemList(i) && x.ut->GetItemList(it_count).GetName().ToString().empty()
				&& wiz::String::startsWith(x.ut->GetItemList(it_count).Get().ToString(), "@"sv)) {

				if (wiz::String::startsWith(x.ut->GetItemList(it_count).Get().ToString(), "@%event_"sv)) {
					std::string event_id = x.ut->GetItemList(it_count).Get().ToString().substr(8);

					wiz::ClauText clautext;
					wiz::ExecuteData executeData;
					executeData.pEvents = insert_ut->GetParent();
					wiz::Option option;

					std::string statements;

					// do not use NONE!(in user?)
					statements += " Event = { id = NONE  ";
					statements += " $call = { id = " + event_id + " ";
					statements += " name = EMPTY_STRING ";
					statements += " is_user_type = FALSE ";
					statements += " } ";

					statements += " $return = { $return_value = { } }  ";
					statements += " } ";
					std::string result = clautext.execute_module(" Main = { $call = { id = NONE  } } " + statements, x.global, executeData, option, 1);

					x.global->AddItem("", result);
				}
				else {
					x.global->AddItem("", x.ut->GetItemList(it_count).Get().ToString().substr(1));
				}

				it_count++;
			}
			else if (x.ut->IsItemList(i) && wiz::String::startsWith(x.ut->GetItemList(it_count).GetName().ToString(), "@"sv)) {
				if (wiz::String::startsWith(x.ut->GetItemList(it_count).Get().ToString(), "%event_"sv)) {
					std::string event_id = x.ut->GetItemList(it_count).Get().ToString().substr(7);

					wiz::ClauText clautext;
					wiz::ExecuteData executeData;
					executeData.pEvents = insert_ut->GetParent();
					wiz::Option option;

					std::string statements;

					// do not use NONE!(in user?)
					statements += " Event = { id = NONE  ";
					statements += " $call = { id = " + event_id + " ";
					statements += " name = EMPTY_STRING ";
					statements += " is_user_type = FALSE ";
					statements += " } ";

					statements += " $return = { $return_value = { } }  ";
					statements += " } ";
					std::string result = clautext.execute_module(" Main = { $call = { id = NONE  } } " + statements, x.global, executeData, option, 1);

					x.global->AddItem(
						x.ut->GetItemList(it_count).GetName().ToString().substr(1),
						result);
				}
				else {
					x.global->AddItem(
						x.ut->GetItemList(it_count).GetName().ToString().substr(1),
						x.ut->GetItemList(it_count).Get().ToString());
				}
				it_count++;
			}
			else if (x.ut->IsUserTypeList(i) && wiz::String::startsWith(x.ut->GetUserTypeList(ut_count)->GetName().ToString(), "@"sv)) {
				x.global->LinkUserType(x.ut->GetUserTypeList(ut_count));
				x.ut->GetUserTypeList(ut_count)->SetName(x.ut->GetUserTypeList(ut_count)->GetName().ToString().substr(1));
				x.ut->GetUserTypeList(ut_count) = nullptr;
			
				x.ut->RemoveUserTypeList(ut_count);
				count--;
				i--;
			}
			else if (x.ut->IsUserTypeList(i) && !wiz::String::startsWith(x.ut->GetUserTypeList(ut_count)->GetName().ToString(), "@"sv)) {
				if (x.ut->GetUserTypeList(ut_count)->GetName() == "$"sv) {
					for (long long j = 0; j < x.global->GetUserTypeListSize(); ++j) {
						if (_InsertFunc(x.global->GetUserTypeList(j), x.ut->GetUserTypeList(ut_count), eventUT)) {
							que.push(UtInfo(x.global->GetUserTypeList(j), x.ut->GetUserTypeList(ut_count)
							, x.dir + "/$ut" + std::to_string(j)
							));
						}
					}
				}
				else {
					auto usertype = x.global->GetUserTypeItemIdx(x.ut->GetUserTypeList(ut_count)->GetName().ToString());

					for (long long j = 0; j < usertype.size(); ++j) {
						if (_InsertFunc(x.global->GetUserTypeList(usertype[j]), x.ut->GetUserTypeList(ut_count), eventUT)) {
							que.push(UtInfo(x.global->GetUserTypeList(usertype[j]), x.ut->GetUserTypeList(ut_count), 
								x.dir + "/$ut" + std::to_string(usertype[j])));
						}
					}
				}
				ut_count++;
			}
			else if(x.ut->IsUserTypeList(i)) {
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

bool RemoveFunc(wiz::load_data::UserType* global, wiz::load_data::UserType* insert_ut, wiz::load_data::UserType* eventUT) {
	if (!_RemoveFunc(global, insert_ut, eventUT)) {
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
		long long ut_count = x.ut->GetUserTypeListSize() -1;
		long long it_count = x.ut->GetItemListSize() - 1;
		long long count = x.ut->GetIListSize() - 1;

		//chk only @  ! - todo
		for (long long i = x.ut->GetIListSize() - 1; i >= 0; --i) {
			if (x.ut->IsItemList(i) && x.ut->GetItemList(it_count).GetName().ToString().empty()
				&& wiz::String::startsWith(x.ut->GetItemList(it_count).Get().ToString(), "@"sv)) {

				if (wiz::String::startsWith(x.ut->GetItemList(it_count).Get().ToString(), "@%event_"sv)) {
					std::string event_id = x.ut->GetItemList(it_count).Get().ToString().substr(8);

					wiz::ClauText clautext;
					wiz::ExecuteData executeData;
					executeData.pEvents = insert_ut->GetParent();
					wiz::Option option;

					wiz::load_data::UserType callUT;
					std::string statements;

					statements += " Event = { id = NONE  ";
					statements += " $call = { id = " + event_id + " ";
					statements += " name = _name  ";
					statements += " value = _value ";
					statements += " is_user_type = FALSE ";
					statements += " real_dir =  _real_dir "; //+wiz::load_data::LoadData::GetRealDir(dir, global) + " ";
					statements += " relative_dir = _dir "; // +dir + " ";
					statements += " idx = _idx "; // +std::to_string(x_idx) + " "; // removal?
					statements += " } ";

					statements += " $return = { $return_value = { } }  ";
					statements += " } ";

					wiz::load_data::LoadData::LoadDataFromString(statements, callUT);

					auto temp = x.global->GetItemIdx("");

					for (long long j = 0; j < temp.size(); ++j) {
						auto callInfo = wiz::load_data::UserType::Find(&callUT, "/./Event/$call");

						callInfo.second[0]->SetItem("name", "$NO_NAME");
						callInfo.second[0]->SetItem("value", x.global->GetItemList(temp[j]).Get());
						callInfo.second[0]->SetItem("real_dir", wiz::load_data::LoadData::GetRealDir(x.dir, x.global));
						callInfo.second[0]->SetItem("relative_dir", x.dir);
						callInfo.second[0]->SetItem("idx", std::to_string(temp[j]));

						std::string result = clautext.execute_module(" Main = { $call = { id = NONE  } } " + callUT.ToString(), x.global,
							executeData, option, 1);

						if (result == "TRUE"sv) { //x.ut->GetItemList(it_count).Get().ToString().substr(1)) {
							x.global->RemoveItemList(temp[j]);
						}
					}
				}
				else {
					x.global->RemoveItemList("", x.ut->GetItemList(it_count).Get().ToString().substr(1));
				}
				it_count--;
				//x.global->AddItemType(wiz::load_data::ItemType<WIZ_STRING_TYPE>("", x.ut->GetItemList(it_count).Get().ToString().substr(1)));
			}
			else if (x.ut->IsItemList(i) && wiz::String::startsWith(x.ut->GetItemList(it_count).GetName().ToString(), "@"sv)) {
				//x.global->AddItemType(wiz::load_data::ItemType<WIZ_STRING_TYPE>(
				//	x.ut->GetItemList(it_count).GetName().ToString().substr(1),
				//	x.ut->GetItemList(it_count).Get().ToString()));
				if (wiz::String::startsWith(x.ut->GetItemList(it_count).Get().ToString(), "%event_"sv)) {
					std::string event_id = x.ut->GetItemList(it_count).Get().ToString().substr(7);

					wiz::ClauText clautext;
					wiz::ExecuteData executeData;
					executeData.pEvents = insert_ut->GetParent();
					wiz::Option option;
					wiz::load_data::UserType callUT;
					std::string statements;

					statements += " Event = { id = NONE  ";
					statements += " $call = { id = " + event_id + " ";
					statements += " name = _name  ";
					statements += " value = _value ";
					statements += " is_user_type = FALSE ";
					statements += " real_dir =  _real_dir "; //+wiz::load_data::LoadData::GetRealDir(dir, global) + " ";
					statements += " relative_dir = _dir "; // +dir + " ";
					statements += " idx = _idx "; // +std::to_string(x_idx) + " "; // removal?
					statements += " } ";

					statements += " $return = { $return_value = { } }  ";
					statements += " } ";

					wiz::load_data::LoadData::LoadDataFromString(statements, callUT);

					auto name = x.ut->GetItemList(it_count).GetName().ToString().substr(1);
					auto temp = x.global->GetItemIdx(name);

					if (wiz::String::startsWith(name, "&"sv) && name.size() >= 2) {
						long long idx = std::stoll(name.substr(1));

						if (idx < 0 || idx >= x.global->GetItemListSize()) { // .size()) {
							return false;
						}
						auto valName = x.ut->GetItemList(it_count).Get().ToString();

						auto callInfo = wiz::load_data::UserType::Find(&callUT, "/./Event/$call");

						callInfo.second[0]->SetItem("name", name);
						callInfo.second[0]->SetItem("value", x.global->GetItemList(idx).Get());
						callInfo.second[0]->SetItem("real_dir", wiz::load_data::LoadData::GetRealDir(x.dir, x.global));
						callInfo.second[0]->SetItem("relative_dir", x.dir);
						callInfo.second[0]->SetItem("idx", std::to_string(idx));

						std::string result = clautext.execute_module(" Main = { $call = { id = NONE  } } " + callUT.ToString(), x.global,
							executeData, option, 1);

						if (result == "TRUE"sv) {
							x.global->RemoveItemList(idx);
						}
					}
					else {
						for (long long j = 0; j < temp.size(); ++j) {
							auto callInfo = wiz::load_data::UserType::Find(&callUT, "/./Event/$call");

							callInfo.second[0]->SetItem("name", name);
							callInfo.second[0]->SetItem("value", x.global->GetItemList(temp[j]).Get());
							callInfo.second[0]->SetItem("real_dir", wiz::load_data::LoadData::GetRealDir(x.dir, x.global));
							callInfo.second[0]->SetItem("relative_dir", x.dir);
							callInfo.second[0]->SetItem("idx", std::to_string(temp[j]));

							std::string result = clautext.execute_module(" Main = { $call = { id = NONE  } } " + callUT.ToString(), x.global,
								executeData, option, 1);

							if (result == "TRUE"sv) {
								x.global->RemoveItemList(temp[j]);
							}
						}
					}
				}
				else {
					x.global->RemoveItemList(x.ut->GetItemList(it_count).GetName().ToString().substr(1), x.ut->GetItemList(it_count).Get().ToString());
				}

				it_count--;
			}
			else if (x.ut->IsUserTypeList(i) && wiz::String::startsWith(x.ut->GetUserTypeList(ut_count)->GetName().ToString(), "@"sv)) {
				if (x.ut->GetUserTypeList(ut_count)->GetName() == "@$"sv) {
					for (long long j = x.global->GetUserTypeListSize() - 1; j >= 0; --j) {
						if (_RemoveFunc(x.global->GetUserTypeList(j), x.ut->GetUserTypeList(ut_count), eventUT)) {
							delete[] x.global->GetUserTypeList(j);
							x.global->GetUserTypeList(j) = nullptr;
							x.global->RemoveUserTypeList(j);
						}
					}
				}
				else {
					auto usertype = x.global->GetUserTypeItemIdx(x.ut->GetUserTypeList(ut_count)->GetName().ToString().substr(1));

					for (long long j = usertype.size() - 1; j >= 0; --j) {
						if (_RemoveFunc(x.global->GetUserTypeList(usertype[j]), x.ut->GetUserTypeList(ut_count), eventUT)) {
							x.global->RemoveUserTypeList(usertype[j]);
						}
					}
				}
				ut_count--;
			}
			else if (x.ut->IsUserTypeList(i) && false == wiz::String::startsWith(x.ut->GetUserTypeList(ut_count)->GetName().ToString(), "@"sv)) {
				if (x.ut->GetUserTypeList(ut_count)->GetName() == "$"sv) {
					for (long long j = 0; j < x.global->GetUserTypeListSize(); ++j) {
						if (_RemoveFunc(x.global->GetUserTypeList(j), x.ut->GetUserTypeList(ut_count), eventUT)) {
							que.push(UtInfo(x.global->GetUserTypeList(j), x.ut->GetUserTypeList(ut_count), x.dir + "/$ut" + std::to_string(j)));
						}
					}
				}
				else {
					auto usertype = x.global->GetUserTypeItemIdx(x.ut->GetUserTypeList(ut_count)->GetName().ToString());

					for (long long j = 0; j < usertype.size(); ++j) {
						if (_RemoveFunc(x.global->GetUserTypeList(usertype[j]), x.ut->GetUserTypeList(ut_count), eventUT)) {
							que.push(UtInfo(x.global->GetUserTypeList(usertype[j]), x.ut->GetUserTypeList(ut_count), 
								x.dir + "/$ut" + std::to_string(usertype[j]) ));
						}
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

bool UpdateFunc(wiz::load_data::UserType* global, wiz::load_data::UserType* insert_ut, wiz::load_data::UserType* eventUT) {
	if (!_UpdateFunc(global, insert_ut, eventUT)) {
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
			if (x.ut->IsItemList(i) && x.ut->GetItemList(it_count).GetName().ToString().empty()
				&& wiz::String::startsWith(x.ut->GetItemList(it_count).Get().ToString(), "@"sv)) {
				// think @&0 = 3 # 0 <- index, 3 <- value.
				//x.global->GetItemList(0).Set(0, x.ut->GetItemList(it_count).Get());
			}
			else if (x.ut->IsItemList(i) && wiz::String::startsWith(x.ut->GetItemList(it_count).GetName().ToString(), "@"sv)) {
				if (wiz::String::startsWith(x.ut->GetItemList(it_count).Get().ToString(), "%event_"sv)) {
					std::string event_id = x.ut->GetItemList(it_count).Get().ToString().substr(7);
					
					wiz::ClauText clautext;
					wiz::ExecuteData executeData;
					executeData.pEvents = insert_ut->GetParent();
					wiz::Option option;
					
					wiz::load_data::UserType callUT;
					std::string statements;

					statements += " Event = { id = NONE  ";
					statements += " $call = { id = " + event_id + " ";
					statements += " name = _name  ";
					statements += " value = _value ";
					statements += " is_user_type = FALSE ";
					statements += " real_dir =  _real_dir "; //+wiz::load_data::LoadData::GetRealDir(dir, global) + " ";
					statements += " relative_dir = _dir "; // +dir + " ";
					statements += " idx = _idx "; // +std::to_string(x_idx) + " "; // removal?
					statements += " } ";

					statements += " $return = { $return_value = { } }  ";
					statements += " } ";

					wiz::load_data::LoadData::LoadDataFromString(statements, callUT);

					auto temp = wiz::load_data::UserType::Find(&callUT, "/./Event/$call").second[0];
					
					std::string name = x.ut->GetItemList(it_count).GetName().ToString().substr(1);
					auto position = x.global->GetItemIdx(name);
					
					{
						std::string name = x.ut->GetItemList(it_count).GetName().ToString().substr(1);
						if (wiz::String::startsWith(name, "&"sv) && name.size() >= 2) {
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

							temp->SetItem("name", x.ut->GetItemList(it_count).GetName().ToString().substr(1));
							temp->SetItem("value", x.global->GetItemList(position[j]).Get());
							temp->SetItem("relative_dir", x.dir);
							temp->SetItem("real_dir", wiz::load_data::LoadData::GetRealDir(x.dir, x.global));
							temp->SetItem("idx", std::to_string(x.global->GetIlistIndex(position[j], 1)));

							std::string result = clautext.execute_module(" Main = { $call = { id = NONE  } } " + callUT.ToString(), x.global, executeData, option, 1);

							x.global->GetItemList(position[j]).Set(0, result);

						}
					}
				}
				else {
					std::string name = x.ut->GetItemList(it_count).GetName().ToString().substr(1);
					if (wiz::String::startsWith(name, "&"sv) && name.size() >= 2) {
						long long idx = std::stoll(name.substr(1));
						if (idx < 0 || idx >= x.global->GetItemListSize()) {
							return false;
						}
						auto value = x.ut->GetItemList(it_count).Get();
						x.global->GetItemList(idx).Set(0, value);
						return true;
					}
					else {
						x.global->SetItem(WIZ_STRING_TYPE(x.ut->GetItemList(it_count).GetName().ToString().substr(1)),
							x.ut->GetItemList(it_count).Get());
					}
				}
			}
			else if (x.ut->IsUserTypeList(i) && !wiz::String::startsWith(x.ut->GetUserTypeList(ut_count)->GetName().ToString(), "@"sv)) {
				if (x.ut->GetUserTypeList(ut_count)->GetName() == "$"sv) {
					for (long long j = 0; j < x.global->GetUserTypeListSize(); ++j) {
						if (_UpdateFunc(x.global->GetUserTypeList(j), x.ut->GetUserTypeList(ut_count), eventUT)) {
							que.push(UtInfo(x.global->GetUserTypeList(j), x.ut->GetUserTypeList(ut_count),
								x.dir + "/$ut" + std::to_string(j)));
						}
					}
				}
				else {
					auto usertype = x.global->GetUserTypeItemIdx(x.ut->GetUserTypeList(ut_count)->GetName().ToString());

					for (long long j = 0; j < usertype.size(); ++j) {
						if (_UpdateFunc(x.global->GetUserTypeList(usertype[j]), x.ut->GetUserTypeList(ut_count), eventUT)) {
							que.push(UtInfo(x.global->GetUserTypeList(usertype[j]), x.ut->GetUserTypeList(ut_count), 
								x.dir + "/$ut" + std::to_string(usertype[j])));
						}
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

///////////////////////////////////////////////////////////////////////////
#define NK_ENTER 13
#define NK_BACKSPACE 8

using namespace std;

class ChangeWindow : public wxDialog
{
private:
	// function??
	int view_mode;
	wiz::load_data::UserType* ut;
	bool isUserType; // ut(true) or it(false)
	int idx; // utidx or itidx. or ilist idx(type == insert)
	int type; // change 1, insert 2
protected:
	wxTextCtrl* var_text;
	wxTextCtrl* val_text;
	wxButton* ok;

	// Virtual event handlers, overide them in your derived class
	virtual void okOnButtonClick(wxCommandEvent& event) {
		string var(var_text->GetValue().ToUTF8());
		string val(val_text->GetValue().ToUTF8());

		if (1 == type) {
			if (isUserType) {
				ut->GetUserTypeList(idx)->SetName(var);
			}
			else {
				if (!val.empty()) {
					ut->GetItemList(idx).SetName(var);
					ut->GetItemList(idx).Set(0, val);
				}
			}
		}
		else if (2 == type && 2 == view_mode) {
			if (val.empty()) {
				ut->InsertUserTypeByIlist(idx, wiz::load_data::UserType(var));
			}
			else {
				if (!val.empty()) {
					ut->InsertItemByIlist(idx, var, val);
				}
			}
		}

		Close();
	}

public:
	ChangeWindow(wxWindow* parent, wiz::load_data::UserType* ut, bool isUserType, int idx, int type, int view_mode, wxWindowID id = wxID_ANY, const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(580, 198), long style = wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL);
	~ChangeWindow();
};

ChangeWindow::ChangeWindow(wxWindow* parent, wiz::load_data::UserType* ut, bool isUserType, int idx, int type, int view_mode, wxWindowID id, const wxPoint& pos, const wxSize& size, long style)
	: ut(ut), isUserType(isUserType), idx(idx), type(type), view_mode(view_mode), wxDialog(parent, id, "change/insert window", pos, size, style)
{

	wxBoxSizer* bSizer4;
	bSizer4 = new wxBoxSizer(wxVERTICAL);

	var_text = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	bSizer4->Add(var_text, 1, wxALL | wxEXPAND, 5);

	val_text = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, 0);
	bSizer4->Add(val_text, 1, wxALL | wxEXPAND, 5);

	ok = new wxButton(this, wxID_ANY, wxT("OK"), wxDefaultPosition, wxDefaultSize, 0);
	bSizer4->Add(ok, 0, wxALL | wxEXPAND, 5);

	if (1 == type) {
		if (isUserType) {
			var_text->SetValue(wiz::ToString(ut->GetUserTypeList(idx)->GetName()));
		}
		else {
			var_text->SetValue(wiz::ToString(ut->GetItemList(idx).GetName()));
			val_text->SetValue(wiz::ToString(ut->GetItemList(idx).Get(0)));
		}
	}


	this->SetSizer(bSizer4);
	this->Layout();

	// Connect Events
	ok->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ChangeWindow::okOnButtonClick), NULL, this);
}

ChangeWindow::~ChangeWindow()
{
	// Disconnect Events
	ok->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(ChangeWindow::okOnButtonClick), NULL, this);
}

///////////////////////////////////////////////////////////////////////////////
/// Class MainFrame
///////////////////////////////////////////////////////////////////////////////
class MainFrame : public wxFrame
{
private:
	bool isMain = false;
	int view_mode = 1; // todo, insert : when view_mode == 2.
	wiz::load_data::UserType global;
	wiz::load_data::UserType* now = nullptr;

	int dataViewListCtrlNo = -1;
	int position = -1;

private:
	void RefreshTable(wiz::load_data::UserType* now)
	{
		m_dataViewListCtrl1->DeleteAllItems();
		m_dataViewListCtrl2->DeleteAllItems();
		m_dataViewListCtrl3->DeleteAllItems();
		m_dataViewListCtrl4->DeleteAllItems();

		AddData(now);

		dataViewListCtrlNo = -1;
		position = -1;

		{
			wxDataViewListCtrl* ctrl[4];
			ctrl[0] = m_dataViewListCtrl1;
			ctrl[1] = m_dataViewListCtrl2;
			ctrl[2] = m_dataViewListCtrl3;
			ctrl[3] = m_dataViewListCtrl4;

			for (int i = 0; i < 4; ++i) {
				if (ctrl[i]->GetItemCount() > 0) {
					dataViewListCtrlNo = i;
					position = 0;

					ctrl[dataViewListCtrlNo]->SelectRow(position);
					ctrl[dataViewListCtrlNo]->SetFocus();
					break;
				}
			}
		}
	}
	void AddData(wiz::load_data::UserType* global)
	{
		if (1 == view_mode) {
			const int size = global->GetUserTypeListSize() + global->GetItemListSize();
			const int utSize = global->GetUserTypeListSize();
			const int size_per_unit = size / 4;
			const int last_size = size - size_per_unit * 3;
			int count = 0;
			int utCount = 0;
			int itCount = 0;

			wxVector<wxVariant> value;

			for (int i = 0; i < size_per_unit; ++i) {
				value.clear();

				if (count < utSize) {
					if (wiz::ToString(global->GetUserTypeList(utCount)->GetName()).empty()) {
						value.push_back(wxVariant(wxT("@NO_NAME")));
					}
					else {
						value.push_back(wxVariant(wxString(wiz::ToString(global->GetUserTypeList(utCount)->GetName()).c_str(), wxConvUTF8)));
					}
					value.push_back(wxVariant(wxT("")));
					utCount++;
				}
				else {
					value.push_back(wxVariant(wxString(wiz::ToString(global->GetItemList(itCount).GetName()).c_str(), wxConvUTF8)));
					value.push_back(wxVariant(wxString(wiz::ToString(global->GetItemList(itCount).Get(0)).c_str(), wxConvUTF8)));
					itCount++;
				}

				m_dataViewListCtrl1->AppendItem(value);
				count++;
			}
			for (int i = 0; i < size_per_unit; ++i) {
				value.clear();

				if (count < utSize) {
					if (wiz::ToString(global->GetUserTypeList(utCount)->GetName()).empty()) {
						value.push_back(wxVariant(wxT("@NO_NAME")));
					}
					else {
						value.push_back(wxVariant(wxString(wiz::ToString(global->GetUserTypeList(utCount)->GetName()).c_str(), wxConvUTF8)));
					}
					value.push_back(wxVariant(wxT("")));
					utCount++;
				}
				else {
					value.push_back(wxVariant(wxString(wiz::ToString(global->GetItemList(itCount).GetName()).c_str(), wxConvUTF8)));
					value.push_back(wxVariant(wxString(wiz::ToString(global->GetItemList(itCount).Get(0)).c_str(), wxConvUTF8)));
					itCount++;
				}

				m_dataViewListCtrl2->AppendItem(value);
				count++;
			}
			for (int i = 0; i < size_per_unit; ++i) {
				value.clear();

				if (count < utSize) {
					if (wiz::ToString(global->GetUserTypeList(utCount)->GetName()).empty()) {
						value.push_back(wxVariant(wxT("@NO_NAME")));
					}
					else {
						value.push_back(wxVariant(wxString(wiz::ToString(global->GetUserTypeList(utCount)->GetName()).c_str(), wxConvUTF8)));
					}
					value.push_back(wxVariant(wxT("")));
					utCount++;
				}
				else {
					value.push_back(wxVariant(wxString(wiz::ToString(global->GetItemList(itCount).GetName()).c_str(), wxConvUTF8)));
					value.push_back(wxVariant(wxString(wiz::ToString(global->GetItemList(itCount).Get(0)).c_str(), wxConvUTF8)));
					itCount++;
				}

				m_dataViewListCtrl3->AppendItem(value);
				count++;
			}
			for (int i = 0; i < last_size; ++i) {
				value.clear();

				if (count < utSize) {
					if (wiz::ToString(global->GetUserTypeList(utCount)->GetName()).empty()) {
						value.push_back(wxVariant(wxT("@NO_NAME")));
					}
					else {
						value.push_back(wxVariant(wxString(wiz::ToString(global->GetUserTypeList(utCount)->GetName()).c_str(), wxConvUTF8)));
					}
					value.push_back(wxVariant(wxT("")));
					utCount++;
				}
				else {
					value.push_back(wxVariant(wxString(wiz::ToString(global->GetItemList(itCount).GetName()).c_str(), wxConvUTF8)));
					value.push_back(wxVariant(wxString(wiz::ToString(global->GetItemList(itCount).Get(0)).c_str(), wxConvUTF8)));
					itCount++;
				}

				m_dataViewListCtrl4->AppendItem(value);
				count++;
			}
		}
		else {
			const int size = global->GetUserTypeListSize() + global->GetItemListSize();
			const int utSize = global->GetUserTypeListSize();
			const int size_per_unit = size / 4;
			const int last_size = size - size_per_unit * 3;
			int count = 0;
			int utCount = 0;
			int itCount = 0;

			wxVector<wxVariant> value;

			for (int i = 0; i < size_per_unit; ++i) {
				value.clear();

				if (global->IsUserTypeList(count)) {
					if (wiz::ToString(global->GetUserTypeList(utCount)->GetName()).empty()) {
						value.push_back(wxVariant(wxT("@NO_NAME")));
					}
					else {
						value.push_back(wxVariant(wxString(wiz::ToString(global->GetUserTypeList(utCount)->GetName()).c_str(), wxConvUTF8)));
					}
					value.push_back(wxVariant(wxT("")));
					utCount++;
				}
				else {
					value.push_back(wxVariant(wxString(wiz::ToString(global->GetItemList(itCount).GetName()).c_str(), wxConvUTF8)));
					value.push_back(wxVariant(wxString(wiz::ToString(global->GetItemList(itCount).Get(0)).c_str(), wxConvUTF8)));
					itCount++;
				}

				m_dataViewListCtrl1->AppendItem(value);
				count++;
			}
			for (int i = 0; i < size_per_unit; ++i) {
				value.clear();

				if (global->IsUserTypeList(count)) {
					if (wiz::ToString(global->GetUserTypeList(utCount)->GetName()).empty()) {
						value.push_back(wxVariant(wxT("@NO_NAME")));
					}
					else {
						value.push_back(wxVariant(wxString(wiz::ToString(global->GetUserTypeList(utCount)->GetName()).c_str(), wxConvUTF8)));
					}
					value.push_back(wxVariant(wxT("")));
					utCount++;
				}
				else {
					value.push_back(wxVariant(wxString(wiz::ToString(global->GetItemList(itCount).GetName()).c_str(), wxConvUTF8)));
					value.push_back(wxVariant(wxString(wiz::ToString(global->GetItemList(itCount).Get(0)).c_str(), wxConvUTF8)));
					itCount++;
				}

				m_dataViewListCtrl2->AppendItem(value);
				count++;
			}
			for (int i = 0; i < size_per_unit; ++i) {
				value.clear();

				if (global->IsUserTypeList(count)) {
					if (wiz::ToString(global->GetUserTypeList(utCount)->GetName()).empty()) {
						value.push_back(wxVariant(wxT("@NO_NAME")));
					}
					else {
						value.push_back(wxVariant(wxString(wiz::ToString(global->GetUserTypeList(utCount)->GetName()).c_str(), wxConvUTF8)));
					}
					value.push_back(wxVariant(wxT("")));
					utCount++;
				}
				else {
					value.push_back(wxVariant(wxString(wiz::ToString(global->GetItemList(itCount).GetName()).c_str(), wxConvUTF8)));
					value.push_back(wxVariant(wxString(wiz::ToString(global->GetItemList(itCount).Get(0)).c_str(), wxConvUTF8)));
					itCount++;
				}

				m_dataViewListCtrl3->AppendItem(value);
				count++;
			}
			for (int i = 0; i < last_size; ++i) {
				value.clear();

				if (global->IsUserTypeList(count)) {
					if (wiz::ToString(global->GetUserTypeList(utCount)->GetName()).empty()) {
						value.push_back(wxVariant(wxT("@NO_NAME")));
					}
					else {
						value.push_back(wxVariant(wxString(wiz::ToString(global->GetUserTypeList(utCount)->GetName()).c_str(), wxConvUTF8)));
					}
					value.push_back(wxVariant(wxT("")));
					utCount++;
				}
				else {
					value.push_back(wxVariant(wxString(wiz::ToString(global->GetItemList(itCount).GetName()).c_str(), wxConvUTF8)));
					value.push_back(wxVariant(wxString(wiz::ToString(global->GetItemList(itCount).Get(0)).c_str(), wxConvUTF8)));
					itCount++;
				}

				m_dataViewListCtrl4->AppendItem(value);
				count++;
			}
		}
	}
protected:
	wxStyledTextCtrl* m_code;
	wxButton* m_code_run_button;

	wxMenuBar* menuBar;
	wxMenu* FileMenu;
	wxMenu* DoMenu;
	wxMenu* ViewMenu;
	wxMenu* WindowMenu;
	wxButton* back_button;
	wxTextCtrl* dir_text;
	wxButton* refresh_button;
	wxDataViewListCtrl* m_dataViewListCtrl1;
	wxDataViewListCtrl* m_dataViewListCtrl2;
	wxDataViewListCtrl* m_dataViewListCtrl3;
	wxDataViewListCtrl* m_dataViewListCtrl4;
	wxStatusBar* m_statusBar1;

	// Virtual event handlers, overide them in your derived class
	virtual void FileLoadMenuOnMenuSelection(wxCommandEvent& event) {
		if (!isMain) { return; }
		wxFileDialog* openFileDialog = new wxFileDialog(this);

		if (openFileDialog->ShowModal() == wxID_OK) {
			wxString _fileName = openFileDialog->GetPath();
			std::string fileName(_fileName.c_str());

			global.Remove();
			wiz::load_data::LoadData::LoadDataFromFile(fileName, global, 0, 0);
			now = &global;

			RefreshTable(now);

			SetTitle(wxT("ClauExplorer : ") + _fileName);
		}
		openFileDialog->Destroy();
	}
	virtual void FileSaveMenuOnMenuSelection(wxCommandEvent& event) {
		if (!isMain) { return; }
		wxFileDialog* saveFileDialog = new wxFileDialog(this, _("Save"), "", "",
			"", wxFD_SAVE | wxFD_OVERWRITE_PROMPT);

		if (saveFileDialog->ShowModal() == wxID_OK)
		{
			string fileName(saveFileDialog->GetPath().c_str());

			wiz::load_data::LoadData::SaveWizDB(global, fileName, "1");
		}
		saveFileDialog->Destroy();
	}
	virtual void FileExitMenuOnMenuSelection(wxCommandEvent& event) { Close(true); }
	virtual void InsertMenuOnMenuSelection(wxCommandEvent& event) {

		if (1 == view_mode) { return; }
		if (-1 == position) {
			ChangeWindow* changeWindow = new ChangeWindow(this, now, 0, std::max<int>(0, now->GetIListSize()), 2, view_mode);

			changeWindow->ShowModal();

			if (now) {
				RefreshTable(now);
			}
			return;
		}

		int idx = position + ((now->GetUserTypeListSize() + now->GetItemListSize()) / 4) * dataViewListCtrlNo;
		bool isUserType = now->IsUserTypeList(idx);

		if (dataViewListCtrlNo == -1) { return; }

		ChangeWindow* changeWindow = new ChangeWindow(this, now, isUserType, idx, 2, view_mode);

		changeWindow->ShowModal();

		if (now) {
			RefreshTable(now);
		}
	}
	virtual void ChangeMenuOnMenuSelection(wxCommandEvent& event) {
		if (-1 == position) { return; }

		if (1 == view_mode) {
			int idx = position + ((now->GetUserTypeListSize() + now->GetItemListSize()) / 4) * dataViewListCtrlNo;
			bool isUserType = (idx < now->GetUserTypeListSize());

			ChangeWindow* changeWindow = new ChangeWindow(this, now, isUserType,
				isUserType ? idx : idx - now->GetUserTypeListSize(), 1, view_mode);

			changeWindow->ShowModal();

			if (now) {
				RefreshTable(now);
			}
		}
		else {
			int idx = position + ((now->GetUserTypeListSize() + now->GetItemListSize()) / 4) * dataViewListCtrlNo;
			bool isUserType = now->IsUserTypeList(idx);

			ChangeWindow* changeWindow = new ChangeWindow(this, now, isUserType,
				isUserType ? now->GetUserTypeIndexFromIlistIndex(idx) : now->GetItemIndexFromIlistIndex(idx), 1, view_mode);

			changeWindow->ShowModal();

			if (now) {
				RefreshTable(now);
			}
		}
	}
	virtual void RemoveMenuOnMenuSelection(wxCommandEvent& event) {
		if (-1 == position) { return; }
		int idx = position + ((now->GetUserTypeListSize() + now->GetItemListSize()) / 4) * dataViewListCtrlNo;
		int type = 1;

		if (1 == view_mode) {
			bool isUserType = (idx < now->GetUserTypeListSize());

			if (isUserType) {
				now->RemoveUserTypeList(idx);
			}
			else {
				now->RemoveItemList(idx - now->GetUserTypeListSize());
			}
			RefreshTable(now);
		}
		else {
			bool isUserType = now->IsUserTypeList(idx);

			if (isUserType) {
				now->RemoveUserTypeList(now->GetUserTypeIndexFromIlistIndex(idx));
			}
			else {
				now->RemoveItemList(now->GetItemIndexFromIlistIndex(idx));
			}
			RefreshTable(now);
		}
	}
	virtual void back_buttonOnButtonClick(wxCommandEvent& event) {
		if (now && now->GetParent()) {
			RefreshTable(now->GetParent());
			now = now->GetParent();
		}
	}
	virtual void dir_textOnTextEnter(wxCommandEvent& event) {

		wiz::load_data::UserType* ut;

		// todo

	}
	virtual void refresh_buttonOnButtonClick(wxCommandEvent& event) {
		if (now) {
			RefreshTable(now);
		}
	}

	virtual void m_dataViewListCtrl1OnChar(wxKeyEvent& event) {
		dataViewListCtrlNo = 0; position = m_dataViewListCtrl1->GetSelectedRow();
		if (WXK_ESCAPE == event.GetKeyCode()) {
			wxDataViewListCtrl* ctrl[4];
			ctrl[0] = m_dataViewListCtrl1;
			ctrl[1] = m_dataViewListCtrl2;
			ctrl[2] = m_dataViewListCtrl3;
			ctrl[3] = m_dataViewListCtrl4;

			for (int i = 0; i < 4; ++i) {
				ctrl[i]->UnselectAll();
			}
			position = -1;
		}
		else if (1 == view_mode && NK_ENTER == event.GetKeyCode() && position >= 0 && position < now->GetUserTypeListSize()) {
			now = now->GetUserTypeList(position);
			RefreshTable(now);
		}
		else  if (2 == view_mode && NK_ENTER == event.GetKeyCode() && position >= 0 && position < m_dataViewListCtrl1->GetItemCount()) {
			if (now->IsUserTypeList(position)) {
				const int idx = now->GetUserTypeIndexFromIlistIndex(position);
				now = now->GetUserTypeList(idx);
				RefreshTable(now);
			}
		}
		else if (NK_BACKSPACE == event.GetKeyCode() && now->GetParent() != nullptr) {
			now = now->GetParent();
			RefreshTable(now);
		}
		else {
			wxDataViewListCtrl* ctrl[4];
			ctrl[0] = m_dataViewListCtrl1;
			ctrl[1] = m_dataViewListCtrl2;
			ctrl[2] = m_dataViewListCtrl3;
			ctrl[3] = m_dataViewListCtrl4;

			ctrl[dataViewListCtrlNo]->UnselectRow(position);

			if (WXK_UP == event.GetKeyCode() && dataViewListCtrlNo > -1 && position > 0)//< ctrl[dataViewListCtrlNo]->GetItemCount())
			{
				event.Skip();
				return;
			}
			else if (WXK_DOWN == event.GetKeyCode() && dataViewListCtrlNo > -1 && position >= 0 && position < ctrl[dataViewListCtrlNo]->GetItemCount() - 1)
			{
				event.Skip();
				return;
			}
			else if (WXK_LEFT == event.GetKeyCode() && dataViewListCtrlNo > 0 && position >= 0 && position < ctrl[dataViewListCtrlNo - 1]->GetItemCount())
			{
				dataViewListCtrlNo--;
			}
			else if (WXK_RIGHT == event.GetKeyCode() && dataViewListCtrlNo < 3 && position >= 0 && position < ctrl[dataViewListCtrlNo + 1]->GetItemCount())
			{
				dataViewListCtrlNo++;
			}

			ctrl[dataViewListCtrlNo]->SelectRow(position);
			ctrl[dataViewListCtrlNo]->SetFocus();
		}
	}
	virtual void m_dataViewListCtrl2OnChar(wxKeyEvent& event) {
		dataViewListCtrlNo = 1; position = m_dataViewListCtrl2->GetSelectedRow();
		if (WXK_ESCAPE == event.GetKeyCode()) {
			wxDataViewListCtrl* ctrl[4];
			ctrl[0] = m_dataViewListCtrl1;
			ctrl[1] = m_dataViewListCtrl2;
			ctrl[2] = m_dataViewListCtrl3;
			ctrl[3] = m_dataViewListCtrl4;

			for (int i = 0; i < 4; ++i) {
				ctrl[i]->UnselectAll();
			}
			position = -1;
		}
		else if (1 == view_mode && NK_ENTER == event.GetKeyCode() && dataViewListCtrlNo == 1 && position >= 0 && position + ((now->GetUserTypeListSize() + now->GetItemListSize()) / 4) < now->GetUserTypeListSize()) {
			now = now->GetUserTypeList(position + ((now->GetUserTypeListSize() + now->GetItemListSize()) / 4));
			RefreshTable(now);
		}
		else  if (2 == view_mode && NK_ENTER == event.GetKeyCode() && position >= 0 && position < m_dataViewListCtrl2->GetItemCount()) {
			const int pos = (position + ((now->GetUserTypeListSize() + now->GetItemListSize()) / 4));
			if (now->IsUserTypeList(pos)) {
				const int idx = now->GetUserTypeIndexFromIlistIndex(pos);
				now = now->GetUserTypeList(idx);
				RefreshTable(now);
			}
		}
		else if (NK_BACKSPACE == event.GetKeyCode() && now->GetParent() != nullptr) {
			now = now->GetParent();
			RefreshTable(now);
		}
		else {
			wxDataViewListCtrl* ctrl[4];
			ctrl[0] = m_dataViewListCtrl1;
			ctrl[1] = m_dataViewListCtrl2;
			ctrl[2] = m_dataViewListCtrl3;
			ctrl[3] = m_dataViewListCtrl4;

			ctrl[dataViewListCtrlNo]->UnselectRow(position);

			if (WXK_UP == event.GetKeyCode() && dataViewListCtrlNo > -1 && position > 0)//< ctrl[dataViewListCtrlNo]->GetItemCount())
			{
				event.Skip();
				return;
			}
			else if (WXK_DOWN == event.GetKeyCode() && dataViewListCtrlNo > -1 && position >= 0 && position < ctrl[dataViewListCtrlNo]->GetItemCount() - 1)
			{
				event.Skip();
				return;
			}
			else if (WXK_LEFT == event.GetKeyCode() && dataViewListCtrlNo > 0 && position >= 0 && position < ctrl[dataViewListCtrlNo - 1]->GetItemCount())
			{
				dataViewListCtrlNo--;
			}
			else if (WXK_RIGHT == event.GetKeyCode() && dataViewListCtrlNo < 3 && position >= 0 && position < ctrl[dataViewListCtrlNo + 1]->GetItemCount())
			{
				dataViewListCtrlNo++;
			}

			ctrl[dataViewListCtrlNo]->SelectRow(position);
			ctrl[dataViewListCtrlNo]->SetFocus();
		}

	}
	virtual void m_dataViewListCtrl3OnChar(wxKeyEvent& event) {
		dataViewListCtrlNo = 2; position = m_dataViewListCtrl3->GetSelectedRow();
		if (WXK_ESCAPE == event.GetKeyCode()) {
			wxDataViewListCtrl* ctrl[4];
			ctrl[0] = m_dataViewListCtrl1;
			ctrl[1] = m_dataViewListCtrl2;
			ctrl[2] = m_dataViewListCtrl3;
			ctrl[3] = m_dataViewListCtrl4;

			for (int i = 0; i < 4; ++i) {
				ctrl[i]->UnselectAll();
			}
			position = -1;
		}
		else if (1 == view_mode && NK_ENTER == event.GetKeyCode() && dataViewListCtrlNo == 2 && position >= 0 && position + ((now->GetUserTypeListSize() + now->GetItemListSize()) / 4) * 2 < now->GetUserTypeListSize()) {
			now = now->GetUserTypeList(position + ((now->GetUserTypeListSize() + now->GetItemListSize()) / 4) * 2);
			RefreshTable(now);
		}
		else  if (2 == view_mode && NK_ENTER == event.GetKeyCode() && position >= 0 && position < m_dataViewListCtrl3->GetItemCount()) {
			const int pos = (position + ((now->GetUserTypeListSize() + now->GetItemListSize()) / 4) * 2);
			if (now->IsUserTypeList(pos)) {
				const int idx = now->GetUserTypeIndexFromIlistIndex(pos);
				now = now->GetUserTypeList(idx);
				RefreshTable(now);
			}
		}
		else if (NK_BACKSPACE == event.GetKeyCode() && now->GetParent() != nullptr) {
			now = now->GetParent();
			RefreshTable(now);
		}
		else {
			wxDataViewListCtrl* ctrl[4];
			ctrl[0] = m_dataViewListCtrl1;
			ctrl[1] = m_dataViewListCtrl2;
			ctrl[2] = m_dataViewListCtrl3;
			ctrl[3] = m_dataViewListCtrl4;

			ctrl[dataViewListCtrlNo]->UnselectRow(position);

			if (WXK_UP == event.GetKeyCode() && dataViewListCtrlNo > -1 && position > 0)//< ctrl[dataViewListCtrlNo]->GetItemCount())
			{
				event.Skip();
				return;
			}
			else if (WXK_DOWN == event.GetKeyCode() && dataViewListCtrlNo > -1 && position >= 0 && position < ctrl[dataViewListCtrlNo]->GetItemCount() - 1)
			{
				event.Skip();
				return;
			}
			else if (WXK_LEFT == event.GetKeyCode() && dataViewListCtrlNo > 0 && position >= 0 && position < ctrl[dataViewListCtrlNo - 1]->GetItemCount())
			{
				dataViewListCtrlNo--;
			}
			else if (WXK_RIGHT == event.GetKeyCode() && dataViewListCtrlNo < 3 && position >= 0 && position < ctrl[dataViewListCtrlNo + 1]->GetItemCount())
			{
				dataViewListCtrlNo++;
			}

			ctrl[dataViewListCtrlNo]->SelectRow(position);
			ctrl[dataViewListCtrlNo]->SetFocus();
		}
	}
	virtual void m_dataViewListCtrl4OnChar(wxKeyEvent& event) {
		dataViewListCtrlNo = 3; position = m_dataViewListCtrl4->GetSelectedRow();
		if (WXK_ESCAPE == event.GetKeyCode()) {
			wxDataViewListCtrl* ctrl[4];
			ctrl[0] = m_dataViewListCtrl1;
			ctrl[1] = m_dataViewListCtrl2;
			ctrl[2] = m_dataViewListCtrl3;
			ctrl[3] = m_dataViewListCtrl4;

			for (int i = 0; i < 4; ++i) {
				ctrl[i]->UnselectAll();
			}
			position = -1;
		}
		else if (1 == view_mode && NK_ENTER == event.GetKeyCode() && dataViewListCtrlNo == 3 && position >= 0 && position + ((now->GetUserTypeListSize() + now->GetItemListSize()) / 4) * 3 < now->GetUserTypeListSize()) {
			now = now->GetUserTypeList(position + ((now->GetUserTypeListSize() + now->GetItemListSize()) / 4) * 3);
			RefreshTable(now);
		}
		else  if (2 == view_mode && NK_ENTER == event.GetKeyCode() && position >= 0 && position < m_dataViewListCtrl4->GetItemCount()) {
			const int pos = (position + ((now->GetUserTypeListSize() + now->GetItemListSize()) / 4) * 3);
			if (now->IsUserTypeList(pos)) {
				const int idx = now->GetUserTypeIndexFromIlistIndex(pos);
				now = now->GetUserTypeList(idx);
				RefreshTable(now);
			}
		}
		else if (NK_BACKSPACE == event.GetKeyCode() && now->GetParent() != nullptr) {
			now = now->GetParent();
			RefreshTable(now);
		}
		else {
			wxDataViewListCtrl* ctrl[4];
			ctrl[0] = m_dataViewListCtrl1;
			ctrl[1] = m_dataViewListCtrl2;
			ctrl[2] = m_dataViewListCtrl3;
			ctrl[3] = m_dataViewListCtrl4;

			ctrl[dataViewListCtrlNo]->UnselectRow(position);

			if (WXK_UP == event.GetKeyCode() && dataViewListCtrlNo > -1 && position > 0)//< ctrl[dataViewListCtrlNo]->GetItemCount())
			{
				event.Skip();
				return;
			}
			else if (WXK_DOWN == event.GetKeyCode() && dataViewListCtrlNo > -1 && position >= 0 && position < ctrl[dataViewListCtrlNo]->GetItemCount() - 1)
			{
				event.Skip();
				return;
			}
			else if (WXK_LEFT == event.GetKeyCode() && dataViewListCtrlNo > 0 && position >= 0 && position < ctrl[dataViewListCtrlNo - 1]->GetItemCount())
			{
				dataViewListCtrlNo--;
			}
			else if (WXK_RIGHT == event.GetKeyCode() && dataViewListCtrlNo < 3 && position >= 0 && position < ctrl[dataViewListCtrlNo + 1]->GetItemCount())
			{
				dataViewListCtrlNo++;
			}

			ctrl[dataViewListCtrlNo]->SelectRow(position);
			ctrl[dataViewListCtrlNo]->SetFocus();
		}
	}

	virtual void m_dataViewListCtrl1OnDataViewListCtrlItemActivated(wxMouseEvent& event) {
		dataViewListCtrlNo = 0; position = m_dataViewListCtrl1->GetSelectedRow();

		if (1 == view_mode && position >= 0 && position < now->GetUserTypeListSize()) {
			now = now->GetUserTypeList(position);
			RefreshTable(now);
		}
		else  if (2 == view_mode && position >= 0 && position < m_dataViewListCtrl1->GetItemCount()) {
			if (now->IsUserTypeList(position)) {
				const int idx = now->GetUserTypeIndexFromIlistIndex(position);
				now = now->GetUserTypeList(idx);
				RefreshTable(now);
			}
		}
		event.Skip();
	}
	virtual void m_dataViewListCtrl2OnDataViewListCtrlItemActivated(wxMouseEvent& event) {
		dataViewListCtrlNo = 1; position = m_dataViewListCtrl2->GetSelectedRow();
		if (1 == view_mode && dataViewListCtrlNo == 1 && position >= 0 && position + ((now->GetUserTypeListSize() + now->GetItemListSize()) / 4) < now->GetUserTypeListSize()) {
			now = now->GetUserTypeList(position + ((now->GetUserTypeListSize() + now->GetItemListSize()) / 4));
			RefreshTable(now);
		}
		else  if (2 == view_mode && position >= 0 && position < m_dataViewListCtrl2->GetItemCount()) {
			const int pos = (position + ((now->GetUserTypeListSize() + now->GetItemListSize()) / 4));
			if (now->IsUserTypeList(pos)) {
				const int idx = now->GetUserTypeIndexFromIlistIndex(pos);
				now = now->GetUserTypeList(idx);
				RefreshTable(now);
			}
		}
	}
	virtual void m_dataViewListCtrl3OnDataViewListCtrlItemActivated(wxMouseEvent& event) {
		dataViewListCtrlNo = 2; position = m_dataViewListCtrl3->GetSelectedRow();
		if (1 == view_mode && dataViewListCtrlNo == 2 && position >= 0 && position + ((now->GetUserTypeListSize() + now->GetItemListSize()) / 4) * 2 < now->GetUserTypeListSize()) {
			now = now->GetUserTypeList(position + ((now->GetUserTypeListSize() + now->GetItemListSize()) / 4) * 2);
			RefreshTable(now);
		}
		else  if (2 == view_mode && position >= 0 && position < m_dataViewListCtrl3->GetItemCount()) {
			const int pos = (position + ((now->GetUserTypeListSize() + now->GetItemListSize()) / 4) * 2);
			if (now->IsUserTypeList(pos)) {
				const int idx = now->GetUserTypeIndexFromIlistIndex(pos);
				now = now->GetUserTypeList(idx);
				RefreshTable(now);
			}
		}
	}
	virtual void m_dataViewListCtrl4OnDataViewListCtrlItemActivated(wxMouseEvent& event) {
		dataViewListCtrlNo = 3; position = m_dataViewListCtrl4->GetSelectedRow();
		if (1 == view_mode && dataViewListCtrlNo == 3 && position >= 0 && position + ((now->GetUserTypeListSize() + now->GetItemListSize()) / 4) * 3 < now->GetUserTypeListSize()) {
			now = now->GetUserTypeList(position + ((now->GetUserTypeListSize() + now->GetItemListSize()) / 4) * 3);
			RefreshTable(now);
		}
		else  if (2 == view_mode && position >= 0 && position < m_dataViewListCtrl4->GetItemCount()) {
			const int pos = (position + ((now->GetUserTypeListSize() + now->GetItemListSize()) / 4) * 3);
			if (now->IsUserTypeList(pos)) {
				const int idx = now->GetUserTypeIndexFromIlistIndex(pos);
				now = now->GetUserTypeList(idx);
				RefreshTable(now);
			}
		}
	}

	virtual void m_dataViewListCtrl1OnDataViewListCtrlItemContextMenu(wxDataViewEvent& event) {
		if (now->GetParent()) {
			now = now->GetParent();
			RefreshTable(now);
		}
	}
	virtual void m_dataViewListCtrl2OnDataViewListCtrlItemContextMenu(wxDataViewEvent& event) {
		if (now->GetParent()) {
			now = now->GetParent();
			RefreshTable(now);
		}
	}
	virtual void m_dataViewListCtrl3OnDataViewListCtrlItemContextMenu(wxDataViewEvent& event) {
		if (now->GetParent()) {
			now = now->GetParent();
			RefreshTable(now);
		}
	}
	virtual void m_dataViewListCtrl4OnDataViewListCtrlItemContextMenu(wxDataViewEvent& event) {
		if (now->GetParent()) {
			now = now->GetParent();
			RefreshTable(now);
		}
	}


	virtual void m_dataViewListCtrl1OnDataViewListCtrlSelectionChanged(wxDataViewEvent& event) {
		dataViewListCtrlNo = 0;
		position = m_dataViewListCtrl1->GetSelectedRow();
	}
	virtual void m_dataViewListCtrl2OnDataViewListCtrlSelectionChanged(wxDataViewEvent& event) {
		dataViewListCtrlNo = 1;
		position = m_dataViewListCtrl2->GetSelectedRow();
	}
	virtual void m_dataViewListCtrl3OnDataViewListCtrlSelectionChanged(wxDataViewEvent& event) {
		dataViewListCtrlNo = 2;
		position = m_dataViewListCtrl3->GetSelectedRow();
	}
	virtual void m_dataViewListCtrl4OnDataViewListCtrlSelectionChanged(wxDataViewEvent& event) {
		dataViewListCtrlNo = 3;
		position = m_dataViewListCtrl4->GetSelectedRow();
	}
	
	virtual void m_dataViewListCtrl1OnSize(wxUpdateUIEvent& event) {
		m_dataViewListCtrl1->GetColumn(0)->SetWidth(m_dataViewListCtrl1->GetSize().GetWidth() / 2 * 0.92); // check...
		m_dataViewListCtrl1->GetColumn(1)->SetWidth(m_dataViewListCtrl1->GetSize().GetWidth() / 2 * 0.92);
	}
	virtual void m_dataViewListCtrl2OnSize(wxUpdateUIEvent& event) {
		m_dataViewListCtrl2->GetColumn(0)->SetWidth(m_dataViewListCtrl2->GetSize().GetWidth() / 2 * 0.92); // check...
		m_dataViewListCtrl2->GetColumn(1)->SetWidth(m_dataViewListCtrl2->GetSize().GetWidth() / 2 * 0.92);
	}	
	virtual void m_dataViewListCtrl3OnSize(wxUpdateUIEvent& event) {
		m_dataViewListCtrl3->GetColumn(0)->SetWidth(m_dataViewListCtrl3->GetSize().GetWidth() / 2 * 0.92); // check...
		m_dataViewListCtrl3->GetColumn(1)->SetWidth(m_dataViewListCtrl3->GetSize().GetWidth() / 2 * 0.92);
	}	
	virtual void m_dataViewListCtrl4OnSize(wxUpdateUIEvent& event) {
		m_dataViewListCtrl4->GetColumn(0)->SetWidth(m_dataViewListCtrl4->GetSize().GetWidth() / 2 * 0.92); // check...
		m_dataViewListCtrl4->GetColumn(1)->SetWidth(m_dataViewListCtrl4->GetSize().GetWidth() / 2 * 0.92);
	}

	virtual void DefaultViewMenuOnMenuSelection(wxCommandEvent& event) {
		view_mode = 1;
		m_statusBar1->SetLabelText(wxT("View Mode A"));
		if (now) {
			RefreshTable(now);
		}
	}
	virtual void IListViewMenuOnMenuSelection(wxCommandEvent& event) {
		view_mode = 2;
		m_statusBar1->SetLabelText(wxT("View Mode B"));
		if (now) {
			RefreshTable(now);
		}
	}

	virtual void OtherWindowMenuOnMenuSelection(wxCommandEvent& event) {
		if (!isMain) { return; }
		MainFrame* frame = new MainFrame(this);
		frame->view_mode = this->view_mode;
		frame->now = this->now;
		frame->RefreshTable(frame->now);

		frame->SetTitle(GetTitle() + wxT(" : other window"));

		frame->Show(true);
	}

	virtual void m_code_run_buttonOnButtonClick(wxCommandEvent& event) {
		string mainStr = "Main = { $call = { id = main } }";
		string eventStr(m_code->GetValue().ToUTF8());
		wiz::load_data::UserType eventUT;
		//wiz::ExecuteData executeData;

		wiz::load_data::LoadData::LoadDataFromString(eventStr, eventUT);

		//executeData.pEvents = &eventUT;
		//executeData.noUseInput = true;
		//executeData.noUseOutput = true;

		try {
			wiz::Option opt;


			for (long long i = 0; i < eventUT.GetUserTypeListSize(); ++i) {
				if (eventUT.GetUserTypeList(i)->GetName() == "$insert"sv) {
					::InsertFunc(now, eventUT.GetUserTypeList(i), &eventUT);
				}
				else if (eventUT.GetUserTypeList(i)->GetName() == "$update"sv) {
					::UpdateFunc(now, eventUT.GetUserTypeList(i), &eventUT);
				}
				else if (eventUT.GetUserTypeList(i)->GetName() == "$delete"sv) {
					::RemoveFunc(now, eventUT.GetUserTypeList(i), &eventUT);
				}
			}
			//wiz::ClauText().execute_module(mainStr, now, executeData, opt, 0);

			RefreshTable(now);
		}
		catch (...) {
			//
		}
	}
	/*
	virtual void CodeViewMenuOnMenuSelection(wxCommandEvent& event) {
		if (false == m_code->IsShownOnScreen())
		{
			m_code->Show();
			m_code_run_button->Show();
		}
		else {
			m_code->Hide();
			m_code_run_button->Hide();
		}
	}
	*/
public:

	MainFrame(wxWindow* parent, wxWindowID id = wxID_ANY, const wxString& title = wxT("ClauExplorer"), const wxPoint& pos = wxDefaultPosition, const wxSize& size = wxSize(1024, 512), long style = wxDEFAULT_FRAME_STYLE | wxTAB_TRAVERSAL);

	~MainFrame();

	void FirstFrame() {
		isMain = true;
	}
};

MainFrame::MainFrame(wxWindow* parent, wxWindowID id, const wxString& title, const wxPoint& pos, const wxSize& size, long style) : wxFrame(parent, id, title, pos, size, style)
{
	now = &global;


	this->SetSizeHints(wxDefaultSize, wxDefaultSize);

	menuBar = new wxMenuBar(0);
	FileMenu = new wxMenu();
	wxMenuItem* FileLoadMenu;
	FileLoadMenu = new wxMenuItem(FileMenu, wxID_ANY, wxString(wxT("Load")), wxEmptyString, wxITEM_NORMAL);
	FileMenu->Append(FileLoadMenu);

	wxMenuItem* FileSaveMenu;
	FileSaveMenu = new wxMenuItem(FileMenu, wxID_ANY, wxString(wxT("Save")), wxEmptyString, wxITEM_NORMAL);
	FileMenu->Append(FileSaveMenu);

	FileMenu->AppendSeparator();

	wxMenuItem* FileExitMenu;
	FileExitMenu = new wxMenuItem(FileMenu, wxID_ANY, wxString(wxT("Exit")), wxEmptyString, wxITEM_NORMAL);
	FileMenu->Append(FileExitMenu);

	FileMenu->AppendSeparator();

	menuBar->Append(FileMenu, wxT("File"));

	DoMenu = new wxMenu();
	wxMenuItem* InsertMenu;
	InsertMenu = new wxMenuItem(DoMenu, wxID_ANY, wxString(wxT("Insert")), wxEmptyString, wxITEM_NORMAL);
	DoMenu->Append(InsertMenu);

	wxMenuItem* ChangeMenu;
	ChangeMenu = new wxMenuItem(DoMenu, wxID_ANY, wxString(wxT("Change")), wxEmptyString, wxITEM_NORMAL);
	DoMenu->Append(ChangeMenu);

	wxMenuItem* RemoveMenu;
	RemoveMenu = new wxMenuItem(DoMenu, wxID_ANY, wxString(wxT("Remove")), wxEmptyString, wxITEM_NORMAL);
	DoMenu->Append(RemoveMenu);

	menuBar->Append(DoMenu, wxT("Do"));

	ViewMenu = new wxMenu();
	wxMenuItem* DefaultViewMenu;
	DefaultViewMenu = new wxMenuItem(ViewMenu, wxID_ANY, wxString(wxT("ViewA")), wxEmptyString, wxITEM_NORMAL);
	ViewMenu->Append(DefaultViewMenu);

	wxMenuItem* IListViewMenu;
	IListViewMenu = new wxMenuItem(ViewMenu, wxID_ANY, wxString(wxT("ViewB")), wxEmptyString, wxITEM_NORMAL);
	ViewMenu->Append(IListViewMenu);

	//wxMenuItem* CodeViewMenu;
	//CodeViewMenu = new wxMenuItem(ViewMenu, wxID_ANY, wxString(wxT("CodeView")), wxEmptyString, wxITEM_NORMAL);
	//ViewMenu->Append(CodeViewMenu);

	menuBar->Append(ViewMenu, wxT("View"));


	WindowMenu = new wxMenu();
	wxMenuItem* OtherWindowMenu;
	OtherWindowMenu = new wxMenuItem(WindowMenu, wxID_ANY, wxString(wxT("OtherWindow")), wxEmptyString, wxITEM_NORMAL);
	WindowMenu->Append(OtherWindowMenu);

	menuBar->Append(WindowMenu, wxT("Window"));


	this->SetMenuBar(menuBar);

	wxBoxSizer* bSizer;
	bSizer = new wxBoxSizer(wxVERTICAL);

	wxBoxSizer* bSizer2;
	bSizer2 = new wxBoxSizer(wxHORIZONTAL);

	back_button = new wxButton(this, wxID_ANY, wxT("��"), wxDefaultPosition, wxDefaultSize, 0);
	back_button->SetFont(wxFont(15, wxFONTFAMILY_DEFAULT, wxFONTSTYLE_NORMAL, wxFONTWEIGHT_NORMAL, false, wxEmptyString));

	bSizer2->Add(back_button, 0, wxALL, 5);

	dir_text = new wxTextCtrl(this, wxID_ANY, wxEmptyString, wxDefaultPosition, wxDefaultSize, wxTE_PROCESS_ENTER);
	bSizer2->Add(dir_text, 1, wxALL, 5);

	refresh_button = new wxButton(this, wxID_ANY, wxT("Refresh"), wxDefaultPosition, wxDefaultSize, 0);
	bSizer2->Add(refresh_button, 0, wxALL, 5);


	bSizer->Add(bSizer2, 0, wxEXPAND, 5);

	wxBoxSizer* bSizer3;
	bSizer3 = new wxBoxSizer(wxHORIZONTAL);

	m_dataViewListCtrl1 = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_HORIZ_RULES | wxDV_ROW_LINES | wxDV_SINGLE);
	bSizer3->Add(m_dataViewListCtrl1, 1, wxALL | wxEXPAND, 5);

	m_dataViewListCtrl2 = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_HORIZ_RULES | wxDV_ROW_LINES | wxDV_SINGLE);
	bSizer3->Add(m_dataViewListCtrl2, 1, wxALL | wxEXPAND, 5);

	m_dataViewListCtrl3 = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_HORIZ_RULES | wxDV_ROW_LINES | wxDV_SINGLE);
	bSizer3->Add(m_dataViewListCtrl3, 1, wxALL | wxEXPAND, 5);

	m_dataViewListCtrl4 = new wxDataViewListCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, wxDV_HORIZ_RULES | wxDV_ROW_LINES | wxDV_SINGLE);
	bSizer3->Add(m_dataViewListCtrl4, 1, wxALL | wxEXPAND, 5);

	wxBoxSizer* bSizer6;
	bSizer6 = new wxBoxSizer(wxVERTICAL);

	m_code = new wxStyledTextCtrl(this, wxID_ANY, wxDefaultPosition, wxDefaultSize, 0, wxEmptyString);
	m_code->SetText(wxT("#ClauExplorer (https://github.com/ClauParser/ClauExplorer) \n#\t\t������ vztpv@naver.com\n"));
	m_code->SetUseTabs(true);
	m_code->SetTabWidth(4);
	m_code->SetIndent(4);
	m_code->SetTabIndents(true);
	m_code->SetBackSpaceUnIndents(true);
	m_code->SetViewEOL(false);
	m_code->SetViewWhiteSpace(false);
	m_code->SetMarginWidth(2, 0);
	m_code->SetIndentationGuides(true);
	m_code->SetMarginType(1, wxSTC_MARGIN_SYMBOL);
	m_code->SetMarginMask(1, wxSTC_MASK_FOLDERS);
	m_code->SetMarginWidth(1, 16);
	m_code->SetMarginSensitive(1, true);
	m_code->SetProperty(wxT("fold"), wxT("1"));
	m_code->SetFoldFlags(wxSTC_FOLDFLAG_LINEBEFORE_CONTRACTED | wxSTC_FOLDFLAG_LINEAFTER_CONTRACTED);
	m_code->SetMarginType(0, wxSTC_MARGIN_NUMBER);
	m_code->SetMarginWidth(0, m_code->TextWidth(wxSTC_STYLE_LINENUMBER, wxT("_99999")));
	m_code->MarkerDefine(wxSTC_MARKNUM_FOLDER, wxSTC_MARK_BOXPLUS);
	m_code->MarkerSetBackground(wxSTC_MARKNUM_FOLDER, wxColour(wxT("BLACK")));
	m_code->MarkerSetForeground(wxSTC_MARKNUM_FOLDER, wxColour(wxT("WHITE")));
	m_code->MarkerDefine(wxSTC_MARKNUM_FOLDEROPEN, wxSTC_MARK_BOXMINUS);
	m_code->MarkerSetBackground(wxSTC_MARKNUM_FOLDEROPEN, wxColour(wxT("BLACK")));
	m_code->MarkerSetForeground(wxSTC_MARKNUM_FOLDEROPEN, wxColour(wxT("WHITE")));
	m_code->MarkerDefine(wxSTC_MARKNUM_FOLDERSUB, wxSTC_MARK_EMPTY);
	m_code->MarkerDefine(wxSTC_MARKNUM_FOLDEREND, wxSTC_MARK_BOXPLUS);
	m_code->MarkerSetBackground(wxSTC_MARKNUM_FOLDEREND, wxColour(wxT("BLACK")));
	m_code->MarkerSetForeground(wxSTC_MARKNUM_FOLDEREND, wxColour(wxT("WHITE")));
	m_code->MarkerDefine(wxSTC_MARKNUM_FOLDEROPENMID, wxSTC_MARK_BOXMINUS);
	m_code->MarkerSetBackground(wxSTC_MARKNUM_FOLDEROPENMID, wxColour(wxT("BLACK")));
	m_code->MarkerSetForeground(wxSTC_MARKNUM_FOLDEROPENMID, wxColour(wxT("WHITE")));
	m_code->MarkerDefine(wxSTC_MARKNUM_FOLDERMIDTAIL, wxSTC_MARK_EMPTY);
	m_code->MarkerDefine(wxSTC_MARKNUM_FOLDERTAIL, wxSTC_MARK_EMPTY);
	m_code->SetSelBackground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHT));
	m_code->SetSelForeground(true, wxSystemSettings::GetColour(wxSYS_COLOUR_HIGHLIGHTTEXT));
	bSizer6->Add(m_code, 7, wxEXPAND | wxALL, 5);

	//m_code->Hide();

	m_code_run_button = new wxButton(this, wxID_ANY, wxT("Run"), wxDefaultPosition, wxDefaultSize, 0);
	bSizer6->Add(m_code_run_button, 1, wxALL | wxEXPAND, 5);

	//m_code_run_button->Hide();

	bSizer3->Add(bSizer6, 2, wxEXPAND, 5);


	bSizer->Add(bSizer3, 1, wxEXPAND, 5);


	m_dataViewListCtrl1->AppendTextColumn(wxT("name"));
	m_dataViewListCtrl1->AppendTextColumn(wxT("value"));

	m_dataViewListCtrl2->AppendTextColumn(wxT("name"));
	m_dataViewListCtrl2->AppendTextColumn(wxT("value"));

	m_dataViewListCtrl3->AppendTextColumn(wxT("name"));
	m_dataViewListCtrl3->AppendTextColumn(wxT("value"));

	m_dataViewListCtrl4->AppendTextColumn(wxT("name"));
	m_dataViewListCtrl4->AppendTextColumn(wxT("value"));

	m_statusBar1 = this->CreateStatusBar(1, wxST_SIZEGRIP, wxID_ANY);

	m_statusBar1->SetLabelText(wxT("View Mode A"));

	this->SetSizer(bSizer);
	this->Layout();

	this->Centre(wxBOTH);

	// Connect Events
	this->Connect(FileLoadMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::FileLoadMenuOnMenuSelection));
	this->Connect(FileSaveMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::FileSaveMenuOnMenuSelection));
	this->Connect(FileExitMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::FileExitMenuOnMenuSelection));
	this->Connect(InsertMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::InsertMenuOnMenuSelection));
	this->Connect(ChangeMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::ChangeMenuOnMenuSelection));
	this->Connect(RemoveMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::RemoveMenuOnMenuSelection));


	this->Connect(DefaultViewMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::DefaultViewMenuOnMenuSelection));
	this->Connect(IListViewMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::IListViewMenuOnMenuSelection));

	back_button->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainFrame::back_buttonOnButtonClick), NULL, this);
	dir_text->Connect(wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler(MainFrame::dir_textOnTextEnter), NULL, this);
	refresh_button->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainFrame::refresh_buttonOnButtonClick), NULL, this);


	m_dataViewListCtrl1->Connect(wxEVT_SIZE, wxUpdateUIEventHandler(MainFrame::m_dataViewListCtrl1OnSize), NULL, this );
	m_dataViewListCtrl2->Connect(wxEVT_SIZE, wxUpdateUIEventHandler(MainFrame::m_dataViewListCtrl2OnSize), NULL, this);
	m_dataViewListCtrl3->Connect(wxEVT_SIZE, wxUpdateUIEventHandler(MainFrame::m_dataViewListCtrl3OnSize), NULL, this);
	m_dataViewListCtrl4->Connect(wxEVT_SIZE, wxUpdateUIEventHandler(MainFrame::m_dataViewListCtrl4OnSize), NULL, this);

	m_dataViewListCtrl1->Connect(wxEVT_CHAR, wxKeyEventHandler(MainFrame::m_dataViewListCtrl1OnChar), NULL, this);
	m_dataViewListCtrl2->Connect(wxEVT_CHAR, wxKeyEventHandler(MainFrame::m_dataViewListCtrl2OnChar), NULL, this);
	m_dataViewListCtrl3->Connect(wxEVT_CHAR, wxKeyEventHandler(MainFrame::m_dataViewListCtrl3OnChar), NULL, this);
	m_dataViewListCtrl4->Connect(wxEVT_CHAR, wxKeyEventHandler(MainFrame::m_dataViewListCtrl4OnChar), NULL, this);

	// double click
	m_dataViewListCtrl1->Connect(wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxMouseEventHandler(MainFrame::m_dataViewListCtrl1OnDataViewListCtrlItemActivated), NULL, this);
	m_dataViewListCtrl2->Connect(wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxMouseEventHandler(MainFrame::m_dataViewListCtrl2OnDataViewListCtrlItemActivated), NULL, this);
	m_dataViewListCtrl3->Connect(wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxMouseEventHandler(MainFrame::m_dataViewListCtrl3OnDataViewListCtrlItemActivated), NULL, this);
	m_dataViewListCtrl4->Connect(wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxMouseEventHandler(MainFrame::m_dataViewListCtrl4OnDataViewListCtrlItemActivated), NULL, this);

	// right click
	m_dataViewListCtrl1->Connect(wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl1OnDataViewListCtrlItemContextMenu), NULL, this);
	m_dataViewListCtrl2->Connect(wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl2OnDataViewListCtrlItemContextMenu), NULL, this);
	m_dataViewListCtrl3->Connect(wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl3OnDataViewListCtrlItemContextMenu), NULL, this);
	m_dataViewListCtrl4->Connect(wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl4OnDataViewListCtrlItemContextMenu), NULL, this);

	m_dataViewListCtrl1->Connect(wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl1OnDataViewListCtrlSelectionChanged), NULL, this);
	m_dataViewListCtrl2->Connect(wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl2OnDataViewListCtrlSelectionChanged), NULL, this);
	m_dataViewListCtrl3->Connect(wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl3OnDataViewListCtrlSelectionChanged), NULL, this);
	m_dataViewListCtrl4->Connect(wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl4OnDataViewListCtrlSelectionChanged), NULL, this);

	this->Connect(OtherWindowMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::OtherWindowMenuOnMenuSelection));
	m_code_run_button->Connect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainFrame::m_code_run_buttonOnButtonClick), NULL, this);
	//this->Connect(CodeViewMenu->GetId(), wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::CodeViewMenuOnMenuSelection));
}

MainFrame::~MainFrame()
{
	// Disconnect Events
	this->Disconnect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::FileLoadMenuOnMenuSelection));
	this->Disconnect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::FileSaveMenuOnMenuSelection));
	this->Disconnect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::FileExitMenuOnMenuSelection));
	this->Disconnect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::InsertMenuOnMenuSelection));
	this->Disconnect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::ChangeMenuOnMenuSelection));
	this->Disconnect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::RemoveMenuOnMenuSelection));

	back_button->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainFrame::back_buttonOnButtonClick), NULL, this);
	dir_text->Disconnect(wxEVT_COMMAND_TEXT_ENTER, wxCommandEventHandler(MainFrame::dir_textOnTextEnter), NULL, this);
	refresh_button->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainFrame::refresh_buttonOnButtonClick), NULL, this);

	this->Disconnect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::DefaultViewMenuOnMenuSelection));
	this->Disconnect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::IListViewMenuOnMenuSelection));

	m_dataViewListCtrl1->Disconnect(wxEVT_SIZE, wxUpdateUIEventHandler(MainFrame::m_dataViewListCtrl1OnSize), NULL, this);
	m_dataViewListCtrl2->Disconnect(wxEVT_SIZE, wxUpdateUIEventHandler(MainFrame::m_dataViewListCtrl2OnSize), NULL, this);
	m_dataViewListCtrl3->Disconnect(wxEVT_SIZE, wxUpdateUIEventHandler(MainFrame::m_dataViewListCtrl3OnSize), NULL, this);
	m_dataViewListCtrl4->Disconnect(wxEVT_SIZE, wxUpdateUIEventHandler(MainFrame::m_dataViewListCtrl4OnSize), NULL, this);


	m_dataViewListCtrl1->Disconnect(wxEVT_CHAR, wxKeyEventHandler(MainFrame::m_dataViewListCtrl1OnChar), NULL, this);
	m_dataViewListCtrl2->Disconnect(wxEVT_CHAR, wxKeyEventHandler(MainFrame::m_dataViewListCtrl2OnChar), NULL, this);
	m_dataViewListCtrl3->Disconnect(wxEVT_CHAR, wxKeyEventHandler(MainFrame::m_dataViewListCtrl3OnChar), NULL, this);
	m_dataViewListCtrl4->Disconnect(wxEVT_CHAR, wxKeyEventHandler(MainFrame::m_dataViewListCtrl4OnChar), NULL, this);

	m_dataViewListCtrl1->Disconnect(wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxMouseEventHandler(MainFrame::m_dataViewListCtrl1OnDataViewListCtrlItemActivated), NULL, this);
	m_dataViewListCtrl2->Disconnect(wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxMouseEventHandler(MainFrame::m_dataViewListCtrl2OnDataViewListCtrlItemActivated), NULL, this);
	m_dataViewListCtrl3->Disconnect(wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxMouseEventHandler(MainFrame::m_dataViewListCtrl3OnDataViewListCtrlItemActivated), NULL, this);
	m_dataViewListCtrl4->Disconnect(wxEVT_COMMAND_DATAVIEW_ITEM_ACTIVATED, wxMouseEventHandler(MainFrame::m_dataViewListCtrl4OnDataViewListCtrlItemActivated), NULL, this);

	m_dataViewListCtrl1->Disconnect(wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl1OnDataViewListCtrlItemContextMenu), NULL, this);
	m_dataViewListCtrl2->Disconnect(wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl2OnDataViewListCtrlItemContextMenu), NULL, this);
	m_dataViewListCtrl3->Disconnect(wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl3OnDataViewListCtrlItemContextMenu), NULL, this);
	m_dataViewListCtrl4->Disconnect(wxEVT_COMMAND_DATAVIEW_ITEM_CONTEXT_MENU, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl4OnDataViewListCtrlItemContextMenu), NULL, this);


	m_dataViewListCtrl1->Disconnect(wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl1OnDataViewListCtrlSelectionChanged), NULL, this);
	m_dataViewListCtrl2->Disconnect(wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl2OnDataViewListCtrlSelectionChanged), NULL, this);
	m_dataViewListCtrl3->Disconnect(wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl3OnDataViewListCtrlSelectionChanged), NULL, this);
	m_dataViewListCtrl4->Disconnect(wxEVT_COMMAND_DATAVIEW_SELECTION_CHANGED, wxDataViewEventHandler(MainFrame::m_dataViewListCtrl4OnDataViewListCtrlSelectionChanged), NULL, this);

	this->Disconnect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::OtherWindowMenuOnMenuSelection));

	m_code_run_button->Disconnect(wxEVT_COMMAND_BUTTON_CLICKED, wxCommandEventHandler(MainFrame::m_code_run_buttonOnButtonClick), NULL, this);
	//this->Disconnect(wxID_ANY, wxEVT_COMMAND_MENU_SELECTED, wxCommandEventHandler(MainFrame::CodeViewMenuOnMenuSelection));
}

class TestApp : public wxApp {
public:
	virtual bool OnInit() {
		MainFrame* frame = new MainFrame(nullptr);
		frame->FirstFrame();
		frame->Show(true);
		return true;
	}
};

IMPLEMENT_APP(TestApp)

