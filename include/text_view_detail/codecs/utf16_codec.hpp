// Copyright (c) 2017, Tom Honermann
//
// This file is distributed under the MIT License. See the accompanying file
// LICENSE.txt or http://www.opensource.org/licenses/mit-license.php for terms
// and conditions.

#if !defined(TEXT_VIEW_CODECS_UTF16_CODEC_HPP) // {
#define TEXT_VIEW_CODECS_UTF16_CODEC_HPP


#include <climits>
#include <text_view_detail/codecs/codec_util.hpp>
#include <text_view_detail/concepts.hpp>
#include <text_view_detail/error_status.hpp>
#include <text_view_detail/character.hpp>
#include <text_view_detail/trivial_encoding_state.hpp>


namespace std {
namespace experimental {
inline namespace text {
namespace text_detail {


template<typename CT, typename CUT,
CONCEPT_REQUIRES_(
    Character<CT>(),
    CodeUnit<CUT>())>
class utf16_codec {
public:
    using state_type = trivial_encoding_state;
    using state_transition_type = trivial_encoding_state_transition;
    using character_type = CT;
    using code_unit_type = CUT;
    static constexpr int min_code_units = 1;
    static constexpr int max_code_units = 2;

    static_assert(sizeof(code_unit_type) * CHAR_BIT >= 16, "");

    template<typename CUIT,
    CONCEPT_REQUIRES_(CodeUnitOutputIterator<CUIT, code_unit_type>())>
    static encode_status encode_state_transition(
        state_type &state,
        CUIT &out,
        const state_transition_type &stt,
        int &encoded_code_units)
    noexcept
    {
        encoded_code_units = 0;

        return encode_status::no_error;
    }

    template<typename CUIT,
    CONCEPT_REQUIRES_(CodeUnitOutputIterator<CUIT, code_unit_type>())>
    static encode_status encode(
        state_type &state,
        CUIT &out,
        character_type c,
        int &encoded_code_units)
    noexcept(text_detail::NoExceptOutputIterator<CUIT, code_unit_type>())
    {
        encoded_code_units = 0;

        using code_point_type =
            code_point_type_t<character_set_type_t<character_type>>;
        code_point_type cp{c.get_code_point()};

        if (cp >= 0xD800 && cp <= 0xDFFF) {
            return encode_status::invalid_character;
        }

        if (cp <= 0xFFFF) {
            *out++ = code_unit_type(cp);
            ++encoded_code_units;
        } else {
            *out++ = code_unit_type(0xD800 + (((cp - 0x10000) >> 10) & 0x03FF));
            ++encoded_code_units;
            *out++ = code_unit_type(0xDC00 + ((cp - 0x10000) & 0x03FF));
            ++encoded_code_units;
        }

        return encode_status::no_error;
    }

    template<typename CUIT, typename CUST,
    CONCEPT_REQUIRES_(
        CodeUnitIterator<CUIT>(),
        ranges::ForwardIterator<CUIT>(),
        ranges::ConvertibleTo<ranges::value_type_t<CUIT>, code_unit_type>(),
        ranges::Sentinel<CUST, CUIT>())>
    static decode_status decode(
        state_type &state,
        CUIT &in_next,
        CUST in_end,
        character_type &c,
        int &decoded_code_units)
    noexcept(text_detail::NoExceptInputIterator<CUIT, CUST>())
    {
        decoded_code_units = 0;

        using code_point_type =
            code_point_type_t<character_set_type_t<character_type>>;
        code_point_type cp;

        if (in_next == in_end)
            return decode_status::underflow;
        code_unit_type cu1 = *in_next++;
        ++decoded_code_units;
        if (cu1 >= 0xD800 && cu1 <= 0xDBFF) {
            if (in_next == in_end)
                return decode_status::underflow;
            code_unit_type cu2 = *in_next++;
            ++decoded_code_units;
            if (cu2 < 0xDC00 || cu2 > 0xDFFF) {
                return decode_status::invalid_code_unit_sequence;
            }
            cp = 0x10000 + (((cu1 & 0x3FF) << 10) | (cu2 & 0x3FF));
            c.set_code_point(cp);
        } else if (cu1 >= 0xDC00 && cu1 <= 0xDFFF) {
            return decode_status::invalid_code_unit_sequence;
        } else {
            cp = cu1;
            c.set_code_point(cp);
        }

        return decode_status::no_error;
    }

    template<typename CUIT, typename CUST,
    CONCEPT_REQUIRES_(
        CodeUnitIterator<CUIT>(),
        ranges::ForwardIterator<CUIT>(),
        ranges::ConvertibleTo<ranges::value_type_t<CUIT>, code_unit_type>(),
        ranges::Sentinel<CUST, CUIT>())>
    static decode_status rdecode(
        state_type &state,
        CUIT &in_next,
        CUST in_end,
        character_type &c,
        int &decoded_code_units)
    noexcept(text_detail::NoExceptInputIterator<CUIT, CUST>())
    {
        decoded_code_units = 0;

        using code_point_type =
            code_point_type_t<character_set_type_t<character_type>>;
        code_point_type cp;

        if (in_next == in_end)
            return decode_status::underflow;
        code_unit_type rcu1 = *in_next++;
        ++decoded_code_units;
        if (rcu1 >= 0xDC00 && rcu1 <= 0xDFFF) {
            if (in_next == in_end)
                return decode_status::underflow;
            code_unit_type rcu2 = *in_next++;
            ++decoded_code_units;
            if (rcu2 < 0xD800 || rcu2 > 0xDBFF) {
                return decode_status::invalid_code_unit_sequence;
            }
            cp = 0x10000 + (((rcu2 & 0x3FF) << 10) | (rcu1 & 0x3FF));
            c.set_code_point(cp);
        } else if (rcu1 >= 0xD800 && rcu1 <= 0xDBFF) {
            return decode_status::invalid_code_unit_sequence;
        } else {
            cp = rcu1;
            c.set_code_point(cp);
        }

        return decode_status::no_error;
    }
};


} // namespace text_detail
} // inline namespace text
} // namespace experimental
} // namespace std


#endif // } TEXT_VIEW_CODECS_UTF16_CODEC_HPP
