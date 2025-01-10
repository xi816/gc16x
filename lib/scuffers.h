// Buffer with a pointer to the end and some functions
#include <holyc-types.h>
#define SCUFFERSIZE 128
struct Scuffer {
  U8 cont[SCUFFERSIZE];
  U16 end;
};
typedef struct Scuffer Scuffer;

U0 scuffer_clear(Scuffer* s) {
  s->end = 0;
}

U0 scuffer_add(Scuffer* s, U8 byte) {
  if (s->end < (SCUFFERSIZE-1)) {
    s->cont[s->end++] = byte;
    s->cont[s->end] = 0x00;
  }
}

U0 scuffer_remove(Scuffer* s) {
  s->end--;
}

