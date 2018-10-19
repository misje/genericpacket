#pragma once
#include <cstdint>
#include <QByteArray>

template<typename S = std::uint32_t, typename T = std::uint32_t>
class GenericPacket
{
public:
	template<typename U>
	struct NamedType
	{
		explicit NamedType(T value) : value(value) {}
		inline operator T() { return value; }
		T value;
	};
	using Size = NamedType<S>;
	using Type = NamedType<T>;

	class Header
	{
	public:
		Header() = default;
		Header(Size size, Type type);

		/** \brief Check if data has a complete header */
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

private:
	explicit GenericPacket(const QByteArray &packet);
	explicit GenericPacket(QByteArray &packet);

	Header m_header;
	QByteArray m_payload;
};

#include "GenericPacketT.cpp"
