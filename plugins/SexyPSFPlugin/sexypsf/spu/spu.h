int SPUinit(void);
int SPUopen(void);
void SPUsetlength(s32 stop, s32 fade);
int SPUclose(void);
int SPUendflush(void);

// External, called by SPU code.
void SPUirq(void);
