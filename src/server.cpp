#include <iostream>
#include <vector>
using namespace std;

#include "Sockets.hpp"
#include "Enforcer.hpp"
#include "Clock.hpp"
using namespace cat;

#include <sys/socket.h>

static Clock m_clock;

struct Connexion
{
	NetAddr addr;

	u32 lastData;

	// TODO: Shorthair
};

static vector<Connexion*> m_conns;
static SocketHandle m_s;

static Connexion *findConn(const NetAddr addr)
{
	// TODO: Hash table
	for (int ii = 0; ii < m_conns.size(); ++ii) {
		Connexion *conn = m_conns[ii];

		if (conn->addr == addr) {
			return conn;
		}
	}

	return 0;
}

static void send_data(Connexion *conn, const char *data, int len)
{
	// TODO: Repack data here

	NetAddr::SockAddr addr_out;
	socklen_t addrlen_out;

	CAT_ENFORCE(conn->addr.Unwrap(addr_out, addrlen_out));

	int result = sendto(m_s, data, len, 0, (sockaddr*)&addr_out, addrlen_out);

	CAT_ENFORCE(result == len);
}

static void on_data(Connexion *conn, const char *data, int len)
{
	// TODO: Unpack data here

	//char ipname[50];
	//cout << "DATA " << conn->addr.IPToString(ipname, sizeof(ipname)) << " : " << conn->addr.GetPort() << " length " << len << endl;

	u32 now = m_clock.msec();

	conn->lastData = now;

	for (int ii = 0; ii < m_conns.size(); ++ii) {
		Connexion *other = m_conns[ii];

		if (conn != other) {
			if (now - other->lastData >= 10000) {
				char ipname[50];
				cout << "++ User left " << other->addr.IPToString(ipname, sizeof(ipname)) << " : " << other->addr.GetPort();
				delete other;
				m_conns.erase(m_conns.begin() + ii);
				--ii;
			} else {
				send_data(other, data, len);
			}
		}
	}
}

int main()
{
	m_clock.OnInitialize();

	UDPSocket s;

	CAT_ENFORCE(s.Create(false));
	CAT_ENFORCE(s.Bind(5000));
	CAT_ENFORCE(s.Valid());
	CAT_ENFORCE(s.DontFragment());
	CAT_ENFORCE(s.IgnoreUnreachable());

	m_s = s.GetSocket();

	char buffer[2048];

	for (;;) {
		sockaddr_in6 addr;
		socklen_t addrlen = sizeof(addr);

		int len = recvfrom(m_s, buffer, sizeof(buffer), 0, (sockaddr*)&addr, &addrlen);

		if (len < 0) {
			cout << "++ Socket error " << Sockets::GetLastErrorString() << ".  Giving up!" << endl;
			break;
		}

		NetAddr src_addr(addr);
		CAT_ENFORCE(src_addr.Valid());

		Connexion *conn = findConn(src_addr);

		if (!conn) {
			conn = new Connexion;
			conn->addr = addr;

			m_conns.push_back(conn);

			char ipname[50];
			cout << "+ New user: " << src_addr.IPToString(ipname, sizeof(ipname)) << " : " << src_addr.GetPort() << endl;
		}

		on_data(conn, buffer, len);
	}

	cout << "Good bye." << endl;

	m_clock.OnFinalize();

	return 0;
}

