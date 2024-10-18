#
# Copyright Contributors to the Eclipse BlueChi project
#
# SPDX-License-Identifier: LGPL-2.1-or-later

want_man = get_option('with_man_pages')
md2man = find_program('go-md2man', required : false)
if want_man and not md2man.found()
  message('go-md2man not found, man page generation disabled automatically')
endif
want_man = want_man and md2man.found()

man1 = [
  'bluechi-controller',
  'bluechi-agent',
  'bluechictl',
  'bluechi-proxy',
  'bluechi-is-online',
]

man5 = [
    'bluechi-controller.conf',
    'bluechi-agent.conf',
]

man8 = [
    'bluechi-controller-selinux',
    'bluechi-agent-selinux'
]

if want_man
  foreach pair : [[man1, '1'], [man5, '5'], [man8, '8']]
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
          install_dir : get_option('mandir') / ('man' + section),
        )
    endforeach
  endforeach
endif
