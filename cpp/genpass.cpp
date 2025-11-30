#include <cstddef>
#include <fast_io.h>
#include <fast_io_dsal/array.h>
#include <fast_io_dsal/string.h>
#include <fast_io_dsal/string_view.h>

namespace
{

inline ::fast_io::u8string html_str;
inline ::fast_io::u8string saved_str;
inline ::fast_io::u8string elapsed_time;
inline ::fast_io::u8string last_timestamp;

enum class data_category : ::std::uint_least8_t
{
	username,
	password,
	passwordspecial,
	pin4,
	pin6,
	pin12
};

inline data_category last_generated_category{};

inline void reset_category(data_category cate) noexcept
{
	last_generated_category = cate;
	html_str.clear();
	saved_str.clear();
	elapsed_time.clear();
	last_timestamp.clear();
}

struct letter_freq
{
	char8_t ch{};
	::std::uint_least32_t freq{};
};

inline constexpr ::fast_io::array<letter_freq, 26> english_letter_freq{
    {{u8'e', 1260}, {u8't', 937}, {u8'a', 834}, {u8'o', 770}, {u8'n', 680},
     {u8'i', 671},  {u8's', 611}, {u8'h', 611}, {u8'r', 568}, {u8'l', 424},
     {u8'd', 414},  {u8'u', 285}, {u8'c', 273}, {u8'm', 253}, {u8'w', 234},
     {u8'y', 204},  {u8'f', 203}, {u8'g', 192}, {u8'p', 166}, {u8'b', 154},
     {u8'v', 106},  {u8'k', 87},  {u8'j', 23},	{u8'x', 20},  {u8'q', 9},
     {u8'z', 6}}};

// Step 1: compute total frequency sum at compile time
inline constexpr ::std::size_t english_letter_total_freq() noexcept
{
	::std::size_t sum{};
	for (auto const &lf : english_letter_freq)
	{
		sum += lf.freq;
	}
	return sum;
}

inline constexpr ::std::size_t english_letter_total{
    english_letter_total_freq()};

// Step 2: build expanded array
inline constexpr auto build_expanded_array() noexcept
{
	::fast_io::array<char8_t, english_letter_total> arr;
	::std::size_t pos{};
	for (auto const &lf : english_letter_freq)
	{
		for (::std::size_t i{}; i != lf.freq; ++i)
		{
			arr[pos] = lf.ch;
			++pos;
		}
	}
	return arr;
}

// Step 3: final expanded array
inline constexpr auto english_letter_expanded{build_expanded_array()};
inline constexpr ::std::size_t maximum_display{1000};

inline void generate_username(::std::size_t n) noexcept
{
	using namespace ::fast_io::io;
	::fast_io::u8ostring_ref_fast_io obf{::std::addressof(saved_str)};
	::fast_io::u8ostring_ref_fast_io obfhtml{::std::addressof(html_str)};
	::fast_io::ibuf_white_hole_engine eng;
	constexpr ::std::size_t english_letter_total_m1{english_letter_total -
							1u};
	std::uniform_int_distribution<std::size_t> ud(0,
						      english_letter_total_m1);
	std::uniform_int_distribution<std::size_t> rlen(6, 12);
	for (std::size_t i{}; i != n; ++i)
	{
		for (std::size_t j{}, s(rlen(eng)); j != s; ++j)
		{
			auto ch{english_letter_expanded[ud(eng)]};
			print(obf, fast_io::mnp::chvw(ch));
			if (i < maximum_display)
			{
				print(obfhtml, ::fast_io::mnp::chvw(ch));
			}
		}
		println(obf);
		if (i < maximum_display)
		{
			print(obfhtml, u8"<br/>\n");
		}
	}
}

// Use constexpr fast_io::array for compile-time safety
inline constexpr ::fast_io::array<char8_t, 16> special_chars{
    u8'!', u8'@', u8'#', u8'$', u8'%', u8'^', u8'&', u8'*',
    u8'(', u8')', u8'-', u8'_', u8'=', u8'+', u8'[', u8']'};
inline void generate_password(data_category cate, ::std::size_t n) noexcept
{
	using namespace ::fast_io::io;
	::fast_io::u8ostring_ref_fast_io obf{::std::addressof(saved_str)};
	::fast_io::u8ostring_ref_fast_io obfhtml{::std::addressof(html_str)};
	::fast_io::ibuf_white_hole_engine eng;

	// Decide upper bound based on category
	::std::size_t upper{};
	if (cate == data_category::pin4 || cate == data_category::pin6 ||
	    cate == data_category::pin12)
	{
		upper = 9;
	}
	else if (cate == data_category::passwordspecial)
	{
		upper = 77;
	}
	else
	{
		upper = 61;
	}

	::std::uniform_int_distribution<::std::size_t> ud(0, upper);
	::std::uniform_int_distribution<::std::size_t> rlen(12, 20);

	bool pins{};
	::std::size_t fixed_len{};
	auto set_pin(
	    [&](::std::size_t setval) noexcept
	    {
		    fixed_len = setval;
		    pins = true;
	    });
	if (cate == data_category::pin4)
	{
		set_pin(4);
	}
	else if (cate == data_category::pin6)
	{
		set_pin(6);
	}
	else if (cate == data_category::pin12)
	{
		set_pin(12);
	}
	for (::std::size_t i{}; i != n; ++i)
	{
		::std::size_t s{pins ? fixed_len : rlen(eng)};

		for (::std::size_t j{}; j != s; ++j)
		{
			::std::size_t val{ud(eng)};
			char8_t ch{};

			if (val < 10u)
			{
				ch = static_cast<char8_t>(val + u8'0');
			}
			else if (val < 36u)
			{
				ch = static_cast<char8_t>(val - 10u + u8'a');
			}
			else if (val < 62u)
			{
				ch = static_cast<char8_t>(val - 36u + u8'A');
			}
			else
			{
				ch = special_chars[val - 62u];
			}

			print(obf, ::fast_io::mnp::chvw(ch));
			if (i < maximum_display)
			{
				print(obfhtml, ::fast_io::mnp::chvw(ch));
			}
		}
		println(obf);
		if (i < maximum_display)
		{
			print(obfhtml, u8"<br/>\n");
		}
	}
}

} // namespace

