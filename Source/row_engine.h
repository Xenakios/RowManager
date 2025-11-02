#pragma once

#include <array>
#include <cassert>
#include <cstdint>

class Row
{
  public:
    static constexpr size_t maxElements = 32;
    Row()
    {
        std::fill(entries.begin(), entries.end(), 0);
        std::fill(transformed_entries.begin(), transformed_entries.end(), 0);
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
    std::array<uint16_t, maxElements> transformed_entries;
    size_t num_active_entries = 0;
    struct transform_props
    {
        uint16_t transpose = 0;
        bool inverted = false;
        bool reversed = false;
    } tprops;
    void setTransform(uint16_t transpose, bool inv, bool rev)
    {
        for (size_t i = 0; i < num_active_entries; ++i)
        {
            auto &e = entries[i];
            auto elem = (e + transpose) % num_active_entries;
            if (inv)
                elem = (num_active_entries - elem) % num_active_entries;
            transformed_entries[i] = elem;
        }
        if (rev)
            std::reverse(transformed_entries.begin(),
                         transformed_entries.begin() + num_active_entries);
        tprops.transpose = transpose;
        tprops.reversed = rev;
        tprops.inverted = inv;
    }
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
        Iterator() = default;
        Iterator(Row &r, bool iterate_transformed = false)
            : row(&r), it_transformed(iterate_transformed)
        {
            if (iterate_transformed)
                row->setTransform(row->tprops.transpose, row->tprops.inverted,
                                  row->tprops.reversed);
        }
        uint16_t next()
        {
            assert(row);
            uint16_t result = 0;
            if (it_transformed)
                result = row->transformed_entries[pos];
            else
                result = row->entries[pos];
            ++pos;
            if (pos == row->num_active_entries)
                pos = 0;
            return result;
        }
        Row *row = nullptr;
        size_t pos = 0;
        bool it_transformed = false;
    };
};
