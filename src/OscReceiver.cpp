#include "OscReceiver.h"

namespace
{
	constexpr DWORD         kReceiveTimeoutMs = 250;  // how often the thread checks for a stop request
	constexpr std::uint32_t kMaxBundleDepth = 8;
	constexpr auto          kJawOpenLogInterval = std::chrono::milliseconds{ 100 };
	constexpr std::size_t   kMaxLoggedAddresses = 512;
	constexpr std::size_t   kMaxLoggedValues = 8;

	// Bounds-checked reader for OSC's big-endian, 4-byte-aligned wire format (OSC 1.0 spec).
	class Reader
	{
	public:
		explicit Reader(std::span<const std::byte> a_data) noexcept :
			_data(a_data)
		{}

		[[nodiscard]] bool AtEnd() const noexcept { return _pos >= _data.size(); }
		[[nodiscard]] std::size_t Remaining() const noexcept { return _data.size() - _pos; }

		// Null-terminated string padded with nulls to a multiple of 4 bytes.
		bool ReadString(std::string_view& a_out) noexcept
		{
			const auto* begin = reinterpret_cast<const char*>(_data.data() + _pos);
			const auto  length = ::strnlen(begin, Remaining());
			if (length == Remaining()) {
				return false;  // unterminated
			}
			a_out = { begin, length };
			return Skip((length + 4) & ~std::size_t{ 3 });
		}

		bool ReadU32(std::uint32_t& a_out) noexcept
		{
			if (Remaining() < sizeof(a_out)) {
				return false;
			}
			std::memcpy(&a_out, _data.data() + _pos, sizeof(a_out));
			a_out = std::byteswap(a_out);  // network order -> x64 little-endian
			_pos += sizeof(a_out);
			return true;
		}

		bool ReadU64(std::uint64_t& a_out) noexcept
		{
			if (Remaining() < sizeof(a_out)) {
				return false;
			}
			std::memcpy(&a_out, _data.data() + _pos, sizeof(a_out));
			a_out = std::byteswap(a_out);
			_pos += sizeof(a_out);
			return true;
		}

		bool Skip(std::size_t a_count) noexcept
		{
			if (Remaining() < a_count) {
				return false;
			}
			_pos += a_count;
			return true;
		}

		bool Take(std::size_t a_count, std::span<const std::byte>& a_out) noexcept
		{
			if (Remaining() < a_count) {
				return false;
			}
			a_out = _data.subspan(_pos, a_count);
			_pos += a_count;
			return true;
		}

	private:
		std::span<const std::byte> _data;
		std::size_t                _pos{ 0 };
	};

	// Reads one argument of the given type tag. Returns false if the message is malformed or the
	// tag is unknown (an unknown tag means the remaining arguments can't be located).
	bool ReadArgument(Reader& a_reader, char a_tag, std::optional<float>& a_out)
	{
		std::uint32_t u32{};
		std::uint64_t u64{};
		switch (a_tag) {
		case 'f':
			return a_reader.ReadU32(u32) && (a_out = std::bit_cast<float>(u32), true);
		case 'i':
			return a_reader.ReadU32(u32) && (a_out = static_cast<float>(std::bit_cast<std::int32_t>(u32)), true);
		case 'd':
			return a_reader.ReadU64(u64) && (a_out = static_cast<float>(std::bit_cast<double>(u64)), true);
		case 'h':
			return a_reader.ReadU64(u64) && (a_out = static_cast<float>(std::bit_cast<std::int64_t>(u64)), true);
		case 'T':
			a_out = 1.0f;
			return true;
		case 'F':
			a_out = 0.0f;
			return true;
		case 'N':  // nil
		case 'I':  // impulse
			return true;
		case 's':
		case 'S':
			{
				std::string_view ignored;
				return a_reader.ReadString(ignored);
			}
		case 'b':
			return a_reader.ReadU32(u32) && a_reader.Skip((static_cast<std::size_t>(u32) + 3) & ~std::size_t{ 3 });
		case 't':  // timetag
			return a_reader.ReadU64(u64);
		case 'c':
		case 'r':
		case 'm':
			return a_reader.ReadU32(u32);
		default:
			return false;
		}
	}
}

