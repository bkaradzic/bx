/*
 * Copyright 2010-2026 Branimir Karadzic. All rights reserved.
 * License: https://github.com/bkaradzic/bx/blob/master/LICENSE
 */

#include "test.h"
#include <bx/string.h>
#include <bx/readerwriter.h>
#include <bx/rng.h>

#include <limits>
#include <inttypes.h>

TEST_CASE("No output buffer provided.", "[string][printf]")
{
	REQUIRE(4 == bx::snprintf(NULL,  0, "test") );
	REQUIRE(4 == bx::snprintf(NULL, 10, "test") );
	REQUIRE(1 == bx::snprintf(NULL,  0, "%d", 1) );
	REQUIRE(1 == bx::snprintf(NULL, 10, "%d", 1) );
}

TEST_CASE("Truncated output buffer.", "[string][printf]")
{
	REQUIRE(4 == bx::snprintf(NULL, 0, "abvg") );

	char buffer15[15]; // fit
	REQUIRE(4    == bx::snprintf(buffer15, BX_COUNTOF(buffer15), "abvg") );
	REQUIRE('\0' == buffer15[4]);
	REQUIRE(0    == bx::strCmp(buffer15, "abvg") );

	char buffer1[1]; // truncate
	REQUIRE(4    == bx::snprintf(buffer1, BX_COUNTOF(buffer1), "abvg") );
	REQUIRE('\0' == buffer1[BX_COUNTOF(buffer1)-1]);

	buffer1[0] = '\xfb'; // null destination
	REQUIRE(4      == bx::snprintf(NULL, BX_COUNTOF(buffer1), "abvg") );
	REQUIRE('\xfb' == buffer1[0]);

	buffer1[0] = '\xbf'; // one byte destination
	REQUIRE(4    == bx::snprintf(buffer1, 1, "abvg") );
	REQUIRE('\0' == buffer1[0]);

	char buffer7[7]; // truncate
	REQUIRE(10   == bx::snprintf(NULL, 0, "Ten chars!") );
	REQUIRE(10   == bx::snprintf(buffer7, BX_COUNTOF(buffer7), "Ten chars!") );
	REQUIRE('\0' == buffer7[BX_COUNTOF(buffer7)-1]);
	REQUIRE(0    == bx::strCmp(buffer7, "Ten ch") );

	REQUIRE(7    == bx::snprintf(NULL, 0, "Seven67") );
	REQUIRE(7    == bx::snprintf(buffer7, BX_COUNTOF(buffer7), "Seven67") );
	REQUIRE('\0' == buffer7[BX_COUNTOF(buffer7)-1]);
	REQUIRE(0    == bx::strCmp(buffer7, "Seven6") );

	REQUIRE(11   == bx::snprintf(NULL, 0, "SevenEleven") );
	REQUIRE(11   == bx::snprintf(buffer7, BX_COUNTOF(buffer7), "SevenEleven") );
	REQUIRE('\0' == buffer7[BX_COUNTOF(buffer7)-1]);
	REQUIRE(0    == bx::strCmp(buffer7, "SevenE") );
}

template<bool StdCompliantT>
static bool test(const char* _expected, const char* _format, va_list _argList)
{
	const int32_t expectedLen = bx::strLen(_expected);
	int32_t max = expectedLen + 1024;
	char* bxTemp = (char*)BX_STACK_ALLOC(max);

	va_list argList;
	va_copy(argList, _argList);
	const int32_t bxLen = bx::vsnprintf(bxTemp, max, _format, argList);

	bool result = true
		&& bxLen == expectedLen
		&&     0 == bx::strCmp(_expected, bxTemp)
		;

	char*  crtTemp = NULL;
	int32_t crtLen = 0;

	if (!result
	||  StdCompliantT)
	{
		BX_ASSERT(bx::strFind(_format, "%S").isEmpty()
			, "String format test is using '%%S' bx::StringView specific format specifier which is not standard compliant. "
			  "Use `testNotStdCompliant` string testing method."
			);

		crtTemp = (char*)BX_STACK_ALLOC(max);

		va_copy(argList, _argList);
		crtLen = ::vsnprintf(crtTemp, max, _format, argList);

		result &= true
			&& crtLen == bxLen
			&&      0 == bx::strCmp(bx::StringView(bxTemp, bxLen), bx::StringView(crtTemp, crtLen) )
			;
	}

	if (!result)
	{
		printf("---\n");
		printf("printf format '%s'\n", _format);
		printf("    bx result (%4d) '%s'\n", bxLen, bxTemp);
		printf("     expected (%4d) '%s'\n", expectedLen, _expected);
		printf("CRT vsnprintf (%4d) '%s'\n", crtLen, crtTemp);
	}

	return result;
}

// Test against CRT's vsnprintf implementation.
static bool test(const char* _expected, const char* _format, ...)
{
	va_list argList;
	va_start(argList, _format);
	const bool result = test<true>(_expected, _format, argList);
	va_end(argList);

	return result;
}

// Skip test against CRT's vsnprintf implementation.
static bool testNotStdCompliant(const char* _expected, const char* _format, ...)
{
	va_list argList;
	va_start(argList, _format);
	const bool result = test<false>(_expected, _format, argList);
	va_end(argList);

	return result;
}

