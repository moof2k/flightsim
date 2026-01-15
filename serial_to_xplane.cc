#ifndef APL
    #define APL 1
#endif
#define XPLM300 1
#define XPLM303 1

#include <string>
#include <cstring>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

typedef void * XPLMCommandRef;
typedef float (* XPLMFlightLoop_f)(float inElapsedSinceLastCall, float inElapsedSinceLastFlightLoop, int inCounter, void * inRefcon);

extern "C" {
    extern XPLMCommandRef XPLMFindCommand(const char * inName);
    extern void XPLMCommandOnce(XPLMCommandRef inCommand);
    extern void XPLMRegisterFlightLoopCallback(XPLMFlightLoop_f inCallback, float inInterval, void * inRefcon);
    extern void XPLMUnregisterFlightLoopCallback(XPLMFlightLoop_f inCallback, void * inRefcon);
    extern void XPLMDebugString(const char * inString);
}

int serial_fd = -1;
std::string leftover_data = "";
const char* device = "/dev/cu.usbmodem5B3D0680591";

float SerialUpdateLoop(float elapsedMe, float elapsedSim, int counter, void* refcon) {
    if (serial_fd == -1) return 1.0f;
    char read_buf[512];
    int n = read(serial_fd, read_buf, sizeof(read_buf) - 1);
    if (n > 0) {
        read_buf[n] = '\0';
        leftover_data += read_buf;
        size_t pos;
        while ((pos = leftover_data.find_first_of("\r\n")) != std::string::npos) {
            std::string cmd_str = leftover_data.substr(0, pos);
            if (!cmd_str.empty()) {
                XPLMCommandRef xcmd = XPLMFindCommand(cmd_str.c_str());
                if (xcmd) XPLMCommandOnce(xcmd);
            }
            leftover_data.erase(0, pos + 1);
        }
    }
    return -1.0f; 
}

extern "C" __attribute__((visibility("default"))) int XPluginStart(char* outName, char* outSig, char* outDesc) {
    std::strcpy(outName, "G1000 Serial Bridge");
    std::strcpy(outSig, "com.rose.g1000serial");
    std::strcpy(outDesc, "Direct Serial bridge for ESP32");
    serial_fd = open(device, O_RDONLY | O_NOCTTY | O_NONBLOCK);
    if (serial_fd != -1) {
        struct termios options;
        tcgetattr(serial_fd, &options);
        cfsetispeed(&options, B115200);
        cfsetospeed(&options, B115200);
        options.c_cflag |= (CLOCAL | CREAD);
        options.c_cflag &= ~PARENB;
        options.c_cflag &= ~CSTOPB;
        options.c_cflag &= ~CSIZE;
        options.c_cflag |= CS8;
        tcsetattr(serial_fd, TCSANOW, &options);
    }
    XPLMRegisterFlightLoopCallback(SerialUpdateLoop, -1.0f, NULL);
    return 1;
}

extern "C" __attribute__((visibility("default"))) void XPluginStop(void) {
    XPLMUnregisterFlightLoopCallback(SerialUpdateLoop, NULL);
    if (serial_fd != -1) close(serial_fd);
}

extern "C" __attribute__((visibility("default"))) void XPluginDisable(void) {}
extern "C" __attribute__((visibility("default"))) int XPluginEnable(void) { return 1; }
extern "C" __attribute__((visibility("default"))) void XPluginReceiveMessage(int from, int msg, void* param) {}
