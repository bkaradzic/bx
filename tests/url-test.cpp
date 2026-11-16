/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"
#include <bx/filepath.h>
#include <bx/string.h>
#include <bx/url.h>

struct UrlTest
{
	bool result;
	const char* url;
	const char* tokens[bx::UrlView::Count];
};

static const UrlTest s_urlTest[] =
{
	{ true
	, "scheme://username:password@host.rs:80/this/is/path/index.php?query=\"value\"#fragment",
	{ "scheme", "username", "password", "host.rs", "80", "/this/is/path/index.php", "query=\"value\"", "fragment" }
	},
	{ true
	, "scheme://host.rs/",
	{ "scheme", "", "", "host.rs", "", "/", "", "" },
	},
	{ true
	, "scheme://host.rs:1389/",
	{ "scheme", "", "", "host.rs", "1389", "/", "", "" },
	},
	{ true
	, "host.rs/abvgd.html",
	{ "", "", "", "host.rs", "", "/abvgd.html", "", "" },
	},
	{ true
	, "https://192.168.0.1:8080/",
	{ "https", "", "", "192.168.0.1", "8080", "/", "", "" },
	},

	{ true
	, "file:///d:/tmp/archive.tar.gz",
	{ "file", "", "", "", "", "/d:/tmp/archive.tar.gz", "", "" },
	},

	{ true
	, "scheme://host.rs/path#fragment",
	{ "scheme", "", "", "host.rs", "", "/path", "", "fragment" },
	},
	{ true
	, "scheme://host.rs/path?query#fragment",
	{ "scheme", "", "", "host.rs", "", "/path", "query", "fragment" },
	},
	{ true
	, "scheme://host.rs/path?",
	{ "scheme", "", "", "host.rs", "", "/path", "", "" },
	},
	{ true
	, "scheme://username:password@host.rs/",
	{ "scheme", "username", "password", "host.rs", "", "/", "", "" },
	},
	{ true
	, "scheme://host.rs",
	{ "scheme", "", "", "host.rs", "", "", "", "" },
	},
	{ true
	, "scheme://host.rs/path#fragment?query",
	{ "scheme", "", "", "host.rs", "", "/path", "", "fragment?query" },
	},
	{ true
	, "http://[::1]:8080/index.html",
	{ "http", "", "", "[::1]", "8080", "/index.html", "", "" },
	},
	{ true
	, "http://[fe80::1]/",
	{ "http", "", "", "[fe80::1]", "", "/", "", "" },
	},
	{ true
	, "http://example.com?x=1",
	{ "http", "", "", "example.com", "", "", "x=1", "" },
	},
	{ true
	, "http://example.com#top",
	{ "http", "", "", "example.com", "", "", "", "top" },
	},
	{ true
	, "/path?redirect=http://other/",
	{ "", "", "", "", "", "/path", "redirect=http://other/", "" },
	},
	{ true
	, "host.rs/a?u=http://x/",
	{ "", "", "", "host.rs", "", "/a", "u=http://x/", "" },
	},
	{ true
	, "svn+ssh://host/repo",
	{ "svn+ssh", "", "", "host", "", "/repo", "", "" },
	},
	{ true
	, "ms-settings://display",
	{ "ms-settings", "", "", "display", "", "", "", "" },
	},
	{ true
	, "h2c://host/",
	{ "h2c", "", "", "host", "", "/", "", "" },
	},
	{ true
	, "file:/tmp/a.txt",
	{ "file", "", "", "", "", "/tmp/a.txt", "", "" },
	},
	{ true
	, "mailto:a@b.com",
	{ "mailto", "", "", "", "", "a@b.com", "", "" },
	},
	{ true
	, "urn:isbn:0451450523",
	{ "urn", "", "", "", "", "isbn:0451450523", "", "" },
	},
	{ true
	, "http://user:p@ss@host/",
	{ "http", "user", "p@ss", "host", "", "/", "", "" },
	},
	{ true
	, "http://host:/",
	{ "http", "", "", "host", "", "/", "", "" },
	},
	{ true
	, "host.rs:80/path",
	{ "", "", "", "host.rs", "80", "/path", "", "" },
	},
	{ false // Port must be digits.
	, "http://host:abc/",
	{ "", "", "", "", "", "", "", "" },
	},
	{ false // IPv6 address without end.
	, "http://[::1/",
	{ "", "", "", "", "", "", "", "" },
	},
	{ false // Something between IPv6 address and port.
	, "http://[::1]x:80/",
	{ "", "", "", "", "", "", "", "" },
	},
	{ false // Scheme is empty.
	, "://host/",
	{ "", "", "", "", "", "", "", "" },
	},
	{ false // Neither scheme nor path.
	, "host.rs",
	{ "", "", "", "", "", "", "", "" },
	},
	{ false // Scheme must be alpha.
	, "1scheme://host.rs/",
	{ "", "", "", "", "", "", "", "" },
	},
};