TEST_CASE("Format %f", "[string][printf]")
{
	constexpr double kDoubleNan = bx::bitsToDouble(bx::kDoubleExponentMask | bx::kDoubleMantissaMask);

	REQUIRE(test("1.337",    "%0.3f", 1.337) );
	REQUIRE(test("  13.370", "%8.3f", 13.37) );
	REQUIRE(test("  13.370", "%*.*f", 8, 3, 13.37) );
	REQUIRE(test("13.370  ", "%-8.3f", 13.37) );
	REQUIRE(test("13.370  ", "%*.*f", -8, 3, 13.37) );
	REQUIRE(test(" -13.370", "% 8.3f", -13.37) );
	REQUIRE(test("          13.370", "% 16.3f",  13.37) );
	REQUIRE(test("         -13.370", "% 16.3f", -13.37) );

	REQUIRE(test("nan     ", "%-8f",  kDoubleNan) );
	REQUIRE(test("     nan", "%8f",   kDoubleNan) );
	REQUIRE(testNotStdCompliant("-NAN    ", "%-8F", -kDoubleNan) );

	REQUIRE(test("     inf", "%8f",   bx::kDoubleInfinity) );
	REQUIRE(test("inf     ", "%-8f",  bx::kDoubleInfinity) );
	REQUIRE(test("    -INF", "%8F",  -bx::kDoubleInfinity) );

	REQUIRE(test(" 1.0",     "%4.1f",    1.0) );
	REQUIRE(test(" 1.500",   "%6.3f",    1.5) );
	REQUIRE(test("0001.500", "%08.3f",   1.5) );
	REQUIRE(test("+001.500", "%+08.3f",  1.5) );
	REQUIRE(test("-001.500", "%+08.3f", -1.5) );
	REQUIRE(test("0.0039",   "%.4f",     0.00390625) );

	REQUIRE(test("0.003906",     "%f",   0.00390625) );
	REQUIRE(test("-0.000000",    "%f",  -1.234567e-9) );

	REQUIRE(test("-0",               "%.0f",  -1.234567e-9) );
	REQUIRE(test("-0.0",             "%.1f",  -1.234567e-9) );
	REQUIRE(test("-0.00",            "%.2f",  -1.234567e-9) );
	REQUIRE(test("-0.000",           "%.3f",  -1.234567e-9) );
	REQUIRE(test("-0.0000",          "%.4f",  -1.234567e-9) );
	REQUIRE(test("-0.00000",         "%.5f",  -1.234567e-9) );
	REQUIRE(test("-0.000000",        "%.6f",  -1.234567e-9) );
	REQUIRE(test("-0.0000000",       "%.7f",  -1.234567e-9) );
	REQUIRE(test("-0.00000000",      "%.8f",  -1.234567e-9) );
	REQUIRE(test("-0.000000001",     "%.9f",  -1.234567e-9) );
	REQUIRE(test("-0.0000000012",    "%.10f", -1.234567e-9) );

	REQUIRE(test("3.141593",           "%f",    3.1415926535897932) );
	REQUIRE(test("3.141593",           "%F",    3.1415926535897932) );
	REQUIRE(test("3",                  "%.0f",  3.1415926535897932) );
	REQUIRE(test("3.1",                "%.1f",  3.1415926535897932) );
	REQUIRE(test("3.14",               "%.2f",  3.1415926535897932) );
	REQUIRE(test("3.142",              "%.3f",  3.1415926535897932) );
	REQUIRE(test("3.1416",             "%.4f",  3.1415926535897932) );
	REQUIRE(test("3.14159",            "%.5f",  3.1415926535897932) );
	REQUIRE(test("3.141593",           "%.6f",  3.1415926535897932) );
	REQUIRE(test("3.1415927",          "%.7f",  3.1415926535897932) );
	REQUIRE(test("3.14159265",         "%.8f",  3.1415926535897932) );
	REQUIRE(test("3.141592654",        "%.9f",  3.1415926535897932) );
	REQUIRE(test("3.1415926536",       "%.10f", 3.1415926535897932) );
	REQUIRE(test("3.14159265359",      "%.11f", 3.1415926535897932) );
	REQUIRE(test("3.141592653590",     "%.12f", 3.1415926535897932) );
	REQUIRE(test("3.1415926535898",    "%.13f", 3.1415926535897932) );
	REQUIRE(test("3.14159265358979",   "%.14f", 3.1415926535897932) );
	REQUIRE(test("3.141592653589793",  "%.15f", 3.1415926535897932) );
	REQUIRE(test("3.1415926535897931", "%.16f", 3.1415926535897932) );
	REQUIRE(test("3.1415926535897931", "%.16F", 3.1415926535897932) );

	REQUIRE(test("-0.000000",              "%f",    -3.1415926535897932e-9) );
	REQUIRE(test("-0.000000",              "%F",    -3.1415926535897932e-9) );
	REQUIRE(test("-0",                     "%.0f",  -3.1415926535897932e-9) );
	REQUIRE(test("-0.0",                   "%.1f",  -3.1415926535897932e-9) );
	REQUIRE(test("-0.00",                  "%.2f",  -3.1415926535897932e-9) );
	REQUIRE(test("-0.000",                 "%.3f",  -3.1415926535897932e-9) );
	REQUIRE(test("-0.0000",                "%.4f",  -3.1415926535897932e-9) );
	REQUIRE(test("-0.00000",               "%.5f",  -3.1415926535897932e-9) );
	REQUIRE(test("-0.000000",              "%.6f",  -3.1415926535897932e-9) );
	REQUIRE(test("-0.0000000",             "%.7f",  -3.1415926535897932e-9) );
	REQUIRE(test("-0.00000000",            "%.8f",  -3.1415926535897932e-9) );
	REQUIRE(test("-0.000000003",           "%.9f",  -3.1415926535897932e-9) );
	REQUIRE(test("-0.0000000031",          "%.10f", -3.1415926535897932e-9) );
	REQUIRE(test("-0.00000000314",         "%.11f", -3.1415926535897932e-9) );
	REQUIRE(test("-0.000000003142",        "%.12f", -3.1415926535897932e-9) );
	REQUIRE(test("-0.0000000031416",       "%.13f", -3.1415926535897932e-9) );
	REQUIRE(test("-0.00000000314159",      "%.14f", -3.1415926535897932e-9) );
	REQUIRE(test("-0.000000003141593",     "%.15f", -3.1415926535897932e-9) );
	REQUIRE(test("-0.0000000031415927",    "%.16f", -3.1415926535897932e-9) );
	REQUIRE(test("-0.0000000031415927",    "%.16F", -3.1415926535897932e-9) );

	REQUIRE(test("0.000000", "%f", 1e-12));

	REQUIRE(test("0.00390625",          "%.8f",  0.00390625) );
	REQUIRE(test("-0.00390625",         "%.8f", -0.00390625) );
	REQUIRE(test("1.50000000000000000", "%.17f", 1.5) );

	REQUIRE(test("-3.000000",      "%f",      -3.0) );
	REQUIRE(test("-0.000000",      "%f",      -0.) );
	REQUIRE(test("4.1",            "%.1f",    4.1) );
	REQUIRE(test("0.00",           "%.2f",    1e-4) );
	REQUIRE(test("-5.20",          "%+4.2f", -5.2) );

	volatile double zero = 0.0; // /fp:fast allows the compiler to disregard the sign of zero.
	REQUIRE(test("0.0       ",     "%-10.1f", zero) );
	REQUIRE(test("-8.8888888800",  "%.10f",  -8.88888888) );
	REQUIRE(test("880.0888888800", "%.10f",   880.08888888) );
	REQUIRE(test("100056789.0",    "%.1f",    100056789.0) );
	REQUIRE(test(" 0",             "% .0f",   0.1) );
}

