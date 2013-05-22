
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

#include <jni.h>
#include <android/log.h>

extern "C" {
	#include "sexypsf/driver.h"
}

#include "com_ssb_droidsound_plugins_SexyPSFPlugin.h"


#include "../common/Fifo.h"
#include "../common/Misc.h"


static Fifo *fifo = NULL;
static bool playing = false;


void sexyd_update(unsigned char *pSound, long lBytes)
{
	//__android_log_print(ANDROID_LOG_VERBOSE, "SexyPSF", "update with %d bytes", lBytes);
	fifo->putBytes((char*)pSound, lBytes);
}


JNIEXPORT jlong JNICALL Java_com_ssb_droidsound_plugins_SexyPSFPlugin_N_1load(JNIEnv *env, jobject obj, jstring fname)
{
	const char *filename = env->GetStringUTFChars(fname, NULL);
	__android_log_print(ANDROID_LOG_VERBOSE, "SexyPSF", "Trying to load '%s'", filename);

	playing = false;

	if(fifo == NULL)
		fifo = new Fifo(1024 * 128);

	char temp[1024];
	strcpy(temp, filename);

	PSFINFO *psfInfo = sexy_load(temp);
	__android_log_print(ANDROID_LOG_VERBOSE, "SexyPSF", "Got %p", psfInfo);
	return (long)psfInfo;

}

JNIEXPORT void JNICALL Java_com_ssb_droidsound_plugins_SexyPSFPlugin_N_1unload(JNIEnv *env, jobject obj, jlong song)
{
	__android_log_print(ANDROID_LOG_VERBOSE, "SexyPSF", "Unload while %splaying", playing ? "" : "NOT ");

	playing = false;

	if(fifo != NULL)
		delete fifo;
	fifo = NULL;

	PSFINFO *psfInfo = (PSFINFO*)song;
	sexy_freepsfinfo(psfInfo);
	sexy_shutdown();
}

JNIEXPORT jint JNICALL Java_com_ssb_droidsound_plugins_SexyPSFPlugin_N_1getSoundData(JNIEnv *env, jobject obj, jlong song, jshortArray sArray, jint size)
{
	playing = true;

	while(fifo->filled() < size*2) {
		int rc = sexy_execute();
		//__android_log_print(ANDROID_LOG_VERBOSE, "SexyPSF", "Execute:%d", rc);
		if(rc <= 0)
			return rc;
	}

	if(fifo->filled() == 0)
		return 0;

	jshort *dest = env->GetShortArrayElements(sArray, NULL);
	int len = fifo->getShorts(dest, size);
	env->ReleaseShortArrayElements(sArray, dest, 0);

	return len;
}


JNIEXPORT jstring JNICALL Java_com_ssb_droidsound_plugins_SexyPSFPlugin_N_1getStringInfo(JNIEnv *env, jobject obj, jlong song, jint what) {
	PSFINFO *info = (PSFINFO*)song;

	switch(what) {
	case INFO_AUTHOR:
		return NewString(env, info->artist ? info->artist : "");
	case INFO_TITLE:
		return NewString(env, info->title ? info->title : "");
	case INFO_COPYRIGHT:
		return NewString(env, info->copyright ? info->copyright : "");
	case INFO_GAME:
		return NewString(env, info->game ? info->game : "");
	}
	return NULL;

}

JNIEXPORT jint JNICALL Java_com_ssb_droidsound_plugins_SexyPSFPlugin_N_1getIntInfo(JNIEnv *env, jobject obj, jlong song, jint what) {
	PSFINFO *info = (PSFINFO*)song;
		switch(what) {
		case INFO_LENGTH:
			return info->length;
		}
		return 0;
}