extern "C"
{
	[[__gnu__::__visibility__("default")]]
	::std::size_t generate_data(data_category cate,
				    ::std::size_t n) noexcept
	{
		using namespace ::fast_io::io;
		if (static_cast<::std::size_t>(data_category::pin12) <
		    static_cast<::std::size_t>(cate))
		{
			return 1;
		}
		reset_category(cate);
		::fast_io::unix_timestamp start_ts{
		    ::fast_io::posix_clock_gettime(
			::fast_io::posix_clock_id::realtime)};
		switch (cate)
		{
		case data_category::username:
			generate_username(n);
			break;
		case data_category::password:
		case data_category::passwordspecial:
		case data_category::pin4:
		case data_category::pin6:
		case data_category::pin12:
			generate_password(cate, n);
			break;
		default:
		}
		::fast_io::unix_timestamp end_ts{::fast_io::posix_clock_gettime(
		    ::fast_io::posix_clock_id::realtime)};
		::fast_io::unix_timestamp diff_ts{end_ts - start_ts};
		::fast_io::u8ostring_ref_fast_io elapsed_time_oref{
		    ::std::addressof(elapsed_time)};
		print(elapsed_time_oref, diff_ts, "s");
		::fast_io::u8ostring_ref_fast_io last_timestame_oref{
		    ::std::addressof(last_timestamp)};
		print(last_timestame_oref, end_ts.seconds);
		return 0;
	}
	[[__gnu__::__visibility__("default")]]
	data_category get_last_generated_category() noexcept
	{
		return last_generated_category;
	}
	[[__gnu__::__visibility__("default")]]
	char8_t const *get_html_ptr() noexcept
	{
		return html_str.c_str();
	}
	[[__gnu__::__visibility__("default")]]
	::std::size_t get_html_len() noexcept
	{
		return html_str.size();
	}
	[[__gnu__::__visibility__("default")]]
	char8_t const *get_saved_ptr() noexcept
	{
		return saved_str.c_str();
	}
	[[__gnu__::__visibility__("default")]]
	::std::size_t get_saved_len() noexcept
	{
		return saved_str.size();
	}
	[[__gnu__::__visibility__("default")]]
	char8_t const *generated_elapsed_time_ptr() noexcept
	{
		return elapsed_time.c_str();
	}
	[[__gnu__::__visibility__("default")]]
	::std::size_t generated_elapsed_time_len() noexcept
	{
		return elapsed_time.size();
	}
	[[__gnu__::__visibility__("default")]]
	char8_t const *generated_last_timestamp_ptr() noexcept
	{
		return last_timestamp.c_str();
	}
	[[__gnu__::__visibility__("default")]]
	::std::size_t generated_last_timestamp_len() noexcept
	{
		return last_timestamp.size();
	}
}
