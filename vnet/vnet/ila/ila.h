

#ifndef ILA_H
#define ILA_H

#include <vnet/vnet.h>
#include <vnet/ip/ip.h>

#include <vppinfra/bihash_24_8.h>
#include <vppinfra/bihash_template.h>

#define ila_foreach_type                  \
  _(IID, 0, "iid")       \
  _(LUID, 1, "luid") \
  _(VNID4, 2, "vnid-ip4")              \
  _(VNID6, 3, "vnid-ip6")              \
  _(VNIDM, 4, "vnid-multicast")

typedef enum {
#define _(i,n,s) ILA_TYPE_##i = n,
  ila_foreach_type
#undef _
} ila_type_t;

typedef enum {
  ILA_CSUM_MODE_NO_ACTION,
  ILA_CSUM_MODE_NEUTRAL_MAP,
  ILA_CSUM_MODE_ADJUST_TRANSPORT
} ila_csum_mode_t;

typedef struct {
  ila_type_t type;
  ip6_address_t sir_address;
  ip6_address_t ila_address;
  u32 ila_adj_index;
  ila_csum_mode_t csum_mode;
} ila_entry_t;

typedef struct {
  ila_entry_t *entries; //Pool of ILA entries

  u64 lookup_table_nbuckets;
  u64 lookup_table_size;
  clib_bihash_24_8_t id_to_entry_table;

  u32 ila_sir2ila_feature_index;

  u32 ip6_lookup_next_index;
} ila_main_t;


typedef struct {
  ila_type_t type;
  ip6_address_t sir_address;
  u64 locator;
  u32 vnid;
  u32 local_adj_index;
  ila_csum_mode_t csum_mode;
  u8 is_del;
} ila_add_del_entry_args_t;

int ila_add_del_entry(ila_add_del_entry_args_t *args);
int ila_interface(u32 sw_if_index, u8 disable);

#endif //ILA_H


