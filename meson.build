project('webrtc', ['c', 'cpp'], version : '1.0', license : 'LGPL')

webrtc_sources = [
  'webrtc/webrtc.cpp'
]

webrtcstatic_dep = dependency('webrtcstatic')
glib_dep = dependency('glib-2.0')

webrtc = library('webrtc',
  webrtc_sources,
  dependencies : [webrtcstatic_dep, glib_dep],
)
