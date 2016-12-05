/*
 * Copyright 2010-2016 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#ifndef BX_CRTIMPL_H_HEADER_GUARD
#define BX_CRTIMPL_H_HEADER_GUARD

#include "bx.h"

#if BX_CONFIG_ALLOCATOR_CRT
#	include <cstdlib>
#	include "allocator.h"
#endif // BX_CONFIG_ALLOCATOR_CRT

#if BX_CONFIG_CRT_FILE_READER_WRITER
#	include "readerwriter.h"
#endif // BX_CONFIG_CRT_FILE_READER_WRITER

namespace bx
{
#if BX_CONFIG_ALLOCATOR_CRT
	class CrtAllocator : public AllocatorI
	{
	public:
		CrtAllocator()
		{
		}

		virtual ~CrtAllocator()
		{
		}

		virtual void* realloc(void* _ptr, size_t _size, size_t _align, const char* _file, uint32_t _line) BX_OVERRIDE
		{
			if (0 == _size)
			{
				if (NULL != _ptr)
				{
					if (BX_CONFIG_ALLOCATOR_NATURAL_ALIGNMENT >= _align)
					{
						::free(_ptr);
						return NULL;
					}

#	if BX_COMPILER_MSVC
					BX_UNUSED(_file, _line);
					_aligned_free(_ptr);
#	else
					bx::alignedFree(this, _ptr, _align, _file, _line);
#	endif // BX_
				}

				return NULL;
			}
			else if (NULL == _ptr)
			{
				if (BX_CONFIG_ALLOCATOR_NATURAL_ALIGNMENT >= _align)
				{
					return ::malloc(_size);
				}

#	if BX_COMPILER_MSVC
				BX_UNUSED(_file, _line);
				return _aligned_malloc(_size, _align);
#	else
				return bx::alignedAlloc(this, _size, _align, _file, _line);
#	endif // BX_
			}

			if (BX_CONFIG_ALLOCATOR_NATURAL_ALIGNMENT >= _align)
			{
				return ::realloc(_ptr, _size);
			}

#	if BX_COMPILER_MSVC
			BX_UNUSED(_file, _line);
			return _aligned_realloc(_ptr, _size, _align);
#	else
			return bx::alignedRealloc(this, _ptr, _size, _align, _file, _line);
#	endif // BX_
		}
	};
#endif // BX_CONFIG_ALLOCATOR_CRT

#if BX_CONFIG_CRT_FILE_READER_WRITER

#	if BX_CRT_MSVC
#		define fseeko64 _fseeki64
#		define ftello64 _ftelli64
#	elif 0 \
	  || BX_PLATFORM_ANDROID \
	  || BX_PLATFORM_BSD \
	  || BX_PLATFORM_IOS \
	  || BX_PLATFORM_OSX \
	  || BX_PLATFORM_QNX
#		define fseeko64 fseeko
#		define ftello64 ftello
#	elif BX_PLATFORM_PS4
#		define fseeko64 fseek
#		define ftello64 ftell
#	endif // BX_

	class CrtFileReader : public FileReaderI
	{
	public:
		CrtFileReader()
			: m_file(NULL)
		{
		}

		virtual ~CrtFileReader()
		{
		}

		virtual bool open(const char* _filePath, Error* _err) BX_OVERRIDE
		{
			BX_CHECK(NULL != _err, "Reader/Writer interface calling functions must handle errors.");

			m_file = fopen(_filePath, "rb");
			if (NULL == m_file)
			{
				BX_ERROR_SET(_err, BX_ERROR_READERWRITER_OPEN, "CrtFileReader: Failed to open file.");
				return false;
			}

			return true;
		}

		virtual void close() BX_OVERRIDE
		{
			fclose(m_file);
		}

		virtual int64_t seek(int64_t _offset = 0, Whence::Enum _whence = Whence::Current) BX_OVERRIDE
		{
			fseeko64(m_file, _offset, _whence);
			return ftello64(m_file);
		}

		virtual int32_t read(void* _data, int32_t _size, Error* _err) BX_OVERRIDE
		{
			BX_CHECK(NULL != _err, "Reader/Writer interface calling functions must handle errors.");

			int32_t size = (int32_t)fread(_data, 1, _size, m_file);
			if (size != _size)
			{
				if (0 != feof(m_file) )
				{
					BX_ERROR_SET(_err, BX_ERROR_READERWRITER_EOF, "CrtFileWriter: EOF.");
				}
				else if (0 != ferror(m_file) )
				{
					BX_ERROR_SET(_err, BX_ERROR_READERWRITER_READ, "CrtFileWriter: read error.");
				}

				return size >= 0 ? size : 0;
			}

			return size;
		}

	private:
		FILE* m_file;
	};

	class CrtFileWriter : public FileWriterI
	{
	public:
		CrtFileWriter()
			: m_file(NULL)
		{
		}

		virtual ~CrtFileWriter()
		{
		}

		virtual bool open(const char* _filePath, bool _append, Error* _err) BX_OVERRIDE
		{
			m_file = fopen(_filePath, _append ? "ab" : "wb");

			if (NULL == m_file)
			{
				BX_ERROR_SET(_err, BX_ERROR_READERWRITER_OPEN, "CrtFileWriter: Failed to open file.");
				return false;
			}

			return true;
		}

		virtual void close() BX_OVERRIDE
		{
			fclose(m_file);
		}

		virtual int64_t seek(int64_t _offset = 0, Whence::Enum _whence = Whence::Current) BX_OVERRIDE
		{
			fseeko64(m_file, _offset, _whence);
			return ftello64(m_file);
		}

		virtual int32_t write(const void* _data, int32_t _size, Error* _err) BX_OVERRIDE
		{
			BX_CHECK(NULL != _err, "Reader/Writer interface calling functions must handle errors.");

			int32_t size = (int32_t)fwrite(_data, 1, _size, m_file);
			if (size != _size)
			{
				BX_ERROR_SET(_err, BX_ERROR_READERWRITER_WRITE, "CrtFileWriter: write failed.");
				return size >= 0 ? size : 0;
			}

			return size;
		}

	private:
		FILE* m_file;
	};
#endif // BX_CONFIG_CRT_FILE_READER_WRITER

#if BX_CONFIG_CRT_PROCESS

#if BX_CRT_MSVC
#	define popen  _popen
#	define pclose _pclose
#endif // BX_CRT_MSVC

	class ProcessReader : public ReaderOpenI, public CloserI, public ReaderI
	{
	public:
		ProcessReader()
			: m_file(NULL)
		{
		}

		~ProcessReader()
		{
			BX_CHECK(NULL == m_file, "Process not closed!");
		}

		virtual bool open(const char* _command, Error* _err) BX_OVERRIDE
		{
			BX_CHECK(NULL != _err, "Reader/Writer interface calling functions must handle errors.");

			m_file = popen(_command, "r");
			if (NULL == m_file)
			{
				BX_ERROR_SET(_err, BX_ERROR_READERWRITER_OPEN, "ProcessReader: Failed to open process.");
				return false;
			}

			return true;
		}

		virtual void close() BX_OVERRIDE
		{
			BX_CHECK(NULL != m_file, "Process not open!");
			m_exitCode = pclose(m_file);
			m_file = NULL;
		}

		virtual int32_t read(void* _data, int32_t _size, Error* _err) BX_OVERRIDE
		{
			BX_CHECK(NULL != _err, "Reader/Writer interface calling functions must handle errors."); BX_UNUSED(_err);

			int32_t size = (int32_t)fread(_data, 1, _size, m_file);
			if (size != _size)
			{
				if (0 != feof(m_file) )
				{
					BX_ERROR_SET(_err, BX_ERROR_READERWRITER_EOF, "CrtFileWriter: EOF.");
				}
				else if (0 != ferror(m_file) )
				{
					BX_ERROR_SET(_err, BX_ERROR_READERWRITER_READ, "CrtFileWriter: read error.");
				}

				return size >= 0 ? size : 0;
			}

			return size;
		}

		int32_t getExitCode() const
		{
			return m_exitCode;
		}

	private:
		FILE* m_file;
		int32_t m_exitCode;
	};

	class ProcessWriter : public WriterOpenI, public CloserI, public WriterI
	{
	public:
		ProcessWriter()
			: m_file(NULL)
		{
		}

		~ProcessWriter()
		{
			BX_CHECK(NULL == m_file, "Process not closed!");
		}

		virtual bool open(const char* _command, bool, Error* _err) BX_OVERRIDE
		{
			BX_CHECK(NULL != _err, "Reader/Writer interface calling functions must handle errors.");

			m_file = popen(_command, "w");
			if (NULL == m_file)
			{
				BX_ERROR_SET(_err, BX_ERROR_READERWRITER_OPEN, "ProcessWriter: Failed to open process.");
				return false;
			}

			return true;
		}

		virtual void close() BX_OVERRIDE
		{
			BX_CHECK(NULL != m_file, "Process not open!");
			m_exitCode = pclose(m_file);
			m_file = NULL;
		}

		virtual int32_t write(const void* _data, int32_t _size, Error* _err) BX_OVERRIDE
		{
			BX_CHECK(NULL != _err, "Reader/Writer interface calling functions must handle errors."); BX_UNUSED(_err);

			int32_t size = (int32_t)fwrite(_data, 1, _size, m_file);
			if (size != _size)
			{
				if (0 != ferror(m_file) )
				{
					BX_ERROR_SET(_err, BX_ERROR_READERWRITER_WRITE, "CrtFileWriter: write error.");
				}

				return size >= 0 ? size : 0;
			}

			return size;
		}

		int32_t getExitCode() const
		{
			return m_exitCode;
		}

	private:
		FILE* m_file;
		int32_t m_exitCode;
	};

#endif // BX_CONFIG_CRT_PROCESS

} // namespace bx

#endif // BX_CRTIMPL_H_HEADER_GUARD
