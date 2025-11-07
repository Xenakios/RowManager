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
    choc::audio::AudioFileProperties props;
    props.bitDepth = choc::audio::BitDepth::float32;
    props.numChannels = 1;
    props.sampleRate = 44100;
    auto writer = wavformat.createWriter(
        R"(C:\MusicAudio\sourcesamples\test_signals\tones\choc_test_öäå.wav)", props);
    if (writer)
    {
        
    }
}

int main()
{
    // test_row_iterator();
    return 0;
}