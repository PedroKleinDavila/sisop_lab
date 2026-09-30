################################################################################
# TP2 SSTF scheduler and raw disk workload
################################################################################
LAB_SSTF_VERSION = 1
LAB_SSTF_SITE = $(BR2_EXTERNAL_LAB_PATH)/../entregas/sstf
LAB_SSTF_SITE_METHOD = local

define LAB_SSTF_BUILD_CMDS
	$(TARGET_CC) $(TARGET_CFLAGS) -Wall -Wextra -o $(@D)/sector_read $(@D)/sector_read.c
endef

define LAB_SSTF_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/sector_read $(TARGET_DIR)/usr/bin/sector_read
	$(INSTALL) -D -m 0755 $(@D)/run_sstf.sh $(TARGET_DIR)/usr/bin/run_sstf
endef

$(eval $(kernel-module))
$(eval $(generic-package))
