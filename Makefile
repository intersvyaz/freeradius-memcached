TARGET          = rlm_memcache_ops
SRCS            = rlm_memcache_ops.c
RLM_LIBS        = -lmemcached

include ../rules.mak
