{{include "license.py"}}

from gi.repository import Gtk
from .gi_composites import GtkTemplate

@GtkTemplate(ui='{{appid_path}}/{{ui_file}}')
class {{Prefix}}Window(Gtk.ApplicationWindow):
    __gtype_name__ = '{{Prefix}}Window'

    def __init__(self, **kwargs):
        super().__init__(**kwargs)
        self.init_template()

