BLE51_DIR = protocol/ble51

SRC +=  $(BLE51_DIR)/recore/serial_uart.c \
        $(BLE51_DIR)/recore/action.c \
        $(BLE51_DIR)/recore/bootloader.c \
        $(BLE51_DIR)/recore/suspend.c \
	$(BLE51_DIR)/ble51.c \
        $(BLE51_DIR)/ble51_task.c \
        $(BLE51_DIR)/main.c


OPT_DEFS += -DPROTOCOL_BLE51
OPT_DEFS += -DRECORE

VPATH += $(TMK_DIR)/$(BLE51_DIR)
VPATH += $(TMK_DIR)/$(BLE51_DIR)/recore