TEST_CASE("Format %d, %i, %o, %u, %x", "[string][printf]")
{
	REQUIRE(test("1337", "%d", 1337) );
	REQUIRE(test("-1337", "% d", -1337) );
	REQUIRE(test("1337                ", "%-20d",  1337) );
	REQUIRE(test("-1337               ", "%-20d", -1337) );
	REQUIRE(test("               -1337", "% 20d", -1337) );

	REQUIRE(test("1337", "%i", 1337) );
	REQUIRE(test("1337                ", "%-20i",  1337) );
	REQUIRE(test("-1337               ", "%-20i", -1337) );

	REQUIRE(test("1337", "%o", 01337) );
	REQUIRE(test("2471", "%o", 1337) );
	REQUIRE(test("1337                ", "%-20o",  01337) );
	REQUIRE(test("37777776441         ", "%-20o", -01337) );
	REQUIRE(test("                2471", "%20o",    1337) );
	REQUIRE(test("00000000000000002471", "%020o",   1337) );

	REQUIRE(test("1337", "%u", 1337) );
	REQUIRE(test("1337                ", "%-20u",  1337) );
	REQUIRE(test("4294965959          ", "%-20u", -1337) );

	REQUIRE(test("1337", "%x", 0x1337) );
	REQUIRE(test("1234abcd            ", "%-20x",  0x1234abcd) );
	REQUIRE(test("1234ABCD            ", "%-20X",  0x1234abcd) );
	REQUIRE(test("edcb5433            ", "%-20x", -0x1234abcd) );
	REQUIRE(test("EDCB5433            ", "%-20X", -0x1234abcd) );
	REQUIRE(test("            1234abcd", "% 20x",  0x1234abcd) );
	REQUIRE(test("            1234ABCD", "% 20X",  0x1234abcd) );
	REQUIRE(test("            edcb5433", "% 20x", -0x1234abcd) );
	REQUIRE(test("            EDCB5433", "% 20X", -0x1234abcd) );
	REQUIRE(test("0000000000001234abcd", "%020x",  0x1234abcd) );
	REQUIRE(test("0000000000001234ABCD", "%020X",  0x1234abcd) );
	REQUIRE(test("000000000000edcb5433", "%020x", -0x1234abcd) );
	REQUIRE(test("000000000000EDCB5433", "%020X", -0x1234abcd) );

	REQUIRE(test("0xffffffff", "0x%01x", -1) );
	REQUIRE(test("0xffffffff", "0x%08x", -1) );

	REQUIRE(test("  -1", "% 4i", -1) );
	REQUIRE(test("   0", "% 4i",  0) );
	REQUIRE(test("   1", "% 4i",  1) );
	REQUIRE(test("   1", "% 4o",  1) );
	REQUIRE(test("  +1", "%+4i",  1) );
	REQUIRE(testNotStdCompliant("  +1", "%+4o",  1) );
	REQUIRE(test("  +0", "%+4i",  0) );
	REQUIRE(test("  -1", "%+4i", -1) );
	REQUIRE(test("0001", "%04i",  1) );
	REQUIRE(test("0001", "%04o",  1) );
	REQUIRE(test("0000", "%04i",  0) );
	REQUIRE(test("0000", "%04o",  0) );
	REQUIRE(test("-001", "%04i", -1) );
	REQUIRE(test("+001", "%+04i", 1) );

	if constexpr (sizeof(intmax_t) == 4)
	{
		REQUIRE(test("2147483647", "%jd", INTMAX_MAX) );
	}
	else
	{
		REQUIRE(test("9223372036854775807", "%jd", INTMAX_MAX) );
	}

	REQUIRE(test("18446744073709551615", "%" PRIu64, UINT64_MAX) );
	REQUIRE(test("ffffffffffffffff", "%016" PRIx64, UINT64_MAX) );

	REQUIRE(test("33 555",  "%hi %ld", (short)33, 555l) );
	REQUIRE(test("20 0020", "%u %04u", 20u, 20u) );

	REQUIRE(test("012 0x1e 0X3C", "%#o %#x %#X", 10u, 30u, 60u) );
	REQUIRE(test(" 12 1e 3C ",    "%3o %2x %-3X", 10u, 30u, 60u) );
	REQUIRE(test("12 1e 3C",      "%o %x %X", 10u, 30u, 60u) );
}

