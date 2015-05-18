# rlm_memcache_ops

### Build instructions
Separate build Using CMAKE:
```bash
mkdir build
cd build
cmake ..
make
```

For in-tree build just copy source files to `src/modules/rlm_memcache_ops` and add module in Make.inc MDOULES section.


### Config examples

```
# Get value
memcache_ops get_example {
    config = "--SERVER=127.0.0.1"
    action = "get"
    key = "ip:%{User-Name}"
    output_attr = "Calling-Station-Id"
}

# Set value
memcache_ops set_example {
    config = "--SERVER=127.0.0.1"
    action = "set"
    key = "nasip:%{Calling-Station-Id}"
    value = "%{Nas-Ip-Address}"
}
```