
#include "MusicDatabase.h"
#include "MusicPlayerList.h"

#include <atomic>

typedef void* Handle;
#include "chiplib.h"

 
using namespace chipmachine;

static std::shared_ptr<IncrementalQuery> query;
static std::mutex m;
static std::atomic<int> db_state{NOT_CREATED};
static std::unique_ptr<MusicPlayerList> mp;


void database_init(const char* workDir)
{
	db_state = INDEXING;
	MusicDatabase::getInstance().initFromLua(utils::File(workDir));
	query = MusicDatabase::getInstance().createQuery();
	db_state = READY;
}

extern "C" void database_search(const char* line)
{
	db_state = SEARCHING;
	query->setString(line);
	db_state = READY;
}


extern "C" int database_state()
{
	return db_state;
}

extern "C" int database_hits()
{
	return query->numHits();
}

extern "C" const char* database_get_result(int i, char* dest, int size)
{
	auto res = query->getResult(i);
	strcpy(dest, &res[0]);
	return dest;
}


extern "C" int database_get_index(int index)
{
	return query->getIndex(index);
}

extern "C" Info* database_get_song_info(int index)
{
	SongInfo si = MusicDatabase::getInstance().getSongInfo(index);

	int len = 0;
	std::string* fields[] = {&si.path, &si.game, &si.title, &si.composer, &si.format};
	int offsets[5];

	int i = 0;
	for(const auto& c : fields) {
		offsets[i++] = len;
		len += (c->length() + 1);
	}

	char* data = (char*)malloc(sizeof(Info) + len);
	auto* info = new (data) Info;

	const char** fields2[] = {&info->path, &info->game, &info->title, &info->composer, &info->format};

	for(int i=0; i<5; i++) {
		char* dest = data + sizeof(Info) + offsets[i];
		strcpy(dest, fields[i]->data());
		*fields2[i] = dest;
	}
	info->numtunes = si.numtunes;
	info->starttune = si.starttune;
	info->metadata = nullptr;

	return info;

}


extern "C" void musicplayer_create(const char* workDir)
{
	mp = std::make_unique<MusicPlayerList>(workDir);
}

extern "C" void musicplayer_play(const char* song)
{
	if(song)
		mp->playSong(SongInfo(song));
	else
		mp->pause(false);
}

extern "C" void musicplayer_pause()
{
	mp->pause();
}

extern "C" void musicplayer_seek(int song, int seconds)
{
}



