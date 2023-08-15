
add_custom_target(_sync_k230_sdk_overlay
    COMMAND make -f ${SCRIPTS_DIR}/helper.mk sync_k230_sdk_overlay
        PROJECT_ROOT_DIR=${PROJECT_ROOT_DIR}
    WORKING_DIRECTORY ${PROJECT_ROOT_DIR}
)

execute_process(
    COMMAND make prepare_sourcecode
    WORKING_DIRECTORY ${K230_SDK}
)

add_custom_target(_uboot
    COMMAND make uboot
    WORKING_DIRECTORY ${K230_SDK}
    DEPENDS _sync_k230_sdk_overlay
)

add_custom_target(_rt-smart-kernel
    COMMAND make rt-smart-kernel
    COMMAND make mpp-kernel
    COMMAND make mpp-apps
    WORKING_DIRECTORY ${K230_SDK}
    DEPENDS _sync_k230_sdk_overlay
)

add_custom_target(_build-image ALL
    COMMAND make rt-smart-apps
    COMMAND make big-core-opensbi
    COMMAND make build-image CANMV_BUILD_DIR=${PROJECT_BINARY_DIR}
    WORKING_DIRECTORY ${K230_SDK}
    DEPENDS _sync_k230_sdk_overlay
    DEPENDS _uboot
    DEPENDS _rt-smart-kernel
    DEPENDS micropython
)
