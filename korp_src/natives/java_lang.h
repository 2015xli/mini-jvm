
#ifndef _JAVA_LANG_H
#define _JAVA_LANG_H

#include "korp_types.h"
#include "natives.h"

#ifdef __cplusplus
extern "C" {
#endif

void JNICALL com_sun_cldc_io_j2me_debug_PrivateOutputStream_putchar(korp_class* kclass, jchar ch);
jobject JNICALL java_lang_Class_forName(korp_class* jclass, jobject jstr);
jobject JNICALL java_lang_Class_newInstance(jobject jclass);
jchar JNICALL java_lang_String_charAt(jobject jstr, jint index);
jboolean JNICALL java_lang_String_equals(jobject jself, jobject jcomprand );
jint JNICALL java_lang_String_indexOf(jobject jstr, jint ch);
jobject JNICALL java_lang_StringBuffer_append(jobject jbuf, jobject jstr);
jobject JNICALL java_lang_StringBuffer_toString(jobject buf);
void JNICALL java_lang_System_arraycopy(korp_class*, jarray src, jint src_offset,
                                      jarray dst, jint dst_offset,
                                      jint length);
jobject JNICALL java_lang_System_getProperty0(korp_class*, jobject key);

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _JAVA_LANG_H */
