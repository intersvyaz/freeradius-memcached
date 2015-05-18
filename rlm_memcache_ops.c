#ifdef STANDALONE_BUILD
#include <freeradius/radiusd.h>
#include <freeradius/modules.h>
#else
#include <freeradius-devel/radiusd.h>
#include <freeradius-devel/modules.h>
#endif
#include <libmemcached/memcached.h>

enum {
    RLM_MEMCACHE_OPS_GET,
    RLM_MEMCACHE_OPS_SET
};

typedef struct rlm_memcache_ops_t {
    struct {
        char *config;       /* memcache config string, like "--SERVER=127.0.0.1" */
        char *action;       /* get or set */
        char *key;          /* key string */
        char *value;        /* value string */
        char *output_attr;  /* output attribute name */
        int ttl;            /* memcache record ttl in seconds */
    } cfg;

    struct memcached_st *mc;
    int action;
    DICT_ATTR *output_attr;
} rlm_memcache_ops_t;

static CONF_PARSER module_config[] = {
    {"config", PW_TYPE_STRING_PTR, offsetof(rlm_memcache_ops_t, cfg.config), NULL, ""},
    {"action", PW_TYPE_STRING_PTR, offsetof(rlm_memcache_ops_t, cfg.action), NULL, ""},
    {"key", PW_TYPE_STRING_PTR, offsetof(rlm_memcache_ops_t, cfg.key), NULL, ""},
    {"value", PW_TYPE_STRING_PTR, offsetof(rlm_memcache_ops_t, cfg.value), NULL, ""},
    {"output_attr", PW_TYPE_STRING_PTR, offsetof(rlm_memcache_ops_t, cfg.output_attr), NULL, ""},
    {"ttl", PW_TYPE_INTEGER, offsetof(rlm_memcache_ops_t, cfg.ttl), NULL, "0"},
    {NULL, -1, 0, NULL, NULL}
};

static int memcache_ops_instantiate(CONF_SECTION *conf, void **instance)
{
    rlm_memcache_ops_t *inst;

    inst = rad_malloc(sizeof(rlm_memcache_ops_t));
    if (!inst) {
        return -1;
    }
    memset(inst, 0, sizeof(*inst));

    if (cf_section_parse(conf, inst, module_config) < 0) {
        goto err;
    }

    if (strcasecmp(inst->cfg.action, "get") == 0) {
        inst->action = RLM_MEMCACHE_OPS_GET;
    } else if (strcasecmp(inst->cfg.action, "set") == 0) {
        inst->action = RLM_MEMCACHE_OPS_SET;
    } else {
        radlog(L_ERR, "rlm_memcache_ops: Invalid action '%s', only 'get' or 'set' is acceptable", inst->cfg.action);
        goto err;
    }

    if (RLM_MEMCACHE_OPS_GET == inst->action) {
        inst->output_attr = dict_attrbyname(inst->cfg.output_attr);
        if (!inst->output_attr) {
            radlog(L_ERR, "rlm_memcache_ops: No such attribute: %s", inst->cfg.output_attr);
            goto err;
        }
    }



    char err_buf[256] = "\0";
    memcached_return_t mret = libmemcached_check_configuration(inst->cfg.config, strlen(inst->cfg.config), err_buf, sizeof(err_buf));
    if (MEMCACHED_SUCCESS != mret) {
        radlog(L_ERR, "rlm_memcache_ops: Invalid memcache config string: %s", err_buf);
        goto err;
    }

    inst->mc = memcached(inst->cfg.config, strlen(inst->cfg.config));
    if(inst->mc == NULL){
        radlog(L_ERR, "rlm_memcache_ops: Failed to create memcached instance");
        goto err;
    }

    *instance = inst;
    return 0;

err:
    free(inst);
    return -1;
}

static int memcache_ops_detach(void *instance)
{
    rlm_memcache_ops_t *inst = instance;

    memcached_free(inst->mc);
    free(instance);

    return 0;
}

static int memcache_ops_proc(void *instance, REQUEST *request)
{
    rlm_memcache_ops_t *inst = instance;
    memcached_return_t mret;

    char key[MAX_STRING_LEN];
    int key_len = radius_xlat(key, sizeof(key), inst->cfg.key, request, NULL);

    if (!key_len) {
        return RLM_MODULE_NOOP;
    }

    if (RLM_MEMCACHE_OPS_GET == inst->action) {
        size_t value_len = 0;
        uint32_t flags = 0;
        char *value = memcached_get(inst->mc, key, key_len, &value_len, &flags, &mret);

        if (MEMCACHED_SUCCESS != mret) {
            DEBUG2("rlm_memcache_ops: get failed: %s", memcached_strerror(inst->mc, mret));
            return RLM_MODULE_NOOP;
        } else {
            VALUE_PAIR *vvp = pairfind(request->packet->vps, inst->output_attr->attr);
            if (!vvp) {
                vvp = paircreate(inst->output_attr->attr, inst->output_attr->type);
                pairadd(&request->packet->vps, vvp);
            }
            pairparsevalue(vvp, value);

            DEBUG2("rlm_memcache_ops: get success %s=%s", inst->cfg.output_attr, vvp->vp_strvalue);
            return RLM_MODULE_UPDATED;
        }
    } else {
        char value[MAX_STRING_LEN];
        int value_len = radius_xlat(value, sizeof(value), inst->cfg.value, request, NULL);

        mret = memcached_set(inst->mc, key, key_len, value, value_len, inst->cfg.ttl, 0);
        if (MEMCACHED_SUCCESS != mret) {
            DEBUG2("rlm_memcache_ops: set failed: %s", memcached_strerror(inst->mc, mret));
        } else {
            DEBUG2("rlm_memcache_ops: set success %s=%s", key, value);
        }

        return RLM_MODULE_NOOP;
    }
}

/* globally exported name */
module_t rlm_memcache_ops = {
    RLM_MODULE_INIT,
    "memcache_ops",
    RLM_TYPE_THREAD_UNSAFE,
    memcache_ops_instantiate,   /* instantiation */
    memcache_ops_detach,        /* detach */
    {
        memcache_ops_proc,  /* authentication */
        memcache_ops_proc,  /* authorization */
        memcache_ops_proc,  /* preaccounting */
        memcache_ops_proc,  /* accounting */
        NULL,               /* checksimul */
        NULL,               /* pre-proxy */
        NULL,               /* post-proxy */
        NULL                /* post-auth */
    },
};
