#include <algorithm>
#include <iostream>
#include <limits>
#include <stack>
#include <stdexcept>

#include <re2tbl/re2tbl.hpp>

struct RegexParseState
{
	const std::vector<re2tbl::RegexToken>& tokens;
	typename std::remove_reference_t<decltype(tokens)>::size_type position;
	re2tbl::NfaNode* start;
	re2tbl::NfaNode* end;
	re2tbl::Nfa* nfa;
};

// Thompson's construction functions

/// Parse alternatives, of the form a|b.
static void ParseAlternatives(RegexParseState& state);
/// Parse concatenations, of the form ab.
static void ParseConcatenations(RegexParseState& state);
/// Parse closures, of the form a* or a+.
static void ParseClosure(RegexParseState& state);
/// Parse an atom.
///
/// An atom is of the form a, where a is some literal character,
/// or a parenthesized subexpression. For example, (a|b) is an atom,
/// due to the parentheses.
static void ParseAtom(RegexParseState& state);

// Subset construction functions

/// Compute the "epsilon closure" of a particular NFA state.
///
/// The epsilon closure is the set of all NFA states that can
/// be reached from the current state via only epsilon moves.
static std::unordered_set<std::size_t>
ComputeEpsilonClosure(const re2tbl::Nfa& nfa, std::size_t node);
static std::unordered_set<std::size_t>
MoveOn(const re2tbl::Nfa& nfa, std::unordered_set<std::size_t> nodes, char c);

re2tbl::NfaNode* re2tbl::Nfa::AddNode()
{
	nodes.emplace_back(std::make_unique<NfaNode>(NfaNode {
	    .id = nodes.size(),
	    .accepting = false,
	    .next = std::vector<NfaEdge> {},
	}));
	return nodes.back().get();
}

void re2tbl::Nfa::ConnectNodes(NfaNode* a, NfaNode* b, char transition)
{
	NfaEdge edge = {
	    .epsilon = transition == '\0',
	    .chars = {},
	    .next = b,
	};
	if (transition != '\0')
	{
		edge.chars.set(transition);
	}
	a->next.push_back(edge);
}

void re2tbl::Nfa::Display(std::ostream& os) const
{
	os << "Start node is (" << start->id << ")\n";
	for (const auto& node : nodes)
	{
		os << "Node (" << node->id << ") ";
		for (const auto& edge : node->next)
		{
			os << "->";
			if (edge.epsilon)
			{
				os << "eps";
			}
			else
			{
				os << "'";
				for (int c = '\0'; c <= std::numeric_limits<char>::max(); ++c)
				{
					if (edge.chars.test(c))
					{
						os << static_cast<char>(c);
					}
				}
				os << "'";
			}
			os << " (" << edge.next->id << ") ";
		}
		if (node->accepting)
		{
			os << "accepting";
		}
		os << "\n";
	}
}

std::vector<re2tbl::RegexToken> re2tbl::LexRegex(std::string_view text)
{
	std::vector<RegexToken> result;

	for (char c : text)
	{
		RegexToken tmp = {
		    .type = RegexTokenType::kLiteral,
		    .literal = c,
		};
		switch (c)
		{
		case '(':
			tmp.type = RegexTokenType::kLeftParen;
			break;
		case ')':
			tmp.type = RegexTokenType::kRightParen;
			break;
		case '|':
			tmp.type = RegexTokenType::kPipe;
			break;
		case '*':
			tmp.type = RegexTokenType::kStar;
			break;
		case '+':
			tmp.type = RegexTokenType::kPlus;
			break;
		default:
			break;
		}
		result.push_back(tmp);
	}
	result.push_back(RegexToken {
	    .type = RegexTokenType::kEof,
	    .literal = '\0',
	});

	return result;
}

re2tbl::Nfa re2tbl::ParseRegex(const std::vector<RegexToken>& tokens)
{
	Nfa result;
	// expand the machine by parsing based on precedence
	auto state = RegexParseState {
	    .tokens = tokens,
	    .position = 0,
	    .start = nullptr,
	    .end = nullptr,
	    .nfa = &result,
	};
	ParseAlternatives(state);
	if (state.position < state.tokens.size() - 1)
	{
		throw std::runtime_error {"There's too much regex in this regex"};
	}
	// if the machine is empty, start will be nullptr
	result.start = state.start;
	if (state.end != nullptr)
	{
		state.end->accepting = true;
	}

	// all done!
	return result;
}

