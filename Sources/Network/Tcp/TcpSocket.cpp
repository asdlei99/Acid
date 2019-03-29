#include "TcpSocket.hpp"

#if defined(ACID_BUILD_WINDOWS)

#include <WinSock2.h>

#else
#include <sys/select.h>
#include <netinet/in.h>
#endif

#include "Engine/Log.hpp"
#include "Network/IpAddress.hpp"
#include "Network/Packet.hpp"

#ifdef _MSC_VER
#pragma warning(disable: 4127) // "conditional expression is constant" generated by the FD_SET macro
#endif

namespace acid
{
// Define the low-level send/receive flags, which depends on the OS.
#if defined(ACID_PLATFORM_LINUX)
const int flags = MSG_NOSIGNAL;
#else
const int flags = 0;
#endif

TcpSocket::TcpSocket() :
	Socket(Type::Tcp)
{
}

uint16_t TcpSocket::GetLocalPort() const
{
	if (GetHandle() != InvalidSocketHandle())
	{
		// Retrieve informations about the local end of the socket.
		sockaddr_in address;
		SocketAddrLength size = sizeof(address);

		if (getsockname(GetHandle(), reinterpret_cast<sockaddr *>(&address), &size) != -1)
		{
			return ntohs(address.sin_port);
		}
	}

	// We failed to retrieve the port.
	return 0;
}

IpAddress TcpSocket::GetRemoteAddress() const
{
	if (GetHandle() != InvalidSocketHandle())
	{
		// Retrieve informations about the remote end of the socket.
		sockaddr_in address;
		SocketAddrLength size = sizeof(address);

		if (getpeername(GetHandle(), reinterpret_cast<sockaddr *>(&address), &size) != -1)
		{
			return IpAddress(ntohl(address.sin_addr.s_addr));
		}
	}

	// We failed to retrieve the address.
	return IpAddress::None;
}

uint16_t TcpSocket::GetRemotePort() const
{
	if (GetHandle() != InvalidSocketHandle())
	{
		// Retrieve informations about the remote end of the socket.
		sockaddr_in address;
		SocketAddrLength size = sizeof(address);

		if (getpeername(GetHandle(), reinterpret_cast<sockaddr *>(&address), &size) != -1)
		{
			return ntohs(address.sin_port);
		}
	}

	// We failed to retrieve the port.
	return 0;
}

Socket::Status TcpSocket::Connect(const IpAddress &remoteAddress, const uint16_t &remotePort, const Time &timeout)
{
	// Disconnect the socket if it is already connected.
	Disconnect();

	// Create the internal socket if it doesn't exist.
	Create();

	// Create the remote address
	sockaddr_in address = CreateAddress(remoteAddress.ToInteger(), remotePort);

	if (timeout <= Time::Zero)
	{
		// We're not using a timeout: just try to connect.

		// Connect the socket
		if (connect(GetHandle(), reinterpret_cast<sockaddr *>(&address), sizeof(address)) == -1)
		{
			return GetErrorStatus();
		}

		// Connection succeeded.
		return Status::Done;
	}
	// We're using a timeout: we'll need a few tricks to make it work.

	// Save the previous blocking state.
	bool blocking = IsBlocking();

	// Switch to non-blocking to enable our connection timeout.
	if (blocking)
	{
		SetBlocking(false);
	}

	// Try to connect to the remote address
	if (connect(GetHandle(), reinterpret_cast<sockaddr *>(&address), sizeof(address)) >= 0)
	{
		// We got instantly connected! (it may no happen a lot...).
		SetBlocking(blocking);
		return Status::Done;
	}

	// Get the error status.
	Status status = GetErrorStatus();

	// If we were in non-blocking mode, return immediately.
	if (!blocking)
	{
		return status;
	}

	// Otherwise, wait until something happens to our socket (success, timeout or error).
	if (status == Status::NotReady)
	{
		// Setup the selector.
		fd_set selector;
		FD_ZERO(&selector);
		FD_SET(GetHandle(), &selector);

		// Setup the timeout.
		timeval time = {};
		time.tv_sec = static_cast<long>(timeout.AsMicroseconds() / 1000000);
		time.tv_usec = static_cast<long>(timeout.AsMicroseconds() % 1000000);

		// Wait for something to write on our socket (which means that the connection request has returned).
		if (select(static_cast<int>(GetHandle() + 1), nullptr, &selector, nullptr, &time) > 0)
		{
			// At this point the connection may have been either accepted or refused.
			// To know whether it's a success or a failure, we must check the address of the connected peer.
			if (GetRemoteAddress() != IpAddress::None)
			{
				// Connection accepted.
				status = Status::Done;
			}
			else
			{
				// Connection refused.
				status = GetErrorStatus();
			}
		}
		else
		{
			// Failed to connect before timeout is over.
			status = GetErrorStatus();
		}
	}

	// Switch back to blocking mode.
	SetBlocking(true);
	return status;
}

void TcpSocket::Disconnect()
{
	// Close the socket.
	Close();

	// Reset the pending packet data.
	m_pendingPacket = PendingPacket();
}

Socket::Status TcpSocket::Send(const void *data, const std::size_t &size)
{
	if (!IsBlocking())
	{
		Log::Error("Warning: Partial sends might not be handled properly.\n");
	}

	std::size_t sent;
	return Send(data, size, sent);
}

Socket::Status TcpSocket::Send(const void *data, const std::size_t &size, std::size_t &sent)
{
	// Check the parameters.
	if (!data || (size == 0))
	{
		Log::Error("Cannot send data over the network (no data to send)\n");
		return Status::Error;
	}

	// Loop until every byte has been sent.
	int result = 0;

	for (sent = 0; sent < size; sent += result)
	{
		// Send a chunk of data
		result = send(GetHandle(), static_cast<const char *>(data) + sent, static_cast<int>(size - sent), flags);

		// Check for errors.
		if (result < 0)
		{
			Status status = GetErrorStatus();

			if ((status == Status::NotReady) && sent)
			{
				return Status::Partial;
			}

			return status;
		}
	}

	return Status::Done;
}

Socket::Status TcpSocket::Receive(void *data, const std::size_t &size, std::size_t &received)
{
	// First clear the variables to fill.
	received = 0;

	// Check the destination buffer.
	if (!data)
	{
		Log::Error("Cannot receive data from the network (the destination buffer is invalid)\n");
		return Status::Error;
	}

	// Receive a chunk of bytes.
	int sizeReceived = recv(GetHandle(), static_cast<char *>(data), static_cast<int>(size), flags);

	// Check the number of bytes received.
	if (sizeReceived > 0)
	{
		received = static_cast<std::size_t>(sizeReceived);
		return Status::Done;
	}

	if (sizeReceived == 0)
	{
		return Status::Disconnected;
	}

	return GetErrorStatus();
}

Socket::Status TcpSocket::Send(Packet &packet)
{
	// TCP is a stream protocol, it doesn't preserve messages boundaries.
	// This means that we have to send the packet size first, so that the
	// receiver knows the actual end of the packet in the data stream.

	// We allocate an extra memory block so that the size can be sent
	// together with the data in a single call. This may seem inefficient,
	// but it is actually required to avoid partial send, which could cause
	// data corruption on the receiving end.

	// Get the data to send from the packet.
	auto dataSize = packet.OnSend();

	// First convert the packet size to network byte order
	uint32_t packetSize = htonl(static_cast<uint32_t>(dataSize.second));

	// Allocate memory for the data block to send
	std::vector<char> blockToSend(sizeof(packetSize) + dataSize.second);

	// Copy the packet size and data into the block to send
	std::memcpy(&blockToSend[0], &packetSize, sizeof(packetSize));

	if (dataSize.second > 0)
	{
		std::memcpy(&blockToSend[0] + sizeof(packetSize), dataSize.first, dataSize.second);
	}

	// Send the data block.
	std::size_t sent;
	Status status = Send(&blockToSend[0] + packet.m_sendPos, blockToSend.size() - packet.m_sendPos, sent);

	// In the case of a partial send, record the location to resume from
	if (status == Status::Partial)
	{
		packet.m_sendPos += sent;
	}
	else if (status == Status::Done)
	{
		packet.m_sendPos = 0;
	}

	return status;
}

Socket::Status TcpSocket::Receive(Packet &packet)
{
	// First clear the variables to fill.
	packet.Clear();

	// We start by getting the size of the incoming packet.
	uint32_t packetSize = 0;
	std::size_t received = 0;

	if (m_pendingPacket.m_sizeReceived < sizeof(m_pendingPacket.m_size))
	{
		// Loop until we've received the entire size of the packet (even a 4 byte variable may be received in more than one call).
		while (m_pendingPacket.m_sizeReceived < sizeof(m_pendingPacket.m_size))
		{
			char *data = reinterpret_cast<char *>(&m_pendingPacket.m_size) + m_pendingPacket.m_sizeReceived;
			Status status = Receive(data, sizeof(m_pendingPacket.m_size) - m_pendingPacket.m_sizeReceived, received);
			m_pendingPacket.m_sizeReceived += received;

			if (status != Status::Done)
			{
				return status;
			}
		}

		// The packet size has been fully received.
		packetSize = ntohl(m_pendingPacket.m_size);
	}
	else
	{
		// The packet size has already been received in a previous call.
		packetSize = ntohl(m_pendingPacket.m_size);
	}

	// Loop until we receive all the packet data.
	char buffer[1024];

	while (m_pendingPacket.m_data.size() < packetSize)
	{
		// Receive a chunk of data.
		std::size_t sizeToGet = std::min(static_cast<std::size_t>(packetSize - m_pendingPacket.m_data.size()), sizeof(buffer));
		Status status = Receive(buffer, sizeToGet, received);

		if (status != Status::Done)
		{
			return status;
		}

		// Append it into the packet.
		if (received > 0)
		{
			m_pendingPacket.m_data.resize(m_pendingPacket.m_data.size() + received);
			char *begin = &m_pendingPacket.m_data[0] + m_pendingPacket.m_data.size() - received;
			std::memcpy(begin, buffer, received);
		}
	}

	// We have received all the packet data: we can copy it to the user packet.
	if (!m_pendingPacket.m_data.empty())
	{
		packet.OnReceive(&m_pendingPacket.m_data[0], m_pendingPacket.m_data.size());
	}

	// Clear the pending packet data.
	m_pendingPacket = PendingPacket();
	return Status::Done;
}
}
