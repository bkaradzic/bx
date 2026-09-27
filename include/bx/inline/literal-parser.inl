/*
 * Copyright 2011-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_LITERAL_PARSER_H_HEADER_GUARD
#	error "Must be included from bx/literal-parser.h!"
#endif // BX_LITERAL_PARSER_H_HEADER_GUARD

namespace bx
{
	inline bool LiteralParser::isOk() const
	{
		return m_ok;
	}

	inline const StringView& LiteralParser::getLiteral() const
	{
		return m_literal;
	}

	inline LiteralParser::Type LiteralParser::getType() const
	{
		return m_type;
	}

} // namespace bx
