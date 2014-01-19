#include <stdint.h>
#include <stdbool.h>
#include <avr/eeprom.h>
#include "eeconfig.h"

static uint16_t EEMEM ee_magic;
static uint8_t EEMEM ee_debug;
static uint8_t EEMEM ee_default_layer;
static uint8_t EEMEM ee_keymap;
static uint8_t EEMEM ee_mousekey_accel;
static uint8_t EEMEM ee_backlight;

void eeconfig_init(void)
{
    eeprom_write_word(&ee_magic,          EECONFIG_MAGIC_NUMBER);
    eeprom_write_byte(&ee_debug,          0);
    eeprom_write_byte(&ee_default_layer,  0);
    eeprom_write_byte(&ee_keymap,         0);
    eeprom_write_byte(&ee_mousekey_accel, 0);
#ifdef BACKLIGHT_ENABLE
    eeprom_write_byte(&ee_backlight,      0);
#endif
}

void eeconfig_enable(void)
{
    eeprom_write_word(&ee_magic, EECONFIG_MAGIC_NUMBER);
}

void eeconfig_disable(void)
{
    eeprom_write_word(&ee_magic, 0xFFFF);
}

bool eeconfig_is_enabled(void)
{
    return (eeprom_read_word(&ee_magic) == EECONFIG_MAGIC_NUMBER);
}

uint8_t eeconfig_read_debug(void)      { return eeprom_read_byte(&ee_debug); }
void eeconfig_write_debug(uint8_t val) { eeprom_write_byte(&ee_debug, val); }

uint8_t eeconfig_read_default_layer(void)      { return eeprom_read_byte(&ee_default_layer); }
void eeconfig_write_default_layer(uint8_t val) { eeprom_write_byte(&ee_default_layer, val); }

uint8_t eeconfig_read_keymap(void)      { return eeprom_read_byte(&ee_keymap); }
void eeconfig_write_keymap(uint8_t val) { eeprom_write_byte(&ee_keymap, val); }

#ifdef BACKLIGHT_ENABLE
uint8_t eeconfig_read_backlight(void)      { return eeprom_read_byte(&ee_backlight); }
void eeconfig_write_backlight(uint8_t val) { eeprom_write_byte(&ee_backlight, val); }
#endif
