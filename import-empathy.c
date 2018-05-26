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

static gboolean
load_account_cfg (GKeyFile *account_cfg, gchar *path)
{
    gboolean res;

    if (path == NULL) {
        path = g_build_filename (g_get_user_data_dir (), "telepathy",
                                 "mission-control", "accounts.cfg", NULL);
    }

    purple_debug_info ("import-empathy", "Loading Empathy accounts from %s\n", path);

    res = g_key_file_load_from_file (account_cfg, path, G_KEY_FILE_NONE, NULL);
    if (!res) {
        purple_debug_error ("import-empathy", "Error loading account.cfg\n");
    }

    g_free (path);
    return res;
}

static void
import_accounts (GKeyFile *account_cfg)
{
    gchar **accounts = g_key_file_get_groups (account_cfg, NULL);
    gchar **account;

    for (account = accounts; *account; account++) {
        gchar *prot;

        prot = g_key_file_get_string (account_cfg, *account, "protocol", NULL);
        if (g_strcmp0 (prot, "irc") == 0)
            import_account_irc (account_cfg, account);

        {
        }
        // TODO support more protocols
        else {
            purple_debug_warning ("import-empathy", "Currently only the following protocols are supported: irc\n");
            purple_debug_warning ("import-empathy", "Skipping account %s with unsupported protocol %s\n", *account, str1);
            continue;
        }
    }
    g_strfreev (accounts);
}

static void
import_account_irc (GKeyFile *account_cfg, gchar *account)
{
    PurpleAccount *prpl_account = NULL;
    const gchar *protocol = "prpl-irc";
    gchar *name;
    gchar *s1, *s2;

    s1 = g_key_file_get_string (account_cfg, *account, "param-account", NULL);
    s2 = g_key_file_get_string (account_cfg, *account, "param-server", NULL);
    if (!s1 || !s2) {
        purple_debug_warning ("import-empathy", "Invalid param_account or param_server in account %s\n", *account);
        goto out;
    }
    name = g_strconcat (s1, "@", s2, NULL);

    prpl_account = purple_accounts_find (name, protocol);
    if (prpl_account) {
        purple_debug_warning ("import-empathy", "Skip existing account, protocol: %s, username: %s\n", protocol, name);
        goto out;
    }
    prpl_account = purple_account_new (name, protocol);
    purple_accounts_add (prpl_account);

    purple_account_set_username (prpl_account, name);
    purple_account_set_protocol_id (prpl_account, protocol);
    /* alias */
    s1 = g_key_file_get_string (account_cfg, *account, "Nickname", NULL);
    purple_account_set_alias (prpl_account, s1);
    /* enabled */
    s1 = g_key_file_get_string (account_cfg, *account, "Enabled", NULL);
    purple_account_set_enabled (prpl_account, purple_core_get_ui (), !g_strcmp0 (s1, "true"));
    /* encoding */
    s1 = g_key_file_get_string (account_cfg, *account, "param-charset", NULL);
    purple_account_set_string(prpl_account, "encoding", s1);
    /* port */
    s1 = g_key_file_get_string (account_cfg, *account, "param-port", NULL);
    purple_account_set_int(prpl_account, "port", atoi(s1));
    /* ssl */
    s1 = g_key_file_get_string (account_cfg, *account, "param-use-ssl", NULL);
    purple_account_set_bool(prpl_account, "ssl", !g_strcmp0 (s1, "true"));
    // TODO get empathy password from keyring

out:
    g_free (name);
}

// TODO import logs
static void
import_logs (GKeyFile *account_cfg)
{
    gchar **accounts = g_key_file_get_groups (account_cfg, NULL);
    gchar *basedir_name;

    accounts = g_key_file_get_groups (account_cfg, NULL);

    // TODO get accounts by folder
    basedir_name = g_build_filename (g_get_user_data_dir (), "TpLogger", "logs", NULL);

    // TODO iter accounts
        // TODO compare with g_strdelimit
        // TODO if match, iter log files
            // TODO make conversations, new log, write conv to logs

    g_strfreev (accounts);
    g_free (basedir_name);
}

static void
import_empathy (gchar *path)
{
    GKeyFile *account_cfg = g_key_file_new ();

    if (!load_account_cfg (account_cfg, path))
        return;

    import_accounts (account_cfg);
    import_logs (account_cfg);

    g_key_file_free (account_cfg);
}

static gboolean
plugin_load ()
{
    purple_debug_info ("import-empathy", "Loading plugin\n");

    // TODO prompt for import if empathy profiles found on the first run

    return TRUE;
}

static void
plugin_destroy ()
{
}

static void
plugin_action_import_cb (PurplePluginAction *action)
{
    import_empathy (NULL);
}

static GList *
plugin_actions (PurplePlugin *plugin, gpointer context)
{
	GList *actions = NULL;
    PurplePluginAction *action = NULL;

    action = purple_plugin_action_new("Import from Empathy", plugin_action_import_cb);
	actions = g_list_append(actions, action);

	return actions;
}

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
	plugin_actions, //plugin_actions,
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
