################################################################################
# TP2 syscall user-space tests
################################################################################
LAB_SYSCALLS_VERSION = 1
LAB_SYSCALLS_SITE = $(BR2_EXTERNAL_LAB_PATH)/../entregas/tutorial-2.2
LAB_SYSCALLS_SITE_METHOD = local

define LAB_SYSCALLS_BUILD_CMDS
	$(TARGET_CC) $(TARGET_CFLAGS) -Wall -Wextra -o $(@D)/process_info_test $(@D)/exemplo/process_info_test.c
	$(TARGET_CC) $(TARGET_CFLAGS) -Wall -Wextra -o $(@D)/sleeping_test $(@D)/desafio1/sleeping_test.c
	$(TARGET_CC) $(TARGET_CFLAGS) -Wall -Wextra -o $(@D)/log_message_test $(@D)/desafio2/log_message_test.c
endef

define LAB_SYSCALLS_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/process_info_test $(TARGET_DIR)/usr/bin/process_info_test
	$(INSTALL) -D -m 0755 $(@D)/sleeping_test $(TARGET_DIR)/usr/bin/sleeping_test
	$(INSTALL) -D -m 0755 $(@D)/log_message_test $(TARGET_DIR)/usr/bin/log_message_test
endef

$(eval $(generic-package))
