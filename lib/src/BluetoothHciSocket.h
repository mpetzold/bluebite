/*
 * JNI (Java) fork of node-bluetooth-hci-socket 0.4.2 - BluetoothHciSocket.h (6ec5a42 on 28 Oct 2015)
 *
 * Copyright (c) 2016, Martin Petzold
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
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
