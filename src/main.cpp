#include <iostream>
#include <stdexcept>
#include <string>

#include <re2tbl/re2tbl.hpp>

int main()
{
	std::string input;
	if (!std::getline(std::cin, input))
	{
		throw std::runtime_error {"Expected a regex"};
	}
	auto tokens = re2tbl::LexRegex(input);
	auto nfa = re2tbl::ParseRegex(tokens);
	nfa.Display(std::cout);
	auto dfa = re2tbl::Dfa {nfa};
    dfa.Display(std::cout);
}
