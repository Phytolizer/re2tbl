#pragma once
/// @file re2tbl.hpp
///
/// Contains types and function declarations for the library.

#include <bitset>
#include <limits>
#include <memory>
#include <ostream>
#include <string_view>
#include <unordered_set>
#include <vector>

/// All of the `re2tbl` declarations are within this namespace.
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

/// Classifies a regular expression component for parsing.
///
/// Primarily used as a member for @c RegexToken.
enum class RegexTokenType
{
	/// A literal.
	/// This is the "catch-all" category, and includes any
	/// character not specially handled.
	kLiteral,
	/// A left parenthesis;
	/// begins a grouping to override the precedence rules.
	kLeftParen,
	/// A right parenthesis;
	/// closes a grouping.
	kRightParen,
	/// A pipe character.
	/// Represents multiple alternatives that are valid
	/// in a particular regular expression.
	kPipe,
	/// A star character.
	/// When placed after a subexpression, that subexpression
	/// can occur any number of times, including zero.
	kStar,
	/// A plus character.
	/// When placed after a subexpression, that subexpression
	/// can occur any number of times, but must occur at least once.
	kPlus,
	/// The end of the expression,
	/// used for convenience in the parser implementation.
	kEof,
};

/// A single component of a regular expression.
struct RegexToken
{
	/// The category of this token.
	RegexTokenType type;
	/// The text that makes up this token.
	///
	/// Luckily, in lexing regular expressions,
	/// all symbols with special meaning are one
	/// character long.
	char literal;
};

/// Classify all components of a regular expression.
///
/// This is the "entry point" of the library.
///
/// After calling this function, you should pass the return
/// value to @c ParseRegex().
///
/// @param text the regular expression
/// @return the tokens making up @c text
std::vector<RegexToken> LexRegex(std::string_view text);
/// Construct an NFA from a list of tokens.
///
/// @param tokens the tokens to parse, usually as a return value
/// of @c LexRegex().
/// @return the constructed NFA.
///
/// @see Nfa
Nfa ParseRegex(const std::vector<RegexToken>& tokens);

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
