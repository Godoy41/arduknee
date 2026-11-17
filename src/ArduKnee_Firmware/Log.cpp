#include "Log.h"
#include <Arduino.h>
#include <stdarg.h>
#include <stdio.h>
#include <string.h>

void logInfo(const char* formato, ...) {
  char buf[160];
  buf[0] = '#'; buf[1] = ' ';
  va_list args;
  va_start(args, formato);
  int n = vsnprintf(buf + 2, sizeof(buf) - 4, formato, args);
  va_end(args);
  if (n < 0) return;
  size_t len = 2 + (size_t)n;
  if (len > sizeof(buf) - 3) len = sizeof(buf) - 3;  // truncou
  buf[len++] = '\r';
  buf[len++] = '\n';
  if ((size_t)Serial.availableForWrite() < len) return;  // buffer cheio: descarta
  Serial.write((const uint8_t*)buf, len);
}
