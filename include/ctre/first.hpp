#ifndef CTRE__FIRST__HPP
#define CTRE__FIRST__HPP

#include "atoms.hpp"
#include "atoms_unicode.hpp"

namespace ctre {
	
struct can_be_anything {};
	

template <typename... Content> 
constexpr auto first(ctll::list<Content...> l, ctll::list<>) noexcept {
	return l;
}

template <typename... Content, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<accept, Tail...>) noexcept {
	return l;
}

template <typename... Content, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<end_mark, Tail...>) noexcept {
	return l;
}

template <typename... Content, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<end_cycle_mark, Tail...>) noexcept {
	return l;
}

template <typename... Content, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<end_lookahead_mark, Tail...>) noexcept {
	return l;
}

template <typename... Content, size_t Id, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<numeric_mark<Id>, Tail...>) noexcept {
	return first(l, ctll::list<Tail...>{});
}

// empty
template <typename... Content, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<empty, Tail...>) noexcept {
	return first(l, ctll::list<Tail...>{});
}

// boundary
template <typename... Content, typename CharLike, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<boundary<CharLike>, Tail...>) noexcept {
	return first(l, ctll::list<Tail...>{});
}

// asserts
template <typename... Content, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<assert_subject_begin, Tail...>) noexcept {
	return first(l, ctll::list<Tail...>{});
}

template <typename... Content, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<assert_subject_end, Tail...>) noexcept {
	return l;
}

template <typename... Content, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<assert_subject_end_line, Tail...>) noexcept {
	// FIXME allow endline here
	return l;
}

template <typename... Content, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<assert_line_begin, Tail...>) noexcept {
	// FIXME line begin is a bit different than subject begin
	return first(l, ctll::list<Tail...>{});
}

template <typename... Content, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<assert_line_end, Tail...>) noexcept {
	// FIXME line end is a bit different than subject begin
	return l;
}

// sequence
template <typename... Content, typename... Seq, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<sequence<Seq...>, Tail...>) noexcept {
	return first(l, ctll::list<Seq..., Tail...>{});
}

// plus
template <typename... Content, typename... Seq, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<plus<Seq...>, Tail...>) noexcept {
	return first(l, ctll::list<Seq..., Tail...>{});
}

// lazy_plus
template <typename... Content, typename... Seq, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<lazy_plus<Seq...>, Tail...>) noexcept {
	return first(l, ctll::list<Seq..., Tail...>{});
}

// possessive_plus
template <typename... Content, typename... Seq, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<possessive_plus<Seq...>, Tail...>) noexcept {
	return first(l, ctll::list<Seq..., Tail...>{});
}

// star
template <typename... Content, typename... Seq, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<star<Seq...>, Tail...>) noexcept {
	return first(first(l, ctll::list<Tail...>{}), ctll::list<Seq..., Tail...>{});
}

// lazy_star
template <typename... Content, typename... Seq, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<lazy_star<Seq...>, Tail...>) noexcept {
	return first(first(l, ctll::list<Tail...>{}), ctll::list<Seq..., Tail...>{});
}

// possessive_star
template <typename... Content, typename... Seq, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<possessive_star<Seq...>, Tail...>) noexcept {
	return first(first(l, ctll::list<Tail...>{}), ctll::list<Seq..., Tail...>{});
}

// lazy_repeat
template <typename... Content, size_t A, size_t B, typename... Seq, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<lazy_repeat<A, B, Seq...>, Tail...>) noexcept {
	return first(l, ctll::list<Seq..., Tail...>{});
}

template <typename... Content, size_t B, typename... Seq, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<lazy_repeat<0, B, Seq...>, Tail...>) noexcept {
	return first(first(l, ctll::list<Tail...>{}), ctll::list<Seq..., Tail...>{});
}

// possessive_repeat
template <typename... Content, size_t A, size_t B, typename... Seq, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<possessive_repeat<A, B, Seq...>, Tail...>) noexcept {
	return first(l, ctll::list<Seq..., Tail...>{});
}