bool OscReceiver::Start(const std::string& a_bindAddress, std::uint16_t a_port)
{
	if (IsRunning()) {
		logger::warn("OSC receiver is already running.");
		return true;
	}

	WSADATA wsaData{};
	if (const int error = ::WSAStartup(MAKEWORD(2, 2), &wsaData); error != 0) {
		logger::error("WSAStartup failed (error {}).", error);
		return false;
	}

	SOCKET     sock = INVALID_SOCKET;
	const auto fail = [&](std::string_view a_what) {
		logger::error("Failed to open OSC port {}:{}: {} (error {}).", a_bindAddress, a_port, a_what, ::WSAGetLastError());
		if (sock != INVALID_SOCKET) {
			::closesocket(sock);
		}
		::WSACleanup();
		return false;
	};

	sockaddr_in address{};
	address.sin_family = AF_INET;
	address.sin_port = ::htons(a_port);
	if (::inet_pton(AF_INET, a_bindAddress.c_str(), &address.sin_addr) != 1) {
		return fail("invalid bind address");
	}

	sock = ::socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	if (sock == INVALID_SOCKET) {
		return fail("socket() failed");
	}

	// A receive timeout lets the thread notice stop requests without closing the socket under it.
	const DWORD timeout = kReceiveTimeoutMs;
	if (::setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, reinterpret_cast<const char*>(&timeout), sizeof(timeout)) == SOCKET_ERROR) {
		return fail("setsockopt(SO_RCVTIMEO) failed");
	}

	if (::bind(sock, reinterpret_cast<const sockaddr*>(&address), sizeof(address)) == SOCKET_ERROR) {
		return fail("bind() failed, is another program already using this port?");
	}

	_socket = sock;
	_thread = std::jthread([this](std::stop_token a_stop) { Run(std::move(a_stop)); });

	logger::info("Listening for OSC data on {}:{} (UDP).", a_bindAddress, a_port);
	return true;
}

void OscReceiver::Stop()
{
	if (!IsRunning()) {
		return;
	}

	_thread.request_stop();
	_thread.join();
	_thread = {};

	::closesocket(static_cast<SOCKET>(_socket));
	_socket = INVALID_SOCKET;
	::WSACleanup();

	logger::info("OSC receiver stopped.");
}

std::optional<float> OscReceiver::GetValue(std::string_view a_address) const
{
	std::scoped_lock lock(_lock);
	if (const auto it = _values.find(a_address); it != _values.end()) {
		return it->second;
	}
	return std::nullopt;
}

OscReceiver::ValueMap OscReceiver::GetSnapshot() const
{
	std::scoped_lock lock(_lock);
	return _values;
}

void OscReceiver::Run(std::stop_token a_stop)
{
	const auto sock = static_cast<SOCKET>(_socket);
	std::vector<std::byte> buffer(64 * 1024);  // max UDP payload

	while (!a_stop.stop_requested()) {
		const int received = ::recv(sock, reinterpret_cast<char*>(buffer.data()), static_cast<int>(buffer.size()), 0);
		if (received == SOCKET_ERROR) {
			switch (const int error = ::WSAGetLastError()) {
			case WSAETIMEDOUT:   // no data this interval
			case WSAEMSGSIZE:    // oversized datagram, dropped
			case WSAECONNRESET:  // stale ICMP "port unreachable", harmless for UDP
				continue;
			default:
				logger::error("OSC receive failed (error {}), stopping listener.", error);
				return;
			}
		}

		if (_packetCount.fetch_add(1, std::memory_order_relaxed) == 0) {
			logger::info("Received first OSC packet ({} bytes).", received);
		}

		HandlePacket({ buffer.data(), static_cast<std::size_t>(received) });
	}
}

