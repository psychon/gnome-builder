const Gio = imports.gi.Gio;
const GLib = imports.gi.GLib;
const Gtk = imports.gi.Gtk;
const Lang = imports.lang;

var {{Prefix}}Window = new Lang.Class({
    Name: '{{Prefix}}Window',
    GTypeName: '{{Prefix}}Window',
    Extends: Gtk.ApplicationWindow,
    Template: 'resource://{{appid_path}}/{{prefix}}Window.ui',

    _init(application) {
        this.parent({
            application,
            default_width: 600,
            default_height: 300,
        });
    },
});