TEST_CASE("Format modifiers", "[string][printf]")
{
	REQUIRE(test("|  1.000000|", "|%10f|",      1.0f) );
	REQUIRE(test("|1.000000  |", "|%-10f|",     1.0f) );
	REQUIRE(test("|001.000000|", "|%010f|",     1.0f) );
	REQUIRE(test("|0000000001|", "|%010.0f|",   1.0f) );
	REQUIRE(test("|000000001.|", "|%#010.0f|",  1.0f) );
	REQUIRE(test("|         1|", "|%10.0f|",    1.0f) );
	REQUIRE(test("|        1.|", "|%#10.0f|",   1.0f) );
	REQUIRE(test("|       +1.|", "|%#+10.0f|",  1.0f) );
	REQUIRE(test("|1         |", "|%-10.0f|",   1.0f) );
	REQUIRE(test("|1.        |", "|%#-10.0f|",  1.0f) );
	REQUIRE(test("|+1.       |", "|%+#-10.0f|", 1.0f) );

	REQUIRE(test("|     00013:    -00089|", "|%10.5d:%10.5d|",   13, -89) );
	REQUIRE(test("|    -00013:    +00089|", "|%10.5d:%+10.5d|", -13,  89) );
	REQUIRE(test("|    -00013:    -00089|", "|%10.5d:%10.5d|",  -13, -89) );
}

TEST_CASE("Format %p", "[string][printf]")
{
	REQUIRE(testNotStdCompliant("0xbadc0de", "%p", (void*)0xbadc0de) );
	REQUIRE(testNotStdCompliant("0xbadc0de           ", "%-20p", (void*)0xbadc0de) );
}

TEST_CASE("Format %s", "[string][printf]")
{
	REQUIRE(test("(null)", "%s", NULL) );
}

TEST_CASE("Format %td", "[string][printf]")
{
	ptrdiff_t size = ptrdiff_t(-1);

	REQUIRE(test("-1", "%td", size) );

#if BX_ARCH_32BIT
	REQUIRE(test("-1073741824", "%td", ptrdiff_t(-1073741824) ) );
#else
	REQUIRE(test("3221225472", "%td", ptrdiff_t(3221225472) ) );
#endif // BX_ARCH_32BIT
}

TEST_CASE("Format %n", "[string][printf]")
{
	char temp[64];

	int32_t p0, p1, p2;
	bx::snprintf(temp, sizeof(temp), "%n", &p0);
	REQUIRE(0 == p0);

	bx::snprintf(temp, sizeof(temp), "01%n23%n45%n", &p0, &p1, &p2);
	REQUIRE(2 == p0);
	REQUIRE(4 == p1);
	REQUIRE(6 == p2);
}

TEST_CASE("Format %g", "[string][printf]")
{
	REQUIRE(test("   0.01",  "%7.2g", .01) );
	REQUIRE(test(" 0.0123",  "%7.4G", .0123) );
	REQUIRE(test("1.23e+05", "%.3g",  123000.25) );
	REQUIRE(test("1e+05",    "%.0g",  123000.25) );
	REQUIRE(test("1",       "%.0g", 1.2) );

	REQUIRE(test("100000",              "%g",    100000.0) );
	REQUIRE(test("1e+06",               "%g",    1000000.0) );
	REQUIRE(test("0.0001",              "%g",    0.0001) );
	REQUIRE(test("1e-05",               "%g",    0.00001) );
	REQUIRE(test("1.00000",             "%#g",   1.0) );
	REQUIRE(test("0",                   "%g",    0.0) );
	REQUIRE(test("3.14159",             "%g",    3.1415926535897932) );
	REQUIRE(test("0.10000000000000001", "%.17g", 0.1) );
	REQUIRE(test("1E-10",               "%G",    1e-10) );
}

