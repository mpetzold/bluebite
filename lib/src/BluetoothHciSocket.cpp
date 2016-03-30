/*
 * Java fork of node-bluetooth-hci-socket 0.4.2 - BluetoothHciSocket.cpp (c5d7f84 on 26 Oct 2015)
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

#include <errno.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <uv.h>
#include <jni.h>

#include "BluetoothHciSocket.h"

#define BTPROTO_L2CAP 0
#define BTPROTO_HCI   1

#define SOL_HCI       0
#define HCI_FILTER    2

#define HCIGETDEVLIST _IOR('H', 210, int)
#define HCIGETDEVINFO _IOR('H', 211, int)

#define HCI_CHANNEL_RAW     0
#define HCI_CHANNEL_USER    1
#define HCI_CHANNEL_CONTROL 3

#define HCI_DEV_NONE  0xffff

#define HCI_MAX_DEV 16

#define ATT_CID 4

enum {
  HCI_UP,
  HCI_INIT,
  HCI_RUNNING,

  HCI_PSCAN,
  HCI_ISCAN,
  HCI_AUTH,
  HCI_ENCRYPT,
  HCI_INQUIRY,

  HCI_RAW,
};

struct sockaddr_hci {
  sa_family_t     hci_family;
  unsigned short  hci_dev;
  unsigned short  hci_channel;
};

struct hci_dev_req {
  uint16_t dev_id;
  uint32_t dev_opt;
};

struct hci_dev_list_req {
  uint16_t dev_num;
  struct hci_dev_req dev_req[0];
};

typedef struct {
  uint8_t b[6];
} __attribute__((packed)) bdaddr_t;

struct hci_dev_info {
  uint16_t dev_id;
  char     name[8];

  bdaddr_t bdaddr;

  uint32_t flags;
  uint8_t  type;

  uint8_t  features[8];

  uint32_t pkt_type;
  uint32_t link_policy;
  uint32_t link_mode;

  uint16_t acl_mtu;
  uint16_t acl_pkts;
  uint16_t sco_mtu;
  uint16_t sco_pkts;

  // hci_dev_stats
  uint32_t err_rx;
  uint32_t err_tx;
  uint32_t cmd_tx;
  uint32_t evt_rx;
  uint32_t acl_tx;
  uint32_t acl_rx;
  uint32_t sco_tx;
  uint32_t sco_rx;
  uint32_t byte_rx;
  uint32_t byte_tx;
};

struct sockaddr_l2 {
  sa_family_t    l2_family;
  unsigned short l2_psm;
  bdaddr_t       l2_bdaddr;
  unsigned short l2_cid;
  uint8_t        l2_bdaddr_type;
};

BluetoothHciSocket::BluetoothHciSocket(JNIEnv *env, jobject thisObj) {

  env->GetJavaVM(&this->javaVM);

  jclass clazz = env->GetObjectClass(thisObj);
  this->activityClass = (jclass)env->NewGlobalRef(clazz);
  this->activityObj = env->NewGlobalRef(thisObj);

  // TEST
  jmethodID method = env->GetMethodID((jclass)this->activityClass, "emit", "([B)V");
  env->CallVoidMethod((jobject)this->activityObj, method);
  // END

  this->_socket = socket(AF_BLUETOOTH, SOCK_RAW | SOCK_CLOEXEC, BTPROTO_HCI);

  uv_poll_init(uv_default_loop(), &this->_pollHandle, this->_socket);

  this->_pollHandle.data = this;
}

BluetoothHciSocket::~BluetoothHciSocket() {
  uv_close((uv_handle_t*)&this->_pollHandle, (uv_close_cb)BluetoothHciSocket::PollCloseCallback);

  close(this->_socket);
}

void BluetoothHciSocket::start() {
  uv_poll_start(&this->_pollHandle, UV_READABLE, BluetoothHciSocket::PollCallback);
}

int BluetoothHciSocket::bindRaw(int* devId) {
  struct sockaddr_hci a;

  memset(&a, 0, sizeof(a));
  a.hci_family = AF_BLUETOOTH;
  a.hci_dev = this->devIdFor(devId, true);
  a.hci_channel = HCI_CHANNEL_RAW;

  this->_devId = a.hci_dev;
  this->_mode = HCI_CHANNEL_RAW;

  bind(this->_socket, (struct sockaddr *) &a, sizeof(a));

  return this->_devId;
}

int BluetoothHciSocket::bindUser(int* devId) {
  struct sockaddr_hci a;

  memset(&a, 0, sizeof(a));
  a.hci_family = AF_BLUETOOTH;
  a.hci_dev = this->devIdFor(devId, false);
  a.hci_channel = HCI_CHANNEL_USER;

  this->_devId = a.hci_dev;
  this->_mode = HCI_CHANNEL_USER;

  bind(this->_socket, (struct sockaddr *) &a, sizeof(a));

  return this->_devId;
}

void BluetoothHciSocket::bindControl() {
  struct sockaddr_hci a;

  memset(&a, 0, sizeof(a));
  a.hci_family = AF_BLUETOOTH;
  a.hci_dev = HCI_DEV_NONE;
  a.hci_channel = HCI_CHANNEL_CONTROL;

  this->_mode = HCI_CHANNEL_CONTROL;

  bind(this->_socket, (struct sockaddr *) &a, sizeof(a));
}

bool BluetoothHciSocket::isDevUp() {
  struct hci_dev_info di;
  bool isUp = false;

  memset(&di, 0x00, sizeof(di));
  di.dev_id = this->_devId;

  if (ioctl(this->_socket, HCIGETDEVINFO, (void *)&di) > -1) {
    isUp = (di.flags & (1 << HCI_UP)) != 0;
  }

  return isUp;
}

void BluetoothHciSocket::setFilter(char* data, int length) {
  if (setsockopt(this->_socket, SOL_HCI, HCI_FILTER, data, length) < 0) {
    this->emitErrnoError();
  }
}

void BluetoothHciSocket::poll() {
  int length = 0;
  char data[1024];

  length = read(this->_socket, data, sizeof(data));

  if (length > 0) {
    if (this->_mode == HCI_CHANNEL_RAW) {
      this->kernelDisconnectWorkArounds(length, data);
    }

    JNIEnv *env;
    this->javaVM->AttachCurrentThread((void**)&env, NULL);

    jbyteArray byteArray = env->NewByteArray (length);
    env->SetByteArrayRegion (byteArray, 0, length, reinterpret_cast<jbyte*>(data));

    jmethodID method = env->GetMethodID((jclass)this->activityClass, "emit", "([B)V");
    env->CallVoidMethod((jobject)this->activityObj, method, byteArray);
  }
}

void BluetoothHciSocket::stop() {
  uv_poll_stop(&this->_pollHandle);
}

void BluetoothHciSocket::write_(char* data, int length) {
  if (write(this->_socket, data, length) < 0) {
    this->emitErrnoError();
  }
}

void BluetoothHciSocket::emitErrnoError() {
  /*Nan::HandleScope scope;

  Local<Object> globalObj = Nan::GetCurrentContext()->Global();
  Local<Function> errorConstructor = Local<Function>::Cast(globalObj->Get(Nan::New("Error").ToLocalChecked()));

  Local<Value> constructorArgs[1] = {
    Nan::New(strerror(errno)).ToLocalChecked()
  };

  Local<Value> error = errorConstructor->NewInstance(1, constructorArgs);

  Local<Value> argv[2] = {
    Nan::New("error").ToLocalChecked(),
    error
  };

  Nan::MakeCallback(Nan::New<Object>(this->This), Nan::New("emit").ToLocalChecked(), 2, argv);*/

  /*JNIEnv *env;
  javaVM->AttachCurrentThread(&env, NULL);
  jmethodID method = env->GetMethodID(activityClass, "error", "([B)V");
  env->CallVoidMethod(activityObj, method);*/
}

