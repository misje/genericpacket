# Generic packet

There are heaps of serialisation libraries, but most are not self-delimiting or
selv-describing. With stream-oriented protocols just serialising your message
isn't enough. At a bare minimum you need to know the length of the message you
need to put together.

This component is a small wrapper around your payload data, giving you the
payload's length and a variable to identify what the payload is (the `type`).
It is written using Qt's `QByteArray` as the payload data type, but you can
adapt it fairly easily to your need.

## Usage

Decide what data types you need to represent your packages' maximum length and
type. The default is using std::uint32_t for both. Only the following are
supported, because the `hton{s,l}`/`ntoh{s,l}` functions are used to convert the length and
type to and from network byte order:

* std::uint8_t (no byte order conversion is done)
* std::uint16_t (ntohs/htons is used)
* std::uint32_t (ntohl/htnl is used)

### Packing
```c++
using Packet = GenericPacket<std::uint16_t, std::uint8_t>;

socket.write(Packet{Packet::Type{42}, {"My beautiful message"}}.toData());
/* Let us see what this is packed as: */
qDebug() << Packet{Type{42}, {"My beautiful message"}}.toData().toHex(' ');
```
The result is *00 14 2a 4d 79 20 62 65 61 75 74 69 66 75 6c 20 6d 65 73 73 61 67 65*.
- *00 14* is the packet length (16-bit little endian) (the length of the string *My beautiful message*)
- *2a* is the packet type (42)
- The rest is the payload as-is

The length is calculated automatically based on the payload and does not
include the three-byte (in this case) header. The type can be used as you
please, but it needs to specified as a named/strong type to avoid unintentional
bugs mixing up *size* and *type* in the API.

The API provides more methods, like setting the payload and manually
manipulating the header, but in most cases you'd like to use it like shown
above. Note that that exceptions may be thrown in the following case:

If `data` in `GenericPacket(Type type, const QByteArray &payload)` or
`GenericPacket(Type type, QByteArray &&payload)` is larger than the maximum
number the chosen **size** data type is able to represent, a
`std::range_error` is thrown. Make sure the *size* data type can handle all the
payloads you want to pack.

### Unpacking
The functions
```c++
static bool GenericPacket::Header::hasCompleteHeader(const QByteArray &data);
/* and */
static bool GenericPacket::hasCompletePacket(const QByteArray &data);
```
allows you to check a data array of arbitrary length to ensure that either
- the header (the first *n* bytes) are present, or that
- the whole packet, the header and the following bytes matching the header
  *size* variable, is present in the raw data

Once you are sure that the raw data can contain a complete packet, you can
either create a `GenericPacket` on a copy of the raw data or replace the
relevant raw data with a packet:
```c++
using Packet = GenericPacket<std::uint16_t, std::uint8_t>;
const auto data = socket.readAll();
if (Packet::hasCompletePacket(data))
{
	const auto packet = Packet::fromData(data);
	qDebug() << "Type:" << packet.header().type();
	qDebug() << "Size:" << packet.header().size();
	qDebug() << "Data:" << packet.payload().toHex(' ');
}
```
or similarly by removing the bytes:
```c++
using Packet = GenericPacket<std::uint16_t, std::uint8_t>;
while (Packet::hasCompletePacket(buffer))
{
	/* The 'buffer' does not longer contain whatever makes up 'packet': */
	auto packet = Packet::extractFromData(buffer);
	if (packet.header().type() != MyExpectedPacketType)
		continue;

	doSomethingWithPayload(packet.payload());
}
```

Beware that exceptions may be thrown if you attempt to parse an imcomplete
packet. Specifially, if you try to construct a `Header`from raw data whose size
is smaller than that of the header (the sum of the size of the data types used
for *size* and *type*) a `std::range_error` is thrown:
```c++
/* When calling */
GenericPacket::Header GenericPacket::Header::fromData(const QByteArray &data)
/* or */
GenericPacket::Header GenericPacket::Header::extractFromData(QByteArray &data)
/* make sure that that 'data' is big enough to populate the header */
```

The same thing goes for trying to construct a whole packet from the raw data.
Make sure the raw data actually contains
- Enough bytes to cover the *size* and *type* field of the header, and
- 0 or more bytes to make up the payload (it can be empty)
```c++
/* When calling */
GenericPacket GenericPacket::fromData(const QByteArray &data)
/* or */
GenericPacket GenericPacket::extractFromData(QByteArray &data)
/* make sure that 'data' is large enough. Typically you do this by calling */
GenericPacket::hasCompletePacket() beforehand */
```
