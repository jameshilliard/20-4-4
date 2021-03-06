//
// Copyright (C) 2004 Free Software Foundation, Inc.
//
// This file is part of the GNU ISO C++ Library.  This library is free
// software; you can redistribute it and/or modify it under the
// terms of the GNU General Public License as published by the
// Free Software Foundation; either version 2, or (at your option)
// any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License along
// with this library; see the file COPYING.  If not, write to the Free
// Software Foundation, 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301,
// USA.

// 20.4.1.1 allocator members

#include <ext/bitmap_allocator.h>
#include <testsuite_allocator.h>

typedef int value_type;

int main()
{ 
  typedef __gnu_cxx::bitmap_allocator<value_type> allocator_type;
  __gnu_test::check_allocate_max_size<allocator_type>();
  return 0;
}

#if !__GXX_WEAK
// Explicitly instantiatiate for systems without weak symbols.
template class __gnu_cxx::bitmap_allocator<value_type>;
#endif
