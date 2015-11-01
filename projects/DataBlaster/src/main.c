/* DataBlaster: UDP constant-bandwidth file transfer utility
 * Copyright (C) 2013 Calvin Owens
 *
 * This program is free software: you can redistribute it and/or modify it
 * under the terms of version 2 of the GNU General Public License as published
 * by the Free Software Foundation.
 *
 * THIS SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE COPYRIGHT HOLDER(S) OR AUTHOR(S) BE LIABLE FOR ANY CLAIM, DAMAGES OR
 * OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
 * ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
 * OTHER DEALINGS IN THIS SOFTWARE. */

#include <stdlib.h>
#include <string.h>
#include <stddef.h>

int recv_main(int, char**);
int send_main(int, char**);

int main(int nargs, char* args[])
{
	if(strcmp(args[1],"-R") == 0)
		recv_main(nargs, args);
	if(strcmp(args[1],"-S") == 0)
		send_main(nargs, args);
	return EXIT_SUCCESS;
}
