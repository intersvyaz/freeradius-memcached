#include <freeradius-devel/radiusd.h>
#include <freeradius-devel/modules.h>
#include <libmemcached/memcached.h>

enum {
  RLM_MEMCACHED_OPS_GET,
  RLM_MEMCACHED_OPS_SET
};

typedef struct rlm_memcached {
  struct {
    const char *action;
    const char *config;
    vp_tmpl_t *key;
    vp_tmpl_t *value;
    vp_tmpl_t *output_attr;
    int ttl;
  } cfg;

  int action;
  const char *name;
  fr_connection_pool_t *pool;
} rlm_memcached_t;

typedef struct rlm_memcached_conn {
  struct memcached_st *mc;
} rlm_memcached_conn_t;

static const CONF_PARSER module_config[] = {
    {"action", FR_CONF_OFFSET(PW_TYPE_STRING | PW_TYPE_NOT_EMPTY, rlm_memcached_t, cfg.action), NULL},
    {"config", FR_CONF_OFFSET(PW_TYPE_STRING | PW_TYPE_NOT_EMPTY, rlm_memcached_t, cfg.config), NULL},
    {"key", FR_CONF_OFFSET(PW_TYPE_STRING | PW_TYPE_TMPL | PW_TYPE_NOT_EMPTY, rlm_memcached_t, cfg.key), NULL},
    {"value", FR_CONF_OFFSET(PW_TYPE_STRING | PW_TYPE_TMPL, rlm_memcached_t, cfg.value), NULL},
    {"output_attr", FR_CONF_OFFSET(PW_TYPE_STRING | PW_TYPE_TMPL, rlm_memcached_t, cfg.output_attr), NULL},
    {"ttl", FR_CONF_OFFSET(PW_TYPE_SIGNED, rlm_memcached_t, cfg.ttl), "0"},
    CONF_PARSER_TERMINATOR
};

/**
 * Detach module.
 * @param[in] instance Module instance.
 * @return Zero on success.
 */
static int mod_detach(void *instance) {
  rlm_memcached_t *inst = instance;
  fr_connection_pool_free(inst->pool);
  return 0;
}

/**
 * Module connection destructor.
 * @param[in] conn Connection handle.
 * @return Zero on success.
 */
static int mod_conn_free(rlm_memcached_conn_t *conn) {
  memcached_free(conn->mc);
  DEBUG("rlm_memcached: closed connection");
  return 0;
}

/**
 * Module connection constructor.
 * @param[in] ctx Talloc context.
 * @param[in] instance Module instance.
 * @return NULL on error, else a connection handle.
 */
static void *mod_conn_create(TALLOC_CTX *ctx, void *instance) {
  rlm_memcached_t *inst = instance;

  struct memcached_st *mc = memcached(inst->cfg.config, strlen(inst->cfg.config));
  if (!mc) {
    ERROR("rlm_memcached (%s): failed to create memcached instance", inst->name);
    return NULL;
  }

  rlm_memcached_conn_t *conn = talloc_zero(ctx, rlm_memcached_conn_t);
  conn->mc = mc;
  talloc_set_destructor(conn, mod_conn_free);

  return conn;
}

/**
 * Instantiate module.
 * @param[in] conf Module config.
 * @param[in] instance Module instance.
 * @return Zero on success.
 */
static int mod_instantiate(CONF_SECTION *conf, void *instance) {
  rlm_memcached_t *inst = instance;
  bool ok = true;

  inst->name = cf_section_name2(conf);
  if (!inst->name) {
    inst->name = cf_section_name1(conf);
  }

  if (!strcasecmp(inst->cfg.action, "get")) {
    inst->action = RLM_MEMCACHED_OPS_GET;
    if (!inst->cfg.output_attr || (inst->cfg.output_attr->type != TMPL_TYPE_ATTR)) {
      cf_log_err_cs(conf, "invalid option 'output_attr'");
      ok = false;
    }
  } else if (!strcasecmp(inst->cfg.action, "set")) {
    inst->action = RLM_MEMCACHED_OPS_SET;
    if (!inst->cfg.value) {
      cf_log_err_cs(conf, "invalid option 'value'");
      ok = false;
    }
  } else {
    cf_log_err_cs(conf, "invalid option 'action', use 'get' or 'set'");
    ok = false;
  }

  char err_buf[256] = {0};
  memcached_return_t mret = libmemcached_check_configuration(inst->cfg.config, strlen(inst->cfg.config),
                                                             err_buf, sizeof(err_buf));
  if (mret != MEMCACHED_SUCCESS) {
    cf_log_err_cs(conf, "invalid option 'config': %s", err_buf);
    ok = false;
  }

  inst->pool = fr_connection_pool_module_init(conf, inst, mod_conn_create, NULL, inst->name);
  if (!inst->pool) {
    ok = false;
  }

  return ok ? 0 : -1;
}

