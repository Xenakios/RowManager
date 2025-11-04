#include <print>
#include "row_engine.h"

using namespace xenakios;

inline void print_row(Row::TIterator &it)
{
    for (int i = 0; i < it.row->num_active_entries; ++i)
    {
        std::print("{:3}", it.next());
    }
    std::print("\n");
}

inline void test_row_iterator()
{
    Row row = Row::make_chromatic(12);
    Row::TIterator iter{row, RowTransform{0, false, false}};
    print_row(iter);
    iter = iter.with_transform({0, false, true});
    print_row(iter);
}

int main()
{
    test_row_iterator();
    return 0;
}