TEST_CASE("Format %e", "[string][printf]")
{
	REQUIRE(test("1.337000e+00",  "%e",    1.337) );
	REQUIRE(test("-1.234567e-09", "%e",   -1.234567e-9) );
	REQUIRE(test("1.500000E+00",  "%E",    1.5) );
	REQUIRE(test("0.000000e+00",  "%e",    0.0) );
	REQUIRE(test("1.798e+308",    "%.3e",  1.7976931348623157e308) );
	REQUIRE(test("4.941e-324",    "%.3e",  4.9406564584124654e-324) );
	REQUIRE(test("1e+01",         "%.0e",  9.5) );
	REQUIRE(test("3.e+00",        "%#.0e", 3.0) );
	REQUIRE(test("   1.234e+03",  "%12.3e", 1234.5) );
}

TEST_CASE("Format %e, %f, %g of random doubles match the CRT", "[string][printf]")
{
	static const char* s_formats[] =
	{
		"%e", "%f", "%g", "%.0e", "%.3e", "%.17e", "%.40e", "%.0f", "%.3f", "%.20f",
		"%.1g", "%.17g", "%#g", "%+.5e", "% 12.4g", "%-14.6e", "%010.3f", "%#.0f",
	};

	bx::RngMwc rng;

	for (uint32_t ii = 0; ii < 20000; ++ii)
	{
		const uint64_t hi   = rng.gen();
		const uint64_t lo   = rng.gen();
		const double value  = bx::bitsToDouble( (hi << 32) | lo);

		if (!bx::isFinite(value) )
		{
			continue;
		}

		const char* format = s_formats[ii % BX_COUNTOF(s_formats)];

		char bxTemp[1024];
		const int32_t bxLen = bx::snprintf(bxTemp, BX_COUNTOF(bxTemp), format, value);

		char crtTemp[1024];
		const int32_t crtLen = ::snprintf(crtTemp, BX_COUNTOF(crtTemp), format, value);

		INFO(format << " " << crtTemp);
		REQUIRE(bxLen == crtLen);
		REQUIRE(0 == bx::strCmp(bxTemp, crtTemp) );
	}

	for (uint32_t ii = 0; ii < 20000; ++ii)
	{
		const uint32_t odd    = rng.gen() | 1;
		const int32_t  shift  = 1 + int32_t(rng.gen() % 40);
		const double   value  = bx::ldexp(double(odd), -shift);
		const int32_t  digits = shift - 1;

		char bxTemp[1024];
		const int32_t bxLen = bx::snprintf(bxTemp, BX_COUNTOF(bxTemp), "%.*f", digits, value);

		char crtTemp[1024];
		const int32_t crtLen = ::snprintf(crtTemp, BX_COUNTOF(crtTemp), "%.*f", digits, value);

		INFO(digits << " " << crtTemp);
		REQUIRE(bxLen == crtLen);
		REQUIRE(0 == bx::strCmp(bxTemp, crtTemp) );
	}
}

TEST_CASE("Format %c, %s, %S", "[string][printf]")
{
	REQUIRE(test("hello               ", "%-20s", "hello") );
	REQUIRE(test("     hello", "%10s", "hello") );
	REQUIRE(test("hello, world!", "%s, %s!", "hello", "world") );

	bx::StringView str("0hello1world2");
	bx::StringView hello(str, 1, 5);
	bx::StringView world(str, 7, 5);
	REQUIRE(test("hello, world!", "%.*s, %.*s!"
		, hello.getLength(), hello.getPtr()
		, world.getLength(), world.getPtr()
		) );

	REQUIRE(testNotStdCompliant("hello, world!", "%S, %S!"
		, &hello
		, &world
		) );
}

TEST_CASE("WriterI", "[string][printf]")
{
	char tmp[64];
	bx::StaticMemoryBlock mb(tmp, sizeof(tmp));
	bx::MemoryWriter writer(&mb);

	bx::Error err;
	int32_t len = bx::write(&writer, &err, "%d", 1389);
	REQUIRE(err.isOk());
	REQUIRE(len == 4);

	bx::StringView str(tmp, len);
	REQUIRE(0 == bx::strCmp(str, "1389") );
}

TEST_CASE("Invalid", "[string][printf]")
{
	char temp[64];
	REQUIRE(0 == bx::snprintf(temp, sizeof(temp), "%", 1) );
	REQUIRE(0 == bx::snprintf(temp, sizeof(temp), "%-", 1) );
	REQUIRE(0 == bx::snprintf(temp, sizeof(temp), "%-0", 1) );
	REQUIRE(0 == bx::snprintf(temp, sizeof(temp), "%-03", 1) );
	REQUIRE(0 == bx::snprintf(temp, sizeof(temp), "%-03.", 1) );
	REQUIRE(0 == bx::snprintf(temp, sizeof(temp), "%-03.0", 1) );
	REQUIRE(0 == bx::snprintf(temp, sizeof(temp), "%-03.0t", 1) );
}

