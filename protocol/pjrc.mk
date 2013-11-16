PJRC_DIR = protocol/pjrc

SRC +=	$(PJRC_DIR)/main.c \
	$(PJRC_DIR)/pjrc.c \
	$(PJRC_DIR)/usb_keyboard.c \
	$(PJRC_DIR)/usb_debug.c \
	$(PJRC_DIR)/usb.c

# Option modules
#ifdef $(or MOUSEKEY_ENABLE, MOUSE_ENABLE)
var_defined=$(if $(findstring undefined,$(origin $(1))),,yes)
ifeq ($(or $(call var_defined,MOUSE_ENABLE),$(call var_defined,MOUSEKEY_ENABLE)),yes)
    SRC += $(PJRC_DIR)/usb_mouse.c
endif

ifdef EXTRAKEY_ENABLE
    SRC += $(PJRC_DIR)/usb_extra.c
endif

# Search Path
VPATH += $(TOP_DIR)/$(PJRC_DIR)

# This indicates using PJRC stack
OPT_DEFS += -DPROTOCOL_PJRC
