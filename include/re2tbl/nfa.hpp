#pragma once

#include <bitset>
#include <limits>
#include <memory>
#include <vector>

namespace re2tbl
{

struct NfaNode;

/// An edge between NFA nodes.
///
/// Can represent an epsilon move or a move on one or more chars.
struct NfaEdge
{
	/// Whether this edge can be traversed "for free" (without
	/// consuming a character).
	///
	/// * ``true``: no character is consumed
	/// * ``false``: a character is consumed
	bool epsilon;
	/// The set of characters that need to be consumed to traverse
	/// this edge.
	///
	/// It will be empty if @c #epsilon is true.
	std::bitset<std::numeric_limits<char>::max() + 1> chars;
	/// The other end of this edge.
	///
	/// When traversing, this pointer is the destination.
	NfaNode* next;
};

/// A single node of the `Nfa` structure.
///
/// Maintains its own index as @c id for convenience.
struct NfaNode
{
	/// The index of this node in the `Nfa`, used as a unique identifier.
	std::size_t id;
	/// Whether the NFA can "accept" on this node.
	///
	/// Accepting means that if the input is exhausted in this state, the machine
	/// reports success.
	bool accepting;
	/// The edges which lead out from this node, if any.
	std::vector<NfaEdge> next;
};

/// A non-deterministic finite automaton (NFA).
///
/// Contains a set of `nodes`, as well as a pointer to its start node.
struct Nfa
{
	/// The nodes which make up this NFA.
	///
	/// Note that you should not iterate these in order, as it
	/// is meaningless. Use each `NfaNode`'s edges to make moves
	/// based on the input string.
	std::vector<std::unique_ptr<NfaNode>> nodes;
	/// A pointer within @c #nodes that refers to the initial state.
	///
	/// In practice, this is always equal to `&nodes[0]`.
	NfaNode* start;

	/// Create a new node within the NFA.
	///
	/// It will initially be nonaccepting and have no edges leading out.
	///
	/// You will probably not call this function directly, as it is called
	/// by @c ParseRegex().
	NfaNode* AddNode();
	/// Create an edge between two nodes.
	/// The edge will require consuming a character to move if `transition`
	/// is not the null character.
	///
	/// Like @c AddNode(), it is called by @c ParseRegex() and calling it
	/// directly is unnecessary.
	///
	/// @param a the source node
	/// @param b the destination node
	/// @param transition the character that is consumed by the new edge,
	/// or @c '\0'.
	void ConnectNodes(NfaNode* a, NfaNode* b, char transition = '\0');

	/// Display a human-readable representation of this NFA.
	///
	/// Describes each node by its ID and the IDs that it "points"
	/// to via its edges.
	///
	/// Additionally, the representation specifies the accepting node(s).
	///
	/// @param os the stream to output to
	void Display(std::ostream& os) const;
};

}  // namespace re2tbl
