#pragma once

// Listens for OSC (Open Sound Control) messages over UDP on a background thread and keeps the
// most recent value received for each address, e.g. "/avatar/parameters/v2/JawOpen" -> 0.42.
//
// Only the first argument of each message is stored (ints, bools and doubles are converted to
// float). That covers face trackers such as VRCFaceTracking, which send one value per address.
class OscReceiver
{
public:
	using ValueMap = std::map<std::string, float, std::less<>>;

	static OscReceiver* GetSingleton()
	{
		static OscReceiver singleton;
		return &singleton;
	}

	bool Start(const std::string& a_bindAddress, std::uint16_t a_port);
	void Stop();

	[[nodiscard]] bool IsRunning() const noexcept { return _thread.joinable(); }

	// Thread-safe. Returns the latest value received on a_address, if any.
	[[nodiscard]] std::optional<float> GetValue(std::string_view a_address) const;

	// Thread-safe copy of every address/value pair received so far.
	[[nodiscard]] ValueMap GetSnapshot() const;

	[[nodiscard]] std::uint64_t GetPacketCount() const noexcept { return _packetCount.load(std::memory_order_relaxed); }

private:
	OscReceiver() = default;
	OscReceiver(const OscReceiver&) = delete;
	OscReceiver(OscReceiver&&) = delete;
	~OscReceiver() = default;

	OscReceiver& operator=(const OscReceiver&) = delete;
	OscReceiver& operator=(OscReceiver&&) = delete;

	void Run(std::stop_token a_stop);
	void HandlePacket(std::span<const std::byte> a_packet, std::uint32_t a_depth = 0);
	void HandleMessage(std::span<const std::byte> a_message);
	void LogJawOpen(std::string_view a_address, float a_value);
	void LogNewAddress(std::string_view a_address, std::string_view a_typeTags, bool a_complete);

	// members
	std::uintptr_t             _socket{ ~std::uintptr_t{ 0 } };  // SOCKET, kept opaque so Winsock stays out of this header
	std::jthread               _thread;
	mutable std::mutex         _lock;
	ValueMap                   _values;
	std::atomic<std::uint64_t> _packetCount{ 0 };

	// Only touched by the receiver thread.
	std::vector<float>                    _args;  // numeric arguments of the message being handled
	std::set<std::string, std::less<>>    _loggedAddresses;
	std::chrono::steady_clock::time_point _lastJawOpenLog{};
	float                                 _lastJawOpenValue{ -1.0f };
};
