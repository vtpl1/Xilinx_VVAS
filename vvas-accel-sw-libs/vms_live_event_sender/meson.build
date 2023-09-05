
# *****************************************************
#    Copyright 2023 Videonetics Technology Pvt Ltd
# *****************************************************

deeperlook_inc = [ 'inc', 'datamodels/inc' ]
inc_dirs = include_directories(deeperlook_inc)
vms_live_event_sender_lib = library('vms_live_event_sender', 
  'src/vms_live_event_sender.cpp',
  'datamodels/src/event_output_v200.cpp',
  'datamodels/src/job.cpp',
  dependencies : [opencv_dep, nng_dep],
  include_directories: [inc_dirs],
  install : true
)

vms_live_event_sender_lib_dep = declare_dependency(
  dependencies : [opencv_dep, nng_dep],
  include_directories : [inc_dirs], 
  link_with : vms_live_event_sender_lib
)