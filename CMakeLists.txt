# CMAKE 
# CMake may not be the worst language ever made but it wasn't for lack of effort!! I wish I didn't have
# to deal with this, but I do.

# cmake_minimum_required(VERSION <min>[...<policy_max>] [FATAL_ERROR])

cmake_minimum_required(VERSION 3.16)

# <help> defining "project"
# project(<PROJECT-NAME>
#        [VERSION <major>[.<minor>[.<patch>[.<tweak>]]]]
#        [DESCRIPTION <project-description-string>]
#        [HOMEPAGE_URL <url-string>]
#        [LANGUAGES <language-name>...])

project(lilcxx)

# <help> Setting language properties for "project"

set(CMAKE_CXX_STANDARD 20)

# set(CMAKE_CXX_STANDARD_REQUIRED ON)
# set(CMAKE_CXX_EXTENSIONS ON)

# Add library to "project"
# add_library(<name> [STATIC | SHARED | MODULE]
#        [EXCLUDE_FROM_ALL]
#        [<source>...])
# EXAMPLE: add_library(BuiltElsewhere STATIC IMPORTED)
# <help> 
# target_compile_definitions(<target>
#        <INTERFACE|PUBLIC|PRIVATE> [items1...]
#        [<INTERFACE|PUBLIC|PRIVATE> [items2...] ...])
# EXAMPLE: target_compile_definitions(foo PUBLIC FOO)
# EXAMPLE: target_compile_definitions(foo PUBLIC -DFOO)  # -D removed
# EXAMPLE: target_compile_definitions(foo PUBLIC "" FOO) # "" ignored
# EXAMPLE: target_compile_definitions(foo PUBLIC -D FOO) # -D becomes "", then  ignored

# <help> set_target_properties(target1 target2 ...
#        PROPERTIES prop1 value1
#        prop2 value2 ...)
# EXAMPLE: set_target_properties(Foo PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
# EXAMPLE: set_target_properties(BuiltElsewhere PROPERTIES IMPORTED_LOCATION /path/to/libSomething.a)


# $<CONFIG>
# CMAKE_<LANG>_FLAGS_<CONFIG>
# CMAKE_<TARGETTYPE>_LINKER_FLAGS_<CONFIG>
# EXAMPLE: set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++0x -Wall")
# EXAMPLE: set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++2a -Wall -g -O2")

# SETTING "target PROPERTIES 
# ================================================================

add_test(NAME lilcxxsh_unittest
        COMMAND ${CMAKE_CURRENT_SOURCE_DIR}/build/normal/lilcxxsh unittest
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/)

# add_executable(<name> [WIN32] [MACOSX_BUNDLE]
#               [EXCLUDE_FROM_ALL]
#               [source1] [source2 ...])
# add_executable(<name> IMPORTED [GLOBAL])
# add_executable(<name> ALIAS <target>)

#         main/unittest.cxx
add_executable(lilcxxsh
        main/main.cpp
        src/lil_cmds.cpp
        src/lil_eval_expr.cpp
        src/lilcxxTest.cpp
        inc/lilcxxTest.h
        src/lil.cpp inc/lil.h
        inc/lil_inter.h
        inc/narrow_cast.h
        inc/MemCache.h
        inc/comp_info.h
        inc/funcPointers.h)

# <help> Add include path to "target"
# target_include_directories(<target> [SYSTEM] [AFTER|BEFORE]
#        <INTERFACE|PUBLIC|PRIVATE> [items1...]
#        [<INTERFACE|PUBLIC|PRIVATE> [items2...] ...])
# EXAMPLE: target_include_directories(myawesomelib PUBLIC include)
# target_include_directories(lil SYSTEM PUBLIC gsl)
target_include_directories(lilcxxsh SYSTEM PUBLIC inc boost_1_79_0)

# <help> Add library to "target"
# target_include_directories(<target> [SYSTEM] [AFTER|BEFORE]
#        <INTERFACE|PUBLIC|PRIVATE> [items1...]
#        [<INTERFACE|PUBLIC|PRIVATE> [items2...] ...])
# EXAMPLE: target_link_libraries(myawesomecli PRIVATE myawesomelib)
# EXAMPLE: link_directories(/Users/User/Projects/libraries/libs)

#         main/unittest.cxx
add_library(lilcxx
        src/lil_cmds.cpp
        src/lil_eval_expr.cpp
        src/lilcxxTest.cpp
        inc/lilcxxTest.h
        src/lil.cpp inc/lil.h
        inc/lil_inter.h
        inc/narrow_cast.h
        inc/MemCache.h
        inc/comp_info.h
        inc/funcPointers.h)

add_custom_command ( OUTPUT inc/git_info.h
        COMMAND sh ${CMAKE_CURRENT_SOURCE_DIR}/gen_gitInfo.sh
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/)

