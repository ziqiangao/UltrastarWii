#ifndef SONG_H
#define SONG_H
#include <fat.h>
#include <stdio.h>
#include <dirent.h>

enum WordType {
    Normal,
    Golden,
    Rap,
    RapGolden,
    Freestyle,
    PageBreak
};

struct meta
{
    char* title = nullptr;
    char* artist = nullptr;
    int bpm;
};


struct lyrics
{
    WordType type;
    char* word = nullptr;
    int time;
    int pitch;
    int duration;
};


struct USF
{
    lyrics* p1 = nullptr;
    lyrics* p2 = nullptr;
    lyrics* base = nullptr;
    meta metadata;
};

struct Song
{
    DIR* folder = nullptr;
    FILE* text = nullptr;
    USF parsed;
    FILE* audio = nullptr;
    FILE* video = nullptr;
    FILE* cover = nullptr;
};

int freeSong(Song* song);
int createparse(DIR* folder, Song* out);

#endif // SONG_H
