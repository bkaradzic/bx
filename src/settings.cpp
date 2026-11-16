/*
 * Copyright 2011-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include <bx/settings.h>
#include <bx/array.h>
#include <bx/filepath.h>
#include <bx/scanner.h>
#include <bx/stringpool.h>
#include <bx/uniqueptr.h>

namespace bx
{
	struct Settings::Ini
	{
		static constexpr int32_t kGlobal  =  0;
		static constexpr int32_t kInvalid = -1;

		struct Property
		{
			BX_CLASS(Property, TRIVIALLY_RELOCATABLE);

			String name;
			String value;
		};

		struct Section
		{
			BX_CLASS(Section, TRIVIALLY_RELOCATABLE);

			String           name;
			ArrayT<Property> props;
		};

		Ini(AllocatorHandle _allocator)
			: m_sections(_allocator)
		{
			clear();
		}

		AllocatorHandle getAllocatorHandle() const
		{
			return m_sections.getAllocatorHandle();
		}

		void clear()
		{
			m_sections.clear();

			addSection(StringView() );
		}

		void load(const StringView& _data)
		{
			clear();

			Scanner scanner(_data);
			int32_t section = kGlobal;

			while (!scanner.isDone() )
			{
				scanner.accept(Scanner::Class::Space);

				if (scanner.isDone() )
				{
					break;
				}

				Scanner line(scanner.acceptUntil(Scanner::Class::EndOfLine) );

				// Line comment.
				if (!line.accept(';').isEmpty() )
				{
					continue;
				}

				// [section] header.
				if (!line.accept('[').isEmpty() )
				{
					line.accept(Scanner::Class::Space);
					const StringView name = strRTrimSpace(line.acceptUntil("]") );

					if (!line.accept(']').isEmpty()
					&&  !name.isEmpty() )
					{
						const int32_t existing = findSection(name);
						section = kInvalid == existing ? addSection(name) : existing;
					}

					continue;
				}

				// name = value property.
				const StringView name = strRTrimSpace(line.acceptUntil("=") );

				if (line.accept('=').isEmpty()
				||  name.isEmpty() )
				{
					continue;
				}

				line.accept(Scanner::Class::Space);
				setProperty(section, name, strRTrimSpace(line.acceptAll() ) );
			}
		}

		int32_t find(const StringView& _name, StringView& _propertyName) const
		{
			const FilePath uri(_name);
			const StringView path(strTrim(uri.getPath(), "/") );

			const StringView slash = strRFind(_name, '/');

			_propertyName = slash.isEmpty()
				? _name
				: StringView(slash.getPtr()+1, _name.getTerm() )
				;

			if (path.isEmpty() )
			{
				return kGlobal;
			}

			const int32_t section = findSection(path);

			return kInvalid == section ? kGlobal : section;
		}

		int32_t findSection(const StringView& _name) const
		{
			for (uint32_t ss = 0, num = m_sections.getCount(); ss < num; ++ss)
			{
				const StringView name = m_sections[ss].name;

				if (isEqual(name, _name, false) )
				{
					return int32_t(ss);
				}
			}

			return kInvalid;
		}

		int32_t addSection(const StringView& _name)
		{
			m_sections.add(Section{ String(_name), ArrayT<Property>(getAllocatorHandle() ) });

			return int32_t(m_sections.getCount()-1);
		}

		void removeSection(int32_t _section)
		{
			if (kGlobal >= _section
			||  uint32_t(_section) >= m_sections.getCount() )
			{
				return;
			}

			m_sections.remove(uint32_t(_section), kOrdered);
		}

		int32_t findProperty(int32_t _section, const StringView& _name) const
		{
			if (0 > _section
			||  uint32_t(_section) >= m_sections.getCount() )
			{
				return kInvalid;
			}

			const Section& section = m_sections[_section];

			for (uint32_t pp = 0, num = section.props.getCount(); pp < num; ++pp)
			{
				const StringView name = section.props[pp].name;

				if (isEqual(name, _name, false) )
				{
					return int32_t(pp);
				}
			}

			return kInvalid;
		}

		void setProperty(int32_t _section, const StringView& _name, const StringView& _value)
		{
			if (0 > _section
			||  uint32_t(_section) >= m_sections.getCount() )
			{
				return;
			}

			const int32_t property = findProperty(_section, _name);

			Section& section = m_sections[_section];

			if (kInvalid != property)
			{
				section.props[property].value = String(_value);
				return;
			}

			section.props.add(Property{ String(_name), String(_value) });
		}

		void removeProperty(int32_t _section, int32_t _property)
		{
			if (0 > _section
			||  uint32_t(_section) >= m_sections.getCount() )
			{
				return;
			}

			Section& section = m_sections[_section];

			if (0 > _property
			||  uint32_t(_property) >= section.props.getCount() )
			{
				return;
			}

			section.props.remove(uint32_t(_property), kOrdered);
		}

		StringView getPropertyValue(int32_t _section, int32_t _property) const
		{
			if (0 > _section
			||  uint32_t(_section) >= m_sections.getCount() )
			{
				return StringView();
			}

			const Section& section = m_sections[_section];

			return (0 <= _property && uint32_t(_property) < section.props.getCount() )
				? StringView(section.props[_property].value)
				: StringView()
				;
		}

		uint32_t getPropertyCount(int32_t _section) const
		{
			return (0 <= _section && uint32_t(_section) < m_sections.getCount() )
				? m_sections[_section].props.getCount()
				: 0
				;
		}

		int32_t save(WriterI* _writer, Error* _err) const
		{
			int32_t total = 0;

			for (uint32_t ss = 0, numSections = m_sections.getCount(); ss < numSections; ++ss)
			{
				const Section& section = m_sections[ss];

				if (!section.name.isEmpty() )
				{
					total += bx::write(_writer, "[", _err);
					total += bx::write(_writer, StringView(section.name), _err);
					total += bx::write(_writer, "]\n", _err);
				}

				for (uint32_t pp = 0, numProps = section.props.getCount(); pp < numProps; ++pp)
				{
					total += bx::write(_writer, StringView(section.props[pp].name), _err);
					total += bx::write(_writer, "=", _err);
					total += bx::write(_writer, StringView(section.props[pp].value), _err);
					total += bx::write(_writer, "\n", _err);
				}

				if (0 != total)
				{
					total += bx::write(_writer, "\n", _err);
				}
			}

			return total;
		}

		ArrayT<Section> m_sections;
	};

	Settings::Settings(AllocatorHandle _allocator, const void* _data, uint32_t _len)
		: m_ini(BX_NEW_PTR(getAllocator(_allocator), Ini)(_allocator) )
	{
		load(_data, _len);
	}

	Settings::Settings(AllocatorI* _allocator, const void* _data, uint32_t _len)
		: Settings(internal::pointerAllocatorAcquire(_allocator), _data, _len)
	{
	}

	Settings::~Settings()
	{
		const AllocatorHandle allocator = m_ini->getAllocatorHandle();
		deletePtr(m_ini);

		internal::pointerAllocatorRelease(allocator);
	}

	void Settings::clear()
	{
		m_ini->clear();
	}

	void Settings::load(const void* _data, uint32_t _len)
	{
		if (NULL == _data
		||  0    == _len)
		{
			m_ini->clear();
			return;
		}

		m_ini->load(StringView( (const char*)_data, int32_t(_len) ) );
	}

	StringView Settings::get(const StringView& _name) const
	{
		StringView propertyName;
		const int32_t section = m_ini->find(_name, propertyName);

		return m_ini->getPropertyValue(section, m_ini->findProperty(section, propertyName) );
	}

	void Settings::set(const StringView& _name, const StringView& _value)
	{
		const FilePath uri(_name);
		const StringView  path(strTrim(uri.getPath(), "/") );
		const StringView& fileName(uri.getFileName() );

		int32_t section = Ini::kGlobal;

		if (!path.isEmpty() )
		{
			section = m_ini->findSection(path);

			if (Ini::kInvalid == section)
			{
				section = m_ini->addSection(path);
			}
		}

		m_ini->setProperty(section, fileName, _value);
	}

	void Settings::remove(const StringView& _name) const
	{
		StringView propertyName;
		const int32_t section = m_ini->find(_name, propertyName);

		const int32_t property = m_ini->findProperty(section, propertyName);

		if (Ini::kInvalid == property)
		{
			return;
		}

		m_ini->removeProperty(section, property);

		if (Ini::kGlobal != section
		&&  0 == m_ini->getPropertyCount(section) )
		{
			m_ini->removeSection(section);
		}
	}

	int32_t Settings::read(ReaderSeekerI* _reader, Error* _err)
	{
		const int32_t size = int32_t(getRemain(_reader) );

		if (0 >= size)
		{
			m_ini->clear();
			return 0;
		}

		UniquePtrT<uint8_t[]> data = BX_MAKE_UNIQUE(m_ini->getAllocatorHandle(), uint8_t[], uint32_t(size) );

		const int32_t total = bx::read(_reader, data.getPtr(), size, _err);
		load(data.getPtr(), size);

		return total;
	}

	int32_t Settings::write(WriterI* _writer, Error* _err) const
	{
		return m_ini->save(_writer, _err);
	}

	int32_t read(ReaderSeekerI* _reader, Settings& _settings, Error* _err)
	{
		BX_ERROR_SCOPE(_err);
		return _settings.read(_reader, _err);
	}

	int32_t write(WriterI* _writer, const Settings& _settings, Error* _err)
	{
		BX_ERROR_SCOPE(_err);
		return _settings.write(_writer, _err);
	}

} // namespace bx
