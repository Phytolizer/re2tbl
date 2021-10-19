#pragma once

#include <bitset>
#include <cstddef>
#include <limits>
#include <unordered_set>
#include <vector>

#include "nfa.hpp"

namespace re2tbl
{
struct DfaNode;

/// A connection between `DfaNode`s.
struct DfaEdge
{
	/// The character(s) which may consumed by a move on this
	/// edge.
	///
	/// When traversing, exactly one character from this set
	/// will be consumed.
	std::bitset<std::numeric_limits<char>::max()> move;
	/// The destination of this edge.
	DfaNode* next;
};

/// A single element of a @c Dfa.
///
/// Maintains a unique identifier for convenience.
struct DfaNode
{
	/// A unique identifier for this node.
	std::size_t id;
	/// The set of NFA states which this node came from.
	std::unordered_set<std::size_t> nfa_equivalent;
	/// Whether this node is "accepting".
	///
	/// The machine can report success if it is in
	/// an accepting state *and* it has reached the end of input.
	bool accepting;
	/// The edges which lead out of this node.
	std::vector<DfaEdge> next;
};

/// A deterministic finite automaton (DFA).
///
/// Deterministic means that given a current state (node)
/// and a current input character, the machine can confidently
/// make a single unambiguous move, or report failure.
///
/// Unlike the NFA, the DFA never contains epsilon moves, which
/// is the reason the above statement holds.
struct Dfa
{
	/// The nodes which make up the DFA, in no particular order.
	std::vector<DfaNode> nodes;
	/// The ID of the start node of this DFA. Can be used as an
	/// index into @c nodes.
	std::size_t start;

	/// Construct the DFA from an existing NFA.
	///
	/// @param nfa the NFA to make deterministic
	Dfa(const Nfa& nfa);

	/// Output the contents of the DFA in a human-readable format.
	///
	/// Useful for debugging.
	void Display(std::ostream& os);
	/// Get rid of some redundant nodes in the DFA.
	///
	/// After the minimization process, the DFA is usually optimized
	/// in terms of memory usage and speed.
	void Minimize();
};

}  // namespace re2tbl
