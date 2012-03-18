/*
 * Copyright (C) 2012 Pauli Nieminen <suokkos@gmail.com>
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; version 2 of the License (not later!)
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 */
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>

size_t getline(char **lineptr, size_t *n, FILE *stream)
{
	size_t lenp;
	char *internal = fgetln(stream, &lenp);
	char *new_line = *lineptr;
	if (!internal) {
		errno = EINVAL;
		return -1;
	}

	if (*n < lenp + 1) {
		new_line = realloc(lineptr, lenp + 1);
		if (!new_line) {
			errno = ENOMEM;
			return -1;
		}
		*n = lenp + 1;
		*lineptr = new_line;
	}
	memcpy(new_line, internal, lenp);
	new_line[lenp] = '\0';
	return 0;
}
