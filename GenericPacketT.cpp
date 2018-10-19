#include <cstddef>
#include <arpa/inet.h>
#include <exception>
#include <memory>
#include <typeinfo>
#include <limits>

namespace GenericPacketHelper
{
	inline std::uint8_t ntoh(std::uint8_t value)
	{
		return value;
	}

	inline std::uint16_t ntoh(std::uint16_t value)
	{
		return ntohs(value);
	}

	inline std::uint32_t ntoh(std::uint32_t value)
	{
		return ntohl(value);
	}

	inline std::uint8_t hton(std::uint8_t value)
	{
		return value;
	}

	inline std::uint16_t hton(std::uint16_t value)
	{
		return htons(value);
	}

	inline std::uint32_t hton(std::uint32_t value)
	{
		return htonl(value);
	}

#pragma pack(1)
template<typename S, typename T>
struct RawHeader
{
	S size;
	T type;

	static inline RawHeader fromData(const QByteArray &data)
	{
		const RawHeader *header = reinterpret_cast<const RawHeader *>(data.constData());
		return { ntoh(header->size), ntoh(header->type) };
	}
};
#pragma pack()
}


template<typename S, typename T>
GenericPacket<S, T>::Header::Header(Size size, Type type)
	: m_size(size),
	m_type(type)
{
	static_assert(std::is_pod<GenericPacketHelper::RawHeader<S, T>>::value, "RawHeader must be a POD type");
}

template<typename S, typename T>
bool GenericPacket<S, T>::Header::hasCompleteHeader(const QByteArray &data)
{
	return static_cast<std::size_t>(data.size()) >= sizeof(GenericPacketHelper::RawHeader<S, T>);
}

template<typename S, typename T>
typename GenericPacket<S, T>::Header GenericPacket<S, T>::Header::fromData(const QByteArray &data)
{
	if (!hasCompleteHeader(data))
		throw std::length_error("Data is not big enough to contain a header");

	return Header{data};
}

template<typename S, typename T>
typename GenericPacket<S, T>::Header GenericPacket<S, T>::Header::extractFromData(QByteArray &data)
{
	if (!hasCompleteHeader(data))
		throw std::length_error("Data is not big enough to contain a header");

	return Header{data};
}

template<typename S, typename T>
typename GenericPacket<S, T>::Header &GenericPacket<S, T>::Header::setSize(Size size)
{
	m_size = size;
	return *this;
}

template<typename S, typename T>
typename GenericPacket<S, T>::Header &GenericPacket<S, T>::Header::setType(Type type)
{
	m_type = type;
	return *this;
}

template<typename S, typename T>
QByteArray GenericPacket<S, T>::Header::toData() const
{
	GenericPacketHelper::RawHeader<S, T> raw{
		GenericPacketHelper::hton(m_size),
		GenericPacketHelper::hton(m_type),
	};
	return { reinterpret_cast<const char *>(&raw), sizeof(raw) };
}

template<typename S, typename T>
GenericPacket<S, T>::Header::Header(const QByteArray &data)
	: m_size(GenericPacketHelper::RawHeader<S, T>::fromData(data).size),
	m_type(GenericPacketHelper::RawHeader<S, T>::fromData(data).type)
{
}

template<typename S, typename T>
GenericPacket<S, T>::Header::Header(QByteArray &data)
	: Header(static_cast<const QByteArray &>(data.remove(0, sizeof(GenericPacketHelper::RawHeader<S, T>))))
{
}

template<typename S, typename T>
GenericPacket<S, T>::GenericPacket(Type type, const QByteArray &payload)
	: m_header(Size(static_cast<S>(payload.size())), type),
	m_payload(payload)
{
	if (static_cast<std::size_t>(m_payload.size()) > std::numeric_limits<S>::max())
		throw std::range_error(QObject::tr("The datatype chosen to represent the "
					"packet length in the header (maximum value %2) cannot hold the payload "
					"size (%3)")
				.arg(std::numeric_limits<S>::max())
				.arg(m_payload.size())
				.toStdString());
}

template<typename S, typename T>
GenericPacket<S, T>::GenericPacket(Type type, QByteArray &&payload)
	: m_header(Size(static_cast<S>(payload.size())), type),
	m_payload(std::move(payload))
{
	if (static_cast<std::size_t>(m_payload.size()) > std::numeric_limits<S>::max())
		throw std::range_error(QObject::tr("The datatype chosen to represent the "
					"packet length in the header (maximum value %2) cannot hold the payload "
					"size (%3)")
				.arg(std::numeric_limits<S>::max())
				.arg(m_payload.size())
				.toStdString());
}

template<typename S, typename T>
bool GenericPacket<S, T>::hasCompletePacket(const QByteArray &data)
{
	return
		Header::hasCompleteHeader(data) &&
		/* Ensure that the data contains the whole payload, determined by the
		 * size field in the header: */
		data.size() - sizeof(GenericPacketHelper::RawHeader<S, T>) >= Header::fromData(data).size();
}

template<typename S, typename T>
GenericPacket<S, T> GenericPacket<S, T>::fromData(const QByteArray &data)
{
	if (!hasCompletePacket(data))
		throw std::length_error("Data is not big enough to contain a packet");

	return GenericPacket<S, T>(data);
}

template<typename S, typename T>
GenericPacket<S, T> GenericPacket<S, T>::extractFromData(QByteArray &data)
{
	if (!hasCompletePacket(data))
		throw std::length_error("Data is not big enough to contain a packet");

	return GenericPacket<S, T>(data);
}

template<typename S, typename T>
GenericPacket<S, T> &GenericPacket<S, T>::setPayload(const QByteArray &payload)
{
	m_payload = payload;
	m_header.setSize(Size{static_cast<S>(m_payload.size())});
	return *this;
}

template<typename S, typename T>
GenericPacket<S, T> &GenericPacket<S, T>::setPayload(QByteArray &&payload)
{
	m_payload = std::move(payload);
	m_header.setSize(Size{static_cast<S>(m_payload.size())});
	return *this;
}

template<typename S, typename T>
QByteArray GenericPacket<S, T>::toData() const
{
	return m_header.toData() + m_payload;
}

template<typename S, typename T>
GenericPacket<S, T>::GenericPacket(const QByteArray &data)
	: m_header(Header::fromData(data)),
	m_payload(data.mid(sizeof(GenericPacketHelper::RawHeader<S, T>), m_header.size()))
{
}

template<typename S, typename T>
GenericPacket<S, T>::GenericPacket(QByteArray &data)
	: m_header(Header::extractFromData(data)),
	m_payload(data.mid(sizeof(GenericPacketHelper::RawHeader<S, T>), m_header.size()))
{
	data.remove(0, sizeof(GenericPacketHelper::RawHeader<S, T>) + m_header.size());
}
