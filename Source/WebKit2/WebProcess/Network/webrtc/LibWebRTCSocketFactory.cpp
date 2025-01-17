/*
 * Copyright (C) 2017 Apple Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. AND ITS CONTRIBUTORS ``AS IS''
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL APPLE INC. OR ITS CONTRIBUTORS
 * BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
 * CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
 * SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
 * INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
 * CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
 * ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
 * THE POSSIBILITY OF SUCH DAMAGE.
 */

#include "config.h"
#include "LibWebRTCSocketFactory.h"

#if USE(LIBWEBRTC)

#include "NetworkProcessConnection.h"
#include "NetworkRTCMonitorMessages.h"
#include "NetworkRTCProviderMessages.h"
#include "WebProcess.h"
#include "WebRTCSocket.h"
#include <wtf/MainThread.h>

namespace WebKit {

uint64_t LibWebRTCSocketFactory::s_uniqueSocketIdentifier = 0;
uint64_t LibWebRTCSocketFactory::s_uniqueResolverIdentifier = 0;

rtc::AsyncPacketSocket* LibWebRTCSocketFactory::CreateServerTcpSocket(const rtc::SocketAddress& socketAddress, uint16_t minPort, uint16_t maxPort, int options)
{
    auto socket = std::make_unique<LibWebRTCSocket>(*this, ++s_uniqueSocketIdentifier, LibWebRTCSocket::Type::ServerTCP, socketAddress, rtc::SocketAddress());
    m_sockets.set(socket->identifier(), socket.get());

    auto identifier = socket->identifier();
    auto address = socket->localAddress().ToString();
    callOnMainThread([identifier, address, minPort, maxPort, options]() {
        String addressString = String(address.data(), address.length());
        if (!WebProcess::singleton().networkConnection().connection().send(Messages::NetworkRTCProvider::CreateServerTCPSocket(identifier, addressString, minPort, maxPort, options), 0)) {
            // FIXME: Set error back to socket
            return;
        }

    });

    return socket.release();
}

rtc::AsyncPacketSocket* LibWebRTCSocketFactory::CreateUdpSocket(const rtc::SocketAddress& socketAddress, uint16_t minPort, uint16_t maxPort)
{
    auto socket = std::make_unique<LibWebRTCSocket>(*this, ++s_uniqueSocketIdentifier, LibWebRTCSocket::Type::UDP, socketAddress, rtc::SocketAddress());
    m_sockets.set(socket->identifier(), socket.get());

    auto identifier = socket->identifier();
    auto address = socket->localAddress().ToString();
    callOnMainThread([identifier, address, minPort, maxPort]() {
        String addressString = String(address.data(), address.length());
        if (!WebProcess::singleton().networkConnection().connection().send(Messages::NetworkRTCProvider::CreateUDPSocket(identifier, addressString, minPort, maxPort), 0)) {
            // FIXME: Set error back to socket
            return;
        }
    });
    return socket.release();
}

rtc::AsyncPacketSocket* LibWebRTCSocketFactory::CreateClientTcpSocket(const rtc::SocketAddress& localSocketAddress, const rtc::SocketAddress& remoteSocketAddress, const rtc::ProxyInfo&, const std::string&, int options)
{
    auto socket = std::make_unique<LibWebRTCSocket>(*this, ++s_uniqueSocketIdentifier, LibWebRTCSocket::Type::ClientTCP, localSocketAddress, remoteSocketAddress);
    socket->setState(LibWebRTCSocket::STATE_CONNECTING);
    m_sockets.set(socket->identifier(), socket.get());

    auto identifier = socket->identifier();
    auto localAddress = socket->localAddress().ToString();
    auto remoteAddress = socket->remoteAddress().ToString();
    callOnMainThread([identifier, localAddress, remoteAddress, options]() {
        String localAddressString = String(localAddress.data(), localAddress.length());
        String remoteAddressString = String(remoteAddress.data(), remoteAddress.length());
        if (!WebProcess::singleton().networkConnection().connection().send(Messages::NetworkRTCProvider::CreateClientTCPSocket(identifier, localAddressString, remoteAddressString, options), 0)) {
            // FIXME: Set error back to socket
            return;
        }
    });

    return socket.release();
}

void LibWebRTCSocketFactory::detach(LibWebRTCSocket& socket)
{
    ASSERT(m_sockets.contains(socket.identifier()));
    m_sockets.remove(socket.identifier());
}

rtc::AsyncResolverInterface* LibWebRTCSocketFactory::CreateAsyncResolver()
{
    auto resolver = std::make_unique<LibWebRTCResolver>(++s_uniqueResolverIdentifier);
    auto* resolverPointer = resolver.get();
    m_resolvers.set(resolverPointer->identifier(), WTFMove(resolver));
    return resolverPointer;
}

} // namespace WebKit

#endif // USE(LIBWEBRTC)
