
#include <stdlib.h>
#include <stdio.h>

#include <string.h>
#include <ctype.h>

#include <android/log.h> 
#include <jni.h>

#include "modplug/modplug.h"

#define MOD_TYPE_NONE		0x00
#define MOD_TYPE_MOD		0x01
#define MOD_TYPE_S3M		0x02
#define MOD_TYPE_XM			0x04
#define MOD_TYPE_MED		0x08
#define MOD_TYPE_MTM		0x10
#define MOD_TYPE_IT			0x20
#define MOD_TYPE_669		0x40
#define MOD_TYPE_ULT		0x80
#define MOD_TYPE_STM		0x100


#include "com_ssb_droidsound_plugins_ModPlugin.h"

//#include "modplug/libmodplug/it_defs.h"
//#include "modplug/libmodplug/sndfile.h"

#define INFO_TITLE 0
#define INFO_AUTHOR 1
#define INFO_LENGTH 2
#define INFO_TYPE 3
#define INFO_COPYRIGHT 4
#define INFO_GAME 5
#define INFO_SUBTUNES 6
#define INFO_STARTTUNE 7

#define INFO_INSTRUMENTS 100
#define INFO_CHANNELS 101
#define INFO_PATTERNS 102


static jstring NewString(JNIEnv *env, const char *str)
{
	static jchar *temp, *ptr;

	temp = (jchar *) malloc((strlen(str) + 1) * sizeof(jchar));

	ptr = temp;
	while(*str) {
		unsigned char c = (unsigned char)*str++;
		*ptr++ = (c < 0x7f && c >= 0x20) || c >= 0xa0 || c == 0xa ? c : '?';
	}
	//*ptr++ = 0;
	jstring j = env->NewString(temp, ptr - temp);

	free(temp);

	return j;
}

/*
JNIEXPORT jboolean JNICALL Java_com_ssb_droidsound_plugins_ModPlugin_N_1canHandle(JNIEnv *env, jobject obj, jstring name)
{
	const char spaces[] = "      ";
	unsigned int ext = 0;
	jboolean iscopy;
	const char *fname = env->GetStringUTFChars(name, &iscopy);
	bool ok = false;

	if(strncasecmp(fname, "MOD.", 4) == 0)
		ok = true;
	else
	{
		const char *ptr = strrchr(fname, '.');
		if(ptr) {
			for(int i=0; i<4; i++)
			{
				ext <<= 8;
				if(!ptr[i+1])
					ptr = spaces;
				ext |= toupper(ptr[i+1]);
			}


			switch(ext)
			{
			case 'MOD ':
			case 'IT  ':
			case 'S3M ':
			case 'XM  ':
			case 'MTM ':
			case 'STM ':
			case '669 ':
			case 'FT  ':
				ok = true;
				break;
			}
		}
	}

	return ok;
}
*/

static int loopMode = 0;

struct ModInfo {
	ModPlugFile *mod;
	const char *modType;
	char author[128];
	char mod_name[128];
	int mod_length;
	jbyte *ptr;
	int size;
	jbyteArray array;
};

JNIEXPORT jlong JNICALL Java_com_ssb_droidsound_plugins_ModPlugin_N_1loadInfo(JNIEnv *env, jobject obj, jbyteArray bArray, jint size)
{
	return 0; //UNUSED
}


JNIEXPORT jlong JNICALL Java_com_ssb_droidsound_plugins_ModPlugin_N_1load(JNIEnv *env, jobject obj, jbyteArray bArray, jint size)
{
	jboolean iscopy;
	jbyte *ptr = env->GetByteArrayElements(bArray, NULL);

	ModPlug_Settings settings;
	ModPlug_GetSettings(&settings);
	settings.mChannels = 2;
	settings.mFrequency = 44100;
	settings.mBits = 16;
	settings.mLoopCount = -1;

	ModPlug_SetSettings(&settings);

	jbyte *ptr2 = (jbyte *)malloc(size);
	memcpy(ptr2, ptr, size);

	ModPlugFile *mod = ModPlug_Load(ptr2, size);
	ModInfo *info = NULL;


	if(mod)
	{
		info = new ModInfo();
		info->mod = mod;
		info->ptr = ptr2;
		info->size = size;
		//info->array = bArray;
		//guessAuthor(info);
		strcpy(info->mod_name, ModPlug_GetName(mod));
		info->mod_length = ModPlug_GetLength(mod);
		info->modType = "ModPlug";
		*info->author = 0;

		int t = ModPlug_GetModuleType(mod);

		switch(t) {
		case MOD_TYPE_MOD:
			info->modType = "MOD";
			break;
		case MOD_TYPE_S3M:
			info->modType = "S3M";
			break;
		case MOD_TYPE_XM:
			info->modType = "XM";
			break;
		case MOD_TYPE_IT:
			info->modType = "IT";
			break;
		case MOD_TYPE_STM:
			info->modType = "STM";
			break;
		}


		settings.mResamplingMode = MODPLUG_RESAMPLE_LINEAR;
		settings.mFlags = MODPLUG_ENABLE_OVERSAMPLING;

		//int t = ModPlug_GetModuleType(mod);
		__android_log_print(ANDROID_LOG_VERBOSE, "ModPlugin", "Type is %d", t);
		if(t == 1) {
			settings.mResamplingMode = MODPLUG_RESAMPLE_NEAREST;
			__android_log_print(ANDROID_LOG_VERBOSE, "ModPlugin", "NEAREST resampling");
		}

		ModPlug_SetSettings(&settings);
	} else {
		free(ptr2);
	}

	env->ReleaseByteArrayElements(bArray, ptr, 0);
	return (long)info;
}

