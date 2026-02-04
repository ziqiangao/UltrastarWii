#if !defined(SCENE_H)
#define SCENE_H
#include <cstdint>

enum class Scene : uint8_t {
    Title,
    Config,
    SongSelect,
    Song
};

void changescene(Scene);
int drawscene();
Scene getscene();

#endif // SCENE_H
