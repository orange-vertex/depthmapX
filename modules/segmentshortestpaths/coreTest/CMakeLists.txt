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

set(segmentpathscoretest segmentpathscoretest)
set(segmentpathscoretest_SRCS
    segmentpathscoretest.cpp)

set(modules_coreTest "${modules_coreTest}" "segmentpathscoretest" CACHE INTERNAL "modules_coreTest" FORCE)

add_compile_definitions(SEGMENTPATHS_CORE_TEST_LIBRARY)

add_library(${segmentpathscoretest} OBJECT ${segmentpathscoretest_SRCS})