template <typename... Content, size_t B, typename... Seq, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<possessive_repeat<0, B, Seq...>, Tail...>) noexcept {
	return first(first(l, ctll::list<Tail...>{}), ctll::list<Seq..., Tail...>{});
}

// repeat
template <typename... Content, size_t A, size_t B, typename... Seq, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<repeat<A, B, Seq...>, Tail...>) noexcept {
	return first(l, ctll::list<Seq..., Tail...>{});
}

template <typename... Content, size_t B, typename... Seq, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<repeat<0, B, Seq...>, Tail...>) noexcept {
	return first(first(l, ctll::list<Tail...>{}), ctll::list<Seq..., Tail...>{});
}

// lookahead_positive
template <typename... Content, typename... Seq, typename... Tail> 
constexpr auto first(ctll::list<Content...>, ctll::list<lookahead_positive<Seq...>, Tail...>) noexcept {
	return ctll::list<can_be_anything>{};
}

// lookahead_negative TODO fixme
template <typename... Content, typename... Seq, typename... Tail> 
constexpr auto first(ctll::list<Content...>, ctll::list<lookahead_negative<Seq...>, Tail...>) noexcept {
	return ctll::list<can_be_anything>{};
}

// capture
template <typename... Content, size_t Id, typename... Seq, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<capture<Id, Seq...>, Tail...>) noexcept {
	return first(l, ctll::list<Seq..., Tail...>{});
}

template <typename... Content, size_t Id, typename Name, typename... Seq, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<capture_with_name<Id, Name, Seq...>, Tail...>) noexcept {
	return first(l, ctll::list<Seq..., Tail...>{});
}

// backreference
template <typename... Content, size_t Id, typename... Tail> 
constexpr auto first(ctll::list<Content...>, ctll::list<back_reference<Id>, Tail...>) noexcept {
	return ctll::list<can_be_anything>{};
}

template <typename... Content, typename Name, typename... Tail> 
constexpr auto first(ctll::list<Content...>, ctll::list<back_reference_with_name<Name>, Tail...>) noexcept {
	return ctll::list<can_be_anything>{};
}


// string First extraction
template <typename... Content, auto First, auto... String, typename... Tail> 
constexpr auto first(ctll::list<Content...>, ctll::list<string<First, String...>, Tail...>) noexcept {
	return ctll::list<Content..., character<First>>{};
}

template <typename... Content, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<string<>, Tail...>) noexcept {
	return first(l, ctll::list<Tail...>{});
}

// optional
template <typename... Content, typename... Opt, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<optional<Opt...>, Tail...>) noexcept {
	return first(first(l, ctll::list<Opt..., Tail...>{}), ctll::list<Tail...>{});
}

template <typename... Content, typename... Opt, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<lazy_optional<Opt...>, Tail...>) noexcept {
	return first(first(l, ctll::list<Opt..., Tail...>{}), ctll::list<Tail...>{});
}

// select (alternation)
template <typename... Content, typename SHead, typename... STail, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<select<SHead, STail...>, Tail...>) noexcept {
	return first(first(l, ctll::list<SHead, Tail...>{}), ctll::list<select<STail...>, Tail...>{});
}

template <typename... Content, typename... Tail> 
constexpr auto first(ctll::list<Content...> l, ctll::list<select<>, Tail...>) noexcept {
	return l;
}


// unicode property => anything
template <typename... Content, auto Property, typename... Tail> 
constexpr auto first(ctll::list<Content...>, ctll::list<ctre::binary_property<Property>, Tail...>) noexcept {
	return ctll::list<can_be_anything>{};
}

template <typename... Content, auto Property, auto Value, typename... Tail> 
constexpr auto first(ctll::list<Content...>, ctll::list<ctre::property<Property, Value>, Tail...>) noexcept {
	return ctll::list<can_be_anything>{};
}

// characters / sets