TEST_CASE("Format %s width does not truncate", "[string][printf]")
{
	SECTION("Width smaller than string length must NOT truncate")
	{
		REQUIRE(test("hello",      "%1s",  "hello") );
		REQUIRE(test("hello",      "%5s",  "hello") );
		REQUIRE(test(" hello",     "%6s",  "hello") );
		REQUIRE(test("  hello",    "%7s",  "hello") );
	}

	SECTION("Left-aligned, width smaller than string length")
	{
		REQUIRE(test("hello",      "%-1s", "hello") );
		REQUIRE(test("hello ",     "%-6s", "hello") );
		REQUIRE(test("hello  ",    "%-7s", "hello") );
	}
}

TEST_CASE("Format %s precision truncates", "[string][printf]")
{
	SECTION("Precision DOES truncate strings")
	{
		REQUIRE(test("",       "%.0s", "hello") );
		REQUIRE(test("h",      "%.1s", "hello") );
		REQUIRE(test("he",     "%.2s", "hello") );
		REQUIRE(test("hel",    "%.3s", "hello") );
		REQUIRE(test("hell",   "%.4s", "hello") );
		REQUIRE(test("hello",  "%.5s", "hello") );
		REQUIRE(test("hello",  "%.10s", "hello") );
	}
}

TEST_CASE("Format %s width and precision combined", "[string][printf]")
{
	SECTION("Width pads, precision truncates")
	{
		REQUIRE(test("         h", "%10.1s", "hello") );
		REQUIRE(test("        he", "%10.2s", "hello") );
		REQUIRE(test("       hel", "%10.3s", "hello") );
		REQUIRE(test("     hello", "%10.5s", "hello") );
	}

	SECTION("Left-aligned with width and precision")
	{
		REQUIRE(test("h         ", "%-10.1s", "hello") );
		REQUIRE(test("he        ", "%-10.2s", "hello") );
		REQUIRE(test("hel       ", "%-10.3s", "hello") );
		REQUIRE(test("hello     ", "%-10.5s", "hello") );
	}

	SECTION("Width smaller than precision and string")
	{
		REQUIRE(test("hel",   "%1.3s",  "hello") );
		REQUIRE(test("hel",   "%-1.3s", "hello") );
	}

	SECTION("Precision with * wildcard")
	{
		REQUIRE(test("hel",   "%.*s", 3, "hello") );
		REQUIRE(test("hello", "%.*s", 10, "hello") );
	}
}

TEST_CASE("Format %s empty string", "[string][printf]")
{
	REQUIRE(test("",           "%s",    "") );
	REQUIRE(test("          ", "%10s",  "") );
	REQUIRE(test("          ", "%-10s", "") );
	REQUIRE(test("",           "%.0s",  "hello") );
	REQUIRE(test("          ", "%10.0s", "hello") );
}

TEST_CASE("Format %d width does not truncate", "[string][printf]")
{
	SECTION("Width smaller than number of digits must NOT truncate")
	{
		REQUIRE(test("1337",   "%1d",  1337) );
		REQUIRE(test("1337",   "%4d",  1337) );
		REQUIRE(test(" 1337",  "%5d",  1337) );
		REQUIRE(test("  1337", "%6d",  1337) );

		REQUIRE(test("-1337",  "%1d",  -1337) );
		REQUIRE(test("-1337",  "%5d",  -1337) );
		REQUIRE(test(" -1337", "%6d",  -1337) );
	}
}

TEST_CASE("Format %x width does not truncate", "[string][printf]")
{
	REQUIRE(test("abcd",   "%1x",  0xabcd) );
	REQUIRE(test("abcd",   "%4x",  0xabcd) );
	REQUIRE(test(" abcd",  "%5x",  0xabcd) );
	REQUIRE(test("ABCD",   "%1X",  0xabcd) );
}

TEST_CASE("Format %% percent literal", "[string][printf]")
{
	REQUIRE(test("%",     "%%") );
	REQUIRE(test("100%",  "100%%") );
	REQUIRE(test("%%",    "%%%%") );
	REQUIRE(test("a%b",   "a%%b") );
	REQUIRE(test("%42%",  "%%%d%%", 42) );
}

TEST_CASE("Format %d edge values", "[string][printf]")
{
	REQUIRE(test("0",           "%d",  0) );
	REQUIRE(test("2147483647",  "%d",  INT32_MAX) );
	REQUIRE(test("-2147483648", "%d",  INT32_MIN) );
	REQUIRE(test("+2147483647", "%+d", INT32_MAX) );

	REQUIRE(test("0",          "%u",  0u) );
	REQUIRE(test("4294967295", "%u",  UINT32_MAX) );

	REQUIRE(test("0", "%x", 0) );
	REQUIRE(test("ffffffff", "%x", UINT32_MAX) );
	REQUIRE(test("FFFFFFFF", "%X", UINT32_MAX) );

	REQUIRE(test("-100006789", "%d", -100006789) );
}

TEST_CASE("Format mixed specifiers", "[string][printf]")
{
	REQUIRE(test("hello 42 3.14 world", "%s %d %.2f %s", "hello", 42, 3.14, "world") );
	REQUIRE(test("a1b2c3", "a%db%dc%d", 1, 2, 3) );
	REQUIRE(test("0xff = 255", "0x%x = %u", 255, 255u) );
}