static void ParseAlternatives(RegexParseState& state)
{
	ParseConcatenations(state);
	while (state.tokens[state.position].type == re2tbl::RegexTokenType::kPipe)
	{
		re2tbl::NfaNode* new_start = state.nfa->AddNode();
		re2tbl::NfaNode* new_end = state.nfa->AddNode();
		//           (1) ->x (2)
		//
		// ---->
		//
		// (0)*->eps*(1) ->x (2)*->eps*(3)
		state.nfa->ConnectNodes(new_start, state.start);
		state.nfa->ConnectNodes(state.end, new_end);
		++state.position;
		ParseConcatenations(state);
		// (0) ->eps (1) ->x (2) ->eps (3)
		//
		// ---->
		//
		//           (1) ->x (2)
		//     ->eps             ->eps
		// (0)                         (3)
		//    *->eps*           *->eps*
		//           (4) ->y (5)
		state.nfa->ConnectNodes(new_start, state.start);
		state.nfa->ConnectNodes(state.end, new_end);

		state.start = new_start;
		state.end = new_end;
	}
}

static void ParseConcatenations(RegexParseState& state)
{
	ParseClosure(state);
	bool can_concat = true;
	while (can_concat)
	{
		switch (state.tokens[state.position].type)
		{
		case re2tbl::RegexTokenType::kPipe:
			// handle this in ParseAlternatives
			can_concat = false;
			continue;
		case re2tbl::RegexTokenType::kPlus:
		case re2tbl::RegexTokenType::kStar:
			throw std::runtime_error {"Misplaced closure symbol in regex"};
		case re2tbl::RegexTokenType::kRightParen:
			// assuming we're inside parens right now, this will be handled in ParseAtom.
			// otherwise, the error is detected by the calling function.
			can_concat = false;
			continue;
		case re2tbl::RegexTokenType::kEof:
			can_concat = false;
			continue;
		case re2tbl::RegexTokenType::kLeftParen:
		case re2tbl::RegexTokenType::kLiteral:
			// finally, something we can concatenate!
			// don't advance though, we're looking at 'b' in the regex 'ab' (for example).
			break;
		}
		re2tbl::NfaNode* old_start = state.start;
		re2tbl::NfaNode* old_end = state.end;
		ParseClosure(state);
		// (0) ->x (1)*->eps*(2) ->y (3)
		state.nfa->ConnectNodes(old_end, state.start);
		// was (2), set it to (0)
		state.start = old_start;
	}
}

static void ParseClosure(RegexParseState& state)
{
	ParseAtom(state);
	if (state.tokens[state.position].type == re2tbl::RegexTokenType::kPlus ||
	    state.tokens[state.position].type == re2tbl::RegexTokenType::kStar)
	{
		re2tbl::NfaNode* new_start = state.nfa->AddNode();
		re2tbl::NfaNode* new_end = state.nfa->AddNode();
		// lots going on here...
		// initially:
		// (0) ->x (1)

		// after this line:
		// (2)*->eps*(0) ->x (1)
		state.nfa->ConnectNodes(new_start, state.start);
		// (2) ->eps (0) ->x (1)*->eps*(3)
		state.nfa->ConnectNodes(state.end, new_end);
		// (2) ->eps (0) ->x (1) ->eps (3)
		//            |*<-eps*|
		state.nfa->ConnectNodes(state.end, state.start);
		if (state.tokens[state.position].type == re2tbl::RegexTokenType::kStar)
		{
			// kleene closure can skip the entire subexpression,
			// whereas plus closure cannot
			// result:
			//  |*------------eps--------->*|
			// (2) ->eps (0) ->x (1) ->eps (3)
			//            | <-eps |
			state.nfa->ConnectNodes(new_start, new_end);
		}
		state.start = new_start;
		state.end = new_end;

		// consume the closure token
		++state.position;
	}
}

static void ParseAtom(RegexParseState& state)
{
	if (state.tokens[state.position].type == re2tbl::RegexTokenType::kLeftParen)
	{
		// parse a subexpression
		++state.position;
		ParseAlternatives(state);
		if (state.tokens[state.position].type != re2tbl::RegexTokenType::kRightParen)
		{
			throw std::runtime_error {"Missing closing parenthesis in regex"};
		}
		++state.position;
	}
	else if (state.tokens[state.position].type == re2tbl::RegexTokenType::kEof)
	{
		state.end->accepting = true;
	}
	else
	{
		re2tbl::NfaNode* start = state.nfa->AddNode();
		re2tbl::NfaNode* end = state.nfa->AddNode();
		state.nfa->ConnectNodes(start, end, state.tokens[state.position].literal);
		state.start = start;
		state.end = end;
		// don't forget to advance!
		++state.position;
	}
}

static std::unordered_set<std::size_t>
ComputeEpsilonClosure(const re2tbl::Nfa& nfa, std::size_t node)
{
	std::unordered_set<std::size_t> result;
	std::stack<std::size_t> to_visit;
	to_visit.push(nfa.nodes[node]->id);

	while (!to_visit.empty())
	{
		std::size_t current = to_visit.top();
		to_visit.pop();
		if (std::find(result.begin(), result.end(), current) != result.end())
		{
			// already visited this node
			continue;
		}
		result.insert(current);

		for (const auto& edge : nfa.nodes[current]->next)
		{
			if (edge.epsilon)
			{
				to_visit.push(edge.next->id);
			}
		}
	}
	return result;
}

