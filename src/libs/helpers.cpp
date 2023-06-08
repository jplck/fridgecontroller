#include "helpers.h"

void serial_printf(const char * format, ...) {
  char buf[128];
  va_list args;
  va_start(args, format);
  vsnprintf(buf, 128, format, args);
  va_end(args);
  Serial.print(buf);
}