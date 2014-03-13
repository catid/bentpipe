#include <iostream>
#include <vector>
using namespace std;

#include "Sockets.hpp"
#include "Enforcer.hpp"
#include "Clock.hpp"
#include "EndianNeutral.hpp"
#include "Shorthair.hpp"
using namespace cat;

#include <sys/socket.h>

static Clock m_clock;

class Connexion : public shorthair::IShorthair
{
public:
	NetAddr addr;

	u32 lastData;

	shorthair::Shorthair codec;

	// Called with the latest data packet from remote host
	virtual void OnPacket(u8 *packet, int bytes);

	// Called with the latest OOB packet from remote host
	virtual void OnOOB(u8 *packet, int bytes);

	// Send raw data to remote host over UDP socket
	virtual void SendData(u8 *buffer, int bytes);

	virtual ~Connexion() {
	}
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
	NetAddr::SockAddr addr_out;
	socklen_t addrlen_out;

	CAT_ENFORCE(conn->addr.Unwrap(addr_out, addrlen_out));

	int result = sendto(m_s, data, len, 0, (sockaddr*)&addr_out, addrlen_out);

	CAT_ENFORCE(result == len);
}

static void handle_special(Connexion *conn, u32 now, const char *data, int len) {
	// Time sync:
	if (len == 6 && data[1] == 0) {
		// Time sync packet
		char response[14];
		response[0] = 0;
		response[1] = 1;

		u32 client_t0 = getLE(*(u32*)(data + 2));
		u32 server_t1 = now;
		u32 server_t2 = m_clock.msec();

		*(u32*)(response + 2) = getLE(client_t0);
		*(u32*)(response + 6) = getLE(server_t1);
		*(u32*)(response + 10) = getLE(server_t2);

		cout << "Sending OOB response" << endl;
		conn->codec.SendOOB((const u8 *)response, 14);
	}
}

static void on_data(Connexion *conn, const char *data, int len)
{
	//char ipname[50];
	//cout << "DATA " << conn->addr.IPToString(ipname, sizeof(ipname)) << " : " << conn->addr.GetPort() << " length " << len << endl;

	conn->codec.Recv((void*)data, len);
}

// Called with the latest data packet from remote host
void Connexion::OnPacket(u8 *packet, int bytes)
{
	char ipname[50];
	cout << "REL " << conn->addr.IPToString(ipname, sizeof(ipname)) << " : " << conn->addr.GetPort() << " length " << bytes << endl;

	u32 now = m_clock.msec();

	this->lastData = now;

	for (int ii = 0; ii < m_conns.size(); ++ii) {
		Connexion *other = m_conns[ii];

		if (this != other) {
			if (now - other->lastData >= 10000) {
				char ipname[50];
				cout << "++ User left " << other->addr.IPToString(ipname, sizeof(ipname)) << " : " << other->addr.GetPort() << endl;
				delete other;
				m_conns.erase(m_conns.begin() + ii);
				--ii;
			} else {
				other->codec.Send(packet, bytes);
			}
		}
	}
}

// Called with the latest OOB packet from remote host
void Connexion::OnOOB(u8 *packet, int bytes)
{
	char ipname[50];
	cout << "OOB " << conn->addr.IPToString(ipname, sizeof(ipname)) << " : " << conn->addr.GetPort() << " length " << bytes << endl;

	u32 now = m_clock.msec();

	this->lastData = now;

	if (packet[0] == 0) {
		cout << "Sync received" << endl;
		handle_special(this, now, (char*)packet, bytes);
	} else if (packet[1] == 1) {
		for (int ii = 0; ii < m_conns.size(); ++ii) {
			Connexion *other = m_conns[ii];

			if (this != other) {
				if (now - other->lastData >= 10000) {
					char ipname[50];
					cout << "++ User left " << other->addr.IPToString(ipname, sizeof(ipname)) << " : " << other->addr.GetPort() << endl;
					delete other;
					m_conns.erase(m_conns.begin() + ii);
					--ii;
				} else {
					other->codec.SendOOB(packet, bytes);
				}
			}
		}
	}
}

// Send raw data to remote host over UDP socket
void Connexion::SendData(u8 *buffer, int bytes)
{
	send_data(this, (const char *)buffer, bytes);
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
	int last_tick = 0;

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

			shorthair::Settings settings;
			settings.target_loss = 0.01;
			settings.min_loss = 0.03;
			settings.max_loss = 0.3;
			settings.min_delay = 100;
			settings.max_delay = 2000;
			settings.max_data_size = 1350;
			settings.interface = conn;

			CAT_ENFORCE(conn->codec.Initialize(settings));

			m_conns.push_back(conn);

			char ipname[50];
			cout << "+ New user: " << src_addr.IPToString(ipname, sizeof(ipname)) << " : " << src_addr.GetPort() << endl;
		}

		on_data(conn, buffer, len);

		u32 now = m_clock.msec();

		if (now - last_tick > 15) {
			last_tick = now;

			for (int ii = 0; ii < m_conns.size(); ++ii) {
				Connexion *conn = m_conns[ii];

				conn->codec.Tick();
			}
		}
	}

	cout << "Good bye." << endl;

	m_clock.OnFinalize();

	return 0;
}

