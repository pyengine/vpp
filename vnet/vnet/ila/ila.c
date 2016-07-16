#include <vnet/ila/ila.h>

static ila_main_t ila_main;

#define ILA_TABLE_DEFAULT_HASH_NUM_BUCKETS (64 * 1024)
#define ILA_TABLE_DEFAULT_HASH_MEMORY_SIZE (32<<20)

#define foreach_ila_error                               \
 _(NONE, "valid ILA packets")

typedef enum {
#define _(sym,str) ILA_ERROR_##sym,
   foreach_ila_error
#undef _
   ILA_N_ERROR,
 } ila_error_t;

 static char *ila_error_strings[] = {
 #define _(sym,string) string,
   foreach_ila_error
 #undef _
 };

typedef enum {
  ILA_ILA2SIR_NEXT_IP6_REWRITE,
  ILA_ILA2SIR_NEXT_DROP,
  ILA_ILA2SIR_N_NEXT,
} ila_ila2sir_next_t;

u8 *
format_ila_ila2sir_trace (u8 *s, va_list *args)
{
  return format(s, "ila-ila2sir");
}

static vlib_node_registration_t ila_ila2sir_node;

static uword
ila_ila2sir (vlib_main_t *vm,
        vlib_node_runtime_t *node,
        vlib_frame_t *frame)
{
  u32 n_left_from, *from, next_index, *to_next, n_left_to_next;
  vlib_node_runtime_t *error_node = vlib_node_get_runtime(vm, ila_ila2sir_node.index);
  __attribute__((unused)) ila_main_t *ilm = &ila_main;

  from = vlib_frame_vector_args(frame);
  n_left_from = frame->n_vectors;
  next_index = node->cached_next_index;

  while (n_left_from > 0) {
    vlib_get_next_frame(vm, node, next_index, to_next, n_left_to_next);

    /* Single loop */
    while (n_left_from > 0 && n_left_to_next > 0) {
      u32 pi0;
      vlib_buffer_t *p0;
      u8 error0 = ILA_ERROR_NONE;
      __attribute__((unused)) ip6_header_t *ip60;
      u32 next0 = ILA_ILA2SIR_NEXT_DROP;

      pi0 = to_next[0] = from[0];
      from += 1;
      n_left_from -= 1;
      to_next +=1;
      n_left_to_next -= 1;

      p0 = vlib_get_buffer(vm, pi0);
      ip60 = vlib_buffer_get_current(p0);

      if (PREDICT_FALSE(p0->flags & VLIB_BUFFER_IS_TRACED)) {

      }

      p0->error = error_node->errors[error0];
      vlib_validate_buffer_enqueue_x1(vm, node, next_index, to_next, n_left_to_next, pi0, next0);
    }
    vlib_put_next_frame(vm, node, next_index, n_left_to_next);
  }

  return frame->n_vectors;
}

VLIB_REGISTER_NODE (ila_ila2sir_node, static) = {
  .function = ila_ila2sir,
  .name = "ila-ila2sir",
  .vector_size = sizeof (u32),

  .format_trace = format_ila_ila2sir_trace,

  .n_errors = ILA_N_ERROR,
  .error_strings = ila_error_strings,

  .n_next_nodes = ILA_ILA2SIR_N_NEXT,
  .next_nodes = {
      [ILA_ILA2SIR_NEXT_IP6_REWRITE] = "ip6-rewrite",
      [ILA_ILA2SIR_NEXT_DROP] = "error-drop"
    },
};

typedef enum {
  ILA_SIR2ILA_NEXT_DROP,
  ILA_SIR2ILA_N_NEXT,
} ila_sir2ila_next_t;

u8 *
format_ila_sir2ila_trace (u8 *s, va_list *args)
{
  return format(s, "ila-sir2ila");
}

static vlib_node_registration_t ila_sir2ila_node;