template <typename... Content, auto V, typename... Tail> 
constexpr auto first(ctll::list<Content...>, ctll::list<character<V>, Tail...>) noexcept {
	return ctll::list<Content..., character<V>>{};
}

template <typename... Content, auto... Values, typename... Tail> 
constexpr auto first(ctll::list<Content...>, ctll::list<enumeration<Values...>, Tail...>) noexcept {
	return ctll::list<Content..., character<Values>...>{};
}

template <typename... Content, typename... SetContent, typename... Tail> 
constexpr auto first(ctll::list<Content...>, ctll::list<set<SetContent...>, Tail...>) noexcept {
	return ctll::list<Content..., SetContent...>{};
}

template <typename... Content, auto A, auto B, typename... Tail> 
constexpr auto first(ctll::list<Content...>, ctll::list<char_range<A,B>, Tail...>) noexcept {
	return ctll::list<Content..., char_range<A,B>>{};
}

template <typename... Content, typename... Tail> 
constexpr auto first(ctll::list<Content...>, ctll::list<any, Tail...>) noexcept {
	return ctll::list<can_be_anything>{};
}

// negative
template <typename... Content, typename... SetContent, typename... Tail> 
constexpr auto first(ctll::list<Content...>, ctll::list<negate<SetContent...>, Tail...>) noexcept {
	return ctll::list<Content..., negative_set<SetContent...>>{};
}

template <typename... Content, typename... SetContent, typename... Tail> 
constexpr auto first(ctll::list<Content...>, ctll::list<negative_set<SetContent...>, Tail...>) noexcept {
	return ctll::list<Content..., negative_set<SetContent...>>{};
}


// user facing interface
template <typename... Content> constexpr auto calculate_first(Content...) noexcept {
	return first(ctll::list<>{}, ctll::list<Content...>{});
}


// calculate mutual exclusivity
template <typename... Content> constexpr size_t calculate_size_of_first(ctre::negative_set<Content...>) {
	return 1 + 1 * sizeof...(Content);
}

template <auto... V> constexpr size_t calculate_size_of_first(ctre::enumeration<V...>) {
	return sizeof...(V);
}

constexpr size_t calculate_size_of_first(...) {
	return 1;
}

template <typename... Content> constexpr size_t calculate_size_of_first(ctll::list<Content...>) {
	return (calculate_size_of_first(Content{}) + ... + 0);
}

template <typename... Content> constexpr size_t calculate_size_of_first(ctre::set<Content...>) {
	return (calculate_size_of_first(Content{}) + ... + 0);
}

template<typename... Content, typename... Tail> constexpr auto first_and_trailing(ctll::list<sequence<Content...>, Tail...>) noexcept {
	if constexpr (sizeof...(Content) || sizeof...(Tail))
		return first_and_trailing(ctll::list<Content..., Tail...>());
	else
		return ctll::list_pop_pair<ctll::list<>, ctll::list<>>();
}

template<typename... Content, typename... Tail> constexpr auto first_and_trailing(ctll::list<ctll::list<Content...>, Tail...>) noexcept {
	if constexpr (sizeof...(Content) || sizeof...(Tail))
		return first_and_trailing(ctll::list<Content..., Tail...>());
	else
		return ctll::list_pop_pair<ctll::list<>, ctll::list<>>();
}

//repeats (we can extract sequences if the repeat is non-optional)
template<size_t A, size_t B, typename... Content, typename... Tail> constexpr auto first_and_trailing(ctll::list<repeat<A,B, Content...>, Tail...>) noexcept {
	if constexpr (A && B) {
		if constexpr (B > 1) {
			return first_and_trailing(ctll::list<Content..., repeat<A - 1, B - 1, Content...>, Tail...>());
		} else {
			return first_and_trailing(ctll::list<Content..., Tail...>());
		}
	} else if constexpr (A) {
		return first_and_trailing(ctll::list<Content..., repeat<A-1, B, Content...>, Tail...>());
	} else { //some form of optional
		return ctll::list_pop_pair<ctll::list<repeat<A, B, Content...>>, ctll::list<Tail...>>();
	}
}

