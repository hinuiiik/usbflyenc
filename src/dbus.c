//
// Created by vikram on 9/30/26.
//

#include "../include/dbus.h"

#include <stdio.h>
#include <glib-2.0/glib.h>
#include <glib-2.0/gio/gio.h>

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

    // Each dbus event seems to create a null value variant around 3 times, maybe check if the sub can be more specific later
    if (value_variant != NULL) {
        GVariantIter *iter;
        GVariant *child;

        g_variant_get(value_variant, "aay", &iter);
        gsize length;
        child = g_variant_iter_next_value(iter);
        const gchar *mount_path = g_variant_get_fixed_array(child, &length, sizeof(gchar));
        printf("Drive mounted at %s\n", mount_path);
        fflush(stdout);

        g_variant_unref(child);
        g_variant_iter_free(iter);
        g_variant_unref(value_variant);
    }
}

int monitor_dbus(void) {
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
        NULL,
        NULL
    );

    g_main_loop_run(loop);

    g_dbus_connection_signal_unsubscribe(connection, subscription_id);
    g_object_unref(connection);
    g_main_loop_unref(loop);

    return 0;
}