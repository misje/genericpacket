#pragma once
#include <cstdint>
#include <QByteArray>
#include <cstddef>
#include <arpa/inet.h>
#include <exception>
#include <memory>
#include <limits>

template<typename S = std::uint32_t, typename T = std::uint32_t>
class GenericPacket
{
public:
	/* Thin named type wrapper with explicit constructor and implicit casting
	 * to integer used to distinguish integer arguments: */
	template<typename U>
	struct NamedType
	{
		explicit NamedType(U val) : value(val) {}
		inline operator U() { return value; }
		U value;
	};
	using Size = NamedType<S>;
	using Type = NamedType<T>;

	class Header
	{
	public:
		Header() = default;
		Header(Size size, Type type);

		/** \brief Check if data may contain a complete header */
		static bool hasCompleteHeader(const QByteArray &data);

		/** \brief Deep-copy a header from the given raw data
		 *
		 * Throws a std::length_error if the header is not complete.
		 */
		static Header fromData(const QByteArray &data);
		/** \brief Extract bytes from the raw data and construct a header
		 *
		 * Throws a std::length_error if the header is not complete.
		 */
		static Header extractFromData(QByteArray &packet);

		/** \brief Size of payload */
		S size() const { return m_size; }
		T type() const { return m_type; }

		Header &setSize(Size size);
		Header &setType(Type type);

		QByteArray toData() const;
		static std::size_t dataSize();

		/** \brief The largest payload size the size type can hold */
		static std::size_t maxSize();

	private:
		explicit Header(const QByteArray &data);
		explicit Header(QByteArray &data);

		S m_size = 0;
		T m_type = 0;
	};

	GenericPacket() = default;
	GenericPacket(Type type, const QByteArray &payload);
	GenericPacket(Type type, QByteArray &&payload);

	/** \brief Check if data has a complete header and a complete payload */
	static bool hasCompletePacket(const QByteArray &data);
	/** \brief Deep-copy a packet from the given raw data
	 *
	 * Throws a std::length_error if the packet is not complete.
	 */
	static GenericPacket fromData(const QByteArray &data);
	/** \brief Extract bytes from the raw data and construct a packet
	 *
	 * Throws a std::length_error if the packet is not complete.
	 */
	static GenericPacket extractFromData(QByteArray &packet);

	Header &header() { return m_header; }
	const Header &header() const { return m_header; }

	QByteArray &payload() { return m_payload; }
	const QByteArray &payload() const { return m_payload; }

	GenericPacket &setPayload(const QByteArray &payload);
	GenericPacket &setPayload(QByteArray &&payload);

	QByteArray toData() const;
	std::size_t dataSize() const;

private:
	explicit GenericPacket(const QByteArray &packet);
	explicit GenericPacket(QByteArray &packet);
	void ensureSizeCanHoldPayload();

	Header m_header;
	QByteArray m_payload;
};


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
		static inline QByteArray toData(S size, T type)
		{
			RawHeader<S, T> raw{ hton(size), hton(type) };
			return { reinterpret_cast<const char *>(&raw), sizeof(raw) };
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
	return static_cast<std::size_t>(data.size()) >= dataSize();
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
	return GenericPacketHelper::RawHeader<S, T>::toData(m_size, m_type);
}

template<typename S, typename T>
std::size_t GenericPacket<S, T>::Header::dataSize()
{
	return sizeof(GenericPacketHelper::RawHeader<S, T>);
}

template<typename S, typename T>
std::size_t GenericPacket<S, T>::Header::maxSize()
{
	return std::numeric_limits<S>::max();
}

template<typename S, typename T>
GenericPacket<S, T>::Header::Header(const QByteArray &data)
	: m_size(GenericPacketHelper::RawHeader<S, T>::fromData(data).size),
	m_type(GenericPacketHelper::RawHeader<S, T>::fromData(data).type)
{
}

template<typename S, typename T>
GenericPacket<S, T>::Header::Header(QByteArray &data)
	: Header(static_cast<const QByteArray &>(data))
{
	/* Ideally the data should be moved and not copied, then removed, but sadly
	 * there is no API for this: */
	data.remove(0, dataSize());
}

template<typename S, typename T>
GenericPacket<S, T>::GenericPacket(Type type, const QByteArray &payload)
	: m_header(Size(static_cast<S>(payload.size())), type),
	m_payload(payload)
{
	ensureSizeCanHoldPayload();
}

template<typename S, typename T>
GenericPacket<S, T>::GenericPacket(Type type, QByteArray &&payload)
	: m_header(Size(static_cast<S>(payload.size())), type),
	m_payload(std::move(payload))
{
	ensureSizeCanHoldPayload();
}

template<typename S, typename T>
bool GenericPacket<S, T>::hasCompletePacket(const QByteArray &data)
{
	return
		Header::hasCompleteHeader(data) &&
		/* Ensure that the data contains the whole payload, determined by the
		 * size field in the header: */
		static_cast<std::size_t>(data.size()) - Header::dataSize() >= Header::fromData(data).size();
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
std::size_t GenericPacket<S, T>::dataSize() const
{
	return m_header.dataSize() + static_cast<std::size_t>(m_payload.size());
}

template<typename S, typename T>
GenericPacket<S, T>::GenericPacket(const QByteArray &data)
	: m_header(Header::fromData(data)),
	m_payload(data.mid(Header::dataSize(), m_header.size()))
{
}

template<typename S, typename T>
GenericPacket<S, T>::GenericPacket(QByteArray &data)
	: m_header(Header::extractFromData(data)),
	m_payload(data.left(m_header.size()))
{
	/* Ideally the data should be moved and not copied, then removed, but sadly
	 * there is no API for this: */
	data.remove(0, m_header.size());
}

template<typename S, typename T>
void GenericPacket<S, T>::ensureSizeCanHoldPayload()
{
	if (static_cast<std::size_t>(m_payload.size()) > Header::maxSize())
		throw std::range_error(QObject::tr("The datatype chosen to represent the "
					"packet length in the header (maximum value %2) cannot hold the payload "
					"size (%3)")
				.arg(Header::maxSize())
				.arg(m_payload.size())
				.toStdString());
}