template<size_t A, size_t B, typename... Content, typename... Tail> constexpr auto first_and_trailing(ctll::list<lazy_repeat<A,B, Content...>, Tail...>) noexcept {
	if constexpr (A && B) {
		if constexpr (B > 1) {
			return first_and_trailing(ctll::list<Content..., lazy_repeat<A - 1, B - 1, Content...>, Tail...>());
		} else {
			return first_and_trailing(ctll::list<Content..., Tail...>());
		}
	} else if constexpr (A) {
		return first_and_trailing(ctll::list<Content..., lazy_repeat<A-1, B, Content...>, Tail...>());
	} else { //some form of optional
		return ctll::list_pop_pair<ctll::list<lazy_repeat<A, B, Content...>>, ctll::list<Tail...>>();
	}
}

template<size_t A, size_t B, typename... Content, typename... Tail> constexpr auto first_and_trailing(ctll::list<possessive_repeat<A,B, Content...>, Tail...>) noexcept {
	if constexpr (A && B) {
		if constexpr (B > 1) {
			return first_and_trailing(ctll::list<Content..., possessive_repeat<A - 1, B - 1, Content...>, Tail...>());
		} else {
			return first_and_trailing(ctll::list<Content..., Tail...>());
		}
	} else if constexpr (A) {
		return first_and_trailing(ctll::list<Content..., possessive_repeat<A-1, B, Content...>, Tail...>());
	} else { //some form of optional
		return ctll::list_pop_pair<ctll::list<possessive_repeat<A, B, Content...>>, ctll::list<Tail...>>();
	}
}

template<auto C, auto... String, typename... Tail> constexpr auto first_and_trailing(ctll::list<string<C, String...>, Tail...>) noexcept {
	if constexpr (sizeof...(String)) {
		return ctll::list_pop_pair<ctll::list<character<C>>, ctll::list<string<String...>, Tail...>>();
	} else {
		return ctll::list_pop_pair<ctll::list<character<C>>, ctll::list<Tail...>>();
	}
}
//special case
template<auto C, typename... Tail> constexpr auto first_and_trailing(ctll::list<set<character<C>>, Tail...>) noexcept {
	return ctll::list_pop_pair<ctll::list<character<C>>, ctll::list<Tail...>>();
}

template<typename T, typename... Types>
constexpr bool is_any_of_v = std::disjunction_v<std::is_same<T, Types>...>;

template<typename T>
constexpr bool is_empty_sequence = is_any_of_v<T, sequence<>, ctll::list<>, ctll::_nothing>;

template<typename... Content, typename... Listed>
constexpr auto CTRE_FORCE_INLINE concatenate_into_sequence(sequence<Content...>, ctll::list<Listed...>) noexcept {
	return concatenate_into_sequence(sequence<Content..., Listed...>{});
}

template<typename... Content, typename... Listed>
constexpr auto CTRE_FORCE_INLINE concatenate_into_sequence(sequence<Content...>, sequence<Listed...>) noexcept {
	return concatenate_into_sequence(sequence<Content..., Listed...>{});
}

template<typename... Content, typename... Tail>
constexpr auto CTRE_FORCE_INLINE concatenate_into_sequence(ctll::list<sequence<Content...>, Tail...>) noexcept {
	return concatenate_into_sequence(sequence<Content...>{});
}
template<typename... Content, typename... Tail>
constexpr auto CTRE_FORCE_INLINE concatenate_into_sequence(sequence<sequence<Content...>, Tail...>) noexcept {
	return concatenate_into_sequence(sequence<Content...>{});
}

template<typename... Content>
constexpr auto CTRE_FORCE_INLINE concatenate_into_sequence(ctll::list<Content...>) noexcept {
	return sequence<Content...>{};
}
template<typename... Content>
constexpr auto CTRE_FORCE_INLINE concatenate_into_sequence(sequence<Content...>) noexcept {
	return sequence<Content...>{};
}

