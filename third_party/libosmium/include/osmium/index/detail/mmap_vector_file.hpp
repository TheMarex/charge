#ifndef OSMIUM_INDEX_DETAIL_MMAP_VECTOR_FILE_HPP
#define OSMIUM_INDEX_DETAIL_MMAP_VECTOR_FILE_HPP

/*

This file is part of Osmium (http://osmcode.org/libosmium).

Copyright 2013-2017 Jochen Topf <jochen@topf.org> and others (see README).

Boost Software License - Version 1.0 - August 17th, 2003

Permission is hereby granted, free of charge, to any person or organization
obtaining a copy of the software and accompanying documentation covered by
this license (the "Software") to use, reproduce, display, distribute,
execute, and transmit the Software, and to prepare derivative works of the
Software, and to permit third-parties to whom the Software is furnished to
do so, all subject to the following:

The copyright notices in the Software and this entire statement, including
the above license grant, this restriction and the following disclaimer,
must be included in all copies of the Software, in whole or in part, and
all derivative works of the Software, unless such copies or derivative
works are solely in the form of machine-executable object code generated by
a source language processor.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE, TITLE AND NON-INFRINGEMENT. IN NO EVENT
SHALL THE COPYRIGHT HOLDERS OR ANYONE DISTRIBUTING THE SOFTWARE BE LIABLE
FOR ANY DAMAGES OR OTHER LIABILITY, WHETHER IN CONTRACT, TORT OR OTHERWISE,
ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
DEALINGS IN THE SOFTWARE.

*/

#include <algorithm>
#include <cstddef>
#include <stdexcept>
#include <string>

#include <osmium/index/detail/mmap_vector_base.hpp>
#include <osmium/index/detail/tmpfile.hpp>
#include <osmium/util/file.hpp>

namespace osmium {

    namespace detail {

        /**
         * This class looks and behaves like STL vector, but mmap's a file
         * internally.
         */
        template <typename T>
        class mmap_vector_file : public mmap_vector_base<T> {

            static std::size_t filesize(int fd) {
                const auto size = osmium::util::file_size(fd);

                if (size % sizeof(T) != 0) {
                    throw std::runtime_error{"Index file has wrong size (must be multiple of " + std::to_string(sizeof(T)) + ")."};
                }

                return size / sizeof(T);
            }

        public:

            mmap_vector_file() :
                mmap_vector_base<T>(
                    osmium::detail::create_tmp_file(),
                    osmium::detail::mmap_vector_size_increment) {
            }

            explicit mmap_vector_file(int fd) :
                mmap_vector_base<T>(
                    fd,
                    std::max(osmium::detail::mmap_vector_size_increment, filesize(fd)),
                    filesize(fd)) {
            }

            ~mmap_vector_file() noexcept = default;

        }; // class mmap_vector_file

    } // namespace detail

} // namespace osmium

#endif // OSMIUM_INDEX_DETAIL_MMAP_VECTOR_FILE_HPP