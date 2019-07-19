# p2pop
_peer-to-peer orchestration protocol_

## What is it?
Writing the network layer for peer-to-peer systems is rather tiresome.
Most RPC frameworks are based on TCP, which requires a whole bunch of
NAT tomfoolery.

The aim of p2pop is to provide a simple message-based RPC layer on UDP,
and the utilities to form high throughput connections from them.

We use blake2s-160 as our hash function, and as such most tokens are 20 bytes
(160 bits) in size.

## How?
p2pop uses capnproto for messages, a low-overhead protocol buffers library,
but not its inbuilt RPC, which requires a TCP stream. However, due to the
flexibility of capnp, the same syntax can be used (_TODO: check this_).

Instead, we use a 160-bit uid (called PUIDs, and are most definitely not UUIDs)
to uniquely identify your protocol.

Each node is also assigned a 160-bit uid (called a NUID, also not a UUID) to
identify it. This can be a hash of a public signing key, or just a randomly
generated value.

If an incoming message has your PUID, your interface will be called to deal
with the message.

## Upcoming
* p2popcrypto - protocol to request various crypto ops, as a sort of library
* caching - so that requests per NUID do not have to be repeated
* Channel orchestration - to allow fast streams to be created

## Caveats
Please generate a random PUID to avoid collisions. No-one cares if you embed
your name into the PUID, as it will not be seen outside of your code.


