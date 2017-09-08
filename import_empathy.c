/*
 * Empathy Importer Plugin
 *
 * Copyright (C) 2017, Felix Zhang <fezhang@suse.com>,
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; either version 2 of the
 * License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02111-1301, USA.
 *
 */


#ifndef PURPLE_PLUGINS
# define PURPLE_PLUGINS
#endif

#include <glib.h>

#include <core.h>
#include <debug.h>
#include <notify.h>
#include <plugin.h>
#include <version.h>

#ifndef G_GNUC_NULL_TERMINATED
# if __GNUC__ >= 4
#  define G_GNUC_NULL_TERMINATED __attribute__((__sentinel__))
# else
#  define G_GNUC_NULL_TERMINATED
# endif
#endif

static GKeyFile *account_cfg = NULL;

static gboolean
load_empathy_account_cfg ()
{
    gchar *path;
    gboolean ret;

    path = g_build_filename (g_get_user_data_dir (), "telepathy",
                             "mission-control", "accounts.cfg", NULL);
    purple_debug_info ("import-empathy", "Loading Empathy accounts from %s\n", path);

    ret = g_key_file_load_from_file (account_cfg, path, G_KEY_FILE_NONE, NULL);
    g_free (path);

    if (!ret) {
        purple_debug_error ("import-empathy", "Error loading account.cfg\n");
    }
    return ret;
}

static void
import_empathy_accounts ()
{
    gchar **empathy_account;
    gchar **empathy_accounts;

    empathy_accounts = g_key_file_get_groups (account_cfg, NULL);

    for (empathy_account = empathy_accounts; *empathy_account; empathy_account++) {
        PurpleAccount *purple_account;
        gchar *str1, *str2;
        gchar *name, *protocol;
        /* account name */
        str1 = g_key_file_get_string (account_cfg, *empathy_account, "param-account", NULL);
        str2 = g_key_file_get_string (account_cfg, *empathy_account, "param-server", NULL);
        name = g_strconcat (str1, "@", str2, NULL);

        str1 = g_key_file_get_string (account_cfg, *empathy_account, "protocol", NULL);
        protocol = g_strconcat ("prpl-", str1, NULL);

        purple_account = purple_accounts_find (name, protocol);
        if (purple_account) {
            purple_debug_warning ("import-empathy", "Skipping existing %s account %s\n", protocol, name);
            continue ;
        }
        purple_account = purple_account_new (name, protocol);
        purple_accounts_add (purple_account);

		purple_account_set_username (purple_account, name);
		purple_account_set_protocol_id (purple_account, protocol);
        /* alias */
        str1 = g_key_file_get_string (account_cfg, *empathy_account, "Nickname", NULL);
		purple_account_set_alias (purple_account, str1);
        /* enabled */
        str1 = g_key_file_get_string (account_cfg, *empathy_account, "Enabled", NULL);
		purple_account_set_enabled (purple_account, purple_core_get_ui (), !g_strcmp0 (str1, "true"));
        // TODO get empathy password from keyring
        /* encoding */
        str1 = g_key_file_get_string (account_cfg, *empathy_account, "param-charset", NULL);
        purple_account_set_string(purple_account, "encoding", str1);
        /* port */
        str1 = g_key_file_get_string (account_cfg, *empathy_account, "param-port", NULL);
        purple_account_set_int(purple_account, "port", atoi(str1));
        /* ssl */
        str1 = g_key_file_get_string (account_cfg, *empathy_account, "param-use-ssl", NULL);
        purple_account_set_bool(purple_account, "ssl", !g_strcmp0 (str1, "true"));
    }
  g_strfreev (empathy_accounts);
}

static void
import_empathy_logs ()
{
    // TODO
}

static void
import_empathy ()
{
    account_cfg = g_key_file_new ();
    if (!load_empathy_account_cfg ())
        return;

    import_empathy_accounts ();
    import_empathy_logs ();
}

static gboolean
plugin_load ()
{
    gboolean do_import = TRUE;

    purple_debug_info ("import-empathy", "Loading plugin\n");
    // TODO: maybe only do import at first start

    if (do_import) {
        import_empathy ();
    }

    return TRUE;
}

static void
plugin_destroy ()
{
    g_key_file_free (account_cfg);
}

// TODO: add a user action to trigger manually

static PurplePluginInfo info = {
	PURPLE_PLUGIN_MAGIC,
	PURPLE_MAJOR_VERSION,
	PURPLE_MINOR_VERSION,
	PURPLE_PLUGIN_STANDARD,
	NULL,
	0,
	NULL,
	PURPLE_PRIORITY_DEFAULT,

	"core-fezhang-empathy_importer",
	"Empathy Importer",
	"0.1",
	"Import Empathy accounts and logs",
	"Import Empathy accounts and logs",
	"Felix Zhang <fezhang@suse.com>",
	"https://github.com/FelixZhang/purple-import-empathy",

	plugin_load,
	NULL,
	plugin_destroy,

	NULL,
	NULL,
	NULL,
	NULL, //plugin_actions,
	NULL,
	NULL,
	NULL,
	NULL
};

static void
init_plugin (PurplePlugin * plugin)
{
}

PURPLE_INIT_PLUGIN (hello_world, init_plugin, info)
