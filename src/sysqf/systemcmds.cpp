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
	value system(virtualmachine* vm, value::cref right)
	{
		auto command = right.as_string();
		std::system(command.c_str());
		return {};
	}

}

void sqf::commandmap::initsystemcmds()
{
	add(unary("system", sqf::type::STRING, "Call the host environment's command processor", system));
}