#ifdef ANDROID
#    include <jni.h>
#endif

extern "C"
{

    enum
    {
        NOT_CREATED,
        INDEXING,
        SEARCHING,
        READY
    };

    struct Info
    {
        const char* path;
        const char* game;
        const char* title;
        const char* composer;
        const char* format;
        const char* metadata;

        int numtunes;
        int starttune;
    };

    void database_init(const char* workDir);
    void database_search(const char* line);
    int database_state();
    int database_hits();
    const char* database_get_result(int i, char* dest, int size);
    Info* database_get_song_info(int index);
    void musicplayer_create(const char* workDir);
    void musicplayer_play(const char* song);
    void musicplayer_pause();
    void musicplayer_seek(int song, int seconds);
}
