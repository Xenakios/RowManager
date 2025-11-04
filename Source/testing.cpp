#include <print>
#include "row_engine.h"

using namespace xenakios;

inline void test_row_iterator()
{
    Row row = Row::make_chromatic(12);
    Row::Iterator iter{row};
    for (int i = 0; i < row.num_active_entries; ++i)
    {
        std::print("{:3}", iter.next());
    }
    std::print("\n");
}

int main()
{
    test_row_iterator();
    return 0;
}