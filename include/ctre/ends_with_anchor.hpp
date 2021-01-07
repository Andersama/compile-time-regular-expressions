#ifndef CTRE__ENDS_WITH_ANCHOR__HPP
#define CTRE__ENDS_WITH_ANCHOR__HPP

#include "flags_and_modes.hpp"

namespace ctre {

template <typename... Content> 
constexpr bool ends_with_anchor(const flags &, ctll::list<Content...>) noexcept {
	return false;
}

template <typename... Content> 
constexpr bool ends_with_anchor(const flags &, ctll::list<Content..., assert_subject_end>) noexcept {
	// yes! end subject anchor is here
	return true;
}

template <typename... Content> 
constexpr bool ends_with_anchor(const flags & f, ctll::list<Content..., assert_line_end>) noexcept {
	// yes! end line anchor is here
	return !ctre::multiline_mode(f) || ends_with_anchor(f, ctll::list<Content...>{});
}

template <typename CharLike, typename... Content> 
constexpr bool ends_with_anchor(const flags & f, ctll::list<Content..., boundary<CharLike>>) noexcept {
	// check if all options ends with anchor or if they are empty, there is an anchor ahead of them
	return ends_with_anchor(f, ctll::list<Content...>{});
}

template <typename... Options, typename... Content> 
constexpr bool ends_with_anchor(const flags & f, ctll::list<Content..., select<Options...>>) noexcept {
	// check if all options ends with anchor or if they are empty, there is an anchor ahead of them
	return (ends_with_anchor(f, ctll::list<Content..., Options>{}) && ... && true);
}

template <typename... Seq, typename... Content> 
constexpr bool ends_with_anchor(const flags & f, ctll::list<Content..., sequence<Seq...>>) noexcept {
	// check if all options ends with anchor or if they are empty, there is an anchor ahead of them
	return ends_with_anchor(f, ctll::list<Content..., Seq...>{});
}

template <size_t A, size_t B, typename... Content, typename... Seq>
constexpr bool ends_with_anchor(const flags & f, ctll::list<Content..., repeat<A, B, Seq...>>) noexcept {
	// check if all options ends with anchor or if they are empty, there is an anchor ahead of them
	return ends_with_anchor(f, ctll::list<Content..., Seq...>{});
}

template <size_t A, size_t B, typename... Content, typename... Seq>
constexpr bool ends_with_anchor(const flags & f, ctll::list<Content..., lazy_repeat<A, B, Seq...>>) noexcept {
	// check if all options ends with anchor or if they are empty, there is an anchor ahead of them
	if constexpr (A > 0)
		return ends_with_anchor(f, ctll::list<Content..., Seq...>{});
	else
		return ends_with_anchor(f, ctll::list<Content..., Seq...>{}) && ends_with_anchor(f, ctll::list<Content...>{});
}

template <size_t A, size_t B, typename... Content, typename... Seq>
constexpr bool ends_with_anchor(const flags & f, ctll::list<Content..., possessive_repeat<A, B, Seq...>>) noexcept {
	// check if all options ends with anchor or if they are empty, there is an anchor ahead of them
	if constexpr (A > 0)
		return ends_with_anchor(f, ctll::list<Content..., Seq...>{});
	else
		return ends_with_anchor(f, ctll::list<Content..., Seq...>{}) && ends_with_anchor(f, ctll::list<Content...>{});
}

template <size_t Index, typename... Content, typename... Seq>
constexpr bool ends_with_anchor(const flags & f, ctll::list<Content..., capture<Index, Seq...>>) noexcept {
	// check if all options ends with anchor or if they are empty, there is an anchor ahead of them
	return ends_with_anchor(f, ctll::list<Content..., Seq...>{});
}

template <size_t Index, typename... Content, typename... Seq>
constexpr bool ends_with_anchor(const flags & f, ctll::list<Content..., capture_with_name<Index, Seq...>>) noexcept {
	// check if all options ends with anchor or if they are empty, there is an anchor behind them
	return ends_with_anchor(f, ctll::list<Content..., Seq...>{});
}

}

#endif