TEST_CASE("Format %c edge cases", "[string][printf]")
{
	REQUIRE(test("A",                    "%c",    'A') );
	REQUIRE(test(" A",                   "%2c",   'A') );
	REQUIRE(test("         A",           "%10c",  'A') );
	REQUIRE(test("A         ",           "%-10c", 'A') );
}

TEST_CASE("Format flags, lengths and precision", "[string][printf]")
{
	constexpr double kDoubleNan = bx::bitsToDouble(bx::kDoubleExponentMask | bx::kDoubleMantissaMask);

	REQUIRE(test("x yz    42",         "%c %s    %d",    'x', "yz", 42) );
	REQUIRE(test("wxy      ",          "%-9.3s",         "wxyz12345") );
	REQUIRE(test("+9",                 "%+2d",           9) );
	REQUIRE(test("  4",                "% 3i",           4) );
	REQUIRE(test("-3   ",              "%-5d",           -3) );
	REQUIRE(test("+0",                 "%+i",            0) );
	REQUIRE(test("   00042:   -00317", "%8.5d:%9.5d",    42, -317) );
	REQUIRE(test("-204060801",         "%d",             -204060801) );
	REQUIRE(test("35 0035",            "%u %04u",        35u, 35u) );
	REQUIRE(test("17 2a 7F",           "%o %x %X",       15u, 42u, 127u) );
	REQUIRE(test(" 17 2a 7F ",         "%3o %2x %-3X",   15u, 42u, 127u) );
	REQUIRE(test("017 0x2a 0X7F",      "%#o %#x %#X",    15u, 42u, 127u) );
	REQUIRE(test("71 812",             "%hi %ld",        short(71), 812l) );
	REQUIRE(test("7123456789",         "%llu",           7123456789llu) );
	REQUIRE(test("-4 5 -6",            "%ji %zi %ti",    intmax_t(-4), ptrdiff_t(5), ptrdiff_t(-6) ) );

	REQUIRE(testNotStdCompliant("5 6 7", "%I64d %I32d %Id", int64_t(5), int32_t(6), ptrdiff_t(7) ) );

	REQUIRE(test("-7.000000",                      "%f",          -7.0) );
	REQUIRE(test("-6.1234567800",                  "%.10f",       -6.12345678) );
	REQUIRE(test("512.0625000000",                 "%.10f",       512.0625) );
	REQUIRE(test("2.5",                            "%.1f",        2.5) );
	REQUIRE(test(" 0",                             "% .0f",       0.3) );
	REQUIRE(test("0.00",                           "%.2f",        3e-4) );
	REQUIRE(test("-7.30",                          "%+5.2f",      -7.3) );
	REQUIRE(test("0.0        ",                    "%-11.1f",     0.0) );
	REQUIRE(test("-0.000",                         "%.3f",        -0.0) );
	REQUIRE(test("0.000001",                       "%f",          7.5e-7) );
	REQUIRE(test("1237940039285380274899124224.0", "%.1f",        1237940039285380274899124224.0) );
	REQUIRE(test("0.0000002999999999999999864",    "%.25f",       3e-7) );
	REQUIRE(test("0.000000000000000040000000",     "%.24f",       4e-17) );
	REQUIRE(test("0.0000000300 300000000",         "%.10f %.0f",  3e-8, 3e+8) );
	REQUIRE(test("200046813.0",                    "%.1f",        200046813.0) );
	REQUIRE(test("  2.72 %",                       "%*.*f %%",    6, 2, 2.718) );
	REQUIRE(test("-7.000000e+00",                  "%e",          -7.0) );
	REQUIRE(test("2.5E+00",                        "%.1E",        2.5) );
	REQUIRE(test("-7.30e+00",                      "%+5.2e",      -7.3) );
	REQUIRE(test("+0.7 -5",                        "%+g %+g",     0.7, -5.0) );
	REQUIRE(test("3",                              "%.1G",        3.2) );
	REQUIRE(test("-7.3",                           "%+4.2g",      -7.3) );
	REQUIRE(test("7e-250",                         "%g",          7e-250) );
	REQUIRE(test(" 4.2 4.21",                      "% .3g %.3g",  4.203, 4.206) );
	REQUIRE(test("3e-316:2e+307",                  "%g:%g",       3e-316, 2e+307) );
	REQUIRE(test("inf -INF nan",                   "%f %F %g",    bx::kDoubleInfinity, -bx::kDoubleInfinity, kDoubleNan) );
	REQUIRE(test("nan",                            "%.3g",        kDoubleNan) );

	int32_t len = 0;
	REQUIRE(testNotStdCompliant("bbbb ", "%.4s %n", "bbbbbbbbb", &len) );
	REQUIRE(5 == len);
}

TEST_CASE("misc", "[string][printf]")
{
	REQUIRE(test("53000atest-20 bit", "%u%u%ctest%d %s", 5, 3000, 'a', -20, "bit") );
	REQUIRE(test("0.33",       "%.*f", 2, 0.33333333) );
	REQUIRE(test("1",          "%.*d", -1, 1) );
	REQUIRE(test(" ",          "% .0d", 0) );
	REQUIRE(test("     00004", "%10.5d", 4) );
	REQUIRE(test("hi x",       "%*sx", -3, "hi") );
	REQUIRE(test("0.33",       "%.*g", 2, 0.33333333) );
	REQUIRE(test("3.33e-01",   "%.*e", 2, 0.33333333) );
	REQUIRE(test("",           "") );
}

