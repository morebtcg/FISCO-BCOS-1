#pragma once

#include "Exceptions.h"
#include "interfaces/crypto/Concepts.h"
#include <bcos-crypto/interfaces/crypto/hasher/Hasher.h>
#include <bits/ranges_algo.h>
#include <boost/format.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/serialization/split_member.hpp>
#include <boost/throw_exception.hpp>
#include <algorithm>
#include <exception>
#include <iterator>
#include <ranges>
#include <stdexcept>
#include <type_traits>

namespace bcos::tool
{

template <class Range, class HashType>
concept InputRange =
    std::ranges::random_access_range<Range> && bcos::crypto::TrivialObject<HashType> &&
    std::is_same_v<std::remove_cvref_t<std::ranges::range_value_t<Range>>, HashType>;

template <class Range, class HashType>
concept OutputRange = std::ranges::random_access_range<Range> &&
    std::ranges::output_range<Range, HashType> && bcos::crypto::TrivialObject<HashType>;

template <class Range, class HashType>
concept Proof =
    std::ranges::input_range<Range> && std::is_same_v<std::ranges::range_value_t<Range>, HashType>;

template <bcos::crypto::Hasher HasherType, class HashType, size_t width = 2>
class Merkle
{
public:
    static_assert(width >= 2, "Width too short, at least 2");

    Merkle() = default;
    Merkle(const Merkle&) = default;
    Merkle(Merkle&&) = default;
    Merkle& operator=(const Merkle&) = default;
    Merkle& operator=(Merkle&&) = default;
    ~Merkle() = default;

    struct Proof
    {
        std::vector<HashType> hashes;
        std::vector<size_t> levels;

        BOOST_SERIALIZATION_SPLIT_MEMBER()

        template <typename Archive>
        void load(Archive& ar, [[maybe_unused]] const unsigned int version)
        {
            size_t inWidth = 0;
            ar& inWidth;

            checkWidth(inWidth);

            ar& hashes;
            ar& levels;
        }

        template <typename Archive>
        void save(Archive& ar, [[maybe_unused]] const unsigned int version) const
        {
            ar& width;
            ar& hashes;
            ar& levels;
        }

        friend std::ostream& operator<<(std::ostream& stream, const Proof& proof)
        {
            auto range = std::ranges::subrange(proof.hashes.begin(), proof.hashes.begin());
            size_t level = 0;
            for (auto length : proof.levels)
            {
                range = {std::end(range), std::end(range) + length};
                stream << "Level " << level;
                for (auto& hash : range)
                {
                    stream << " " << hash;
                }
                stream << std::endl;
                ++level;
            }
            return stream;
        }
    };

    static bool verifyProof(const Proof& proof, HashType hash, const HashType& root)
    {
        if (proof.hashes.empty() || proof.levels.empty()) [[unlikely]]
            BOOST_THROW_EXCEPTION(std::invalid_argument{"Empty input proof!"});

        auto range = std::ranges::subrange{proof.hashes.begin(), proof.hashes.begin()};
        HasherType hasher;
        for (auto length : proof.levels)
        {
            range = {std::end(range), std::end(range) + length};
            if (std::end(range) >= proof.hashes.end() || std::size(range) > width) [[unlikely]]
                BOOST_THROW_EXCEPTION(std::invalid_argument{"Proof level length out of range!"});

            if (std::ranges::find(range, hash) == std::end(range)) [[unlikely]]
                return false;

            for (auto& rangeHash : range)
            {
                hasher.update(rangeHash);
            }
            hash = hasher.final();
        }

        if (hash != root) [[unlikely]]
        {
            return false;
        }

        return true;
    }

    Proof generateProof(const HashType& hash) const
    {
        if (empty()) [[unlikely]]
            BOOST_THROW_EXCEPTION(std::runtime_error{"Empty merkle!"});

        // Query the first level hashes(ordered)
        auto range = std::ranges::subrange{m_nodes.begin(), m_nodes.begin() + m_levels[0]};
        auto it = std::ranges::lower_bound(range, hash);
        if (it == range.end() || *it != hash) [[unlikely]]
            BOOST_THROW_EXCEPTION(std::invalid_argument{"Not found hash in merkle!"});

        auto index = it - std::begin(range);
        auto start = range.begin() + ((index / width) * width);
        auto end = std::min(start + width, range.end());

        Proof proof;
        proof.hashes.reserve(m_levels.size() * width);
        proof.levels.reserve(m_levels.size());

        proof.hashes.insert(proof.hashes.end(), start, end);
        proof.levels.push_back(end - start);

        // Query next level hashes
        for (auto depth : std::ranges::iota_view{decltype(m_levels.size())(1), m_levels.size()})
        {
            auto length = m_levels[depth];
            index = index / width;
            range = std::ranges::subrange{range.end(), range.end() + length};

            auto start = length <= width ? range.begin() : range.begin() + index;
            auto end = std::min(start + width, range.end());

            assert(range.end() <= m_nodes.end());
            proof.hashes.insert(proof.hashes.end(), start, end);
            proof.levels.push_back(end - start);
        }

        return proof;
    }