re2tbl::Dfa::Dfa(const re2tbl::Nfa& nfa)
    : start(0)
{
	// using notation from the book
	auto start_closure = ComputeEpsilonClosure(nfa, nfa.start->id);
	std::stack<std::unordered_set<std::size_t>> work_list;
	work_list.push(start_closure);
	nodes.emplace_back(DfaNode {
	    .id = 0,
	    .nfa_equivalent = start_closure,
	    .accepting = false,
	    .next = {},
	});

	while (!work_list.empty())
	{
		auto current_set = work_list.top();
		DfaNode* current =
		    &*std::find_if(nodes.begin(), nodes.end(), [&current_set](const auto& node) {
			    return node.nfa_equivalent == current_set;
		    });
		std::size_t current_idx = current->id;
		work_list.pop();

		// loop all ASCII characters
		for (int c = 0; c <= std::numeric_limits<char>::max(); ++c)
		{
			auto move_result = MoveOn(nfa, current_set, c);
			if (move_result.empty())
			{
				continue;
			}
			auto edge = std::find_if(
			    current->next.begin(), current->next.end(), [&move_result](const auto& e) {
				    return e.next->nfa_equivalent == move_result;
			    });
			if (edge == current->next.end())
			{
				auto next = std::find_if(nodes.begin(), nodes.end(), [&move_result](const auto& e) {
					return e.nfa_equivalent == move_result;
				});
				if (next != nodes.end())
				{
					current->next.emplace_back(DfaEdge {
					    .move = {},
					    .next = &*next,
					});
					current->next.back().move.set(c);
				}
				else
				{
					nodes.emplace_back(DfaNode {
					    .id = nodes.size(),
					    .nfa_equivalent = move_result,
					    .accepting = std::any_of(
					        move_result.begin(),
					        move_result.end(),
					        [&nfa](const auto& node) { return nfa.nodes[node]->accepting; }),
					    .next = {},
					});
					current = &nodes[current_idx];
					current->next.emplace_back(DfaEdge {
					    .move = {},
					    .next = &nodes.back(),
					});
					current->next.back().move.set(c);
					work_list.push(nodes.back().nfa_equivalent);
				}
			}
			else
			{
				edge->move.set(c);
			}
		}
	}
}

static std::unordered_set<std::size_t>
MoveOn(const re2tbl::Nfa& nfa, std::unordered_set<std::size_t> nodes, char c)
{
	std::unordered_set<std::size_t> result;
	for (std::size_t node : nodes)
	{
		auto it = std::find_if(
		    nfa.nodes[node]->next.begin(), nfa.nodes[node]->next.end(), [c](const auto& edge) {
			    return edge.chars.test(c);
		    });
		if (it != nfa.nodes[node]->next.end())
		{
			for (std::size_t move : ComputeEpsilonClosure(nfa, it->next->id))
			{
				result.insert(move);
			}
		}
	}
	return result;
}

void re2tbl::Dfa::Display(std::ostream& os)
{
	os << "Start node is (" << start << ")\n";
	for (const auto& node : nodes)
	{
		os << "Node (" << node.id << ") ";
		for (const auto& edge : node.next)
		{
			os << "->'";
			for (int c = 0; c < std::numeric_limits<char>::max(); ++c)
			{
				if (edge.move.test(c))
				{
					os << static_cast<char>(c);
				}
			}
			os << "' (" << edge.next->id << ") ";
		}
		if (node.accepting)
		{
			os << "accepting";
		}
		os << "\n";
	}
}

void re2tbl::Dfa::Minimize()
{
	// Hopcroft's algorithm
	std::vector<std::unordered_set<std::size_t>> partitions;
	partitions.resize(2);
	for (const auto& node : nodes)
	{
		partitions[node.accepting ? 0 : 1].emplace(node.id);
	}
	std::vector<std::unordered_set<std::size_t>> w = {partitions[0]};
	while (!w.empty())
	{
		auto a = std::move(w.back());
		w.pop_back();
		for (int c = 0; c <= std::numeric_limits<char>::max(); ++c)
		{
			// let X be the set of states for which a transition on c leads to a state in A
			std::unordered_set<std::size_t> x;
			for (const auto& node : nodes)
			{
				auto edge = std::find_if(node.next.begin(), node.next.end(), [c](const auto& edge) {
					return edge.move.test(c);
				});
				if (edge == node.next.end())
				{
					// doesn't move on c
					continue;
				}
				if (std::find_if(a.begin(), a.end(), [&edge](const auto& n) {
					    return n == edge->next->id;
				    }) != a.end())
				{
					x.emplace(node.id);
				}
			}
		}
	}
}
