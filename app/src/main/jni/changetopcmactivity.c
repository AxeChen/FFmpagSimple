#include <jni.h>

JNIEXPORT void JNICALL
Java_com_app_axe_ffmpagsimple_pcm_ChangeToPcmActivity_changeMp3(JNIEnv *env, jobject instance,
                                                                jstring input_, jstring outStr_) {
    const char *input = (*env)->GetStringUTFChars(env, input_, 0);
    const char *outStr = (*env)->GetStringUTFChars(env, outStr_, 0);

    // TODO

    (*env)->ReleaseStringUTFChars(env, input_, input);
    (*env)->ReleaseStringUTFChars(env, outStr_, outStr);
}