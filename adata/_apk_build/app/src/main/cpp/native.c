#include <jni.h>
JNIEXPORT jstring JNICALL Java_com_aios_a_M_nativeHello(JNIEnv *e, jobject t){
    return (*e)->NewStringUTF(e,"hello from ndk");
}