/**
 * Main module procedure.
 * @param[in] instance Module instance.
 * @param[in] request Radius request.
 * @return One of #rlm_rcode_t.
 */
static rlm_rcode_t mod_proc(void *instance, REQUEST *request) {
  rlm_memcached_t *inst = instance;
  rlm_memcached_conn_t *conn = NULL;
  rlm_rcode_t code = RLM_MODULE_FAIL;
  memcached_return_t mret;

  conn = fr_connection_get(inst->pool);
  if (!conn) {
    goto end;
  }

  char *key = NULL;
  ssize_t key_len = tmpl_aexpand(request, &key, request, inst->cfg.key, NULL, NULL);

  if (key_len < 0) {
    goto end;
  } else if (key_len == 0) {
    RDEBUG2("empty key, will not make request");
    goto end;
  }

  if (inst->action == RLM_MEMCACHED_OPS_GET) {
    uint32_t flags = 0;
    size_t value_len = 0;
    char *value = memcached_get(conn->mc, key, (size_t) key_len, &value_len, &flags, &mret);

    if (mret == MEMCACHED_NOTFOUND) {
      code = RLM_MODULE_NOTFOUND;
      goto end;
    } else if (mret != MEMCACHED_SUCCESS) {
      RWDEBUG2("failed to get %s: %s", key, memcached_strerror(conn->mc, mret));
      goto end;
    }

    VALUE_PAIR *vp = NULL;
    if (tmpl_find_vp(&vp, request, inst->cfg.output_attr) != 0) {
      VALUE_PAIR **vps = radius_list(request, inst->cfg.output_attr->tmpl_list);
      vp = fr_pair_afrom_da(request, inst->cfg.output_attr->tmpl_da);
      fr_pair_add(vps, vp);
    }
    fr_pair_value_from_str(vp, value, (size_t) value_len);

    RDEBUG2("set %s = %s", inst->cfg.output_attr->name, value);
    code = RLM_MODULE_UPDATED;
  } else {
    char *value = NULL;
    ssize_t value_len = tmpl_aexpand(request, &value, request, inst->cfg.value, NULL, NULL);

    if (value_len < 0) {
      RERROR("failed to substitute attributes for value '%s'", inst->cfg.value->name);
      goto end;
    }

    mret = memcached_set(conn->mc, key, (size_t) key_len, value, (size_t) value_len, inst->cfg.ttl, 0);
    if (mret != MEMCACHED_SUCCESS) {
      RERROR("write '%s' = '%s' failed: %s", key, value, memcached_strerror(conn->mc, mret));
      goto end;
    }

    RDEBUG2("wrote '%s' = '%s'", key, value);
    code = RLM_MODULE_OK;
  }

  end:
  if (conn) fr_connection_release(inst->pool, conn);
  return code;
}

/* globally exported name */
extern module_t rlm_memcached;
module_t rlm_memcached = {
    .magic = RLM_MODULE_INIT,
    .name = "memcached",
    .type = RLM_TYPE_THREAD_SAFE | RLM_TYPE_HUP_SAFE,
    .inst_size = sizeof(rlm_memcached_t),
    .config = module_config,
    .bootstrap = NULL,
    .instantiate = mod_instantiate,
    .detach = mod_detach,
    .methods = {
        [MOD_AUTHENTICATE] = mod_proc,
        [MOD_AUTHORIZE] = mod_proc,
        [MOD_PREACCT] = mod_proc,
        [MOD_ACCOUNTING] = mod_proc,
        [MOD_SESSION] = NULL,
        [MOD_PRE_PROXY] = mod_proc,
        [MOD_POST_PROXY] = mod_proc,
        [MOD_POST_AUTH] = mod_proc,
#ifdef WITH_COA
        [MOD_RECV_COA] = mod_proc,
        [MOD_SEND_COA] = mod_proc,
#endif
    },
};
