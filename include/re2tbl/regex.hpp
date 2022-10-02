#pragma once

#include <vector>

#include "nfa.hpp"

namespace re2tbl
{
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

}  // namespace re2tbl