int BluetoothHciSocket::devIdFor(int* pDevId, bool isUp) {
  int devId = 0; // default

  if (pDevId == NULL) {
    struct hci_dev_list_req *dl;
    struct hci_dev_req *dr;

    dl = (hci_dev_list_req*)calloc(HCI_MAX_DEV * sizeof(*dr) + sizeof(*dl), 1);
    dr = dl->dev_req;

    dl->dev_num = HCI_MAX_DEV;

    if (ioctl(this->_socket, HCIGETDEVLIST, dl) > -1) {
      for (int i = 0; i < dl->dev_num; i++, dr++) {
        bool devUp = dr->dev_opt & (1 << HCI_UP);
        bool match = isUp ? devUp : !devUp;

        if (match) {
          // choose the first device that is match
          // later on, it would be good to also HCIGETDEVINFO and check the HCI_RAW flag
          devId = dr->dev_id;
          break;
        }
      }
    }

    free(dl);
  } else {
    devId = *pDevId;
  }

  return devId;
}

void BluetoothHciSocket::kernelDisconnectWorkArounds(int length, char* data) {
  // HCI Event - LE Meta Event - LE Connection Complete => manually create L2CAP socket to force kernel to book keep
  // HCI Event - Disconn Complete =======================> close socket from above

  if (length == 22 && data[0] == 0x04 && data[1] == 0x3e && data[2] == 0x13 && data[3] == 0x01 && data[4] == 0x00 && data[7] == 0x00) {
    int l2socket;
    struct sockaddr_l2 l2a;
    unsigned short l2cid;
    unsigned short handle = *((unsigned short*)(&data[5]));

#if __BYTE_ORDER == __LITTLE_ENDIAN
    l2cid = ATT_CID;
#elif __BYTE_ORDER == __BIG_ENDIAN
    l2cid = bswap_16(ATT_CID);
#else
    #error "Unknown byte order"
#endif

    l2socket = socket(PF_BLUETOOTH, SOCK_SEQPACKET, BTPROTO_L2CAP);

    memset(&l2a, 0, sizeof(l2a));
    l2a.l2_family = AF_BLUETOOTH;
    // NOTE: currently not setting adapter address in bind - l2a.l2_bdaddr, l2a.l2_bdaddr_type
    l2a.l2_cid = l2cid;
    bind(l2socket, (struct sockaddr*)&l2a, sizeof(l2a));

    memset(&l2a, 0, sizeof(l2a));
    l2a.l2_family = AF_BLUETOOTH;
    memcpy(&l2a.l2_bdaddr, &data[9], sizeof(l2a.l2_bdaddr));
    l2a.l2_cid = l2cid;
    l2a.l2_bdaddr_type = data[8] + 1; // BDADDR_LE_PUBLIC (0x01), BDADDR_LE_RANDOM (0x02)

    connect(l2socket, (struct sockaddr *)&l2a, sizeof(l2a));

    this->_l2sockets[handle] = l2socket;
  } else if (length == 7 && data[0] == 0x04 && data[1] == 0x05 && data[2] == 0x04 && data[3] == 0x00) {
    unsigned short handle = *((unsigned short*)(&data[4]));

    if (this->_l2sockets.count(handle) > 0) {
      close(this->_l2sockets[handle]);
      this->_l2sockets.erase(handle);
    }
  }
}

