#include <stdio.h>

#include "k_autoconf_comm.h"

#include "k_sensor_comm.h"

#define BIT(nr) (1UL << (nr))
#define MIRROR BIT(0)
#define FLIP BIT(1)

struct sensor_type_mirror_t {
  k_vicap_sensor_type type;
  k_u32 mirror;
};

#if defined(CONFIG_BOARD_K230_CANMV)
static struct sensor_type_mirror_t type_mirror_tbl[] = {
    // {.type =, .mirror = },
};
#elif defined(CONFIG_BOARD_K230_CANMV_V2)
static struct sensor_type_mirror_t type_mirror_tbl[] = {
    // {.type =, .mirror = },
};
#elif defined(CONFIG_BOARD_K230D_CANMV)
static struct sensor_type_mirror_t type_mirror_tbl[] = {
    // {.type =, .mirror = },
};
#elif defined(CONFIG_BOARD_K230_CANMV_01STUDIO)
static struct sensor_type_mirror_t type_mirror_tbl[] = {
    {.type = OV_OV5647_MIPI_1920X1080_30FPS_10BIT_LINEAR, .mirror = FLIP},
};
#elif defined(CONFIG_BOARD_K230_CANMV_DONGSHANPI)
static struct sensor_type_mirror_t type_mirror_tbl[] = {
    // {.type =, .mirror = },
};
#endif

#if defined(CONFIG_BOARD_K230_CANMV) || defined(CONFIG_BOARD_K230_CANMV_V2) || \
    defined(CONFIG_BOARD_K230D_CANMV) ||                                       \
    defined(CONFIG_BOARD_K230_CANMV_01STUDIO) ||                               \
    defined(CONFIG_BOARD_K230_CANMV_DONGSHANPI)

k_u32 get_mirror_by_sensor_type(k_vicap_sensor_type type) {
  k_u32 mirror = 0;

  struct sensor_type_mirror_t *p = NULL;

  size_t count = (sizeof(type_mirror_tbl) / sizeof(type_mirror_tbl[0]));

  for (size_t i = 0; i < count; i++) {
    p = &type_mirror_tbl[i];

    if (type == p->type) {
      mirror = p->mirror;
      break;
    }
  }

  return mirror;
}
#else
k_u32 get_mirror_by_sensor_type(k_vicap_sensor_type type) {
  printf("unsupport board type %s\n", CONFIG_BOARD_NAME);

  return 0;
}
#endif
