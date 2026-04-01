CONFIG ?= release

CONFIG_FILE := make/configs/$(CONFIG).mk

ifeq ($(wildcard $(CONFIG_FILE)),)
    $(error Unknown CONFIG='$(CONFIG)'. Available configs: $(wildcard make/configs/*.mk))
endif

include $(CONFIG_FILE)


