/*
 * Java fork of node-bluetooth-hci-socket 0.4.2 - BluetoothHciSocket.h (6ec5a42 on 28 Oct 2015)
 *
 * Copyright (c) 2016, Martin Petzold
 *
 * Permission to use, copy, modify, and/or distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */
 
#ifndef _Included_BluetoothHciSocket
#define _Included_BluetoothHciSocket

#include <map>

#include <cstring>
#include <cstdlib>
#include <uv.h>
#include <jni.h>

class BluetoothHciSocket {

public:
  BluetoothHciSocket(JNIEnv *env, jobject thisObj);
  ~BluetoothHciSocket();

  void start();
  int bindRaw(int* devId);
  int bindUser(int* devId);
  void bindControl();
  bool isDevUp();
  void setFilter(char* data, int length);
  void stop();

  void write_(char* data, int length);

  void poll();

  void emitErrnoError();
  int devIdFor(int* devId, bool isUp);
  void kernelDisconnectWorkArounds(int length, char* data);

  static void PollCloseCallback(uv_poll_t* handle);
  static void PollCallback(uv_poll_t* handle, int status, int events);

private:
  JavaVM* javaVM;
  jclass activityClass;
  jobject activityObj;

  int _mode;
  int _socket;
  int _devId;
  uv_poll_t _pollHandle;
  std::map<unsigned short,int> _l2sockets;
};

#ifdef __cplusplus
extern "C" {
#endif

JNIEXPORT void JNICALL Java_technology_tavla_os_system_bluetooth_hcisocket_BluetoothHciSocket_newInstance(JNIEnv *, jobject);
JNIEXPORT jint JNICALL Java_technology_tavla_os_system_bluetooth_hcisocket_BluetoothHciSocket_bindRaw(JNIEnv *, jobject, jint);
JNIEXPORT jint JNICALL Java_technology_tavla_os_system_bluetooth_hcisocket_BluetoothHciSocket_bindUser(JNIEnv *, jobject, jint);
JNIEXPORT void JNICALL Java_technology_tavla_os_system_bluetooth_hcisocket_BluetoothHciSocket_bindControl(JNIEnv *, jobject);
JNIEXPORT jboolean JNICALL Java_technology_tavla_os_system_bluetooth_hcisocket_BluetoothHciSocket_isDevUp(JNIEnv *, jobject);
JNIEXPORT void JNICALL Java_technology_tavla_os_system_bluetooth_hcisocket_BluetoothHciSocket_setFilter(JNIEnv *, jobject, jbyteArray);
JNIEXPORT void JNICALL Java_technology_tavla_os_system_bluetooth_hcisocket_BluetoothHciSocket_start(JNIEnv *, jobject);
JNIEXPORT void JNICALL Java_technology_tavla_os_system_bluetooth_hcisocket_BluetoothHciSocket_stop(JNIEnv *, jobject);
JNIEXPORT void JNICALL Java_technology_tavla_os_system_bluetooth_hcisocket_BluetoothHciSocket_write(JNIEnv *, jobject, jbyteArray);

char* as_char_array(JNIEnv *, jbyteArray);

#ifdef __cplusplus
}
#endif

#endif