static uword
ila_sir2ila (vlib_main_t *vm,
        vlib_node_runtime_t *node,
        vlib_frame_t *frame)
{
  u32 n_left_from, *from, next_index, *to_next, n_left_to_next;
  vlib_node_runtime_t *error_node = vlib_node_get_runtime(vm, ila_sir2ila_node.index);
  __attribute__((unused)) ila_main_t *ilm = &ila_main;

  from = vlib_frame_vector_args(frame);
  n_left_from = frame->n_vectors;
  next_index = node->cached_next_index;

  while (n_left_from > 0) {
    vlib_get_next_frame(vm, node, next_index, to_next, n_left_to_next);

    /* Single loop */
    while (n_left_from > 0 && n_left_to_next > 0) {
      u32 pi0;
      vlib_buffer_t *p0;
      u8 error0 = ILA_ERROR_NONE;
      __attribute__((unused)) ip6_header_t *ip60;
      u32 next0 = ILA_SIR2ILA_NEXT_DROP;

      pi0 = to_next[0] = from[0];
      from += 1;
      n_left_from -= 1;
      to_next +=1;
      n_left_to_next -= 1;

      p0 = vlib_get_buffer(vm, pi0);
      ip60 = vlib_buffer_get_current(p0);

      if (PREDICT_FALSE(p0->flags & VLIB_BUFFER_IS_TRACED)) {

      }

      p0->error = error_node->errors[error0];
      vlib_validate_buffer_enqueue_x1(vm, node, next_index, to_next, n_left_to_next, pi0, next0);
    }
    vlib_put_next_frame(vm, node, next_index, n_left_to_next);
  }

  return frame->n_vectors;
}

VLIB_REGISTER_NODE (ila_sir2ila_node, static) = {
  .function = ila_sir2ila,
  .name = "ila-sir2ila",
  .vector_size = sizeof (u32),

  .format_trace = format_ila_sir2ila_trace,

  .n_errors = ILA_N_ERROR,
  .error_strings = ila_error_strings,

  .n_next_nodes = ILA_SIR2ILA_N_NEXT,
  .next_nodes = {
      [ILA_SIR2ILA_NEXT_DROP] = "error-drop"
    },
};

VNET_IP6_UNICAST_FEATURE_INIT (ila_sir2ila, static) = {
  .node_name = "ila-sir2ila",
  .runs_before = {"ip6-lookup", 0},
  .feature_index = &ila_main.ila_sir2ila_feature_index,
};

int ila_add_del_entry(ila_add_del_entry_args_t *args)
{
  ila_main_t *ilm = &ila_main;
  BVT(clib_bihash_kv) kv, value;

  if (!args->del)
    {
      ila_entry_t *e;
      pool_get(ilm->entries, e);
      e->identifier = args->identifier;
      e->locator = args->locator;
      e->ila_adj_index = args->local_adj_index;
      e->sir_prefix = args->sir_prefix;

      kv.key[0] = args->identifier;
      kv.key[1] = 0;
      kv.key[2] = 0;
      kv.value = e - ilm->entries;
      BV(clib_bihash_add_del) (&ilm->id_to_entry_table, &kv, 0 /* is_add */);

      //TODO: Add the adjacency
    }
  else
    {
      ila_entry_t *e;
      kv.key[0] = args->identifier;
      kv.key[1] = 0;
      kv.key[2] = 0;

      if ((BV(clib_bihash_search)(&ilm->id_to_entry_table, &kv, &value) < 0))
        {
          return -1;
        }

      e = &ilm->entries[value.value];
      //TODO: Del the adjacency

      pool_put(ilm->entries, e);
    }
  return 0;
}

int ila_interface(u32 sw_if_index, u8 disable)
{
  vlib_main_t * vm = vlib_get_main();
  ila_main_t *ilm = &ila_main;
  ip6_main_t * im = &ip6_main;
  ip_lookup_main_t * lm = &im->lookup_main;
  ip_config_main_t * cm = &lm->rx_config_mains[VNET_UNICAST];
  vnet_config_main_t * vcm = &cm->config_main;
  u32 ci, feature_index;

  vec_validate_init_empty (cm->config_index_by_sw_if_index, sw_if_index, ~0);
  ci = cm->config_index_by_sw_if_index[sw_if_index];
  feature_index = ilm->ila_sir2ila_feature_index;

  ci = ((disable)?vnet_config_del_feature:vnet_config_add_feature)
          (vm, vcm,
              ci,
              feature_index,
              /* config data */ 0,
              /* # bytes of config data */ 0);

  cm->config_index_by_sw_if_index[sw_if_index] = ci;
  return 0;
}

