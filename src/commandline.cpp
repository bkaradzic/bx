/*
 * Copyright 2010-2017 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#include <bx/commandline.h>

#include <ctype.h>

namespace bx
{
	// Reference:
	// http://msdn.microsoft.com/en-us/library/a1y7w461.aspx
	const char* tokenizeCommandLine(const char* _commandLine, char* _buffer, uint32_t& _bufferSize, int& _argc, char* _argv[], int _maxArgvs, char _term)
	{
		int argc = 0;
		const char* curr = _commandLine;
		char* currOut = _buffer;
		char term = ' ';
		bool sub = false;

		enum ParserState
		{
			SkipWhitespace,
			SetTerm,
			Copy,
			Escape,
			End,
		};

		ParserState state = SkipWhitespace;

		while ('\0' != *curr
		&&     _term != *curr
		&&     argc < _maxArgvs)
		{
			switch (state)
			{
				case SkipWhitespace:
					for (; isspace(*curr); ++curr) {}; // skip whitespace
					state = SetTerm;
					break;

				case SetTerm:
					if ('"' == *curr)
					{
						term = '"';
						++curr; // skip begining quote
					}
					else
					{
						term = ' ';
					}

					_argv[argc] = currOut;
					++argc;

					state = Copy;
					break;

				case Copy:
					if ('\\' == *curr)
					{
						state = Escape;
					}
					else if ('"' == *curr
						&&  '"' != term)
					{
						sub = !sub;
					}
					else if (isspace(*curr) && !sub)
					{
						state = End;
					}
					else if (term != *curr || sub)
					{
						*currOut = *curr;
						++currOut;
					}
					else
					{
						state = End;
					}
					++curr;
					break;

				case Escape:
					{
						const char* start = --curr;
						for (; '\\' == *curr; ++curr) {};

						if ('"' != *curr)
						{
							int count = (int)(curr-start);

							curr = start;
							for (int ii = 0; ii < count; ++ii)
							{
								*currOut = *curr;
								++currOut;
								++curr;
							}
						}
						else
						{
							curr = start+1;
							*currOut = *curr;
							++currOut;
							++curr;
						}
					}
					state = Copy;
					break;

				case End:
					*currOut = '\0';
					++currOut;
					state = SkipWhitespace;
					break;
			}
		}

		*currOut = '\0';
		if (0 < argc
		&&  '\0' == _argv[argc-1][0])
		{
			--argc;
		}

		_bufferSize = (uint32_t)(currOut - _buffer);
		_argc = argc;

		if ('\0' != *curr)
		{
			++curr;
		}

		return curr;
	}

	CommandLine::CommandLine(int _argc, char const* const* _argv)
		: m_argc(_argc)
		, m_argv(_argv)
	{
	}

	const char* CommandLine::findOption(const char* _long, const char* _default) const
	{
		const char* result = find(0, '\0', _long, 1);
		return result == NULL ? _default : result;
	}

	const char* CommandLine::findOption(const char _short, const char* _long, const char* _default) const
	{
		const char* result = find(0, _short, _long, 1);
		return result == NULL ? _default : result;
	}

	const char* CommandLine::findOption(const char* _long, int _numParams) const
	{
		const char* result = find(0, '\0', _long, _numParams);
		return result;
	}

	const char* CommandLine::findOption(const char _short, const char* _long, int _numParams) const
	{
		const char* result = find(0, _short, _long, _numParams);
		return result;
	}

	const char* CommandLine::findOption(int _skip, const char _short, const char* _long, int _numParams) const
	{
		const char* result = find(_skip, _short, _long, _numParams);
		return result;
	}

	bool CommandLine::hasArg(const char _short, const char* _long) const
	{
		const char* arg = findOption(_short, _long, 0);
		return NULL != arg;
	}

	bool CommandLine::hasArg(const char* _long) const
	{
		const char* arg = findOption('\0', _long, 0);
		return NULL != arg;
	}

	bool CommandLine::hasArg(const char*& _value, const char _short, const char* _long) const
	{
		const char* arg = findOption(_short, _long, 1);
		_value = arg;
		return NULL != arg;
	}

	bool CommandLine::hasArg(int& _value, const char _short, const char* _long) const
	{
		const char* arg = findOption(_short, _long, 1);
		if (NULL != arg)
		{
			_value = atoi(arg);
			return true;
		}

		return false;
	}

	bool CommandLine::hasArg(unsigned int& _value, const char _short, const char* _long) const
	{
		const char* arg = findOption(_short, _long, 1);
		if (NULL != arg)
		{
			_value = atoi(arg);
			return true;
		}

		return false;
	}

	bool CommandLine::hasArg(float& _value, const char _short, const char* _long) const
	{
		const char* arg = findOption(_short, _long, 1);
		if (NULL != arg)
		{
			_value = float(atof(arg));
			return true;
		}

		return false;
	}

	bool CommandLine::hasArg(double& _value, const char _short, const char* _long) const
	{
		const char* arg = findOption(_short, _long, 1);
		if (NULL != arg)
		{
			_value = atof(arg);
			return true;
		}

		return false;
	}

	bool CommandLine::hasArg(bool& _value, const char _short, const char* _long) const
	{
		const char* arg = findOption(_short, _long, 1);
		if (NULL != arg)
		{
			if ('0' == *arg || (0 == stricmp(arg, "false") ) )
			{
				_value = false;
			}
			else if ('0' != *arg || (0 == stricmp(arg, "true") ) )
			{
				_value = true;
			}

			return true;
		}

		return false;
	}

	const char* CommandLine::find(int _skip, const char _short, const char* _long, int _numParams) const
	{
		for (int ii = 0; ii < m_argc; ++ii)
		{
			const char* arg = m_argv[ii];
			if ('-' == *arg)
			{
				++arg;
				if (_short == *arg)
				{
					if (1 == strlen(arg) )
					{
						if (0 == _skip)
						{
							if (0 == _numParams)
							{
								return "";
							}
							else if (ii+_numParams < m_argc
									&& '-' != *m_argv[ii+1] )
							{
								return m_argv[ii+1];
							}

							return NULL;
						}

						--_skip;
						ii += _numParams;
					}
				}
				else if (NULL != _long
						&&  '-' == *arg
						&&  0 == stricmp(arg+1, _long) )
				{
					if (0 == _skip)
					{
						if (0 == _numParams)
						{
							return "";
						}
						else if (ii+_numParams < m_argc
								&&  '-' != *m_argv[ii+1] )
						{
							return m_argv[ii+1];
						}

						return NULL;
					}

					--_skip;
					ii += _numParams;
				}
			}
		}

		return NULL;
	}

} // namespace bx
