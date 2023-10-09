/*
 * Stormphrax, a UCI chess engine
 * Copyright (C) 2023 Ciekce
 *
 * Stormphrax is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Stormphrax is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with Stormphrax. If not, see <https://www.gnu.org/licenses/>.
 */

#pragma once

#include <cstdint>
#include <array>
#include <vector>
#include <utility>
#include <span>
#include <cstring>
#include <algorithm>
#include <cassert>
#include <string>
#include <string_view>

#include "globals.h"

namespace stormphrax::eval
{
    // current arch: (768->768)x2->1, ClippedReLU

    constexpr std::uint32_t InputSize = 768;
    constexpr std::uint32_t Layer1Size = 768;

    constexpr std::int32_t Scale = 400;

    constexpr std::int32_t Q = 255 * 64;

    struct Network
    {
        alignas(32) std::array<std::int16_t, InputSize * Layer1Size> featureWeights;
        alignas(32) std::array<std::int16_t, Layer1Size> featureBiases;
        alignas(32) std::array<std::int16_t, Layer1Size * 2> outputWeights;
        std::int16_t outputBias;
    };

    extern const Network *g_nnue;

    // Perspective network - separate accumulators for
    // each side to allow the network to learn tempo
    // (this is why there are two sets of output weights)
    struct Accumulator
    {
        alignas(32) std::array<std::int16_t, Layer1Size> black;
        alignas(32) std::array<std::int16_t, Layer1Size> white;

        inline auto init(std::span<const std::int16_t, Layer1Size> bias)
        {
            std::copy(bias.begin(), bias.end(), black.begin());
            std::copy(bias.begin(), bias.end(), white.begin());
        }
    };

    class NnueState
    {
    public:
        NnueState()
        {
            m_accumulatorStack.reserve(256);
        }

        NnueState(const NnueState &other)
            : m_accumulatorStack{other.m_accumulatorStack}
        {
            m_accumulatorStack.reserve(256);
            m_curr = &m_accumulatorStack.back();
        }

        inline auto operator=(const NnueState &other) -> NnueState &
        {
            if (&other == this)
                return *this;

            m_accumulatorStack = other.m_accumulatorStack;
            m_accumulatorStack.reserve(256);
            m_curr = &m_accumulatorStack.back();

            return *this;
        }

        inline auto push()
        {
            assert(m_curr == &m_accumulatorStack.back());

            m_accumulatorStack.push_back(*m_curr);
            m_curr = &m_accumulatorStack.back();
        }

        inline auto pop()
        {
            m_accumulatorStack.pop_back();
            m_curr = &m_accumulatorStack.back();
        }

        inline auto reset()
        {
            m_accumulatorStack.clear();
            m_curr = &m_accumulatorStack.emplace_back();

            m_curr->init(g_nnue->featureBiases);
        }

        inline auto moveFeature(int piece, int src, int dst)
        {
            assert(m_curr == &m_accumulatorStack.back());
            moveFeature(*m_curr, piece, src, dst);
        }

        inline auto activateFeature(int piece, int sq)
        {
            assert(m_curr == &m_accumulatorStack.back());
            activateFeature(*m_curr, piece, sq);
        }

        inline auto deactivateFeature(int piece, int sq)
        {
            assert(m_curr == &m_accumulatorStack.back());
            deactivateFeature(*m_curr, piece, sq);
        }

        [[nodiscard]] inline auto evaluate(int stm) const
        {
            assert(m_curr == &m_accumulatorStack.back());
            return evaluate(*m_curr, stm);
        }

    private:
        std::vector<Accumulator> m_accumulatorStack{};
        Accumulator *m_curr{};

        static inline auto moveFeature(Accumulator &accumulator, int piece, int src, int dst) -> void
        {
            const auto [blackSrc, whiteSrc] = featureIndices(piece, src);
            const auto [blackDst, whiteDst] = featureIndices(piece, dst);

            subtractAndAddToAll(accumulator.black, g_nnue->featureWeights, blackSrc * Layer1Size, blackDst * Layer1Size);
            subtractAndAddToAll(accumulator.white, g_nnue->featureWeights, whiteSrc * Layer1Size, whiteDst * Layer1Size);
        }