void OscReceiver::HandlePacket(std::span<const std::byte> a_packet, std::uint32_t a_depth)
{
	// Bundle: "#bundle\0", 8-byte timetag, then [int32 size][element] repeated.
	// Timetags are ignored and every element is applied immediately.
	constexpr std::string_view bundleTag{ "#bundle\0", 8 };
	if (a_packet.size() >= 16 && std::memcmp(a_packet.data(), bundleTag.data(), bundleTag.size()) == 0) {
		if (a_depth >= kMaxBundleDepth) {
			return;
		}

		Reader reader(a_packet.subspan(16));
		while (!reader.AtEnd()) {
			std::uint32_t              size{};
			std::span<const std::byte> element;
			if (!reader.ReadU32(size) || !reader.Take(size, element)) {
				return;
			}
			HandlePacket(element, a_depth + 1);
		}
		return;
	}

	if (!a_packet.empty() && a_packet[0] == std::byte{ '/' }) {
		HandleMessage(a_packet);
	}
}

void OscReceiver::HandleMessage(std::span<const std::byte> a_message)
{
	Reader           reader(a_message);
	std::string_view address;
	std::string_view typeTags;
	if (!reader.ReadString(address) || !reader.ReadString(typeTags) || typeTags.empty() || typeTags[0] != ',') {
		return;
	}

	_args.clear();
	bool complete = true;
	for (const char tag : typeTags.substr(1)) {
		std::optional<float> arg;
		if (!ReadArgument(reader, tag, arg)) {
			complete = false;
			break;
		}
		if (arg) {
			_args.push_back(*arg);
		}
	}

	if (!_loggedAddresses.contains(address)) {
		LogNewAddress(address, typeTags, complete);
	}

	if (_args.empty()) {
		return;
	}

	// Only the first value is kept.
	const float value = _args.front();
	{
		std::scoped_lock lock(_lock);
		if (const auto it = _values.find(address); it != _values.end()) {
			it->second = value;
		} else {
			_values.emplace(address, value);
		}
	}

	// VRCFaceTracking sends JawOpen, Steam Link sends JawDrop.
	if (address.ends_with("/JawOpen"sv) || address.ends_with("/JawDrop"sv)) {
		LogJawOpen(address, value);
	}
}

void OscReceiver::LogNewAddress(std::string_view a_address, std::string_view a_typeTags, bool a_complete)
{
	// Debug output: logs each address the first time it arrives, so the log shows what the
	// tracker actually sends. Capped in case a sender emits an unbounded set of addresses.
	if (_loggedAddresses.size() >= kMaxLoggedAddresses) {
		if (_loggedAddresses.size() == kMaxLoggedAddresses) {
			logger::info("Seen {} OSC addresses, not logging any more new ones.", kMaxLoggedAddresses);
			_loggedAddresses.emplace("");  // pushes the size past the cap so this only logs once
		}
		return;
	}
	_loggedAddresses.emplace(a_address);

	std::string values;
	for (std::size_t i = 0; i < _args.size() && i < kMaxLoggedValues; ++i) {
		values += std::format("{}{:.3f}", i > 0 ? ", " : "", _args[i]);
	}
	if (_args.size() > kMaxLoggedValues) {
		values += ", ...";
	}

	logger::info("New OSC address: {}  types '{}'  {} numeric value(s){}  [{}]",
		a_address,
		a_typeTags.substr(1),
		_args.size(),
		a_complete ? "" : " (stopped at unsupported type)",
		values);
}

void OscReceiver::LogJawOpen(std::string_view a_address, float a_value)
{
	// Debug output: trackers send at 60-120 Hz, so only log changes, at most every kJawOpenLogInterval.
	const auto now = std::chrono::steady_clock::now();
	if (a_value == _lastJawOpenValue || now - _lastJawOpenLog < kJawOpenLogInterval) {
		return;
	}

	_lastJawOpenLog = now;
	_lastJawOpenValue = a_value;
	logger::info("{} = {:.3f}", a_address, a_value);
}
