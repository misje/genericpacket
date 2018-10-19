#include "GenericPacket.h"
#include <cstddef>
#include <arpa/inet.h>
#include <exception>
#include <memory>

#pragma pack(1)
struct RawHeader
{
	std::uint32_t size;
	std::uint32_t type;

	static inline RawHeader fromData(const QByteArray &data)
	{
		const RawHeader *header = reinterpret_cast<const RawHeader *>(data.constData());
		return { ntohl(header->size), ntohl(header->type) };
	}
};
#pragma pack()
static_assert(std::is_pod<RawHeader>::value, "RawHeader must be a POD type");

GenericPacket::Header::Header(Size size, Type type)
	: m_size(size),
	m_type(type)
{
}

bool GenericPacket::Header::hasCompleteHeader(const QByteArray &data)
{
	return static_cast<std::size_t>(data.size()) >= sizeof(RawHeader);
}

GenericPacket::Header GenericPacket::Header::fromData(const QByteArray &data)
{
	if (!hasCompleteHeader(data))
		throw std::length_error("Data is not big enough to contain a header");

	return Header{data};
}

GenericPacket::Header GenericPacket::Header::extractFromData(QByteArray &data)
{
	if (!hasCompleteHeader(data))
		throw std::length_error("Data is not big enough to contain a header");

	return Header{data};
}

GenericPacket::Header &GenericPacket::Header::setSize(Size size)
{
	m_size = size;
	return *this;
}

GenericPacket::Header &GenericPacket::Header::setType(Type type)
{
	m_type = type;
	return *this;
}

QByteArray GenericPacket::Header::toData() const
{
	RawHeader raw{ htonl(m_size), htonl(m_type) };
	return { reinterpret_cast<const char *>(&raw), sizeof(raw) };
}

GenericPacket::Header::Header(const QByteArray &data)
	: m_size(RawHeader::fromData(data).size),
	m_type(RawHeader::fromData(data).type)
{
}

GenericPacket::Header::Header(QByteArray &data)
	: Header(static_cast<const QByteArray &>(data.remove(0, sizeof(RawHeader))))
{
}

GenericPacket::GenericPacket(Type type, const QByteArray &payload)
	: m_header(Size{static_cast<std::uint32_t>(payload.size())}, type),
	m_payload(payload)
{
}

GenericPacket::GenericPacket(Type type, QByteArray &&payload)
	: m_header(Size{static_cast<std::uint32_t>(payload.size())}, type),
	m_payload(std::move(payload))
{
}

bool GenericPacket::hasCompletePacket(const QByteArray &data)
{
	return
		Header::hasCompleteHeader(data) &&
		/* Ensure that the data contains the whole payload, determined by the
		 * size field in the header: */
		data.size() - sizeof(RawHeader) >= Header::fromData(data).size();
}

GenericPacket GenericPacket::fromData(const QByteArray &data)
{
	if (!hasCompletePacket(data))
		throw std::length_error("Data is not big enough to contain a packet");

	return GenericPacket(data);
}

GenericPacket GenericPacket::extractFromData(QByteArray &data)
{
	if (!hasCompletePacket(data))
		throw std::length_error("Data is not big enough to contain a packet");

	return GenericPacket(data);
}

GenericPacket &GenericPacket::setPayload(const QByteArray &payload)
{
	m_payload = payload;
	m_header.setSize(Size{static_cast<std::uint32_t>(m_payload.size())});
	return *this;
}

GenericPacket &GenericPacket::setPayload(QByteArray &&payload)
{
	m_payload = std::move(payload);
	m_header.setSize(Size{static_cast<std::uint32_t>(m_payload.size())});
	return *this;
}

QByteArray GenericPacket::toData() const
{
	return m_header.toData() + m_payload;
}

GenericPacket::GenericPacket(const QByteArray &data)
	: m_header(Header::fromData(data)),
	m_payload(data.mid(sizeof(RawHeader), m_header.size()))
{
}

GenericPacket::GenericPacket(QByteArray &data)
	: m_header(Header::extractFromData(data)),
	m_payload(data.mid(sizeof(RawHeader), m_header.size()))
{
	data.remove(0, sizeof(RawHeader) + m_header.size());
}
