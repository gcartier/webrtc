project('webrtc', ['c', 'cpp'], version : '1.0', license : 'LGPL')

webrtc_sources = [
  'webrtc/webrtc.cpp'
]

webrtcstatic_dep = dependency('webrtcstatic')

webrtc = library('webrtc',
  webrtc_sources,
  dependencies : [webrtcstatic_dep],
)
