project(tthread)

file(GLOB TTHREAD_HEADERS *.h)
file(GLOB TTHREAD_SOURCES *.cpp)

add_library(tthread STATIC ${TTHREAD_HEADERS} ${TTHREAD_SOURCES})
