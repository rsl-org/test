
if(WIN32)
else()
target_sources(rsltest_main PRIVATE
  posix/library.cpp
  posix/taskset.cpp
  posix/event_loop.cpp
  posix/watch.cpp
)
endif()