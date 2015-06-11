# lock-experiments
Experiments with locking primitives on shared filesystems.

Imagine you have 3 hosts: `host1`, `host2` and `host3`. Each would run
the same daemon with slightly different arguments

```
# on host1
./main --uuid host1 host2 host3

# on host2
./main --uuid host2 host1 host3

# on host3
./main --uuid host3 host1 host2
```