JNIEXPORT void JNICALL Java_com_ssb_droidsound_plugins_ModPlugin_N_1unload(JNIEnv *env, jobject obj, jlong song)
{
	ModInfo *info = (ModInfo*)song;
	if(info->mod)
		ModPlug_Unload(info->mod);
	if(info->ptr)
		free(info->ptr);

		//env->ReleaseByteArrayElements(info->array, info->ptr, 0);

	delete info;
	info = NULL;
}

JNIEXPORT void JNICALL Java_com_ssb_droidsound_plugins_ModPlugin_N_1setOption(JNIEnv *env, jobject obj, jint opt, jint val)
{
	loopMode = val;
	__android_log_print(ANDROID_LOG_VERBOSE, "ModPlugin", "Loop mode %d", val);
}


JNIEXPORT jint JNICALL Java_com_ssb_droidsound_plugins_ModPlugin_N_1getSoundData(JNIEnv *env, jobject obj, jlong song, jshortArray bArray, int size)
{
	//unsigned char *ptr = (unsigned char *)env->GetDirectBufferAddress(buffer);
	//int size = env->GetDirectBufferCapacity(buffer);
	ModInfo *info = (ModInfo*)song;

	jbyte *ptr = (jbyte*)env->GetShortArrayElements(bArray, NULL);

	//fprintf(stderr, "ptr %p, size %d\n", ptr, size);
	int rc = ModPlug_Read(info->mod, (void*)ptr, size*2);

	if(rc == 0 && loopMode == 1) {
		ModPlug_Unload(info->mod);
		info->mod = ModPlug_Load(info->ptr, info->size);
		rc = ModPlug_Read(info->mod, (void*)ptr, size*2);
	}

	//__android_log_print(ANDROID_LOG_VERBOSE, "ModPlugin", "RC:%d, size:%d, ptr:%p", rc, size, ptr);
	//__android_log_print(ANDROID_LOG_VERBOSE, "ModPlugin", "(%d) %d %d %d %d\n", rc, ptr[size/2], ptr[size/2+1], ptr[size-10], ptr[size-9]);

	env->ReleaseShortArrayElements(bArray, (jshort*)ptr, 0);

	if(rc == 0) return -1;

	return rc / 2;
}

JNIEXPORT jboolean JNICALL Java_com_ssb_droidsound_plugins_ModPlugin_N_1seekTo(JNIEnv *env, jobject obj, jlong song, int where)
{
	ModInfo *info = (ModInfo*)song;
	ModPlug_Seek(info->mod, where);
	return true;
}


JNIEXPORT jstring JNICALL Java_com_ssb_droidsound_plugins_ModPlugin_N_1getStringInfo(JNIEnv *env, jobject obj, jlong song, jint what)
{
	ModInfo *info = (ModInfo*)song;
	switch(what)
	{
	case INFO_AUTHOR:
		//if(mod)
			return NewString(env, info->author);
		break;
	case INFO_TITLE:
		//if(mod)
			return NewString(env, info->mod_name);
		break;
	case INFO_TYPE:
		//if(mod)
			return NewString(env, info->modType);
		break;
	case INFO_INSTRUMENTS:
	{
		char instruments[2048];

		char *ptr = instruments;
		char *instEnd = instruments + sizeof(instruments) - 48;
		*ptr = 0;

		int ns = ModPlug_NumSamples(info->mod);
		__android_log_print(ANDROID_LOG_VERBOSE, "ModPlugin", "%d instruments", ns);
		if(ns > 0) {
			for(int i=1; i<ns; i++) {
				int l = ModPlug_SampleName(info->mod, i, ptr);

				/*__android_log_print(ANDROID_LOG_VERBOSE, "ModPlugin", "%p %p %d", ptr, instEnd, strlen(ptr)); */
				ptr += l;
				if(ptr >= instEnd)
					break;
				*ptr++ = 0xa;
				*ptr = 0;
			}
		}
		//__android_log_print(ANDROID_LOG_VERBOSE, "ModPlugin", "ILEN %d", strlen(instruments));
		return  NewString(env, instruments);
	}
		break;
	}
	return 0;
}

JNIEXPORT jint JNICALL Java_com_ssb_droidsound_plugins_ModPlugin_N_1getIntInfo(JNIEnv *env, jobject obj, jlong song, jint what)
{
	ModInfo *info = (ModInfo*)song;
	switch(what)
	{
	case INFO_LENGTH:
		return info->mod_length;
	case INFO_SUBTUNES:
		return 0;
	case INFO_STARTTUNE:
		return 0;
	case INFO_CHANNELS:
		return ModPlug_NumChannels(info->mod);
	case INFO_PATTERNS:
		return ModPlug_NumPatterns(info->mod);
	}
	return -1;
}
