/* miniproxy.c: Crappy little proxy to replace '\r\n' with '\n' in HTTP requests.
 *
 * This allows you to (very basically) use the internet through "GoGo" wifi on
 * domestic flights without paying for it. It's pretty useless for actually
 * doing anything (HTTPS doesn't work, among other things), but it's kind of
 * fun.
 *
 * I was playing with netcat, and noticed that I was able to make proxy-style
 * HTTP requests to "google.com" through netcat and get responses. Telling
 * Chromium to use it directly as a proxy didn't work though.
 *
 * It would appear that somebody was not very through with filtering outgoing
 * requests and returning HTTP 302 to redirect to the payment page: it doesn't
 * catch HTTP requests that use '\n' instead of '\r\n'. This is nice for us,
 * since most webservers will accept such requests, even though it doesn't
 * comply with the standard. Gogo's server, apparently running Squid, happily
 * handles such a request. It even caches the pages you look at.
 *
 * I wrote this *very* quickly on a plane, so please don't judge me. It doesn't
 * really work that well: it needs to only do the replace on the HTTP request
 * headers instead of the entire packet, and it needs to be smarter about
 * killing children when the connection dies or looks like it's dead but isn't.
 * Obviously, it doesn't support HTTPS, and has issues with complex pages like
 * Reddit, so IRL it's not that useful.
 *
 * To use this, compile and run it, then point the browser of your choice to an
 * HTTP proxy at 127.0.0.1:8080. You have to try to directly access the internet
 * and see the payment page for it to work, and whatever that does seems to time
 * out every hour or so, so you have to do it again.
 *
 * Copyright (C) 2013 Calvin Owens
 *
 * This program is free software: you can redistribute it and/or modify it under
 * the terms of version 2 of the GNU General Public License as published by the
 * Free Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL THE
 * COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THIS SOFTWARE. */

#include <stdlib.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <arpa/inet.h>

/* Error checking? What's that? */
int get_proxy_socket(void)
{
	int fd = socket(AF_INET, SOCK_STREAM, 0);
	
	struct sockaddr_in bind_address;
	bind_address.sin_family = AF_INET;
	bind_address.sin_port = htons(8080);
	bind_address.sin_addr.s_addr = INADDR_ANY;

	bind(fd, &bind_address, sizeof(bind_address));
	listen(fd, 1024);

	return fd;
}

int fix_crlf_endings(char *buf, int len)
{
	int ret = len;

	for (int i = 0; i < len; i++) {
		if (buf[i] == '\r' && buf[i+1] == '\n') {
			buf[i] = '\n';
			
			/* SUPER PEDANTIC! */
			for (int j = i + 1; j < len; j++)
				buf[j] = buf[j+1];
			ret--;
		}
	}

	return ret;
}

int main(int args, char **argv)
{
	int in_len, out_len, real_out, out_fd, in_fd, listen_fd = get_proxy_socket();
	struct sockaddr_in to_address;
	char buf[32768];

	to_address.sin_family = AF_INET;
	to_address.sin_port = htons(80);
	to_address.sin_addr.s_addr = inet_addr("74.125.225.114"); /* This worked for me - YMMV */

	while (1) {
		in_fd = accept4(listen_fd, NULL, 0, 0);
		if (fork())
			continue;
		out_fd = socket(AF_INET, SOCK_STREAM, 0);
		connect(out_fd, &to_address, sizeof(to_address));
		
		if (fork()) {
			while (1) {
				in_len = read(in_fd, &buf, 1460);
				if (in_len <= 0) return 0;
				out_len = fix_crlf_endings(&buf, in_len);
				if (write(out_fd, &buf, out_len) <= 0) return 0;
			}
		} else {
			while (1) {
				if (read(out_fd, &buf, 1460) <= 0) return 0;
				if (write(in_fd, &buf, 1460) <= 0) return 0;
			}
		}
	}

	return 0;
}
