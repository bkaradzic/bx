/*
 * Copyright 2011-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#ifndef BX_SETTINGS_H_HEADER_GUARD
#define BX_SETTINGS_H_HEADER_GUARD

#include "allocator.h"
#include "blk.h"
#include "readerwriter.h"
#include "string.h"

namespace bx
{
	///
	class Settings
	{
	public:
		/// Constructs settings, optionally loaded from INI text.
		///
		/// @param[in] _allocator Allocator, must be CPU accessible.
		/// @param[in] _data INI text, NULL for none.
		/// @param[in] _len Length of INI text in bytes.
		///
		Settings(AllocatorHandle _allocator, const void* _data = NULL, uint32_t _len = 0);

		///
		Settings(AllocatorI* _allocator, const void* _data = NULL, uint32_t _len = 0);

		///
		~Settings();

		///
		void clear();

		///
		void load(const void* _data, uint32_t _len);

		///
		StringView get(const StringView& _name) const;

		///
		void set(const StringView& _name, const StringView& _value = "");

		///
		void remove(const StringView& _name) const;

		///
		int32_t read(ReaderSeekerI* _reader, Error* _err);

		///
		int32_t write(WriterI* _writer, Error* _err) const;

	private:
		Settings();

		struct Ini;

		Ini* m_ini;
	};

	///
	int32_t read(ReaderSeekerI* _reader, Settings& _settings, Error* _err);

	///
	int32_t write(WriterI* _writer, const Settings& _settings, Error* _err);

} // namespace bx

#endif // BX_SETTINGS_H_HEADER_GUARD
