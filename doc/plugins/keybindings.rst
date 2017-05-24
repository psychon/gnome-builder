#######################
Registering Keybindings
#######################

Defining a Shortcut
===================

Shortcuts in Builder are defined similar to Gtk with the exception that you may define multiple keys separted by the | character.

To define a keyboard shortcut for the combination of Control and C you would use ``<Control>c``.
Modifiers such as ``Control``, ``Super``, ``Hyper``, ``Alt``, ``Primary``, and ``Shift`` are kept within XML style tags.

.. note:: Use ``<Primary>`` instead of ``<Control>`` if you want the command key to be used on macOS™.


Shortcut Examples
-----------------

The following are examples of properly defined shortcuts.

.. code-block:: xml

   <Primary>c
   <Control><Shift>p
   <Control><Alt>n
   <Super>k
   <Alt>1
   <Shift>comma
   <Shift>plus
   <Control>x|<Control>c

For more essoteric keys, the name should match the nickname from the ``GDK_Key_`` enumeration.
See ``gdkkeysyms.h`` for the various definitions.

You can also figure this out using the ``xev`` command and pressing the key you are interested in.
The ``xev`` program will print the name of the key.


Creating a Shortcut Theme
=========================

Builder has support for "shortcut themes" which are a collection of keyboard shortcuts that tend to emulate an existing editor.
Users can select a shortcut theme to make Builder feel more familiar.

In some cases, you may just want to map a series of keybindings to actions in Builder.
In other cases, you may want to emulate complex "modes" similar to editors like Vim.
Builder also support multi-press "chords" familiar to Emacs users.


.. code-block:: xml
   :caption: Save this to ``gedit.keytheme``

   <?xml version="1.0" encoding="UTF-8"?>
   <theme>
     <property name="name">gedit</property>
     <property name="title">Gedit</property>
     <property name="subtitle">Emulates the Gedit text editor</property>

     <!-- Keythemes support both &lt; &gt; < and > -->
     <shortcut accelerator="<control>q" action="app.quit"/>

     <!-- You can set accelerators for widgets by using their name for the context -->
     <context name="GtkEntry">
       <!-- signal="select-all" activates the GtkEntry::select-all signal -->
       <shortcut accelerator="<Control>a" signal="select-all"/>

       <!-- If you need to pass a parameter, use param elements -->
       <shortcut accelerator="Delete">
         <signal name="delete-from-cursor">
           <param>chars</param>
           <param>1</param>
         </signal>
       </shortcut>
     </context>

   </theme>