clib_error_t *ila_init (vlib_main_t *vm) {
  ila_main_t *ilm = &ila_main;
  ilm->entries = NULL;

  ilm->lookup_table_nbuckets = ILA_TABLE_DEFAULT_HASH_NUM_BUCKETS;
  ilm->lookup_table_nbuckets = 1<< max_log2 (ilm->lookup_table_nbuckets);
  ilm->lookup_table_size = ILA_TABLE_DEFAULT_HASH_MEMORY_SIZE;

  BV(clib_bihash_init) (&ilm->id_to_entry_table, "ila id to entry index table",
                          ilm->lookup_table_nbuckets,
                          ilm->lookup_table_size);

  return NULL;
}

VLIB_INIT_FUNCTION(ila_init);

static clib_error_t *
ila_entry_command_fn (vlib_main_t *vm,
                          unformat_input_t *input,
                          vlib_cli_command_t *cmd)
{
  unformat_input_t _line_input, * line_input = &_line_input;
  ila_add_del_entry_args_t args = {0};
  ip6_address_t identifier, locator, sir;

  args.local_adj_index = ~0;

  if (! unformat_user (input, unformat_line_input, line_input))
    return 0;

  if (unformat (line_input, "%U %U %U",
		unformat_ip6_address, &identifier,
		unformat_ip6_address, &locator,
		unformat_ip6_address, &sir))
    ;
  else if (unformat (line_input, "del"))
    args.del = 1;
  else if (unformat (line_input, "adj-index %u", &args.local_adj_index))
    ;
  else
    return clib_error_return (0, "parse error: '%U'",
                              format_unformat_error, line_input);

  unformat_free (line_input);

  if (identifier.as_u64[CLIB_ARCH_IS_BIG_ENDIAN != 0] != 0)
    return clib_error_return (0, "Identifier upper 64 bits should be 0");

  if (locator.as_u64[CLIB_ARCH_IS_BIG_ENDIAN != 1] != 0)
    return clib_error_return (0, "Locator lower 64 bits should be 0");

  if (sir.as_u64[CLIB_ARCH_IS_BIG_ENDIAN != 1] != 0)
    return clib_error_return (0, "SIR lower 64 bits should be 0");

  args.identifier = identifier.as_u64[CLIB_ARCH_IS_BIG_ENDIAN != 1];
  args.locator = locator.as_u64[CLIB_ARCH_IS_BIG_ENDIAN != 0];
  args.sir_prefix = sir.as_u64[CLIB_ARCH_IS_BIG_ENDIAN != 0];

  ila_add_del_entry(&args);

  return NULL;
}

VLIB_CLI_COMMAND(ila_entry_command, static) = {
  .path = "ila entry",
  .short_help = "ila entry <identifier> <locator> <sir-prefix> [adj-index <adj-index>] [del] ",
  .function = ila_entry_command_fn,
};

static clib_error_t *
ila_interface_command_fn (vlib_main_t *vm,
                          unformat_input_t *input,
                          vlib_cli_command_t *cmd)
{
  vnet_main_t * vnm = vnet_get_main();
  u32 sw_if_index = ~0;
  u8 disable = 0;

  if (!unformat (input, "%U", unformat_vnet_sw_interface,
                 vnm, &sw_if_index)) {
      return clib_error_return (0, "Invalid interface name");
  }

  if (unformat(input, "disable")) {
      disable = 1;
  }

  int ret;
  if ((ret = ila_interface(sw_if_index, disable)))
    return clib_error_return (0, "ila_interface returned error %d", ret);

  return NULL;
}

VLIB_CLI_COMMAND(ila_interface_command, static) = {
  .path = "ila interface",
  .short_help = "ila interface <interface-name> [disable]",
  .function = ila_interface_command_fn,
};


