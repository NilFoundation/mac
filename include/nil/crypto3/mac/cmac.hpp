//---------------------------------------------------------------------------//
// Copyright (c) 2019 Nil Foundation AG
// Copyright (c) 2019 Mikhail Komarov <nemo@nil.foundation>
//
// Distributed under the Boost Software License, Version 1.0
// See accompanying file LICENSE_1_0.txt or copy at
// http://www.boost.org/LICENSE_1_0.txt
//---------------------------------------------------------------------------//

#ifndef CRYPTO3_MAC_CMAC_HPP
#define CRYPTO3_MAC_CMAC_HPP

#include <nil/crypto3/detail/poly_dbl.hpp>

#include <nil/crypto3/mac/detail/cmac/cmac_policy.hpp>
#include <nil/crypto3/mac/detail/cmac/accumulator.hpp>

namespace nil {
    namespace crypto3 {
        namespace mac {
            /*!
             * @brief CMAC, also known as OMAC1
             * @tparam BlockCipher
             * @ingroup mac
             */
            template<typename BlockCipher>
            class cmac {
                typedef detail::cmac_policy<BlockCipher> policy_type;

            public:
                typedef BlockCipher cipher_type;

                constexpr static const std::size_t block_bits = policy_type::block_bits;
                constexpr static const std::size_t block_words = policy_type::block_words;
                typedef typename policy_type::block_type block_type;

                constexpr static const std::size_t key_bits = policy_type::key_bits;
                constexpr static const std::size_t key_words = policy_type::key_words;
                typedef typename policy_type::key_type key_type;

                constexpr static const std::size_t digest_bits = policy_type::digest_bits;
                typedef typename policy_type::digest_type digest_type;

                cmac(const cipher_type &cipher) : cipher(cipher) {
                    BOOST_STATIC_ASSERT(poly_double_supported_size(cipher_type::block_bits / CHAR_BIT));
                }

                cmac(const key_type &key) : cipher(key) {

                }

                inline void begin_message(const block_type &block) const {
                    return process_block(block);
                }

                inline void process_block(const block_type &block) const {
                    const size_t bs = output_length();

                    buffer_insert(m_buffer, m_position, input, length);
                    if (m_position + length > bs) {
                        xor_buf(m_state, m_buffer, bs);
                        m_cipher->encrypt(m_state);
                        input += (bs - m_position);
                        length -= (bs - m_position);
                        while (length > bs) {
                            xor_buf(m_state, input, bs);
                            m_cipher->encrypt(m_state);
                            input += bs;
                            length -= bs;
                        }
                        copy_mem(m_buffer.data(), input, length);
                        m_position = 0;
                    }
                    m_position += length;
                }

                inline void end_message(const block_type &block) const {
                    xor_buf(m_state, m_buffer, m_position);

                    if (m_position == output_length()) {
                        xor_buf(m_state, m_B, output_length());
                    } else {
                        m_state[m_position] ^= 0x80;
                        xor_buf(m_state, m_P, output_length());
                    }

                    m_cipher->encrypt(m_state);

                    copy_mem(mac, m_state.data(), output_length());

                    zeroise(m_state);
                    zeroise(m_buffer);
                    m_position = 0;
                }

            protected:
                void schedule_key(const key_type &key) {
                    using namespace nil::crypto3::detail;

                    cipher.encrypt_block(m_B);
                    poly_double_n(m_B.data(), m_B.size());
                    poly_double_n(m_P.data(), m_B.data(), m_P.size());
                }

                cipher_type cipher;
            };

            template<typename BlockCipher>
            using omac1 = cmac<BlockCipher>;
        }    // namespace mac
    }    // namespace crypto3
}    // namespace nil

#endif