TEST_CASE("tokenizeUrl", "[url][string]")
{
	bx::UrlView url;

	for (uint32_t ii = 0; ii < BX_COUNTOF(s_urlTest); ++ii)
	{
		const UrlTest& urlTest = s_urlTest[ii];

		bool result = url.parse(urlTest.url);
		REQUIRE(urlTest.result == result);

		if (result)
		{
			for (uint32_t token = 0; token < bx::UrlView::Count; ++token)
			{
//				char tmp[1024];
//				strCopy(tmp, BX_COUNTOF(tmp), url.get(bx::UrlView::Enum(token)) );
//				printf("`%s`, expected: `%s`\n", tmp, urlTest.tokens[token]);

				REQUIRE(0 == bx::strCmp(urlTest.tokens[token], url.get(bx::UrlView::Enum(token)) ) );
			}
		}
	}
}

struct UrlEncodeTest
{
	const char* input;
	bx::UrlEncoding::Enum encoding;
	const char* expected;
};

static const UrlEncodeTest s_urlEncodeTest[] =
{
	{ "",              bx::UrlEncoding::Component, ""                    },
	{ "abcABC123",     bx::UrlEncoding::Component, "abcABC123"           },
	{ "-_.~",          bx::UrlEncoding::Component, "-_.~"                },
	{ "a b",           bx::UrlEncoding::Component, "a%20b"               },
	{ "100%",          bx::UrlEncoding::Component, "100%25"              },
	{ "a+b=c&d",       bx::UrlEncoding::Component, "a%2Bb%3Dc%26d"       },
	{ "\xc3\xa9",      bx::UrlEncoding::Component, "%C3%A9"              },
	{ "/tmp/a b.txt",  bx::UrlEncoding::Component, "%2Ftmp%2Fa%20b.txt"  },
	{ "/tmp/a b.txt",  bx::UrlEncoding::Path,      "/tmp/a%20b.txt"      },
	{ "\xc3\xa9",      bx::UrlEncoding::Path,      "%C3%A9"              },
};

TEST_CASE("urlEncode", "[url][string]")
{
	for (uint32_t ii = 0; ii < BX_COUNTOF(s_urlEncodeTest); ++ii)
	{
		const UrlEncodeTest& test = s_urlEncodeTest[ii];

		char tmp[128];
		const int32_t len = bx::urlEncode(tmp, BX_COUNTOF(tmp), test.input, test.encoding);

		REQUIRE(0 == bx::strCmp(test.expected, tmp) );
		REQUIRE(len == bx::strLen(test.expected) );
	}
}

TEST_CASE("urlEncode truncate", "[url][string]")
{
	{
		char tmp[4];
		REQUIRE(3 == bx::urlEncode(tmp, BX_COUNTOF(tmp), "abcdef") );
		REQUIRE(0 == bx::strCmp("abc", tmp) );
	}

	{
		char tmp[4];
		REQUIRE(3 == bx::urlEncode(tmp, BX_COUNTOF(tmp), "  ") );
		REQUIRE(0 == bx::strCmp("%20", tmp) );
	}

	{
		char tmp[3];
		REQUIRE(0 == bx::urlEncode(tmp, BX_COUNTOF(tmp), " ") );
		REQUIRE(0 == bx::strCmp("", tmp) );
	}

	{
		char tmp[1];
		REQUIRE(0 == bx::urlEncode(tmp, BX_COUNTOF(tmp), "abc") );
		REQUIRE(0 == bx::strCmp("", tmp) );
	}
}

