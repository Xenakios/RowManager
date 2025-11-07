#pragma once

#include <array>
#include <cassert>
#include <cstdint>
#include <initializer_list>
#include <string>
#include <format>

namespace xenakios
{

enum RowID
{
    RID_PITCHCLASS,
    RID_DELTATIME,
    RID_VELOCITY,
    RID_OCTAVE,
    RID_POLYAT,
    RID_LAST
};

struct RowTransform
{
    uint16_t transpose = 0;
    bool inverted = false;
    bool reversed = false;
    std::string to_string()
    {
        if (!inverted && !reversed)
            return std::format("Prime {}", transpose);
        if (!inverted && reversed)
            return std::format("Retrograde {}", transpose);
        if (inverted && !reversed)
            return std::format("Inverse {}", transpose);
        if (inverted && reversed)
            return std::format("Retrograde Inverse {}", transpose);
        return "ERROR";
    }
};

class Row
{
  public:
    static constexpr size_t maxElements = 32;
    Row() { std::fill(entries.begin(), entries.end(), 0); }
    static Row make_from_init_list(std::initializer_list<uint16_t> ilist)
    {
        Row result;
        result.num_active_entries = ilist.size();
        for (size_t i = 0; i < ilist.size(); ++i)
            result.entries[i] = *(ilist.begin() + i);
        return result;
    }
    static Row make_chromatic(size_t numentries)
    {
        Row result;
        result.num_active_entries = numentries;
        for (size_t i = 0; i < numentries; ++i)
            result.entries[i] = i;
        return result;
    }
    std::array<uint16_t, maxElements> entries;
    uint16_t num_active_entries = 0;

    bool isValid() const
    {
        if (num_active_entries == 0)
            return false;
        std::array<bool, maxElements> seen;
        std::fill(seen.begin(), seen.end(), false);
        for (size_t i = 0; i < num_active_entries; ++i)
        {
            auto &e = entries[i];
            if (seen[e] || e >= num_active_entries)
                return false;
            seen[e] = true;
        }
        return true;
    }
    class Iterator
    {
      public:
        Row *row = nullptr;
        RowTransform transform;
        int pos = 0;
        Iterator() = default;
        Iterator(Row &r, RowTransform t) : row(&r), transform(t) {}
        void set_position(uint16_t p) { pos = p; }
        Iterator with_transform(RowTransform t)
        {
            Iterator result = *this;
            result.transform = t;
            return result;
        }
        uint16_t next()
        {
            auto rpos = get_transformed_position(pos);
            uint16_t result = (row->entries[rpos] + transform.transpose) % row->num_active_entries;
            if (transform.inverted)
                result = (row->num_active_entries - result) % row->num_active_entries;
            ++pos;
            if (pos == row->num_active_entries)
                pos = 0;
            return result;
        }
        uint16_t get_transformed_position(uint64_t p)
        {
            if (transform.reversed)
                return (row->num_active_entries - 1) - p;
            return p;
        }
    };
};
} // namespace xenakios
