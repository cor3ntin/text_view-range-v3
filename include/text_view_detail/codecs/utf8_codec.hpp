// Copyright (c) 2016, Tom Honermann
//
// This file is distributed under the MIT License. See the accompanying file
// LICENSE.txt or http://www.opensource.org/licenses/mit-license.php for terms
// and conditions.

#if !defined(TEXT_VIEW_CODECS_UTF8_CODEC_HPP) // {
#define TEXT_VIEW_CODECS_UTF8_CODEC_HPP


#include <climits>
#include <text_view_detail/concepts.hpp>
#include <text_view_detail/exceptions.hpp>
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
class utf8_codec {
public:
    using state_type = trivial_encoding_state;
    using state_transition_type = trivial_encoding_state_transition;
    using character_type = CT;
    using code_unit_type = CUT;
    static constexpr int min_code_units = 1;
    static constexpr int max_code_units = 4;

    static_assert(sizeof(code_unit_type) * CHAR_BIT >= 8, "");

    template<typename CUIT,
    CONCEPT_REQUIRES_(
        CodeUnitOutputIterator<
            CUIT,
            typename std::make_unsigned<code_unit_type>::type>())>
    static void encode_state_transition(
        state_type &state,
        CUIT &out,
        const state_transition_type &stt,
        int &encoded_code_units)
    {
        encoded_code_units = 0;
    }

    template<typename CUIT,
    CONCEPT_REQUIRES_(
        CodeUnitOutputIterator<
            CUIT,
            typename std::make_unsigned<code_unit_type>::type>())>
    static void encode(
        state_type &state,
        CUIT &out,
        character_type c,
        int &encoded_code_units)
    {
        encoded_code_units = 0;

        using unsigned_code_unit_type =
            typename std::make_unsigned<code_unit_type>::type;
        using code_point_type =
            code_point_type_t<character_set_type_t<character_type>>;
        code_point_type cp{c.get_code_point()};

        if (cp <= 0x0000007F) {
            *out++ = unsigned_code_unit_type(cp);
            ++encoded_code_units;
        } else if (cp <= 0x000007FF) {
            *out++ = unsigned_code_unit_type(0xC0 + ((cp >> 6) & 0x1F));
            ++encoded_code_units;
            *out++ = unsigned_code_unit_type(0x80 + (cp & 0x3F));
            ++encoded_code_units;
        } else if (cp <= 0x0000D7FF) {
            *out++ = unsigned_code_unit_type(0xE0 + ((cp >> 12) & 0x0F));
            ++encoded_code_units;
            *out++ = unsigned_code_unit_type(0x80 + ((cp >> 6) & 0x3F));
            ++encoded_code_units;
            *out++ = unsigned_code_unit_type(0x80 + (cp & 0x3F));
            ++encoded_code_units;
        } else if (cp <= 0x0000E000) {
            throw text_encode_error("Invalid Unicode code point");
        } else if (cp <= 0x0000FFFD) {
            *out++ = unsigned_code_unit_type(0xE0 + ((cp >> 12) & 0x0F));
            ++encoded_code_units;
            *out++ = unsigned_code_unit_type(0x80 + ((cp >> 6) & 0x3F));
            ++encoded_code_units;
            *out++ = unsigned_code_unit_type(0x80 + (cp & 0x3F));
            ++encoded_code_units;
        } else if (cp <= 0x0000FFFF) {
            throw text_encode_error("Invalid Unicode code point");
        } else if (cp <= 0x0010FFFF) {
            *out++ = unsigned_code_unit_type(0xF0 + ((cp >> 18) & 0x07));
            ++encoded_code_units;
            *out++ = unsigned_code_unit_type(0x80 + ((cp >> 12) & 0x3F));
            ++encoded_code_units;
            *out++ = unsigned_code_unit_type(0x80 + ((cp >> 6) & 0x3F));
            ++encoded_code_units;
            *out++ = unsigned_code_unit_type(0x80 + (cp & 0x3F));
            ++encoded_code_units;
        } else {
            throw text_encode_error("Invalid Unicode code point");
        }
    }