TEST_CASE("urlEncode StringView", "[url][string]")
{
	const bx::StringView str("abc def", 3);

	char tmp[128];
	REQUIRE(3 == bx::urlEncode(tmp, BX_COUNTOF(tmp), str) );
	REQUIRE(0 == bx::strCmp("abc", tmp) );
}

TEST_CASE("urlDecode", "[url][string]")
{
	struct Test
	{
		const char* input;
		const char* expected;
	};

	static const Test s_tests[] =
	{
		{ "",                 ""               },
		{ "abc",              "abc"            },
		{ "a%20b",            "a b"            },
		{ "100%25",           "100%"           },
		{ "%C3%A9",           "\xc3\xa9"       },
		{ "%c3%a9",           "\xc3\xa9"       },
		{ "a+b",              "a+b"            },
		{ "100%",             "100%"           },
		{ "%2",               "%2"             },
		{ "%zz",              "%zz"            },
		{ "%2z%20",           "%2z "           },
	};

	for (uint32_t ii = 0; ii < BX_COUNTOF(s_tests); ++ii)
	{
		char tmp[64];
		const int32_t len = bx::urlDecode(tmp, BX_COUNTOF(tmp), s_tests[ii].input);

		REQUIRE(0 == bx::strCmp(s_tests[ii].expected, tmp) );
		REQUIRE(len == bx::strLen(s_tests[ii].expected) );
	}

	char inplace[] = "a%20b%2Fc";
	REQUIRE(5 == bx::urlDecode(inplace, BX_COUNTOF(inplace), inplace) );
	REQUIRE(0 == bx::strCmp("a b/c", inplace) );

	char small[4];
	REQUIRE(3 == bx::urlDecode(small, BX_COUNTOF(small), "a%20bcd") );
	REQUIRE(0 == bx::strCmp("a b", small) );

	REQUIRE(0 == bx::urlDecode(small, 0, "abc") );

	const bx::StringView view("%41%42", 3);
	REQUIRE(1 == bx::urlDecode(small, BX_COUNTOF(small), view) );
	REQUIRE(0 == bx::strCmp("A", small) );
}

TEST_CASE("urlDecode inverts urlEncode", "[url][string]")
{
	const char input[] = "/a b/100%/\xc5\xa1?#&=+";

	char encoded[128];
	bx::urlEncode(encoded, BX_COUNTOF(encoded), input, bx::UrlEncoding::Component);

	char decoded[128];
	bx::urlDecode(decoded, BX_COUNTOF(decoded), encoded);

	REQUIRE(0 == bx::strCmp(input, decoded) );
}

TEST_CASE("urlToFilePath", "[url][string]")
{
	bx::FilePath path;

	REQUIRE(bx::urlToFilePath(path, "file:///tmp/a%20b.txt") );
	REQUIRE(0 == bx::strCmp("/tmp/a b.txt", path) );

	REQUIRE(bx::urlToFilePath(path, "FILE:///tmp/a") );
	REQUIRE(0 == bx::strCmp("/tmp/a", path) );

	REQUIRE(bx::urlToFilePath(path, "file://localhost/tmp/a") );
	REQUIRE(0 == bx::strCmp("/tmp/a", path) );

	REQUIRE(bx::urlToFilePath(path, "file:/tmp/a") );
	REQUIRE(0 == bx::strCmp("/tmp/a", path) );

	REQUIRE(bx::urlToFilePath(path, "file:///tmp/a.html#top") );
	REQUIRE(0 == bx::strCmp("/tmp/a.html", path) );

	REQUIRE(bx::urlToFilePath(path, "file:///tmp/%zz") );
	REQUIRE(0 == bx::strCmp("/tmp/%zz", path) );

	REQUIRE(!bx::urlToFilePath(path, "http://host/tmp/a") );
	REQUIRE(!bx::urlToFilePath(path, "file://remote/tmp/a") );
	REQUIRE(!bx::urlToFilePath(path, "file://") );
	REQUIRE(!bx::urlToFilePath(path, "file:relative") );
	REQUIRE(!bx::urlToFilePath(path, "file:///a%00b") );
	REQUIRE(!bx::urlToFilePath(path, "/tmp/a") );

	REQUIRE(bx::urlToFilePath(path, "file:///a/../b/./c.txt") );
	REQUIRE(0 == bx::strCmp("/b/c.txt", path) );

	char longUrl[bx::kMaxFilePath*4];
	int32_t pos = bx::strCopy(longUrl, BX_COUNTOF(longUrl), "file:///");

	for (int32_t ii = 0; ii < bx::kMaxFilePath; ++ii)
	{
		longUrl[pos++] = 'a';
	}

	longUrl[pos] = '\0';
	REQUIRE(!bx::urlToFilePath(path, longUrl) );

	pos = bx::strCopy(longUrl, BX_COUNTOF(longUrl), "file:///");

	for (int32_t ii = 0; ii < bx::kMaxFilePath/2; ++ii)
	{
		pos += bx::strCopy(&longUrl[pos], BX_COUNTOF(longUrl) - pos, "%41");
	}

	REQUIRE(bx::urlToFilePath(path, longUrl) );
	REQUIRE(bx::kMaxFilePath/2 + 1 == bx::strLen(path) );

#if BX_PLATFORM_WINDOWS
	REQUIRE(bx::urlToFilePath(path, "file:///C:/a%20b/c.txt") );
	REQUIRE(0 == bx::strCmp("C:/a b/c.txt", path) );

	REQUIRE(bx::urlToFilePath(path, "file:///C%3A/a") );
	REQUIRE(0 == bx::strCmp("C:/a", path) );
#else
	REQUIRE(bx::urlToFilePath(path, "file:///C:/a") );
	REQUIRE(0 == bx::strCmp("/C:/a", path) );
#endif // BX_PLATFORM_WINDOWS
}

