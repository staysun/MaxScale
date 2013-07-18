/*
 * This file is distributed as part of the SkySQL Gateway.  It is free
 * software: you can redistribute it and/or modify it under the terms of the
 * GNU General Public License as published by the Free Software Foundation,
 * version 2.
 *
 * This program is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU General Public License for more
 * details.
 *
 * You should have received a copy of the GNU General Public License along with
 * this program; if not, write to the Free Software Foundation, Inc., 51
 * Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 *
 * Copyright SkySQL Ab 2013
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#define _XOPEN_SOURCE
#include <unistd.h>
#include <crypt.h>
#include <users.h>
#include <adminusers.h>

/**
 * @file adminusers.c - Administration user account management
 *
 * @verbatim
 * Revision History
 *
 * Date		Who		Description
 * 18/07/13	Mark Riddoch	Initial implementation
 *
 * @endverbatim
 */

static USERS	*loadUsers();
static void	initialise();

static USERS 	*users = NULL;
static int	admin_init = 0;

/**
 * Admin Users initialisation
 */
static void
initialise()
{
	if (admin_init)
		return;
	admin_init = 1;
	users = loadUsers();
}

/**
 * Verify a username and password
 *
 * @param username	Username to verify
 * @param password	Password to verify
 * @return Non-zero if the username/password combination is valid
 */
int
admin_verify(char *username, char *password)
{
char			*pw;

	initialise();
	if (users == NULL)
	{
		if (strcmp(username, "admin") == 0 && strcmp(password, "skysql") == 0)
			return 1;
	}
	else
	{
		if ((pw = users_fetch(users, username)) == NULL)
			return 0;
		if (strcmp(pw, crypt(password, ADMIN_SALT)) == 0)
			return 1;
	}
	return 0;
}


/**
 * Load the admin users
 *
 * @return Table of users
 */
static USERS	*
loadUsers()
{
USERS	*rval;
FILE	*fp;
char	fname[1024], *home;
char	uname[80], passwd[80];

	initialise();
	if ((home = getenv("MAXSCALE_HOME")) != NULL)
		sprintf(fname, "%s/etc/passwd", home);
	else
		sprintf(fname, "/usr/local/skysql/MaxScale/etc/passwd");
	if ((fp = fopen(fname, "r")) == NULL)
		return NULL;
	if ((rval = users_alloc()) == NULL)
		return NULL;
	while (fscanf(fp, "%[^:]:%s\n", uname, passwd) == 2)
	{
		users_add(rval, uname, passwd);
	}
	fclose(fp);

	return rval;
}

/**
 * Add user
 *
 * @param username	Name of the new user
 * @param password	Password for the new user
 * @return	The number of users added
 */
int
admin_add_user(char *uname, char *passwd)
{
FILE	*fp;
char	fname[1024], *home, *cpasswd;

	initialise();
	if ((home = getenv("MAXSCALE_HOME")) != NULL)
		sprintf(fname, "%s/etc/passwd", home);
	else
		sprintf(fname, "/usr/local/skysql/MaxScale/etc/passwd");
	if (users == NULL)
	{
		if ((users = users_alloc()) == NULL)
			return 0;
		if ((fp = fopen(fname, "w")) == NULL)
			return 0;
		fclose(fp);
	}
	if (users_fetch(users, uname) != NULL)
	{
		return 0;
	}
	cpasswd = crypt(passwd, ADMIN_SALT);
	users_add(users, uname, cpasswd);
	if ((fp = fopen(fname, "a")) == NULL)
		return 0;
	fprintf(fp, "%s:%s\n", uname, cpasswd);
	fclose(fp);
	return 1;
}

/**
 * Check for existance of the user
 *
 * @param user	The user name to test
 * @return 	Non-zero if the user exists
 */
int
admin_test_user(char *user)
{
	initialise();
	if (users == NULL)
		return 0;
	return users_fetch(users, user) != NULL;
}
