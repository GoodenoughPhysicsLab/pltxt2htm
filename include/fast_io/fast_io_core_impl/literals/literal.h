﻿#pragma once

namespace fast_io
{

template <::std::integral ch_type>
#if __has_cpp_attribute(__gnu__::__always_inline__)
[[__gnu__::__always_inline__]]
#elif __has_cpp_attribute(msvc::forceinline)
[[msvc::forceinline]]
#endif
inline constexpr ch_type char_literal(char8_t ch) noexcept
{
#if (('A' != 0x41) || (L'A' != 0x41))
#include "none_ascii.h"
#endif
	{
		using unsigned_t = ::std::make_unsigned_t<ch_type>;
		return static_cast<ch_type>(static_cast<unsigned_t>(ch));
	}
}
template <char8_t ch, ::std::integral ch_type>
inline constexpr ch_type char_literal_v{char_literal<ch_type>(ch)};

template <::std::integral ch_type>
inline constexpr ch_type const *null_terminated_c_str() noexcept
{
	if constexpr (::std::same_as<ch_type, char>)
	{
		return "";
	}
	else if constexpr (::std::same_as<ch_type, wchar_t>)
	{
		return L"";
	}
	else if constexpr (::std::same_as<ch_type, char8_t>)
	{
		return u8"";
	}
	else if constexpr (::std::same_as<ch_type, char16_t>)
	{
		return u"";
	}
	else if constexpr (::std::same_as<ch_type, char32_t>)
	{
		return U"";
	}
	else
	{
		return __builtin_addressof(::fast_io::char_literal_v<0, ch_type>);
	}
}

template <::std::integral ch_type>
inline constexpr ch_type const *null_terminated_c_str_v{::fast_io::null_terminated_c_str<ch_type>()};

template <::std::integral ch_type>
#if __has_cpp_attribute(__gnu__::__always_inline__)
[[__gnu__::__always_inline__]]
#elif __has_cpp_attribute(msvc::forceinline)
[[msvc::forceinline]]
#endif
inline constexpr ch_type arithmetic_char_literal(char8_t ch) noexcept
{
	if constexpr (::std::same_as<ch_type, wchar_t> && ::fast_io::details::wide_is_none_utf_endian)
	{
		using unsigned_t = ::std::make_unsigned_t<ch_type>;
		return static_cast<ch_type>(static_cast<unsigned_t>(ch));
	}
	else
	{
		return char_literal<ch_type>(ch);
	}
}

template <char8_t ch, ::std::integral ch_type>
inline constexpr ch_type arithmetic_char_literal_v{arithmetic_char_literal<ch_type>(ch)};

template <::std::integral char_type, char8_t ch = u8'0', ::std::integral T>
inline constexpr char_type char_literal_add(T offs) noexcept
{
#if __has_cpp_attribute(assume)
	if constexpr (ch == u8'0')
	{
		[[assume(0 <= offs && offs < 10)]];
	}
#endif
	using unsigned_char_type = ::std::make_unsigned_t<char_type>;
	using common_chtype = ::std::common_type_t<char_type, decltype(ch)>;
	using unsigned_result_type = ::std::make_unsigned_t<::std::remove_cvref_t<decltype(static_cast<common_chtype>(arithmetic_char_literal_v<ch, char_type>) + offs)>>;
	if constexpr (::std::same_as<char_type, wchar_t> && ::fast_io::details::wide_is_none_utf_endian)
	{
		static_assert(::std::numeric_limits<::std::uint_least8_t>::digits <= ::std::numeric_limits<wchar_t>::digits);
		constexpr unsigned leftshift_offset{static_cast<unsigned>(::std::numeric_limits<wchar_t>::digits -
																  ::std::numeric_limits<::std::uint_least8_t>::digits)};
		return static_cast<char_type>(static_cast<unsigned_char_type>(static_cast<unsigned_result_type>(
										  static_cast<common_chtype>(arithmetic_char_literal_v<ch, char_type>) + offs))
									  << leftshift_offset);
	}
	else
	{
		return static_cast<char_type>(static_cast<unsigned_char_type>(
			static_cast<unsigned_result_type>(static_cast<common_chtype>(arithmetic_char_literal_v<ch, char_type>) + offs)));
	}
}

} // namespace fast_io
