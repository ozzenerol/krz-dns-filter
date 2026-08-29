# krz-dns-filter

A small UDP DNS server that blocks domains from a blacklist and forwards everything else to an upstream resolver.

I personally point my machines to it so I get less tracked.

!!!!!!!!!! DO NOT ADD THIS TO A ROUTER !!!!!!!!!!

## Requirements

- A C compiler (gcc or clang)
- make
- Linux (uses epoll)

## Build

```
make
```

## Run

```
./bin/krz-dns-filter blacklist.txt [port]
```

The blacklist file should have one domain per line. Lines starting with `#` are ignored. Port defaults to 5354 if not given.

## Test

```
dig @127.0.0.1 -p 5354 example.com
```

Querying a domain that is in the blacklist should return NXDOMAIN. Anything else gets forwarded upstream and the real answer is returned.

## Notes

TCP is not supported yet, only UDP.
The current blocklist has around 260_000 domains registered.

## Clean

```
make clean
```

Removes build artifacts. Use `make distclean` to also remove the compiled binary.
