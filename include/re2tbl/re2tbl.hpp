#pragma once

#include <bitset>
#include <limits>
#include <memory>
#include <ostream>
#include <string_view>
#include <unordered_set>
#include <vector>

namespace re2tbl
{

struct NfaNode;

struct NfaEdge
{
	bool epsilon;
	std::bitset<std::numeric_limits<char>::max() + 1> chars;
	NfaNode* next;
};

struct NfaNode
{
	std::size_t id;
	bool accepting;
	std::vector<NfaEdge> next;
};

struct Nfa
{
	std::vector<std::unique_ptr<NfaNode>> nodes;
	NfaNode* start;

	NfaNode* AddNode();
	void ConnectNodes(NfaNode* a, NfaNode* b, char transition = '\0');

	void Display(std::ostream& os) const;
};

enum class RegexTokenType
{
	kLiteral,
	kLeftParen,
	kRightParen,
	kPipe,
	kStar,
	kPlus,
	kEof,
};

struct RegexToken
{
	RegexTokenType type;
	char literal;
};

std::vector<RegexToken> LexRegex(std::string_view text);
Nfa ParseRegex(const std::vector<RegexToken>& tokens);

struct DfaNode;

struct DfaEdge
{
	std::bitset<std::numeric_limits<char>::max()> move;
	DfaNode* next;
};

struct DfaNode
{
	std::size_t id;
	std::unordered_set<std::size_t> nfa_equivalent;
	bool accepting;
	std::vector<DfaEdge> next;
};

struct Dfa
{
	std::vector<DfaNode> nodes;
	std::size_t start;

	Dfa(const Nfa& nfa);

	void Display(std::ostream& os);
	void Minimize();
};

}  // namespace re2tbl