    HashType root() const
    {
        if (empty()) [[unlikely]]
            BOOST_THROW_EXCEPTION(std::runtime_error{"Empty merkle!"});

        return *m_nodes.rbegin();
    }

    void import(InputRange<HashType> auto&& input)
    {
        if (std::empty(input)) [[unlikely]]
            BOOST_THROW_EXCEPTION(std::invalid_argument{"Empty input"});

        auto inputSize = std::size(input);
        m_nodes.resize(getNodeSize(inputSize));

        std::copy_n(std::begin(input), inputSize, m_nodes.begin());
        std::sort(m_nodes.begin(), m_nodes.begin() + inputSize);

        auto inputRange = std::ranges::subrange{m_nodes.begin(), m_nodes.begin()};
        while (inputSize > 1)  // Ignore only root
        {
            inputRange = {inputRange.end(), inputRange.end() + inputSize};

            assert(inputRange.end() <= m_nodes.end());
            m_levels.push_back(inputSize);

            inputSize = calculateLevelHashes(
                inputRange, std::ranges::subrange{inputRange.end(), m_nodes.end()});
        }
    }

    void clear()
    {
        m_nodes.clear();
        m_levels.clear();
    }

    auto empty() const { return m_nodes.empty() || m_levels.empty(); }

    BOOST_SERIALIZATION_SPLIT_MEMBER()

    template <typename Archive>
    void load(Archive& ar, [[maybe_unused]] const unsigned int version)
    {
        decltype(width) inWidth;
        ar& inWidth;
        checkDepth<HasherType, HashType, width>(inWidth);

        ar& m_nodes;
        ar& m_levels;
    }

    template <typename Archive>
    void save(Archive& ar, [[maybe_unused]] const unsigned int version) const
    {
        ar& width;
        ar& m_nodes;
        ar& m_levels;
    }

    friend std::ostream& operator<<(std::ostream& stream, const Merkle& merkle)
    {
        auto range = std::ranges::subrange(merkle.m_nodes.begin(), merkle.m_nodes.begin());
        size_t level = 0;
        for (auto length : merkle.m_levels)
        {
            range = {std::end(range), std::end(range) + length};
            stream << "Level " << level;
            for (auto& hash : range)
            {
                stream << " " << hash;
            }
            stream << std::endl;
            ++level;
        }
        return stream;
    }

private:
    auto getNodeSize(std::integral auto inputSize) const
    {
        auto nodeSize = inputSize;
        while (inputSize > 1)
        {
            inputSize = getLevelSize(inputSize);
            nodeSize += inputSize;
        }

        return nodeSize;
    }

    auto getLevelSize(std::integral auto inputSize) const
    {
        return inputSize == 1 ? 0 : (inputSize + (width - 1)) / width;
    }

    size_t calculateLevelHashes(
        InputRange<HashType> auto&& input, OutputRange<HashType> auto&& output) const
    {
        auto inputSize = std::size(input);
        auto outputSize = std::size(output);
        auto expectOutputSize = getLevelSize(inputSize);

        assert(inputSize > 0);
        assert(outputSize >= expectOutputSize);

        HasherType hasher;
        for (decltype(inputSize) i = 0; i < inputSize; i += width)
        {
            for (auto j = i; j < i + width && j < inputSize; ++j)
            {
                hasher.update(input[j]);
            }
            auto outputOffset = i / width;
            assert(outputOffset < outputSize);

            hasher.final(output[outputOffset]);
        }

        return expectOutputSize;
    }

    static void checkWidth(size_t inWidth)
    {
        if (inWidth != width)
            BOOST_THROW_EXCEPTION(std::runtime_error{
                (boost::format("Proof width mismatch merkle width! Expect: %lu, got: %lu") % width %
                    inWidth)
                    .str()});
    }

    std::vector<HashType> m_nodes;
    std::vector<typename decltype(m_nodes)::size_type> m_levels;

    constexpr static size_t MIN_PARALLEL_SIZE = 32;
};

}  // namespace bcos::tool