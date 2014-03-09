#include <iostream>
using namespace std;

#include "Sockets.hpp"
#include "Enforcer.hpp"
using namespace cat;

#include <sys/socket.h>

int main() {
	UDPSocket s;

	CAT_ENFORCE(s.Create());
	CAT_ENFORCE(s.Bind(5000));
	CAT_ENFORCE(s.Valid());
	CAT_ENFORCE(s.DontFragment());
	CAT_ENFORCE(s.IgnoreUnreachable());

	SocketHandle sh = s.GetSocket();

	char buffer[1500];

	for (;;) {
		sockaddr_in6 addr;
		socklen_t addrlen = sizeof(addr);

		int result = recvfrom(sh, buffer, sizeof(buffer), 0, (sockaddr*)&addr, &addrlen);

		if (result == -1) {
			cout << "++ Socket error.  Giving up!" << endl;
			break;
		}

		NetAddr src_addr(addr);
		CAT_ENFORCE(src_addr.Valid());

		char ipname[50];
		const char *hostname = src_addr.IPToString(ipname, sizeof(ipname));

		cout << "UDP datagram from " << hostname << " : " << src_addr.GetPort() << " of length " << result << endl;
	}
}

