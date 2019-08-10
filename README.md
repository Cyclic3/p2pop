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
p2pop uses protobuf for messages, a low-overhead protocol buffers library,
but not its inbuilt RPC, which requires a TCP stream. However, due to the
flexibility of protobuf, the same syntax can be used.

Instead, we use a 64-bit uid to uniquely identify your protocol.

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

## RPC description
Use the protobuf service/rpc syntax as normal, but be aware that the RPC's
index is the only thing used to distinguish it, so reordering, insertation
and deletion will break backwards-compatiblity.

## Code style
I use pointers to signify non-owning or copying references. If a constructor
takes a pointer to an object, that object must survive until the object is
moved into, or destroyed.