template<typename... Content>
constexpr bool is_basic_character(ctll::list<Content...>) noexcept {
	return false;
}

template<auto C>
constexpr bool is_basic_character(ctll::list<character<C>>) noexcept {
	return true;
}

template<typename... Content>
constexpr auto get_basic_character(ctll::list<Content...>) noexcept {
	return 0;
}

template<auto C>
constexpr auto get_basic_character(ctll::list<character<C>>) noexcept {
	return C;
}

//recursive helper to extract as many items into a sequence
template<typename... Lead, typename... Content, typename... Tail> constexpr auto first_and_trailing(ctll::list<sequence<Lead...>, select<Content...>, Tail...>) noexcept {
	if constexpr (sizeof...(Content) > 1) {
		using head_element = decltype(ctll::front(ctll::list<Content...>{}));
		using first_pair = decltype(first_and_trailing(ctll::list<head_element>{}));
		using first_element = decltype(first_pair().front);
		using first_trailing = decltype(first_pair().list);

		constexpr bool has_leading_elements = 
			((!is_empty_sequence<decltype(ctll::front(decltype(calculate_first_and_trailing(Content{}).front){}))>) && ...);
		constexpr bool has_same_leading_element =
			(!is_empty_sequence<decltype(ctll::front(first_element{}))>) &&
			((std::is_same_v<first_element, decltype(calculate_first_and_trailing(Content{}).front)>) && ...);

		constexpr bool has_trailing_elements = 
			((!is_empty_sequence<decltype(ctll::front(decltype(calculate_first_and_trailing(Content{}).list){}))>) && ...);
		constexpr bool has_same_trailing_elements =
			((std::is_same_v<first_trailing, decltype(calculate_first_and_trailing(Content{}).list)>) && ...);
		constexpr bool has_no_trailing_elements =
			((is_empty_sequence<decltype(ctll::front(decltype(calculate_first_and_trailing(Content{}).list){}))>) && ...);

		constexpr bool leading_elements_are_characters = 
			((is_basic_character(decltype(calculate_first_and_trailing(Content{}).front){})) && ...);
		constexpr bool leading_elements_are_character_like =
			((MatchesCharacter<
				decltype(ctll::front(decltype(calculate_first_and_trailing(Content{}).front){}))
			>::template value<decltype(*std::declval<::std::string_view::iterator>())>) && ...);

		if constexpr (has_same_leading_element && has_same_trailing_elements) {
			//weird trivial case all select<> branches are the same
			return first_and_trailing(ctll::list<concatenate_into_sequence(sequence<Lead...>{}, head_element{}), Tail...>{});
		} else if constexpr (has_same_leading_element && has_trailing_elements) {
			//first element is the same, we have more select<> to process
			return first_and_trailing(ctll::list<decltype(concatenate_into_sequence(sequence<Lead...>{}, first_element{})),
				select<decltype(concatenate_into_sequence(calculate_first_and_trailing(Content{}).list))...>,
				Tail...
			>{});
		} else if constexpr (has_same_leading_element && has_no_trailing_elements) {
			//first element is the same, no more select<> to process
			return first_and_trailing(ctll::list<decltype(concatenate_into_sequence(sequence<Lead...>{}, first_element{})),
				Tail...
			>{});
		} else if constexpr ((leading_elements_are_characters || leading_elements_are_character_like) && has_trailing_elements && has_same_trailing_elements) {
			//first elements are character-like things, trailing are all the same
			//decltype(concatenate_into_sequence(decltype(concatenate_into_sequence(first_trailing{})), sequence<Tail...>))
			return first_and_trailing(
				ctll::list<sequence<Lead...>, set<
					decltype(ctll::front(decltype(calculate_first_and_trailing(Content{}).front){}))...
				>,
				decltype(concatenate_into_sequence(first_trailing{})),
				Tail...>{}
			);
		} else if constexpr ((leading_elements_are_characters || leading_elements_are_character_like) && has_no_trailing_elements) {
			//first elements are character-like things, no more select<> to process
			return first_and_trailing(ctll::list<sequence<Lead..., set<Content...>>, Tail...>{});
		} else {
			//could not transform select into anything else
			return first_and_trailing(ctll::list<Lead..., select<Content...>, Tail...>{});
		}
	} else {
		if constexpr (sizeof...(Content) == 1) { //trivial cases
			return first_and_trailing(ctll::list<Lead..., Content..., Tail...>{});
		} else { //no branches left -> reject
			return ctll::list_pop_pair<ctll::list<reject>, ctll::list<>>();
		}
	}
}
//base case select
template<typename... Content, typename... Tail> constexpr auto first_and_trailing(ctll::list<select<Content...>, Tail...>) noexcept {
	if constexpr (sizeof...(Content) > 1) {
		using head_element = decltype(ctll::front(ctll::list<Content...>{}));
		using first_pair = decltype(first_and_trailing(ctll::list<head_element>{}));
		using first_element = decltype(first_pair().front);
		using first_trailing = decltype(first_pair().list);

		constexpr bool has_leading_elements = 
			((!is_empty_sequence<decltype(ctll::front(decltype(calculate_first_and_trailing(Content{}).front){}))>) && ...);
		constexpr bool has_same_leading_element =
			((std::is_same_v<first_element, decltype(calculate_first_and_trailing(Content{}).front)>) && ...);

		constexpr bool has_trailing_elements = 
			((!is_empty_sequence<decltype(ctll::front(decltype(calculate_first_and_trailing(Content{}).list){}))>) && ...);
		constexpr bool has_same_trailing_elements =
			((std::is_same_v<first_trailing, decltype(calculate_first_and_trailing(Content{}).list)>) && ...);
		constexpr bool has_no_trailing_elements =
			((is_empty_sequence<decltype(ctll::front(decltype(calculate_first_and_trailing(Content{}).list){}))>) && ...);

		constexpr bool leading_elements_are_characters = 
			((is_basic_character(decltype(calculate_first_and_trailing(Content{}).front){})) && ...);
		constexpr bool leading_elements_are_character_like =
			((MatchesCharacter<
				decltype(ctll::front(decltype(calculate_first_and_trailing(Content{}).front){}))
			>::template value<decltype(*std::declval<::std::string_view::iterator>())>) && ...);

		if constexpr (has_leading_elements && has_same_leading_element && has_same_trailing_elements) {
			//weird trivial case all select<> branches are the same
			return first_and_trailing(ctll::list<head_element, Tail...>{});
		} else if constexpr (has_leading_elements && has_same_leading_element && has_trailing_elements) {
			//first element is the same, we have more select<> to process
			return first_and_trailing(ctll::list<decltype(concatenate_into_sequence(first_element{})),
				select<decltype(concatenate_into_sequence(calculate_first_and_trailing(Content{}).list))...>,
				Tail...
			>{});
		} else if constexpr (has_leading_elements && has_same_leading_element && has_no_trailing_elements) {
			//first element is the same, no more select<> to process
			return first_and_trailing(ctll::list<decltype(concatenate_into_sequence(first_element{})),
				Tail...
			>{});
		} else if constexpr ((leading_elements_are_characters || leading_elements_are_character_like) && has_trailing_elements && has_same_trailing_elements) {
			//first elements are character-like things, trailing are all the same
			//decltype(concatenate_into_sequence(decltype(concatenate_into_sequence(first_trailing{})), sequence<Tail...>))
			return first_and_trailing(
				ctll::list<set<
					decltype(ctll::front(decltype(calculate_first_and_trailing(Content{}).front){}))...
				>,
				decltype(concatenate_into_sequence(first_trailing{})),
				Tail...>{}
			);
		} else if constexpr ((leading_elements_are_characters || leading_elements_are_character_like) && has_no_trailing_elements) {
			//first elements are character-like things, no more select<> to process
			return first_and_trailing(ctll::list<set<Content...>, Tail...>{});
		} else {
			//could not transform select into anything else, return the select element
			return ctll::list_pop_pair<ctll::list<select<Content...>>, ctll::list<Tail...>>();
		}
	} else {
		if constexpr (sizeof...(Content) == 1) { //trivial cases
			return first_and_trailing(ctll::list<sequence<Content...>, Tail...>{});
		} else { //no branches left -> reject
			return ctll::list_pop_pair<ctll::list<reject>, ctll::list<>>();
		}
	}
}

