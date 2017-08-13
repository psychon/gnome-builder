project('{{name}}',{{if language == "c"}}'c',{{else if language == "vala"}}'c', 'vala',{{end}}
  version: '{{project_version}}',
  meson_version: '>= 0.40.0',
)

i18n = import('i18n')

{{if language == "c"}}config_h = configuration_data()
config_h.set_quoted('GETTEXT_PACKAGE', '{{name}}')
config_h.set_quoted('LOCALEDIR', join_paths(get_option('prefix'), get_option('localedir')))
configure_file(
  output: '{{prefix}}-config.h',
  configuration: config_h,
)
add_project_arguments([
  '-I' + meson.build_root(),
], language: '{{language}}')

{{end}}
subdir('data')
subdir('src')
subdir('po')

meson.add_install_script('meson_post_install.py')
