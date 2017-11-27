/*
 * Copyright 2010-2017 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#include "bx_p.h"
#include <bx/file.h>
#include <bx/os.h>
#include <bx/readerwriter.h>

#include <stdio.h>  // remove
#include <dirent.h> // opendir

#if BX_CRT_MSVC
#	include <direct.h>   // _getcwd
#else
#	include <sys/stat.h> // mkdir
#	include <unistd.h>   // getcwd
#endif // BX_CRT_MSVC

#if BX_PLATFORM_WINDOWS
extern "C" __declspec(dllimport) unsigned long __stdcall GetTempPathA(unsigned long _max, char* _ptr);
#endif // BX_PLATFORM_WINDOWS

namespace bx
{
	static bool isPathSeparator(char _ch)
	{
		return false
			|| '/'  == _ch
			|| '\\' == _ch
			;
	}

	static int32_t normalizeFilePath(char* _dst, int32_t _dstSize, const char* _src, int32_t _num)
	{
		// Reference: Lexical File Names in Plan 9 or Getting Dot-Dot Right
		// https://9p.io/sys/doc/lexnames.html

		const int32_t num = strLen(_src, _num);

		if (0 == num)
		{
			return strCopy(_dst, _dstSize, ".");
		}

		int32_t size = 0;

		StaticMemoryBlockWriter writer(_dst, _dstSize);
		Error err;

		int32_t idx      = 0;
		int32_t dotdot   = 0;

		if (2 <= num
		&&  ':' == _src[1])
		{
			size += write(&writer, toUpper(_src[idx]), &err);
			size += write(&writer, ':', &err);
			idx  += 2;
			dotdot = size;
		}

		const int32_t slashIdx = idx;

		bool rooted = isPathSeparator(_src[idx]);
		if (rooted)
		{
			size += write(&writer, '/', &err);
			++idx;
			dotdot = size;
		}

		bool trailingSlash = false;

		while (idx < num && err.isOk() )
		{
			switch (_src[idx])
			{
			case '/':
			case '\\':
				++idx;
				trailingSlash = idx == num;
				break;

			case '.':
				if (idx+1 == num
				||  isPathSeparator(_src[idx+1]) )
				{
					++idx;
					break;
				}

				if ('.' == _src[idx+1]
				&& (idx+2 == num || isPathSeparator(_src[idx+2]) ) )
				{
					idx += 2;

					if (dotdot < size)
					{
						for (--size
							; dotdot < size && !isPathSeparator(_dst[size])
							; --size)
						{
						}
						seek(&writer, size, Whence::Begin);
					}
					else if (!rooted)
					{
						if (0 < size)
						{
							size += write(&writer, '/', &err);
						}

						size += write(&writer, "..", &err);
						dotdot = size;
					}

					break;
				}

				BX_FALLTHROUGH;

			default:
				if ( ( rooted && slashIdx+1 != size)
				||   (!rooted &&          0 != size) )
				{
					size += write(&writer, '/', &err);
				}

				for (; idx < num && !isPathSeparator(_src[idx]); ++idx)
				{
					size += write(&writer, _src[idx], &err);
				}

				break;
			}
		}

		if (0 == size)
		{
			size += write(&writer, '.', &err);
		}

		if (trailingSlash)
		{
			size += write(&writer, '/', &err);
		}

		write(&writer, '\0', &err);

		return size;
	}

	static bool getEnv(const char* _name, FileInfo::Enum _type, char* _out, uint32_t* _inOutSize)
	{
		uint32_t len = *_inOutSize;
		*_out = '\0';

		if (getenv(_name, _out, &len) )
		{
			FileInfo fi;
			if (stat(_out, fi)
			&&  _type == fi.m_type)
			{
				*_inOutSize = len;
				return true;
			}
		}

		return false;
	}

	static char* pwd(char* _buffer, uint32_t _size)
	{
#if BX_PLATFORM_PS4     \
 || BX_PLATFORM_XBOXONE \
 || BX_PLATFORM_WINRT
		BX_UNUSED(_buffer, _size);
		return NULL;
#elif BX_CRT_MSVC
		return ::_getcwd(_buffer, (int32_t)_size);
#else
		return ::getcwd(_buffer, _size);
#endif // BX_COMPILER_
	}

	static bool getCurrentPath(char* _out, uint32_t* _inOutSize)
	{
		uint32_t len = *_inOutSize;
		pwd(_out, len);
		*_inOutSize = strLen(_out);
		return true;
	}

	static bool getHomePath(char* _out, uint32_t* _inOutSize)
	{
		return false
#if BX_PLATFORM_WINDOWS
			|| getEnv("USERPROFILE", FileInfo::Directory, _out, _inOutSize)
#endif // BX_PLATFORM_WINDOWS
			|| getEnv("HOME", FileInfo::Directory, _out, _inOutSize)
			;
	}

	static bool getTempPath(char* _out, uint32_t* _inOutSize)
	{
#if BX_PLATFORM_WINDOWS
		uint32_t len = ::GetTempPathA(*_inOutSize, _out);
		bool result = len != 0 && len < *_inOutSize;
		*_inOutSize = len;
		return result;
#else
		static const char* s_tmp[] =
		{
			"TMPDIR",
			"TMP",
			"TEMP",
			"TEMPDIR",

			NULL
		};

		for (const char** tmp = s_tmp; *tmp != NULL; ++tmp)
		{
			uint32_t len = *_inOutSize;
			*_out = '\0';
			bool ok = getEnv(*tmp, FileInfo::Directory, _out, &len);

			if (ok
			&&  len != 0
			&&  len < *_inOutSize)
			{
				*_inOutSize = len;
				return ok;
			}
		}

		FileInfo fi;
		if (stat("/tmp", fi)
		&&  FileInfo::Directory == fi.m_type)
		{
			strCopy(_out, *_inOutSize, "/tmp");
			*_inOutSize = 4;
			return true;
		}

		return false;
#endif // BX_PLATFORM_*
	}

	FilePath::FilePath()
	{
		set("");
	}

	FilePath::FilePath(Dir::Enum _dir)
	{
		set(_dir);
	}

	FilePath::FilePath(const char* _rhs)
	{
		set(_rhs);
	}

	FilePath::FilePath(const StringView& _filePath)
	{
		set(_filePath);
	}

	FilePath& FilePath::operator=(const StringView& _rhs)
	{
		set(_rhs);
		return *this;
	}

	void FilePath::set(Dir::Enum _dir)
	{
		char tmp[kMaxFilePath];
		uint32_t len = BX_COUNTOF(tmp);

		switch (_dir)
		{
		case Dir::Current:
			getCurrentPath(tmp, &len);
			break;

		case Dir::Temp:
			getTempPath(tmp, &len);
			break;

		case Dir::Home:
			getHomePath(tmp, &len);
			break;

		default:
			len = 0;
			break;
		}

		set(StringView(tmp, len) );
	}

	void FilePath::set(const StringView& _filePath)
	{
		normalizeFilePath(
			  m_filePath
			, BX_COUNTOF(m_filePath)
			, _filePath.getPtr()
			, _filePath.getLength()
			);
	}

	void FilePath::join(const StringView& _str)
	{
		char tmp[kMaxFilePath];
		strCopy(tmp, BX_COUNTOF(tmp), m_filePath);
		strCat(tmp, BX_COUNTOF(tmp), "/");
		strCat(tmp, BX_COUNTOF(tmp), _str);
		set(tmp);
	}

	const char* FilePath::get() const
	{
		return m_filePath;
	}

	const StringView FilePath::getPath() const
	{
		const char* end = strRFind(m_filePath, '/');
		if (NULL != end)
		{
			return StringView(m_filePath, end+1);
		}

		return StringView();
	}

	const StringView FilePath::getFileName() const
	{
		const char* fileName = strRFind(m_filePath, '/');
		if (NULL != fileName)
		{
			return StringView(fileName+1);
		}

		return get();
	}

	const StringView FilePath::getBaseName() const
	{
		const StringView fileName = getFileName();
		if (!fileName.isEmpty() )
		{
			const char* ext = strFind(fileName, '.');
			if (ext != NULL)
			{
				return StringView(fileName.getPtr(), ext);
			}

			return fileName;
		}

		return StringView();
	}

	const StringView FilePath::getExt() const
	{
		const StringView fileName = getFileName();
		if (!fileName.isEmpty() )
		{
			const char* ext = strFind(fileName, '.');
			return StringView(ext);
		}

		return StringView();
	}

	bool FilePath::isAbsolute() const
	{
		return  '/' == m_filePath[0] // no drive letter
			|| (':' == m_filePath[1] && '/' == m_filePath[2]) // with drive letter
			;
	}

	bool make(const FilePath& _filePath, Error* _err)
	{
		BX_ERROR_SCOPE(_err);

		if (!_err->isOk() )
		{
			return false;
		}

#if BX_CRT_MSVC
		int32_t result = ::_mkdir(_filePath.get() );
#elif BX_PLATFORM_WINDOWS
		int32_t result = ::mkdir(_filePath.get());
#else
		int32_t result = ::mkdir(_filePath.get(), 0700);
#endif // BX_CRT_MSVC

		if (0 != result)
		{
			BX_ERROR_SET(_err, BX_ERROR_ACCESS, "The parent directory does not allow write permission to the process.");
			return false;
		}

		return true;
	}

	bool makeAll(const FilePath& _filePath, Error* _err)
	{
		BX_ERROR_SCOPE(_err);

		if (!_err->isOk() )
		{
			return false;
		}

		FileInfo fi;

		if (stat(_filePath, fi) )
		{
			if (FileInfo::Directory == fi.m_type)
			{
				return true;
			}

			BX_ERROR_SET(_err, BX_ERROR_NOT_DIRECTORY, "File already exist, and is not directory.");
			return false;
		}

		const StringView dir = strRTrim(_filePath.get(), "/");
		const char* slash = strRFind(dir, '/');

		if (NULL != slash
		&&  slash - dir.getPtr() > 1)
		{
			if (!makeAll(StringView(dir.getPtr(), slash), _err) )
			{
				return false;
			}
		}

		FilePath path(dir);
		return make(path, _err);
	}

	bool remove(const FilePath& _filePath, Error* _err)
	{
		BX_ERROR_SCOPE(_err);

		if (!_err->isOk() )
		{
			return false;
		}

#if BX_CRT_MSVC
		int32_t result;
		FileInfo fi;
		if (stat(_filePath, fi) )
		{
			if (FileInfo::Directory == fi.m_type)
			{
				result = ::_rmdir(_filePath.get() );
			}
			else
			{
				result = ::remove(_filePath.get() );
			}
		}
#else
		int32_t result = ::remove(_filePath.get() );
#endif // BX_CRT_MSVC

		if (0 != result)
		{
			BX_ERROR_SET(_err, BX_ERROR_ACCESS, "The parent directory does not allow write permission to the process.");
			return false;
		}

		return true;
	}

	bool removeAll(const FilePath& _filePath, Error* _err)
	{
		BX_ERROR_SCOPE(_err);

		if (remove(_filePath, _err) )
		{
			return true;
		}

		_err->reset();

		FileInfo fi;

		if (!stat(_filePath, fi) )
		{
			BX_ERROR_SET(_err, BX_ERROR_ACCESS, "The parent directory does not allow write permission to the process.");
			return false;
		}

		if (FileInfo::Directory != fi.m_type)
		{
			BX_ERROR_SET(_err, BX_ERROR_NOT_DIRECTORY, "File already exist, and is not directory.");
			return false;
		}

		DIR* dir = opendir(_filePath.get() );
		if (NULL == dir)
		{
			BX_ERROR_SET(_err, BX_ERROR_NOT_DIRECTORY, "File already exist, and is not directory.");
			return false;
		}

		for (dirent* item = readdir(dir); NULL != item; item = readdir(dir) )
		{
			if (0 == strCmp(item->d_name, ".")
			||  0 == strCmp(item->d_name, "..") )
			{
				continue;
			}

			FilePath path(_filePath);
			path.join(item->d_name);
			if (!removeAll(path, _err) )
			{
				_err->reset();
				break;
			}
		}

		closedir(dir);

		return remove(_filePath, _err);
	}

} // namespace bx
