#include <filesystem>
#include <print>
#include "row_engine.h"
#include "audio/choc_AudioFileFormat.h"
#include "audio/choc_AudioFileFormat_WAV.h"

using namespace xenakios;

inline void print_row(Row::Iterator &it)
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
    Row::Iterator iter{row, RowTransform{0, false, false}};
    print_row(iter);
    iter = iter.with_transform({0, false, true});
    print_row(iter);
}

inline void test_choc_scandinavian()
{
    choc::audio::WAVAudioFileFormat<true> wavformat;
    std::vector<std::filesystem::path> filenamestotest{
        R"(C:\MusicAudio\sourcesamples\test_signals\tones\😮_count_🧨.wav)",
        R"(C:\MusicAudio\sourcesamples\test_signals\tones\count.wav)",
        R"(C:\MusicAudio\sourcesamples\test_signals\tones\ÄÖÅ_count_äöå.wav)"};
    std::print("testing with std::string paths directly\n");
    for (auto &path : filenamestotest)
    {
        auto reader = wavformat.createReader(path);
        if (reader)
            std::print("{} [OK]\n", path.string());
        else
            std::print("{} [FAILED]\n", path.string());
    }
    std::print("testing with custom created std::ifstream instances from paths\n");
    for (auto &path : filenamestotest)
    {
        //std::filesystem::path infilepath{std::filesystem::u8path(path)};
        std::shared_ptr<std::istream> is{new std::ifstream(path, std::ios::binary)};
        auto reader = wavformat.createReader(is);
        if (reader)
            std::print("{} [OK]\n", path.string());
        else
            std::print("{} [FAILED]\n", path.string());
    }
}

int main()
{
    test_choc_scandinavian();
    // test_row_iterator();
    return 0;
}