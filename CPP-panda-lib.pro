TEMPLATE = app
CONFIG += console c++11
CONFIG -= app_bundle
CONFIG -= qt

INCLUDEPATH += ../src

SOURCES += \
    src/panda/lib/from_chars.cc \
    src/panda/lib/hash.cc \
    src/panda/lib/memory.cc \
    src/panda/lib.cc

HEADERS += \
    src/panda/lib/endian.h \
    src/panda/lib/from_chars.h \
    src/panda/lib/hash.h \
    src/panda/lib/memory.h \
    src/panda/basic_string.h \
    src/panda/basic_string_view.h \
    src/panda/cast.h \
    src/panda/iterator.h \
    src/panda/lib.h \
    src/panda/refcnt.h \
    src/panda/string.h \
    src/panda/string_map.h \
    src/panda/string_set.h \
    src/panda/string_view.h \
    src/panda/unordered_string_map.h \
    src/panda/unordered_string_set.h \
    src/panda/CallbackDispatcher.h \
    src/panda/function.h \
    src/panda/function_utils.h \
    src/panda/lib.h \
    src/panda/optional.h \
    src/panda/log.h