# ALL - Indicate that this target should be added to the default build target so that it
# will be run every time (the command cannot be called ALL).
add_custom_target(buildGitInfo ALL DEPENDS inc/git_info.h)

target_include_directories(lilcxx SYSTEM PUBLIC inc boost_1_79_0)

# <help> apply option to all target files
# target_compile_options(<target> [BEFORE]
#        <INTERFACE|PUBLIC|PRIVATE> [items1...]
#        [<INTERFACE|PUBLIC|PRIVATE> [items2...] ...])
# EXAMPLE: target_compile_options(propertyTree PRIVATE $<$<COMPILE_LANGUAGE:CXX>:-fno-exceptions>)
# EXAMPLE: target_compile_options(Foo PRIVATE -fno-exceptions)
# -Werror -Wold-style-cast  -Wpadding -Wpadded -Waggregate-return
target_compile_options(lilcxxsh PRIVATE  -Wnonnull
        -pedantic -Wall -Wextra -Wcast-align -Wcast-qual
        -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op
        -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept  -Woverloaded-virtual
        -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel
        -Wstrict-overflow=5 -Wswitch-default -Wundef  -Wno-unused

        -pedantic-errors -Wextra  -Wcast-align
        -Wcast-qual -Wconversion
        -Wdisabled-optimization
        -Wfloat-equal -Wformat=2
        -Wformat-nonliteral -Wformat-security
        -Wformat-y2k
        -Wimport  -Winit-self  -Winline -Winvalid-pch
        -Wlong-long
        -Wmissing-field-initializers -Wmissing-format-attribute
        -Wmissing-include-dirs -Wmissing-noreturn
        -Wpacked  -Wpointer-arith
        -Wredundant-decls
        -Wshadow -Wstack-protector
        -Wstrict-aliasing=2 -Wswitch-default
        -Wswitch-enum
        -Wunreachable-code -Wunused
        -Wunused-parameter
        -Wvariadic-macros -Wno-float-equal
        -Wwrite-strings -Wsign-conversion

        -Wno-unknown-pragmas -Wno-sign-compare -Wno-inline)

# -Werror -Wold-style-cast  -Wpadding -Wpadded -Waggregate-return
target_compile_options(lilcxx PRIVATE -Wnonnull
        -pedantic -Wall -Wextra -Wcast-align -Wcast-qual
        -Wctor-dtor-privacy -Wdisabled-optimization -Wformat=2 -Winit-self -Wlogical-op
        -Wmissing-declarations -Wmissing-include-dirs -Wnoexcept  -Woverloaded-virtual
        -Wredundant-decls -Wshadow -Wsign-promo -Wstrict-null-sentinel
        -Wstrict-overflow=5 -Wswitch-default -Wundef  -Wno-unused

        -pedantic-errors -Wextra  -Wcast-align
        -Wcast-qual -Wconversion
        -Wdisabled-optimization
        -Wfloat-equal -Wformat=2
        -Wformat-nonliteral -Wformat-security
        -Wformat-y2k
        -Wimport  -Winit-self  -Winline -Winvalid-pch
        -Wlong-long
        -Wmissing-field-initializers -Wmissing-format-attribute
        -Wmissing-include-dirs -Wmissing-noreturn
        -Wpacked  -Wpointer-arith
        -Wredundant-decls
        -Wshadow -Wstack-protector
        -Wstrict-aliasing=2 -Wswitch-default
        -Wswitch-enum
        -Wunreachable-code -Wunused
        -Wunused-parameter
        -Wvariadic-macros -Wno-float-equal
        -Wwrite-strings -Wsign-conversion

        -Wno-unknown-pragmas -Wno-sign-compare -Wno-inline)

# <help> target_compile_definitions(<target>
#        <INTERFACE|PUBLIC|PRIVATE> [items1...]
#        [<INTERFACE|PUBLIC|PRIVATE> [items2...] ...])
# EXAMPLE: target_compile_definitions(foo PUBLIC FOO)
# EXAMPLE: target_compile_definitions(foo PUBLIC -DFOO)  # -D removed
# EXAMPLE: target_compile_definitions(foo PUBLIC "" FOO) # "" ignored
# EXAMPLE: target_compile_definitions(foo PUBLIC -D FOO) # -D becomes "", then ignored

# <help> set_target_properties(target1 target2 ...
#        PROPERTIES prop1 value1
#        prop2 value2 ...)
# EXAMPLE: set_target_properties(Foo PROPERTIES C_STANDARD 99 C_STANDARD_REQUIRED ON C_EXTENSIONS OFF)
# EXAMPLE: set_target_properties(BuiltElsewhere PROPERTIES IMPORTED_LOCATION /path/to/libSomething.a)

# apply compile option to a file.
# EXAMPLE: set_property(SOURCE main.cpp APPEND PROPERTY COMPILE_OPTIONS -fno-exceptions)