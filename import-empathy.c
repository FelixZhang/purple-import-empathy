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

static gchar *
map_protocol_name (gchar * protocol)
{
    gchar *res;

    if (purple_strequal (protocol, "irc")) {
        res = "prpl-irc";
    } else if (purple_strequal (protocol, "groupwise")) {
        res = "prpl-novell";
    } else {
        purple_debug_error ("import-empathy", "Protocol %s not supported\n", protocol);
    }

    return res;
}

static PurpleAccount *
import_account (GKeyFile *account_cfg, gchar **account)
{
    PurpleAccount *purple_account = NULL;
    gchar *protocol;
    gchar *param_server;
    gchar *param_port;
    gchar *param_charset;
    gchar *param_use_ssl;
    gchar *param_account;
    gchar *enabled;
    gchar *nickname;
    gchar *param_password;

    gchar *pp_protocol;
    gchar *pp_name;

    purple_debug_info ("import-empathy", "Importing account %s", *account);

    protocol = g_key_file_get_string (account_cfg, *account, "protocol", NULL);
    param_server = g_key_file_get_string (account_cfg, *account, "param-server", NULL);
    param_account = g_key_file_get_string (account_cfg, *account, "param-account", NULL);
    nickname = g_key_file_get_string (account_cfg, *account, "Nickname", NULL);
    enabled = g_key_file_get_string (account_cfg, *account, "Enabled", NULL);
    param_charset = g_key_file_get_string (account_cfg, *account, "param-charset", NULL);
    param_port = g_key_file_get_string (account_cfg, *account, "param-port", NULL);
    param_use_ssl = g_key_file_get_string (account_cfg, *account, "param-use-ssl", NULL);
    param_password = g_key_file_get_string (account_cfg, *account, "param-password", NULL);
    // TODO try fetching password from keyring

    g_return_val_if_fail(protocol, NULL);
    g_return_val_if_fail(param_server, NULL);
    g_return_val_if_fail(param_account, NULL);

    pp_name = g_strconcat (param_account, "@", param_server, NULL);
    pp_protocol = map_protocol_name (protocol);

    g_return_val_if_fail(pp_protocol, NULL);

    purple_account = purple_accounts_find (pp_name, pp_protocol);
    if (purple_account) {
        purple_debug_warning ("import-empathy", "Account %s already exists. Protocol: %s, Username: %s\n",
                              *account,
                              pp_protocol,
                              pp_name);
    } else {
        purple_account = purple_account_new (pp_name, pp_protocol);
        purple_accounts_add (purple_account);

        if (purple_strequal (protocol, "irc")) {
            purple_account_set_username (purple_account, pp_name);
        } else {
            purple_account_set_username (purple_account, param_account);
        }
        purple_account_set_protocol_id (purple_account, pp_protocol);
        purple_account_set_alias (purple_account, nickname);
        purple_account_set_enabled (purple_account, purple_core_get_ui (), purple_strequal (enabled, "true"));
        purple_account_set_string(purple_account, "encoding", param_charset);
        purple_account_set_int(purple_account, "port", atoi(param_port));
        purple_account_set_bool(purple_account, "ssl", purple_strequal (param_use_ssl, "true"));
        purple_account_set_password(purple_account, param_password);
    }

    g_free (pp_name);
    return purple_account;
}

static void
import_logs (GKeyFile *account_cfg, gchar **account)
{
    purple_debug_info ("import-empathy", "Importing irc logs for account %s\n", *account);

    gchar *dir_name;
    GDir *dir;

    dir_name = g_build_filename (g_get_user_data_dir (),
                                 "TpLogger",
                                 "logs",
                                 g_strdelimit (*account, "/", '_'),
                                 NULL);
    dir = g_dir_open (dir_name, 0, NULL);
    if (dir) {
        const gchar *file;

        while ((file = g_dir_read_name (dir))) {
            // TODO
        }
        g_dir_close (dir);
    }

    g_free (dir_name);
}

static void
import_empathy (gchar *path)
{
    GKeyFile *account_cfg = g_key_file_new ();
    gchar **accounts;
    gchar **account;

    g_return_if_fail (load_account_cfg (account_cfg, path));

    accounts = g_key_file_get_groups (account_cfg, NULL);
    g_return_if_fail (accounts);

    for (account = accounts; *account; account++) {
        PurpleAccount *purple_account = import_account (account_cfg, account);

        if (purple_account) {
            import_logs (account_cfg, account);
        }
        else {
            continue;
        }
    }

    g_key_file_free (account_cfg);
    g_strfreev (accounts);
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
