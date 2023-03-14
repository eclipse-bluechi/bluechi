# Create RPM spec
githashcmd = run_command(['git', 'rev-parse', '--verify', '--short', 'HEAD'], capture : true, check : true)
githash = githashcmd.stdout()
datecmd = run_command(['date', '+%04Y%02m%02d%02H%02M'], capture : true, check : true)
date = datecmd.stdout()
release = '0.' + date.strip() + '.git' + githash

rpm_spec_conf = configuration_data()
rpm_spec_conf.set('VERSION', meson.project_version())
rpm_spec_conf.set('RELEASE', release.strip())

configure_file(
    output: 'hirte.spec',
    input: 'hirte.spec.in',
    configuration: rpm_spec_conf
)