        static inline auto activateFeature(Accumulator &accumulator, int piece, int sq) -> void
        {
            const auto [blackIdx, whiteIdx] = featureIndices(piece, sq);

            addToAll(accumulator.black, g_nnue->featureWeights, blackIdx * Layer1Size);
            addToAll(accumulator.white, g_nnue->featureWeights, whiteIdx * Layer1Size);
        }

        static inline auto deactivateFeature(Accumulator &accumulator, int piece, int sq) -> void
        {
            const auto [blackIdx, whiteIdx] = featureIndices(piece, sq);

            subtractFromAll(accumulator.black, g_nnue->featureWeights, blackIdx * Layer1Size);
            subtractFromAll(accumulator.white, g_nnue->featureWeights, whiteIdx * Layer1Size);
        }

        [[nodiscard]] static inline auto evaluate(const Accumulator &accumulator, int stm) -> std::int32_t
        {
            const auto output = stm == 0 /* black */
                ? forward(accumulator.black, accumulator.white, g_nnue->outputWeights)
                : forward(accumulator.white, accumulator.black, g_nnue->outputWeights);
            return (output + g_nnue->outputBias) * Scale / Q;
        }

        template <std::size_t Size>
        static inline auto subtractAndAddToAll(std::array<std::int16_t, Size> &input,
            std::span<const std::int16_t> delta, std::uint32_t subOffset, std::uint32_t addOffset) -> void
        {
            assert(subOffset + Size <= delta.size());
            assert(addOffset + Size <= delta.size());

            for (std::uint32_t i = 0; i < Size; ++i)
            {
                input[i] += delta[addOffset + i] - delta[subOffset + i];
            }
        }

        template <std::size_t Size>
        static inline auto addToAll(std::array<std::int16_t, Size> &input,
            std::span<const std::int16_t> delta, std::uint32_t offset) -> void
        {
            assert(offset + Size <= delta.size());

            for (std::uint32_t i = 0; i < Size; ++i)
            {
                input[i] += delta[offset + i];
            }
        }

        template <std::size_t Size>
        static inline auto subtractFromAll(std::array<std::int16_t, Size> &input,
            std::span<const std::int16_t> delta, std::uint32_t offset) -> void
        {
            assert(offset + Size <= delta.size());

            for (std::uint32_t i = 0; i < Size; ++i)
            {
                input[i] -= delta[offset + i];
            }
        }

        [[nodiscard]] static inline auto featureIndices(int piece, int sq) -> std::pair<std::uint32_t, std::uint32_t>
        {
            assert(piece != None);
            assert(sq != 64);

            constexpr std::uint32_t ColorStride = 64 * 6;
            constexpr std::uint32_t PieceStride = 64;

            const auto type = static_cast<std::uint32_t>(getType(piece));
            const std::uint32_t color = getColor(piece) == 1 /* white */ ? 0 : 1;

            const auto blackIdx = !color * ColorStride + type * PieceStride + (static_cast<std::uint32_t>(sq) ^ 0x38);
            const auto whiteIdx =  color * ColorStride + type * PieceStride +  static_cast<std::uint32_t>(sq)        ;

            return {blackIdx, whiteIdx};
        }

        [[nodiscard]] static inline auto forward(
            std::span<const std::int16_t, Layer1Size> us,
            std::span<const std::int16_t, Layer1Size> them,
            std::span<const std::int16_t, Layer1Size * 2> weights
        ) -> std::int32_t
        {
            std::int32_t sum = 0;

            for (std::size_t i = 0; i < Layer1Size; ++i)
            {
                const auto activated = std::clamp(static_cast<std::int32_t>(us[i]), 0, 255);
                sum += activated * weights[i];
            }

            for (std::size_t i = 0; i < Layer1Size; ++i)
            {
                const auto activated = std::clamp(static_cast<std::int32_t>(them[i]), 0, 255);
                sum += activated * weights[Layer1Size + i];
            }

            return sum;
        }
    };
}
