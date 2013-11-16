PROTOCOL_DIR = protocol

ifdef PS2_MOUSE_ENABLE
    SRC += $(PROTOCOL_DIR)/ps2_mouse.c
    OPT_DEFS += -DPS2_MOUSE_ENABLE
ifdef PS2_USE_USART
    SRC += $(PROTOCOL_DIR)/ps2_usart.c
    OPT_DEFS += -DPS2_USE_USART
endif
ifdef PS2_USE_INT
    SRC += $(PROTOCOL_DIR)/ps2.c
    OPT_DEFS += -DPS2_USE_INT
endif
ifdef PS2_USE_BUSYWAIT
    SRC += $(PROTOCOL_DIR)/ps2.c
    OPT_DEFS += -DPS2_USE_BUSYWAIT
endif
endif

# Search Path
VPATH += $(TOP_DIR)/$(PROTOCOL_DIR)
