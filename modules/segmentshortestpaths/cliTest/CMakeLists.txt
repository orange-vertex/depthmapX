# Copyright (C) 2020 Petros Koutsolampros

# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.

# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.

# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

set(segmentpathsclitest segmentpathsclitest)
set(segmentpathsclitest_SRCS
    segmentshortestpathparsertest.cpp)

set(modules_cliTest "${modules_cliTest}" "segmentpathsclitest" CACHE INTERNAL "modules_cliTest" FORCE)

add_compile_definitions(SEGMENTPATHS_CLI_TEST_LIBRARY)

add_library(${segmentpathsclitest} OBJECT ${segmentpathsclitest_SRCS})
