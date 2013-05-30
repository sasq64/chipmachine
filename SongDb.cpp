/*
	telnet.addCommand("play", [&](vector<string> args) {
		ChipPlayer *player = psys.play(name);
	});*/

	/*sqlite3 *db = nullptr;

	int rc = sqlite3_open("hvsc.db", &db);
	if(rc == SQLITE_OK) {
		printf("DB opened\n");
		sqlite3_stmt *s;
		const char *tail;
		rc = sqlite3_prepare(db, "select * from songs;", -1, &s, &tail);
		if(rc == SQLITE_OK) {
			printf("Statement created\n");
			while(true) {
				sqlite3_step(s);
				const char *title = (const char *)sqlite3_column_text(s, 2);
				printf("title %s\n", title);
			}
		} else
			printf("%s\n", sqlite3_errmsg(db));
	} else
		printf("%s\n", sqlite3_errmsg(db));
*/