//general case
template<typename T, typename... Tail> constexpr auto first_and_trailing(ctll::list<T, Tail...>) noexcept {
	return ctll::list_pop_pair<ctll::list<T>, ctll::list<Tail...>>();
}

// user facing interface
template<typename... Content> constexpr auto calculate_first_and_trailing(Content...) noexcept {
	if constexpr (sizeof...(Content) > 0ULL)
		return first_and_trailing(ctll::list<Content...>());
	else
		return ctll::list_pop_pair<ctll::list<>, ctll::list<>>();
}

template <auto A, typename CB> constexpr int64_t negative_helper(ctre::character<A>, CB & cb, int64_t start) {
	if (A != std::numeric_limits<int64_t>::min()) {
		if (start < A) {
			cb(start, A-1);
		}
	}
	if (A != std::numeric_limits<int64_t>::max()) {
		return A+1;
	} else {
		return A;
	}
}  

template <auto A, auto B, typename CB> constexpr int64_t negative_helper(ctre::char_range<A,B>, CB & cb, int64_t start) {
	if (A != std::numeric_limits<int64_t>::min()) {
		if (start < A) {
			cb(start, A-1);
		}
	}
	if (B != std::numeric_limits<int64_t>::max()) {
		return B+1;
	} else {
		return B;
	}
}  