TEST_CASE("Integer precision", "[string][printf]")
{
	SECTION("Precision zero suppresses zero-value output")
	{
		REQUIRE(test("",       "%.0d", 0) );
		REQUIRE(test("",       "%.0i", 0) );
		REQUIRE(test("",       "%.0u", 0u) );
		REQUIRE(test("",       "%.0x", 0) );
		REQUIRE(test("",       "%.0o", 0) );
		REQUIRE(test("",       "%.d",  0) );
		REQUIRE(test("",       "%.i",  0) );
		REQUIRE(test("",       "%.u",  0u) );
		REQUIRE(test("",       "%.x",  0) );
		REQUIRE(test("",       "%.o",  0) );
	}

	SECTION("Precision pads with leading zeros")
	{
		REQUIRE(test("00042",  "%.5d", 42) );
		REQUIRE(test("-00042", "%.5d", -42) );
		REQUIRE(test("00042",  "%.5i", 42) );
		REQUIRE(test("-00042", "%.5i", -42) );
		REQUIRE(test("00042",  "%.5u", 42u) );
		REQUIRE(test("000ff",  "%.5x", 255) );
		REQUIRE(test("000FF",  "%.5X", 255) );
		REQUIRE(test("00777",  "%.5o", 511) );
	}

	SECTION("Precision with sign flags")
	{
		REQUIRE(test("+",      "%+.0d", 0) );
		REQUIRE(test("+",      "%+.0i", 0) );
		REQUIRE(test("+01",    "%+.2i", 1) );
	}

	SECTION("Width + precision combined")
	{
		REQUIRE(test("               01024", "%20.5d",  1024) );
		REQUIRE(test("              -01024", "%20.5d", -1024) );
		REQUIRE(test("               01024", "%20.5u", 1024u) );
		REQUIRE(test("                    ", "%20.d",  0) );
		REQUIRE(test("                    ", "%20.u",  0u) );
		REQUIRE(test("                    ", "%20.x",  0) );
		REQUIRE(test("00123               ", "%-20.5i", 123) );
	}

	SECTION("Precision overrides 0 flag")
	{
		REQUIRE(test("  042",  "%05.3d", 42) );
		REQUIRE(test(" -042",  "%05.3d", -42) );
		REQUIRE(test("  042",  "%05.3i", 42) );
		REQUIRE(test(" -042",  "%05.3i", -42) );
		REQUIRE(test("  00042", "%07.5u", 42u) );
	}
}

TEST_CASE("Space and plus flag", "[string][printf]")
{
	SECTION("Space flag without width")
	{
		REQUIRE(test(" 42",  "% d",  42) );
		REQUIRE(test("-42",  "% d", -42) );
		REQUIRE(test(" 0",   "% d",  0) );
		REQUIRE(test(" 0",   "% i",  0) );
	}

	SECTION("Plus flag without width")
	{
		REQUIRE(test("+42",  "%+d",  42) );
		REQUIRE(test("-42",  "%+d", -42) );
		REQUIRE(test("+0",   "%+i",  0) );
	}

	SECTION("Plus overrides space")
	{
		REQUIRE(test("+42",  "%+ d",  42) );
		REQUIRE(test("+42",  "% +d",  42) );
		REQUIRE(test("-42",  "%+ d", -42) );
		REQUIRE(test("+0",   "%+ d",  0) );
	}

	SECTION("Space flag on float")
	{
		REQUIRE(test(" 1.500000",  "% f",  1.5) );
		REQUIRE(test("-1.500000",  "% f", -1.5) );
	}
}

TEST_CASE("Zero-padding negative numbers", "[string][printf]")
{
	REQUIRE(test("-5",   "%01d",  -5) );
	REQUIRE(test("-5",   "%02d",  -5) );
	REQUIRE(test("-05",  "%03d",  -5) );
	REQUIRE(test("-005", "%04d",  -5) );
}

TEST_CASE("Star args", "[string][printf]")
{
	SECTION("Star width")
	{
		REQUIRE(test("         Z", "%*c", 10, 'Z') );
		REQUIRE(test("               Hello", "%*s", 20, "Hello") );
	}

	SECTION("Negative star width = left-justify")
	{
		REQUIRE(test("42        ", "%*d", -10, 42) );
		REQUIRE(test("-42       ", "%*d", -10, -42) );
		REQUIRE(test("x         ", "%*c", -10, 'x') );
		REQUIRE(test("hello     ", "%*s", -10, "hello") );
	}

	SECTION("Star precision")
	{
		REQUIRE(test("01",    "%.*i", 2, 1) );
		REQUIRE(test("h",     "%.*s", 1, "hello world") );
	}

	SECTION("Negative star precision = precision omitted")
	{
		REQUIRE(test("42",    "%.*d", -1, 42) );
		REQUIRE(test("0",     "%.*d", -1, 0) );
		REQUIRE(test("hello", "%.*s", -1, "hello") );
	}

	SECTION("Star width + star precision")
	{
		REQUIRE(test("        07", "%*.*i", 10, 2, 7) );
	}

	SECTION("Star width/precision with %%")
	{
		REQUIRE(test(" 1.23 %", "%*.*f %%", 5, 2, 1.23) );
	}
}
