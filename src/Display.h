#pragma once
#include <Arduino.h>

// E-paper screens. Weather layout comes later; these cover setup and status.
namespace Display {
void begin();
void showSetup(const String& apSsid);                       // phase 1: join the AP
void showConfigPrompt(const String& host, const String& ip);// phase 2: configure in browser
void showMessage(const String& title, const String& line);  // generic status line
}
