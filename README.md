# FreeRADIUS 3.x memcached module

Simple module for performing get/set memcached operations.

### Config examples
```
# Get value
memcached memcached_get_example {
    # Perform get action
    action = "get"

    # Memcached configuration options, as documented here:
    #    http://docs.libmemcached.org/libmemcached_configuration.html#memcached
    config = "--SERVER=127.0.0.1"

    # Connection pool
    pool {
        start = ${thread[pool].start_servers}
        min = ${thread[pool].min_spare_servers}
        max = ${thread[pool].max_servers}
        spare = ${thread[pool].max_spare_servers}
        uses = 0
        lifetime = 0
        idle_timeout = 60
    }
    
    # Key (support attributes substitution)
    key = "ip:%{User-Name}"
    
    # Output attribute
    output_attr = &Calling-Station-Id
}

# Set value
memcached memcached_set_example {
    # Perform set action
    action = "set"

    # Memcached configuration options, as documented here:
    #    http://docs.libmemcached.org/libmemcached_configuration.html#memcached
    config = "--SERVER=127.0.0.1"
    
    # Connection pool
    pool {
        start = ${thread[pool].start_servers}
        min = ${thread[pool].min_spare_servers}
        max = ${thread[pool].max_servers}
        spare = ${thread[pool].max_spare_servers}
        uses = 0
        lifetime = 0
        idle_timeout = 60
    }
    
    # Key (support attributes substitution)
    key = "nasip:%{Calling-Station-Id}"
    
    # Value (support attributes substitution)
    value = &Nas-Ip-Address
    
    # Record TTL in seconds (optional, default value is 0)
    ttl = 0
}
```
