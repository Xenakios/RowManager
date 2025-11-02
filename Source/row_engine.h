#pragma once

#include <array>
#include <cstdint>
#include <set>

class Row
{
  public:
    static constexpr size_t maxElements = 32;
    Row() { std::fill(entries.begin(), entries.end(), 0); }
    static Row make_chromatic(size_t numentries)
    {
        Row result;
        result.num_active_entries = numentries;
        for (size_t i = 0; i < numentries; ++i)
            result.entries[i] = i;
        return result;
    }
    std::array<uint16_t, maxElements> entries;
    size_t num_active_entries = 0;
    Row transform(int transpose, bool inverse, bool reverse)
    {
        Row result;
        result.num_active_entries = num_active_entries;
        for (size_t i = 0; i < num_active_entries; ++i)
        {
            auto &e = entries[i];
            auto elem = (e + transpose) % num_active_entries;
            if (inverse)
                elem = (num_active_entries - elem) % num_active_entries;
            result.entries[i] = elem;
        }
        if (reverse)
            std::reverse(result.entries.begin(),
                         result.entries.begin() + result.num_active_entries);
        return result;
    }
    bool isValid() const
    {
        std::array<bool, maxElements> seen;
        std::fill(seen.begin(), seen.end(), false);
        for (size_t i = 0; i < num_active_entries; ++i)
        {
            auto &e = entries[i];
            if (seen[e])
                return false;
            seen[e] = true;
        }
        return true;
    }
};