/* Player-2 pad over TCP for the SDL frontend (Legacy of Goku II co-op fork): a tiny replacement for
 * tools/mgba/p2_input.lua so the Lua runtime is not needed. Listens on MGBA_P2_PORT (default 7777)
 * when MGBA_P2 is set, accepts one sender (tools/p2_keys.py), and every frame writes the latest
 * 2-byte pad word | 0x8000 to g_p2_remote (EWRAM 0x0203FF00), or 0 while no sender is connected. */
#include <mgba/core/core.h>

#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define P2_REMOTE_ADDR 0x0203FF00

static int p2Listen = -1;
static int p2Client = -1;
static uint16_t p2Word = 0;
static bool p2Enabled = false;

void mP2PadInit(void) {
	const char* enable = getenv("MGBA_P2");
	if (!enable || !*enable) {
		return;
	}
	int port = 7777;
	const char* portStr = getenv("MGBA_P2_PORT");
	if (portStr && atoi(portStr) > 0) {
		port = atoi(portStr);
	}
	p2Listen = socket(AF_INET, SOCK_STREAM, 0);
	if (p2Listen < 0) {
		return;
	}
	int one = 1;
	setsockopt(p2Listen, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
	struct sockaddr_in addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = htonl(INADDR_ANY);
	addr.sin_port = htons(port);
	if (bind(p2Listen, (struct sockaddr*) &addr, sizeof(addr)) < 0 || listen(p2Listen, 1) < 0) {
		close(p2Listen);
		p2Listen = -1;
		return;
	}
	fcntl(p2Listen, F_SETFL, O_NONBLOCK);
	p2Enabled = true;
	fprintf(stderr, "p2pad: listening on %d (player 2 pad -> 0x%08X)\n", port, P2_REMOTE_ADDR);
}

/* Call once per frame from the frontend loop. */
void mP2PadPoll(struct mCore* core) {
	if (!p2Enabled) {
		return;
	}
	if (p2Client < 0) {
		int c = accept(p2Listen, NULL, NULL);
		if (c >= 0) {
			fcntl(c, F_SETFL, O_NONBLOCK);
			int one = 1;
			setsockopt(c, IPPROTO_TCP, TCP_NODELAY, &one, sizeof(one));
			p2Client = c;
			p2Word = 0;
			fprintf(stderr, "p2pad: sender connected\n");
		}
	} else {
		uint8_t buf[256];
		for (;;) {
			ssize_t n = recv(p2Client, buf, sizeof(buf), 0);
			if (n > 1) {
				p2Word = buf[n - 2] | (buf[n - 1] << 8);   /* newest complete word wins */
				continue;
			}
			if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
				close(p2Client);
				p2Client = -1;
				p2Word = 0;
				fprintf(stderr, "p2pad: sender disconnected\n");
			}
			break;
		}
	}
	core->busWrite16(core, P2_REMOTE_ADDR, p2Client >= 0 ? ((p2Word & 0x3FF) | 0x8000) : 0);
}

void mP2PadDeinit(void) {
	if (p2Client >= 0) {
		close(p2Client);
	}
	if (p2Listen >= 0) {
		close(p2Listen);
	}
	p2Client = p2Listen = -1;
}
