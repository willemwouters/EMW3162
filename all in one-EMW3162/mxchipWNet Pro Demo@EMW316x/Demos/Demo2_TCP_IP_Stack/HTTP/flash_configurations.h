#ifndef __FLASH_CONFIGURATIONS_H
#define __FLASH_CONFIGURATIONS_H

#include "flash_if.h"
#include "mxchipWNET.h"

#define MAGIC_FLAG 0x12345678

#pragma pack(1)
typedef struct _config_ {
  /*OTA options*/
  boot_table_t bootTable;
  u32 magicNumber;
  /*User configurations*/
  char wifi_ssid[32];				
  char wifi_key[32];
}config_t;

void readConfig(config_t *pConfig);
void restoreConfig(void);
void updateConfig(config_t *pConfig);


#endif
