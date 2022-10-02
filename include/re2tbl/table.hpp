#pragma once

#include <array>
#include <limits>
#include <ostream>
#include <vector>

#include "dfa.hpp"

namespace re2tbl
{

/// A state transition table.
///
/// This is the primary output of the library.
/// Tables can be compressed by calling @c Compress().
struct Table
{
	/// The transition table.
	///
	/// The first dimension is the state, and the second dimension
	/// is the input character.
	/// There is room for 1 more "character", which is used to
	/// represent the end of input.
	std::vector<std::array<std::size_t, std::numeric_limits<char>::max() + 2>> transitions;

	/// Construct a Table from an existing DFA.
	///
	/// @param dfa The DFA to construct the table from.
	explicit Table(const Dfa& dfa);

	/// Display the table in a human-readable format.
	///
	/// @param out The stream to write to.
	void Display(std::ostream& os) const;

	/// Compress the table.
	///
	/// This is an optional step, but it can reduce the size of the table
	/// by a significant amount.
	void Compress();

	/// Emit the table as a C++ header file.
	///
	/// The table will be emitted as a global constexpr variable.
	///
	/// @param out The stream to write to.
	/// @param name The name of the table.
	void Emit(std::ostream& os, std::string_view name) const;
};

}  // namespace re2tbl
