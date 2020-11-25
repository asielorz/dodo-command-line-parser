#pragma once

#include <string_view>
#include <charconv>
#include <string>
#include <optional>

template <typename T>
struct parse_traits;

template <typename T>
concept TraitPrintable = requires(T t) { {parse_traits<T>::to_string(t)} -> std::convertible_to<std::string>; };

template <TraitPrintable T>
decltype(auto) to_string(T const & t)
{
	return parse_traits<T>::to_string(t);
}

template <typename T>
struct charconv_to_string_parse_traits
{
	static std::optional<T> parse(std::string_view text) noexcept
	{
		T x;
		auto const result = std::from_chars(text.data(), text.data() + text.size(), x);
		if (result.ec != std::errc() || result.ptr != text.data() + text.size())
			return std::nullopt;
		return x;
	}

	static std::string to_string(T x) noexcept
	{
		return std::to_string(x);
	}
};

template <> struct parse_traits<int16_t> : public charconv_to_string_parse_traits<int16_t> {};
template <> struct parse_traits<uint16_t> : public charconv_to_string_parse_traits<uint16_t> {};
template <> struct parse_traits<int32_t> : public charconv_to_string_parse_traits<int32_t> {};
template <> struct parse_traits<uint32_t> : public charconv_to_string_parse_traits<uint32_t> {};
template <> struct parse_traits<int64_t> : public charconv_to_string_parse_traits<int64_t> {};
template <> struct parse_traits<uint64_t> : public charconv_to_string_parse_traits<uint64_t> {};
template <> struct parse_traits<float> : public charconv_to_string_parse_traits<float> {};
template <> struct parse_traits<double> : public charconv_to_string_parse_traits<double> {};
template <> struct parse_traits<long double> : public charconv_to_string_parse_traits<long double> {};

template <>
struct parse_traits<bool>
{
	static constexpr std::optional<bool> parse(std::string_view text) noexcept
	{
		if (text == "true")
			return true;
		else if (text == "false")
			return false;
		else
			return std::nullopt;
	}

	static std::string to_string(bool x) noexcept
	{
		return x ? "true" : "false";
	}
};

template <>
struct parse_traits<std::string>
{
	static std::optional<std::string> parse(std::string_view text) noexcept
	{
		return std::string(text);
	}

	static std::string const & to_string(std::string const & s) noexcept
	{
		return s;
	}
};

template <>
struct parse_traits<std::string_view>
{
	static constexpr std::optional<std::string_view> parse(std::string_view text) noexcept
	{
		return text;
	}

	static std::string to_string(std::string_view s) noexcept
	{
		return std::string(s);
	}
};
