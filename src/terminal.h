#include "pd_api.h"

void terminalInit(PlaydateAPI* pd);
void terminalTouch(void);
void terminalPutchar(unsigned char c);
void terminalWrite(const char* data, size_t len);
void terminalUpdate(PlaydateAPI* pd);
