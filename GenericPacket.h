#pragma once
#include <cstdint>
#include <QByteArray>

class GenericPacket
{
public:
	template<typename T>
	struct NamedType
	{
		explicit NamedType(T value) : value(value) {}
		inline operator T() { return value; }
		T value;
	};
	using Size = NamedType<std::uint32_t>;
	using Type = NamedType<std::uint32_t>;

	class Header
	{
	public:
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
		std::uint32_t size() const { return m_size; }
		std::uint32_t type() const { return m_type; }

		Header &setSize(Size size);
		Header &setType(Type type);

		QByteArray toData() const;

	private:
		explicit Header(const QByteArray &data);
		explicit Header(QByteArray &data);

		std::uint32_t m_size = 0;
		std::uint32_t m_type = 0;
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