TEST_CASE("urlFromFilePath", "[url][string]")
{
	char url[128];

	REQUIRE(21 == bx::urlFromFilePath(url, BX_COUNTOF(url), "/tmp/a b.txt") );
	REQUIRE(0 == bx::strCmp("file:///tmp/a%20b.txt", url) );

	REQUIRE(0 < bx::urlFromFilePath(url, BX_COUNTOF(url), "/h/\xc5\xa1/100%.png") );
	REQUIRE(0 == bx::strCmp("file:///h/%C5%A1/100%25.png", url) );

	bx::FilePath path;
	REQUIRE(bx::urlToFilePath(path, url) );
	REQUIRE(0 == bx::strCmp("/h/\xc5\xa1/100%.png", path) );

	char small[12];
	REQUIRE(0 == bx::urlFromFilePath(small, BX_COUNTOF(small), "/tmp/a b.txt") );
	REQUIRE(0 == bx::strCmp("", small) );

	char exact[17];
	REQUIRE(0 == bx::urlFromFilePath(exact, BX_COUNTOF(exact) - 1, "a b") );
	REQUIRE(0 <  bx::urlFromFilePath(exact, BX_COUNTOF(exact),     "a b") );
	REQUIRE(0 == bx::strCmp("file://a%20b", exact) );

	char longPath[bx::kMaxFilePath + 1];
	bx::memSet(longPath, 'a', sizeof(longPath) );
	longPath[0] = '/';

	char longUrl[bx::kMaxFilePath*4];
	REQUIRE(0 == bx::urlFromFilePath(longUrl, BX_COUNTOF(longUrl), bx::StringView(longPath, int32_t(sizeof(longPath) ) ) ) );
	REQUIRE(0 == bx::urlFromFilePath(longUrl, BX_COUNTOF(longUrl), bx::StringView(longPath, bx::kMaxFilePath) ) );
	REQUIRE(0 == bx::strCmp("", longUrl) );
	REQUIRE(int32_t(bx::strLen("file://") ) + bx::kMaxFilePath - 1 == bx::urlFromFilePath(longUrl, BX_COUNTOF(longUrl), bx::StringView(longPath, bx::kMaxFilePath - 1) ) );

#if BX_PLATFORM_WINDOWS
	REQUIRE(0 < bx::urlFromFilePath(url, BX_COUNTOF(url), "C:\\a b\\c.txt") );
	REQUIRE(0 == bx::strCmp("file:///C:/a%20b/c.txt", url) );

	REQUIRE(bx::urlToFilePath(path, url) );
	REQUIRE(0 == bx::strCmp("C:/a b/c.txt", path) );
#endif // BX_PLATFORM_WINDOWS
}
