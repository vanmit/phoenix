# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

cmake_minimum_required(VERSION 3.5)

file(MAKE_DIRECTORY
  "/home/admin/.openclaw/workspace/phoenix-engine/build/_deps/bgfx-src"
  "/home/admin/.openclaw/workspace/phoenix-engine/build/_deps/bgfx-build"
  "/home/admin/.openclaw/workspace/phoenix-engine/build/_deps/bgfx-subbuild/bgfx-populate-prefix"
  "/home/admin/.openclaw/workspace/phoenix-engine/build/_deps/bgfx-subbuild/bgfx-populate-prefix/tmp"
  "/home/admin/.openclaw/workspace/phoenix-engine/build/_deps/bgfx-subbuild/bgfx-populate-prefix/src/bgfx-populate-stamp"
  "/home/admin/.openclaw/workspace/phoenix-engine/build/_deps/bgfx-subbuild/bgfx-populate-prefix/src"
  "/home/admin/.openclaw/workspace/phoenix-engine/build/_deps/bgfx-subbuild/bgfx-populate-prefix/src/bgfx-populate-stamp"
)

set(configSubDirs )
foreach(subDir IN LISTS configSubDirs)
    file(MAKE_DIRECTORY "/home/admin/.openclaw/workspace/phoenix-engine/build/_deps/bgfx-subbuild/bgfx-populate-prefix/src/bgfx-populate-stamp/${subDir}")
endforeach()
if(cfgdir)
  file(MAKE_DIRECTORY "/home/admin/.openclaw/workspace/phoenix-engine/build/_deps/bgfx-subbuild/bgfx-populate-prefix/src/bgfx-populate-stamp${cfgdir}") # cfgdir has leading slash
endif()