template <auto Head, auto... Tail, typename CB> constexpr int64_t negative_helper(ctre::enumeration<Head, Tail...>, CB & cb, int64_t start) {
	int64_t nstart = negative_helper(ctre::character<Head>{}, cb, start);
	return negative_helper(ctre::enumeration<Tail...>{}, cb, nstart);
}

template <typename CB> constexpr int64_t negative_helper(ctre::enumeration<>, CB &, int64_t start) {
	return start;
}

template <typename CB> constexpr int64_t negative_helper(ctre::set<>, CB &, int64_t start) {
	return start;
}

template <auto Property, typename CB> 
constexpr auto negative_helper(ctre::binary_property<Property>, CB &&, int64_t start) {
	return start;
}

template <auto Property, auto Value, typename CB> 
constexpr auto negative_helper(ctre::property<Property, Value>, CB &&, int64_t start) {
	return start;
}

template <typename Head, typename... Rest, typename CB> constexpr int64_t negative_helper(ctre::set<Head, Rest...>, CB & cb, int64_t start) {
	start = negative_helper(Head{}, cb, start);
	return negative_helper(ctre::set<Rest...>{}, cb, start);
}

template <typename Head, typename... Rest, typename CB> constexpr void negative_helper(ctre::negative_set<Head, Rest...>, CB && cb, int64_t start = std::numeric_limits<int64_t>::min()) {
	start = negative_helper(Head{}, cb, start);
	negative_helper(ctre::negative_set<Rest...>{}, std::forward<CB>(cb), start);
}

template <typename CB> constexpr void negative_helper(ctre::negative_set<>, CB && cb, int64_t start = std::numeric_limits<int64_t>::min()) {
	if (start < std::numeric_limits<int64_t>::max()) {
		cb(start, std::numeric_limits<int64_t>::max());
	}
}