    template<typename CUIT, typename CUST,
    CONCEPT_REQUIRES_(
        CodeUnitIterator<CUIT>(),
        ranges::InputIterator<CUIT>(),
        ranges::ConvertibleTo<
            ranges::value_type_t<CUIT>,
            typename std::make_unsigned<code_unit_type>::type>(),
        ranges::Sentinel<CUST, CUIT>())>
    static bool decode(
        state_type &state,
        CUIT &in_next,
        CUST in_end,
        character_type &c,
        int &decoded_code_units)
    {
        decoded_code_units = 0;

        using unsigned_code_unit_type =
            typename std::make_unsigned<code_unit_type>::type;
        using code_point_type =
            code_point_type_t<character_set_type_t<character_type>>;
        code_point_type cp;

        if (in_next == in_end)
            throw text_decode_underflow_error("text decode underflow error");
        unsigned_code_unit_type cu1 = *in_next++;
        ++decoded_code_units;
        if (cu1 <= 0x7F) {
            cp = (cu1 & 0x7F);
            c.set_code_point(cp);
            return true;
        }

        if (in_next == in_end)
            throw text_decode_underflow_error("text decode underflow error");
        unsigned_code_unit_type cu2 = *in_next++;
        ++decoded_code_units;
        if (!(cu2 & 0x80)) {
            throw text_decode_error("Invalid UTF-8 code unit sequence");
        }
        if (((cu1 & 0xE0) == 0xC0) &&
            ((cu2 & 0xC0) == 0x80))
        {
            cp = ((cu1 & 0x1F) << 6) + (cu2 & 0x3F);
            c.set_code_point(cp);
            return true;
        }

        if (in_next == in_end)
            throw text_decode_underflow_error("text decode underflow error");
        unsigned_code_unit_type cu3 = *in_next++;
        ++decoded_code_units;
        if (!(cu3 & 0x80)) {
            throw text_decode_error("Invalid UTF-8 code unit sequence");
        }
        if (((cu1 & 0xF0) == 0xE0) &&
            ((cu2 & 0xC0) == 0x80) &&
            ((cu3 & 0xC0) == 0x80))
        {
            cp = ((cu1 & 0x0F) << 12) + ((cu2 & 0x3F) << 6) + (cu3 & 0x3F);
            c.set_code_point(cp);
            return true;
        }

        if (in_next == in_end)
            throw text_decode_underflow_error("text decode underflow error");
        unsigned_code_unit_type cu4 = *in_next++;
        ++decoded_code_units;
        if (!(cu4 & 0x80)) {
            throw text_decode_error("Invalid UTF-8 code unit sequence");
        }
        if (((cu1 & 0xF8) == 0xF0) &&
            ((cu2 & 0xC0) == 0x80) &&
            ((cu3 & 0xC0) == 0x80) &&
            ((cu4 & 0xC0) == 0x80))
        {
            cp = ((cu1 & 0x07) << 18) +
                 ((cu2 & 0x3F) << 12) +
                 ((cu3 & 0x3F) << 6) +
                  (cu4 & 0x3F);
            c.set_code_point(cp);
            return true;
        }

        throw text_decode_error("Invalid UTF-8 code unit sequence");
    }

    template<typename CUIT, typename CUST,
    CONCEPT_REQUIRES_(
        CodeUnitIterator<CUIT>(),
        ranges::InputIterator<CUIT>(),
        ranges::ConvertibleTo<
            ranges::value_type_t<CUIT>,
            typename std::make_unsigned<code_unit_type>::type>(),
        ranges::Sentinel<CUST, CUIT>())>
    static bool rdecode(
        state_type &state,
        CUIT &in_next,
        CUST in_end,
        character_type &c,
        int &decoded_code_units)
    {
        decoded_code_units = 0;

        using unsigned_code_unit_type =
            typename std::make_unsigned<code_unit_type>::type;
        using code_point_type =
            code_point_type_t<character_set_type_t<character_type>>;
        code_point_type cp;

        if (in_next == in_end)
            throw text_decode_underflow_error("text decode underflow error");
        unsigned_code_unit_type rcu1 = *in_next++;
        ++decoded_code_units;
        if (rcu1 <= 0x7F) {
            cp = (rcu1 & 0x7F);
            c.set_code_point(cp);
            return true;
        }

        if (in_next == in_end)
            throw text_decode_underflow_error("text decode underflow error");
        unsigned_code_unit_type rcu2 = *in_next++;
        ++decoded_code_units;
        if (!(rcu2 & 0x80)) {
            throw text_decode_error("Invalid UTF-8 code unit sequence");
        }
        if (rcu2 & 0x40) {
            cp = ((rcu2 & 0x1F) << 6) + (rcu1 & 0x3F);
            c.set_code_point(cp);
            return true;
        }

        if (in_next == in_end)
            throw text_decode_underflow_error("text decode underflow error");
        unsigned_code_unit_type rcu3 = *in_next++;
        ++decoded_code_units;
        if (!(rcu3 & 0x80)) {
            throw text_decode_error("Invalid UTF-8 code unit sequence");
        }
        if (rcu3 & 0x40) {
            cp = ((rcu3 & 0x0F) << 12) + ((rcu2 & 0x3F) << 6) + (rcu1 & 0x3F);
            c.set_code_point(cp);
            return true;
        }

        if (in_next == in_end)
            throw text_decode_underflow_error("text decode underflow error");
        unsigned_code_unit_type rcu4 = *in_next++;
        ++decoded_code_units;
        if (!(rcu4 & 0x80)) {
            throw text_decode_error("Invalid UTF-8 code unit sequence");
        }
        if (rcu4 & 0x40) {
            cp = ((rcu4 & 0x07) << 18) +
                 ((rcu3 & 0x3F) << 12) +
                 ((rcu2 & 0x3F) << 6) +
                  (rcu1 & 0x3F);
            c.set_code_point(cp);
            return true;
        }

        throw text_decode_error("Invalid UTF-8 code unit sequence");
    }
};


} // namespace text_detail
} // inline namespace text
} // namespace experimental
} // namespace std


#endif // } TEXT_VIEW_CODECS_UTF8_CODEC_HPP
