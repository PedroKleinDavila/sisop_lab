################################################################################
# TP2 XTEA character driver
################################################################################
LAB_XTEA_VERSION = 1
LAB_XTEA_SITE = $(BR2_EXTERNAL_LAB_PATH)/../entregas/tutorial-2.3
LAB_XTEA_SITE_METHOD = local

define LAB_XTEA_BUILD_CMDS
	$(TARGET_CC) $(TARGET_CFLAGS) -Wall -Wextra -o $(@D)/xtea_test $(@D)/xtea_test.c
endef

define LAB_XTEA_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/xtea_test $(TARGET_DIR)/usr/bin/xtea_test
endef

$(eval $(kernel-module))
$(eval $(generic-package))
