squidHelper
===========

Multi-threaded external ACL helper for Squid to dynamically deny/allow internet access for clients. Comes with three pluggable lookup backends including Memcached, LDAP and flat text files. Supports IPv6.

### Available Backends

* **txtfile** - Very simple backend using plain text files (one host per line, format: *192.168.1.41 ALLOW*)
* **memcached** - Memcached backend (volatile!) - itended to be used as a first-level cache for other backends
* **activedirectory** - LDAP backend.

### Requirements

* autotools
* libev
* libconfuse
* libldap (required for LDAP backend)
* libmemcached (required for Memcached backend)


### Installation

1. Grab the source: git clone https://github.com/nischu7/squidHelper.git
2. Run ./bootstrap to setup the build environment
3. ./configure && make && make install


### Testing

As Squid simply communicates with external helpers via stdin/stdout, we can easily test the helper by running it interactively:

```text
webcontrol_helper -p "txtfile" -c example.conf -t 4 
```

Whereas **-p** takes a comma-separated list of plugins, **-c** the config file name and **-t** specifies the number of threads to start.

Newer versions of Squid (3.x, 2.7?) support helpers performing multiple lookups simultaneously. This is achieved by prepending a channel number to every lookup request made.
To perform a lookup, simply type an arbitrary channel number followed by the client's IP address (both IPv4 and IPv6 are supported):
```text
> 1 192.168.1.41
```

The helper should reply immeditately:
```text
1 OK
```


### Configuring Squid

Modify your proxy's configuration file (which lives in /etc/squid3/squid.conf on Debian/Ubuntu machines):


```text
external_acl_type webcontrol children=1 concurrency=500 %SRC /full/path/to/helper 
acl allowedmachines external webcontrol
http_access allow allowedmachines
```