void BluetoothHciSocket::PollCloseCallback(uv_poll_t* handle) {
  delete handle;
}

void BluetoothHciSocket::PollCallback(uv_poll_t* handle, int status, int events) {
  BluetoothHciSocket *p = (BluetoothHciSocket*)handle->data;

  p->poll();
}

static BluetoothHciSocket* p;

JNIEXPORT void JNICALL Java_technology_tavla_os_system_bluetooth_hcisocket_BluetoothHciSocket_newInstance(JNIEnv *env, jobject thisObj) {
  p = new BluetoothHciSocket(env, thisObj);
}

JNIEXPORT void JNICALL Java_technology_tavla_os_system_bluetooth_hcisocket_BluetoothHciSocket_start(JNIEnv *env, jobject thisObj) {
  p->start();
}

JNIEXPORT jint JNICALL Java_technology_tavla_os_system_bluetooth_hcisocket_BluetoothHciSocket_bindRaw(JNIEnv *env, jobject thisObj, jint *pDevId) {
  return p->bindRaw(pDevId);
}

JNIEXPORT jint JNICALL Java_technology_tavla_os_system_bluetooth_hcisocket_BluetoothHciSocket_bindUser(JNIEnv *env, jobject thisObj, jint *pDevId) {
  return p->bindUser(pDevId);
}

JNIEXPORT void JNICALL Java_technology_tavla_os_system_bluetooth_hcisocket_BluetoothHciSocket_bindControl(JNIEnv *env, jobject thisObj) {
  p->bindControl();
}

JNIEXPORT jboolean JNICALL Java_technology_tavla_os_system_bluetooth_hcisocket_BluetoothHciSocket_isDevUp(JNIEnv *env, jobject thisObj) {
  return p->isDevUp();
}

JNIEXPORT void JNICALL Java_technology_tavla_os_system_bluetooth_hcisocket_BluetoothHciSocket_setFilter(JNIEnv *env, jobject thisObj, jbyteArray data) {
  p->setFilter(as_char_array(env, data), env->GetArrayLength(data));
}

JNIEXPORT void JNICALL Java_technology_tavla_os_system_bluetooth_hcisocket_BluetoothHciSocket_stop(JNIEnv *env, jobject thisObj) {
  p->stop();
}

JNIEXPORT void JNICALL Java_technology_tavla_os_system_bluetooth_hcisocket_BluetoothHciSocket_write(JNIEnv *env, jobject thisObj, jbyteArray data) {
  p->write_(as_char_array(env, data), env->GetArrayLength(data));
}

char* as_char_array(JNIEnv *env, jbyteArray array) {
  int len = env->GetArrayLength(array);
  char* buf = new char[len];
  env->GetByteArrayRegion(array, 0, len, reinterpret_cast<jbyte*>(buf));
  return buf;
}
