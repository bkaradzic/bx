/*
 * Copyright 2010-2017 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx#license-bsd-2-clause
 */

#ifndef BX_CRTIMPL_H_HEADER_GUARD
#define BX_CRTIMPL_H_HEADER_GUARD

#include "allocator.h"
#include "readerwriter.h"

namespace bx
{
	///
	class DefaultAllocator : public AllocatorI
	{
	public:
		///
		DefaultAllocator();

		///
		virtual ~DefaultAllocator();

		///
		virtual void* realloc(void* _ptr, size_t _size, size_t _align, const char* _file, uint32_t _line) override;
	};

	///
	ReaderI* getStdIn();

	///
	WriterI* getStdOut();

	///
	WriterI* getStdErr();

	///
	class FileReader : public FileReaderI
	{
	public:
		///
		FileReader();

		///
		virtual ~FileReader();

		///
		virtual bool open(const FilePath& _filePath, Error* _err) override;

		///
		virtual void close() override;

		///
		virtual int64_t seek(int64_t _offset = 0, Whence::Enum _whence = Whence::Current) override;

		///
		virtual int32_t read(void* _data, int32_t _size, Error* _err) override;

	private:
		BX_ALIGN_DECL(16, uint8_t) m_internal[64];
	};

	///
	class FileWriter : public FileWriterI
	{
	public:
		///
		FileWriter();

		///
		virtual ~FileWriter();

		///
		virtual bool open(const FilePath& _filePath, bool _append, Error* _err) override;

		///
		virtual void close() override;

		///
		virtual int64_t seek(int64_t _offset = 0, Whence::Enum _whence = Whence::Current) override;

		///
		virtual int32_t write(const void* _data, int32_t _size, Error* _err) override;

	private:
		BX_ALIGN_DECL(16, uint8_t) m_internal[64];
	};

	///
	class ProcessReader
		: public ProcessOpenI
		, public CloserI
		, public ReaderI
	{
	public:
		///
		ProcessReader();

		///
		~ProcessReader();

		///
		virtual bool open(const FilePath& _filePath, const StringView& _args, Error* _err) override;

		///
		virtual void close() override;

		///
		virtual int32_t read(void* _data, int32_t _size, Error* _err) override;

		///
		int32_t getExitCode() const;

	private:
		void* m_file;
		int32_t m_exitCode;
	};

	///
	class ProcessWriter
		: public ProcessOpenI
		, public CloserI
		, public WriterI
	{
	public:
		///
		ProcessWriter();

		///
		~ProcessWriter();

		///
		virtual bool open(const FilePath& _filePath, const StringView& _args, Error* _err) override;

		///
		virtual void close() override;

		///
		virtual int32_t write(const void* _data, int32_t _size, Error* _err) override;

		///
		int32_t getExitCode() const;

	private:
		void* m_file;
		int32_t m_exitCode;
	};

} // namespace bx

#endif // BX_CRTIMPL_H_HEADER_GUARD
