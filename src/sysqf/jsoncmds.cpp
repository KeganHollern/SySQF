#include "../commandmap.h"
#include "../value.h"
#include "../cmd.h"
#include "../virtualmachine.h"
#include <algorithm>
#include <sstream>
#include <cmath>
#include <fstream>
#include <iterator>
#include <iostream>

#include <nlohmann/json.hpp>

using namespace sqf;
using json = nlohmann::json;

//this is for gett the json object from input params
#define GETOBJECT(DEFAULT_VAL) auto index = left.as_int(); \
	auto key = right.as_string(); \
	if (parsed_json.size() <= index) { \
		vm->err() << "Json index invalid! " << std::endl; \
		return DEFAULT_VAL; \
	} \
	json j3 = parsed_json[index]

namespace {
	
	int stream_count;
	std::vector<json> parsed_json;
	value parse_json(virtualmachine* vm, value::cref right)
	{
		auto jsonstring = right.as_string();
		auto j3 = json::parse(jsonstring);
		int index = parsed_json.size();
		parsed_json.push_back(j3);

		auto elem = j3["test"];

		return index;
	}
	value get_object(virtualmachine* vm, value::cref left, value::cref right)
	{
		GETOBJECT(-1);

		if (j3[key].is_object())
		{
			int index = parsed_json.size();
			parsed_json.push_back(j3[key]);
			return index;
		}
		vm->err() << "Json key is not an object! " << key << "." << std::endl << j3.dump(4) << std::endl;
		return -1;
	}
	value get_string(virtualmachine* vm, value::cref left, value::cref right)
	{
		GETOBJECT("");

		if (j3[key].is_string())
		{
			std::string value = j3[key];
			return value;
		}
		vm->err() << "Json key is not a string! " << key << "." << std::endl << j3.dump(4) << std::endl;
		return "";
	}


}

void sqf::commandmap::initjsoncmds()
{
	//add(unary("system", sqf::type::STRING, "Call the host environment's command processor", system));

	add(unary("parseJson", sqf::type::STRING, "Parse json string", parse_json));
	add(binary(4, "getJsonObject", sqf::type::SCALAR, sqf::type::STRING, "Parse json object from key", get_object));
	add(binary(4, "getJsonString", sqf::type::SCALAR, sqf::type::STRING, "Parse json string from key", get_string));

}