// simple fixed set
// TODO: this needs some optimizations
template <size_t Capacity> class point_set {
	struct point {
		int64_t low{};
		int64_t high{};
		constexpr bool operator<(const point & rhs) const {
			return low < rhs.low;
		}
		constexpr point() { }
		constexpr point(int64_t l, int64_t h): low{l}, high{h} { }
	};
	point points[Capacity+1]{};
	size_t used{0};
	constexpr point * begin() {
		return points;
	}
	constexpr point * begin() const {
		return points;
	}
	constexpr point * end() {
		return points + used;
	}
	constexpr point * end() const {
		return points + used;
	}
	constexpr point * lower_bound(point obj) {
		auto first = begin();
		auto last = end();
		auto it = first;
		size_t count = std::distance(first, last);
		while (count > 0) {
			it = first;
			size_t step = count / 2;
			std::advance(it, step);
			if (*it < obj) {
				first = ++it;
				count -= step + 1;
			} else {
				count = step;
			}
		}
		return it;
	}
	constexpr point * insert_point(int64_t position, int64_t other) {
		point obj{position, other};
		auto it = lower_bound(obj);
		if (it == end()) {
			*it = obj;
			used++;
			return it;
		} else {
			auto out = it;
			auto e = end();
			while (it != e) {
				auto tmp = *it;
				*it = obj;
				obj = tmp;
				//std::swap(*it, obj);
				it++;
			}
			auto tmp = *it;
			*it = obj;
			obj = tmp;
			//std::swap(*it, obj);
			
			used++;
			return out;
		}
	}
public:
	constexpr point_set() { }
	constexpr void insert(int64_t low, int64_t high) {
		insert_point(low, high);
		//insert_point(high, low);
	}
	constexpr bool check(int64_t low, int64_t high) {
		for (auto r: *this) {
			if (r.low <= low && low <= r.high) {
				return true;
			} else if (r.low <= high && high <= r.high) {
				return true;
			} else if (low <= r.low && r.low <= high) {
				return true;
			} else if (low <= r.high && r.high <= high) {
				return true;
			}
		}
		return false;
	}
	
	
	template <auto V> constexpr bool check(ctre::character<V>) {
		return check(V,V);
	}
	template <auto A, auto B> constexpr bool check(ctre::char_range<A,B>) {
		return check(A,B);
	}
	constexpr bool check(can_be_anything) {
		return used > 0;
	}
	template <typename... Content> constexpr bool check(ctre::negative_set<Content...> nset) {
		bool collision = false;
		negative_helper(nset, [&](int64_t low, int64_t high){
			collision |= this->check(low, high);
		});
		return collision;
	}
	template <auto... V> constexpr bool check(ctre::enumeration<V...>) {
		
		return (check(V,V) || ... || false);
	}
	template <typename... Content> constexpr bool check(ctll::list<Content...>) {
		return (check(Content{}) || ... || false);
	}
	template <typename... Content> constexpr bool check(ctre::set<Content...>) {
		return (check(Content{}) || ... || false);
	}
	
	
	template <auto V> constexpr void populate(ctre::character<V>) {
		insert(V,V);
	}
	template <auto A, auto B> constexpr void populate(ctre::char_range<A,B>) {
		insert(A,B);
	}
	constexpr void populate(can_be_anything) {
		insert(std::numeric_limits<int64_t>::min(), std::numeric_limits<int64_t>::max());
	}
	template <typename... Content> constexpr void populate(ctre::negative_set<Content...> nset) {
		negative_helper(nset, [&](int64_t low, int64_t high){
			this->insert(low, high);
		});
	}
	template <typename... Content> constexpr void populate(ctre::set<Content...>) {
		(populate(Content{}), ...);
	}
	template <typename... Content> constexpr void populate(ctll::list<Content...>) {
		(populate(Content{}), ...);
	}
};

template <typename... A, typename... B> constexpr bool collides(ctll::list<A...> rhs, ctll::list<B...> lhs) {
	constexpr size_t capacity = calculate_size_of_first(rhs);
	
	point_set<capacity> set;
	set.populate(rhs);
	
	return set.check(lhs);
}


}

#endif
