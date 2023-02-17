want_man = get_option('man')
xsltproc = find_program('xsltproc', required : want_man == 'true')
want_man = want_man != 'false' and xsltproc.found()

want_html = get_option('html')
xmlto = find_program('xmlto', required : get_option('docbook_docs'))
want_html = want_html != 'false' and xmlto.found()


docdir = get_option('docdir')
if docdir == ''
  docdir = get_option('datadir') / 'doc' / 'hirte'
endif

man1 = [
  'hirte',
  'hirte-agent',
  'hirtectl',
]

man5 = [
    'hirte-config',
    'hirte-agent-config',
]

if want_man
  manpages_xsl = 'http://docbook.sourceforge.net/release/xsl/current/manpages/docbook.xsl'

  xsltproc_flags = [
    '--nonet',
    '--stringparam', 'man.output.quietly', '1',
    '--stringparam', 'funcsynopsis.style', 'ansi',
    '--stringparam', 'man.th.extra1.suppress', '1',
    '--stringparam', 'man.authors.section.enabled', '0',
    '--stringparam', 'man.copyright.section.enabled', '0',
  ]

  foreach pair : [[man1, '1'], [man5, '5']]
    pages = pair[0]
    section = pair[1]

    foreach man : pages
        custom_target(
          man + '.' + section,
          input : [man + '.xml'],
          output : [man + '.' + section],
          command : [
            xsltproc,
            '-o', '@OUTPUT@',
          ] + xsltproc_flags + [
            manpages_xsl,
            '@INPUT@',
          ],
          build_by_default : true,
          install : true,
          install_dir : get_option('mandir') / ('man' + section),
        )
    endforeach
  endforeach
endif

if want_html and xmlto.found()
  cdata = configuration_data()
  cdata.set('VERSION', meson.project_version())
  cdata.set('srcdir', meson.current_source_dir())
  hirte_docs_xml = configure_file(
    input : 'hirte-docs.xml.in',
    output : 'hirte-docs.xml',
    configuration : cdata,
  )
  custom_target(
    'hirte-docs.html',
    input : [
      hirte_docs_xml,
      'xmlto-config.xsl',
    ],
    output : ['hirte-docs.html'],
    command : [
      xmlto,
      '-o', meson.current_build_dir(),
    ] + get_option('xmlto_flags') + [
      '--skip-validation',
      'xhtml-nochunks',
      '-m', '@INPUT1@',
      '@INPUT0@',
    ],
    build_by_default : true,
    install : true,
    install_dir : docdir,
  )

  install_data('docbook.css', install_dir : docdir)
endif
