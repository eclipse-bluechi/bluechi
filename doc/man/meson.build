want_man = get_option('man') == 'true'
md2man = find_program('go-md2man', required : false)
if want_man and not md2man.found()
  message('go-md2man not found, man page generation disabled automatically')
endif
want_man = want_man and md2man.found()

man1 = [
  'hirte',
  'hirte-agent',
  'hirtectl',
]

man5 = [
    'hirte.conf',
    'hirte-agent.conf',
]

if want_man
  foreach pair : [[man1, '1'], [man5, '5']]
    pages = pair[0]
    section = pair[1]

    foreach man : pages
        custom_target(
          man + '.' + section,
          input : [man + '.' + section + '.md'],
          output : [man + '.' + section],
          command : [
            md2man,
            '-in', '@INPUT@',
            '-out', '@OUTPUT@',
          ],
          build_by_default : true,
          install : true,
          install_dir : get_option('datadir') / 'man' / ('man' + section),
        )
    endforeach
  endforeach
endif
