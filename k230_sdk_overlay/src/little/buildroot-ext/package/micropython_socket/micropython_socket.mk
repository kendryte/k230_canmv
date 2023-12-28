################################################################################
#
# gpio-keys
#
################################################################################
MICROPYTHON_SOCKET_LOCAL_PATH:= $(realpath $(TOPDIR))"/../package/micropython_socket"
MICROPYTHON_SOCKET_DIR_NAME := micropython_socket
MICROPYTHON_SOCKET_APP_NAME := micropython_socket_server

MICROPYTHON_SOCKET_SITE = $(realpath $(TOPDIR))"/../package/micropython_socket/src"
MICROPYTHON_SOCKET_SITE_METHOD = local
EXTHEAD = $(addprefix -I,  $(realpath $(TOPDIR)/../../../common/cdk/user/component/ipcmsg/include/)  \
			$(realpath $(TOPDIR)/../../../big/mpp/include/) )
EXT_LIBS = $(addprefix -L, \
				 $(realpath $(TOPDIR)/../../../common/cdk/user/component/ipcmsg/host/lib/) ) 

EXT_LIBS += -lipcmsg	-lpthread			

define MICROPYTHON_SOCKET_BUILD_CMDS
	$(TARGET_MAKE_ENV) $(MAKE) CC="$(TARGET_CC)" -C $(@D) EXTHEAD="$(EXTHEAD)" EXT_LIBS="$(EXT_LIBS)"
endef

define MICROPYTHON_SOCKET_INSTALL_TARGET_CMDS
	$(INSTALL) -D -m 0755 $(@D)/$(MICROPYTHON_SOCKET_APP_NAME) $(TARGET_DIR)/usr/bin/$(MICROPYTHON_SOCKET_APP_NAME)
endef

$(eval $(generic-package))
