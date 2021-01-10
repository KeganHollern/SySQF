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

using namespace sqf;
namespace {
	int stream_count;
	std::map<int, FILE*> open_file_streams;

	value open_file(virtualmachine* vm, value::cref right) //_handle = openFile ["file_name.txt", "r"];
	{
		auto arr = right.as_vector();

		if (arr.size() != 2)
		{
			vm->err() << "Array was expected to have two values." << std::endl;
			return 0;
		}
		if (arr[0].dtype() != type::STRING)
		{
			vm->err() << "First element of array was expected to be STRING, got " << sqf::type_str(arr[0].dtype()) << '.' << std::endl;
			return 0;
		}
		if (arr[1].dtype() != type::STRING)
		{
			vm->err() << "Second element of array was expected to be STRING, got " << sqf::type_str(arr[0].dtype()) << '.' << std::endl;
			return 0;
		}

		auto filename = arr[0].as_string();
		auto mode = arr[1].as_string();


		FILE* pFile = std::fopen(filename.c_str(), mode.c_str());
		if (!pFile) {
			vm->err() << "Failed to open file: " << filename << "." << std::endl;
			return 0;
		}

		stream_count++;
		int result = stream_count;
		open_file_streams[result] = pFile; //add to list
		return result; //we return the pointer
	}
	value get_char(virtualmachine* vm, value::cref right) // readChar _handle;
	{
		int index = right.as_int();

		auto res = open_file_streams.find(index);
		if (res == open_file_streams.end())
		{
			vm->err() << "Invalid file handle: " << index << "." << std::endl;
			return 0;
		}

		FILE* pFile = open_file_streams[index];

		if (!pFile)
		{
			vm->err() << "Invalid file handle: " << index << "." << std::endl;
			return 0;
		}

		return std::fgetc(pFile);
	}
	value endof_file(virtualmachine* vm, value::cref right)
	{
		int index = right.as_int();

		auto res = open_file_streams.find(index);
		if (res == open_file_streams.end())
		{
			vm->err() << "Invalid file handle: " << index << "." << std::endl;
			return true;
		}

		FILE* pFile = open_file_streams[index];

		if (!pFile)
		{
			vm->err() << "Invalid file handle: " << index << "." << std::endl;
			return true;
		}

		return (bool)feof(pFile);
	}
	value close_file(virtualmachine* vm, value::cref right) // closeFile _handle;
	{
		int index = right.as_int();

		auto res = open_file_streams.find(index);
		if (res == open_file_streams.end())
		{
			vm->err() << "Invalid file handle: " << index << "." << std::endl;
			return {};
		}
		FILE* pFile = open_file_streams[index];;

		if (!pFile)
		{
			vm->err() << "Invalid file handle: " << index << "." << std::endl;
			return {};
		}

		std::fclose(pFile);

		open_file_streams.erase(index);
		return {};
	}

	value read_file(virtualmachine* vm, value::cref right) //_bytes = readFile "file_name.txt";
	{
		auto filename = right.as_string();
		std::ifstream file(filename.c_str(), std::ios::binary);
		if (file.is_open())
		{
			std::vector<char> bytes(
				(std::istreambuf_iterator<char>(file)),
				(std::istreambuf_iterator<char>()));

			//really shit code
			std::vector<value> result;
			for (auto v : bytes)
				result.push_back((int)v);

			return result;
		}
		vm->err() << "Failed to read file: " << filename << "." << std::endl;
		return std::vector<value>(); //return empty array when fail to read
	}
}
void sqf::commandmap::initfilesystemcmds()
{
	//file stream handling
	add(unary("openFile", sqf::type::ARRAY, "Opens a file stream with the specified mode.", open_file));
	add(unary("isEndOf", sqf::type::SCALAR, "Tests the end-of-file indicator for the given stream.", endof_file));
	add(unary("readChar", sqf::type::SCALAR, "Gets the next character from the specified stream.", get_char));
	add(unary("closeFile", sqf::type::SCALAR, "Gets the next character from the specified stream.", close_file));

	//useful file functionality
	add(unary("readFile", sqf::type::STRING, "Read file bytes to array (does not support wide characters)", read_file));
}