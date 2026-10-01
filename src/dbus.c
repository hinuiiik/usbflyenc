//
// Created by vikram on 9/30/26.
// Goal: Monitor for removable drives being mounted and get the mount point
//

#include "include/dbus.h"

#include <stdio.h>
#include <glib-2.0/glib.h>
#include <glib-2.0/gio/gio.h>

#include "include/drive.h"

static void receive_signal(GDBusConnection *connection,
                           const gchar *sender_name,
                           const gchar *object_path,
                           const gchar *interface_name,
                           const gchar *signal_name,
                           GVariant *parameters,
                           gpointer user_data) {
    // Sender: :_.__
    // Object Path: /org/freedesktop/UDisks2/block_devices/*
    // Interface: org.freedesktop.DBus.Properties
    // Parameters: ('org.freedesktop.UDisks2.Filesystem', {'MountPoints': <[b'/mount/point']>}, @as [])
    // Received signal: PropertiesChanged

    GVariant *changed_properties;

    g_variant_get(parameters, "(&s@a{sv}@as)", NULL, &changed_properties, NULL);
    GVariant *value_variant;
    value_variant = g_variant_lookup_value(changed_properties, "MountPoints", NULL);

    if (value_variant == NULL ||
        !g_variant_is_of_type(value_variant, G_VARIANT_TYPE("aay"))) {
        g_clear_pointer(&value_variant, g_variant_unref);
        g_variant_unref(changed_properties);
        return;
    }

    GVariantIter *iter;
    g_variant_get(value_variant, "aay", &iter);
    GVariant *child = g_variant_iter_next_value(iter);

    // ignore unmounts
    if (child == NULL) {
        g_variant_iter_free(iter);
        g_variant_unref(value_variant);
        g_variant_unref(changed_properties);
        return;
    }

    gsize length;
    const gchar *mount_path = g_variant_get_fixed_array(child, &length, sizeof(gchar));
    if (mount_path != NULL && length > 0) {
        const char *secretpath = user_data;
        printf("Drive mounted at %s\n", mount_path);
        fflush(stdout);
        run(mount_path, secretpath);
    }

    g_variant_unref(child);
    g_variant_iter_free(iter);
    g_variant_unref(value_variant);
    g_variant_unref(changed_properties);
}

int monitor_dbus(const char *secretpath) {
    GMainLoop *loop;
    GDBusConnection *connection;
    GError *error = NULL;
    guint subscription_id;

    connection = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &error);
    if (!connection) {
        g_printerr("Error connecting to D-Bus: %s\n", error->message);
        g_clear_error(&error);
        return 1;
    }

    printf("Monitoring for DBUS events.\n");
    loop = g_main_loop_new(NULL, FALSE);


    subscription_id = g_dbus_connection_signal_subscribe(
        connection,
        NULL,
        "org.freedesktop.DBus.Properties",
        "PropertiesChanged",
        NULL,
        "org.freedesktop.UDisks2.Filesystem",
        G_DBUS_SIGNAL_FLAGS_NONE,
        receive_signal,
        g_strdup(secretpath),
        g_free
    );

    g_main_loop_run(loop);

    g_dbus_connection_signal_unsubscribe(connection, subscription_id);
    g_object_unref(connection);
    g_main_loop_unref(loop);

    return 0;
}