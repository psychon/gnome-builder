if get_option('with_phpize')

install_data('phpize_plugin.py', install_dir: plugindir)

configure_file(
          input: 'phpize.plugin',
         output: 'phpize.plugin',
  configuration: configuration_data(),
        install: true,
    install_dir: plugindir,
)

endif
